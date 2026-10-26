#include "PadChain.h"

namespace thudi
{
int PadChain::latencySamples (int osFactor)
{
    if (osFactor >= 4) return HalfBand::latencyBaseSamples() + (HalfBand::latencyBaseSamples() + 1) / 2;
    if (osFactor >= 2) return HalfBand::latencyBaseSamples();
    return 0;
}

void PadChain::prepare (float sampleRate, int maxBlock, int oversamplingFactor, uint32_t seed)
{
    sr = sampleRate;
    os = oversamplingFactor >= 4 ? 4 : (oversamplingFactor >= 2 ? 2 : 1);
    osBuf.assign ((size_t) (maxBlock * 4 + 8), 0.0f);
    dc.prepare (sr);
    dly.prepare ((int) (0.04f * sr));
    wobL.prepare (sr, 0.7f, seed * 13u + 1u);
    wobR.prepare (sr, 0.9f, seed * 17u + 5u);
    hiss.setSeed (seed * 31u + 7u);
    lfoR = 0.31f;
    reset();
}

void PadChain::reset()
{
    hb1.reset(); hb2a.reset(); hb2b.reset(); dc.reset();
    xoA.reset(); xoB.reset(); xoHA.reset(); xoHB.reset(); monoA.reset(); monoB.reset(); monoHA.reset(); monoHB.reset(); dly.reset();
    tapeL.reset(); tapeR.reset(); lcL.reset(); lcR.reset(); hcL.reset(); hcR.reset();
    crushPhase = 0; held = 0; xoHz = monoHz = tapeHz = lcHz = hcHz = -1;
}

inline float PadChain::driveSample (float x, float gain, float bias, float norm)
{
    return shape (x, gain, bias) * norm;
}

void PadChain::process (const PadSound& s, float* x, float* outL, float* outR, int n)
{
    // ---------------- drive (oversampled) ----------------
    const float drive = s[P::Drive], asym = s[P::Asym];
    if (drive > 0.0005f || asym > 0.0005f)
    {
        const float gain = dbToGain (drive * 30.0f);
        const float bias = asym * 0.9f;
        const float norm = 1.0f / std::max (0.05f, (std::tanh (gain + bias) - std::tanh (bias)));
        for (int i = 0; i < n; ++i)
        {
            float in = x[i], y;
            if (os == 1) y = driveSample (in, gain, bias, norm);
            else if (os == 2)
            {
                float a, b; hb1.up (in, a, b);
                y = hb1.down (driveSample (a, gain, bias, norm), driveSample (b, gain, bias, norm));
            }
            else
            {
                float a, b; hb1.up (in, a, b);
                float a0, a1, b0, b1; hb2a.up (a, a0, a1); hb2a.up (b, b0, b1);
                float da = hb2b.down (driveSample (a0, gain, bias, norm), driveSample (a1, gain, bias, norm));
                float db = hb2b.down (driveSample (b0, gain, bias, norm), driveSample (b1, gain, bias, norm));
                y = hb1.down (da, db);
            }
            x[i] = dc.process (y);
        }
    }
    else if (os > 1)
    {
        // keep latency identical whether drive is on or off
        for (int i = 0; i < n; ++i)
        {
            float in = x[i];
            if (os == 2) { float a, b; hb1.up (in, a, b); x[i] = hb1.down (a, b); }
            else
            {
                float a, b; hb1.up (in, a, b);
                float a0, a1, b0, b1; hb2a.up (a, a0, a1); hb2a.up (b, b0, b1);
                x[i] = hb1.down (hb2b.down (a0, a1), hb2b.down (b0, b1));
            }
        }
    }

    // ---------------- crush (only above crossover) ----------------
    const float crushMix = s[P::CrushMix];
    if (crushMix > 0.0005f)
    {
        const float xo = s[P::CrushXover];
        if (std::abs (xo - xoHz) > 0.5f) { xoA.set (xo, 0.7071f, sr); xoB.set (xo, 0.7071f, sr); xoHA.set (xo, 0.7071f, sr); xoHB.set (xo, 0.7071f, sr); xoHz = xo; }
        const float rate = s[P::CrushRate] / sr;
        const float q = std::exp2 (std::round (s[P::CrushBits]) - 1.0f);
        const bool full = xo <= 20.5f;
        for (int i = 0; i < n; ++i)
        {
            // Linkwitz-Riley 4th order split (lo + hi sums flat)
            float lo = full ? 0.0f : xoB.lp (xoA.lp (x[i]));
            float hi = full ? x[i] : xoHB.hp (xoHA.hp (x[i]));
            crushPhase += rate;
            if (crushPhase >= 1.0f) { crushPhase -= std::floor (crushPhase); held = std::round (hi * q) / q; }
            x[i] = lo + hi + crushMix * (held - hi);
        }
    }

    // ---------------- width / chorus with mono bass ----------------
    const float width = s[P::Width];
    if (width > 0.0005f)
    {
        const float mb = s[P::MonoBelow];
        if (std::abs (mb - monoHz) > 0.5f) { monoA.set (mb, 0.7071f, sr); monoB.set (mb, 0.7071f, sr); monoHA.set (mb, 0.7071f, sr); monoHB.set (mb, 0.7071f, sr); monoHz = mb; }
        const float rate = s[P::ChorusRate], depth = s[P::ChorusDepth] * 0.001f * sr;
        const float incL = rate / sr, incR = rate * 1.44f / sr;
        const float baseL = 0.005f * sr, baseR = 0.011f * sr;
        for (int i = 0; i < n; ++i)
        {
            float lo = monoB.lp (monoA.lp (x[i]));
            float hi = monoHB.hp (monoHA.hp (x[i]));
            dly.push (hi);
            lfoL += incL; lfoL -= std::floor (lfoL);
            lfoR += incR; lfoR -= std::floor (lfoR);
            float dL = baseL + depth * (0.5f + 0.5f * std::sin (kTwoPi * lfoL)) + 0.4f * depth * wobL.tick() * 0.25f;
            float dR = baseR + depth * (0.5f + 0.5f * std::sin (kTwoPi * lfoR)) + 0.4f * depth * wobR.tick() * 0.25f;
            float wl = dly.read (std::max (1.0f, dL)), wr = dly.read (std::max (1.0f, dR));
            float mid = 0.5f * (wl + wr), side = 0.5f * (wl - wr) * 1.5f;
            outL[i] = lo + (1.0f - width) * hi + width * (mid + side);
            outR[i] = lo + (1.0f - width) * hi + width * (mid - side);
        }
    }
    else
    {
        for (int i = 0; i < n; ++i) outL[i] = outR[i] = x[i];
    }

    // ---------------- tape ----------------
    const float tape = s[P::Tape];
    if (tape > 0.0005f)
    {
        const float th = 20000.0f - 13000.0f * tape;
        if (std::abs (th - tapeHz) > 1.0f) { tapeL.set (th, sr); tapeR.set (th, sr); tapeHz = th; }
        const float g = 1.0f + tape * 2.0f, ng = 1.0f / std::tanh (g);
        const float hs = 0.0006f * tape;
        for (int i = 0; i < n; ++i)
        {
            outL[i] = tapeL.lp (std::tanh (outL[i] * g) * ng) + hs * hiss.bi();
            outR[i] = tapeR.lp (std::tanh (outR[i] * g) * ng) + hs * hiss.bi();
        }
    }

    // ---------------- low / high cut ----------------
    const float lc = s[P::LowCut], hc = s[P::HighCut];
    if (lc > 10.5f)
    {
        if (std::abs (lc - lcHz) > 0.5f) { lcL.set (lc, 0.7071f, sr); lcR.set (lc, 0.7071f, sr); lcHz = lc; }
        for (int i = 0; i < n; ++i) { outL[i] = lcL.hp (outL[i]); outR[i] = lcR.hp (outR[i]); }
    }
    if (hc < 21900.0f)
    {
        if (std::abs (hc - hcHz) > 1.0f) { hcL.set (hc, 0.7071f, sr); hcR.set (hc, 0.7071f, sr); hcHz = hc; }
        for (int i = 0; i < n; ++i) { outL[i] = hcL.lp (outL[i]); outR[i] = hcR.lp (outR[i]); }
    }

    // ---------------- level / pan (smoothed) ----------------
    const float lvl = dbToGain (s[P::Level]);
    const float pan = clampf (s[P::Pan], -1.0f, 1.0f);
    const float a = (pan + 1.0f) * 0.25f * kPi;
    const float tL = lvl * std::cos (a) * 1.41421356f, tR = lvl * std::sin (a) * 1.41421356f;
    const float sm = 1.0f - std::exp (-1.0f / (0.005f * sr));
    for (int i = 0; i < n; ++i)
    {
        gL += sm * (tL - gL); gR += sm * (tR - gR);
        outL[i] *= gL; outR[i] *= gR;
    }
}
} // namespace thudi

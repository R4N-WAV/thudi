#include "Voice.h"

namespace thudi
{
static const float kMetalSets[5][6] = {
    { 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f },   // 808
    { 317.0f, 456.0f, 571.0f, 655.0f, 773.0f, 1069.0f },  // Tight
    { 587.0f, 1620.0f, 3171.0f, 5243.0f, 1174.0f, 2349.0f }, // Bell
    { 540.0f, 800.0f, 541.6f, 802.4f, 538.4f, 797.6f },   // Cowbell
    { 120.0f, 187.0f, 251.0f, 333.0f, 427.0f, 521.0f },   // Gong
};

void Voice::prepare (float sampleRate)
{
    sr = sampleRate; invSr = 1.0f / sr;
    toneEnv.prepare (sr); noiseEnv.prepare (sr); metalEnv.prepare (sr);
    active = false;
}

void Voice::setupBody (const PadSound& s, float ratio)
{
    const float f0 = s[P::BodyFreq] * ratio;
    const float decay = s[P::BodyDecay] * 0.001f;
    const float freqs[2] = { f0, f0 * s[P::BodyRatio] };
    for (int m = 0; m < 2; ++m)
    {
        float f = clampf (freqs[m], 20.0f, sr * 0.45f);
        float d = m == 0 ? decay : decay * 0.6f;
        float r = std::exp (-6.9078f / (std::max (0.002f, d) * sr));
        bA1[m] = 2.0f * r * std::cos (kTwoPi * f / sr);
        bA2[m] = -r * r;
        bGain[m] = (1.0f - r * r) * (m == 0 ? 1.0f : 0.6f) * 6.0f;
        bY1[m] = bY2[m] = 0;
    }
}

void Voice::start (const PadSound& s, float pitchSemis, float velocity, uint32_t seed)
{
    active = true; releasing = false; ageSamples = 0; t = 0;
    rng.setSeed (seed);
    drifter.prepare (sr, 1.5f + 2.0f * rng.uni(), seed * 7u + 3u);
    const float vs = s[P::VelSens];
    const float v = clampf (velocity, 0.0f, 1.0f);
    velGain = (1.0f - vs) + vs * std::sqrt (v);

    curSemis = targetSemis = pitchSemis; glideCoef = 1.0f;
    fineSemis = s[P::Fine] * 0.01f;

    // tone
    ph = 0.0f; ph2 = 0.25f; fmPh = 0;
    sweepAmt = s[P::SweepAmt]; sweepTime = std::max (0.001f, s[P::SweepTime] * 0.001f); sweepCurve = s[P::SweepCurve];
    fmDecay = std::max (0.001f, s[P::FmDecay] * 0.001f);
    toneEnv.start (s[P::ToneAttack] * 0.001f, s[P::ToneHold] * 0.001f, s[P::ToneDecay] * 0.001f, s[P::ToneCurve]);
    toneLp.reset(); toneLpHz = -1;

    // noise
    pinkB0 = pinkB1 = pinkB2 = brown = 0; noiseSvf.reset(); noiseCoefCounter = 0;
    numBursts = (int) clampf (std::round (s[P::Bursts]), 1.0f, 8.0f);
    const float spacing = s[P::BurstSpacing] * 0.001f, rnd = s[P::BurstRandom];
    float acc = 0;
    for (int k = 0; k < numBursts; ++k)
    {
        burstTimes[k] = acc;
        acc += spacing * (1.0f + rnd * 0.45f * rng.bi());
    }
    burstDecay = std::max (0.0003f, s[P::BurstDecay] * 0.001f);
    noiseEnv.start (s[P::NoiseAttack] * 0.001f, 0.0f, s[P::NoiseDecay] * 0.001f, s[P::NoiseCurve]);
    if (numBursts > 1) noiseEnv.t = -burstTimes[numBursts - 1]; // main tail starts at the last burst

    // metal
    const int set = (int) clampf (std::round (s[P::MetalSet]), 0.0f, 4.0f);
    for (int i = 0; i < 6; ++i) { metalFreq[i] = kMetalSets[set][i]; metalPh[i] = rng.uni(); }
    metalBp.reset(); metalHp.reset(); metalCoefCounter = 0;
    metalEnv.start (s[P::MetalAttack] * 0.001f, 0.0f, s[P::MetalDecay] * 0.001f, 0.35f);

    // body
    bodyOn = s[P::BodyLevel] > 0.0001f;
    if (bodyOn) setupBody (s, semis (pitchSemis + fineSemis));
    bodyExcEnv = 1.0f; bodyExcCoef = std::exp (-1.0f / (0.004f * sr));
}

void Voice::glideTo (float pitchSemis, float glideMs)
{
    targetSemis = pitchSemis;
    if (glideMs <= 0.5f) { curSemis = pitchSemis; glideCoef = 1.0f; return; }
    glideCoef = 1.0f - std::exp (-1.0f / (glideMs * 0.001f * sr / 3.0f));
}

void Voice::release (float seconds)
{
    if (! active) return;
    releasing = true;
    toneEnv.release (seconds); noiseEnv.release (seconds); metalEnv.release (seconds);
    if (seconds < 0.01f) { for (int m = 0; m < 2; ++m) { bY1[m] *= 0.2f; bY2[m] *= 0.2f; } bodyOn = false; }
}

void Voice::render (const PadSound& s, float* out, int n)
{
    if (! active) return;

    const float toneLevel = s[P::ToneLevel], noiseLevel = s[P::NoiseLevel];
    const float metalLevel = s[P::MetalLevel], bodyLevel = s[P::BodyLevel];
    const float shapeP = s[P::ToneShape], growl = s[P::Growl];
    const float fmAmt = s[P::FmAmt], fmRatio = s[P::FmRatio];
    const float drift = s[P::Drift];
    const float baseHz = s[P::TonePitch];
    const float color = s[P::NoiseColor];
    const int   filtType = (int) std::round (s[P::NoiseFilter]);
    const float cutoff = s[P::NoiseCutoff], reso = s[P::NoiseReso], envAmt = s[P::NoiseEnvAmt];
    const float metalPitch = s[P::MetalPitch], metalTone = s[P::MetalTone], metalQ = s[P::MetalQ], ring = s[P::MetalRing];
    const float bodyExcite = s[P::BodyExcite];
    const float lpHz = s[P::ToneLP];

    if (lpHz < 19000.0f && std::abs (lpHz - toneLpHz) > 0.5f) { toneLp.set (lpHz, 0.707f, sr); toneLpHz = lpHz; }

    bool anyAlive = false;
    for (int i = 0; i < n; ++i)
    {
        // pitch
        curSemis += glideCoef * (targetSemis - curSemis);
        const float dr = drift > 0 ? drifter.tick() * drift : 0.0f;
        const float pitchRatio = semis (curSemis + fineSemis + dr * 0.12f);

        float y = 0.0f;

        // ---------- tone ----------
        float te = toneEnv.tick();
        if (toneLevel > 0.0001f && ! toneEnv.done)
        {
            float sw = sweepAmt > 0 ? decayShape (t / sweepTime, sweepCurve) : 0.0f;
            float f = baseHz * pitchRatio * semis (sweepAmt * sw);
            float inc = clampf (f * invSr, 0.0f, 0.45f);
            float pm = 0.0f;
            if (fmAmt > 0.0001f)
            {
                fmPh += inc * fmRatio; fmPh -= std::floor (fmPh);
                pm = fmAmt * 1.2f * std::exp (-t / fmDecay) * std::sin (kTwoPi * fmPh);
            }
            float p = ph + pm; p -= std::floor (p);
            float sine = std::sin (kTwoPi * p);
            float osc = sine;
            if (shapeP > 0.001f)
            {
                float tri = 1.0f - 4.0f * std::abs (p - 0.5f);
                float saw = 2.0f * p - 1.0f - polyBlep (p, inc);
                float sq = (p < 0.5f ? 1.0f : -1.0f) + polyBlep (p, inc) - polyBlep (std::fmod (p + 0.5f, 1.0f), inc);
                float x = shapeP * 3.0f;
                if (x < 1.0f)      osc = sine + x * (tri - sine);
                else if (x < 2.0f) osc = tri + (x - 1.0f) * (saw - tri);
                else               osc = saw + (x - 2.0f) * (sq - saw);
            }
            if (growl > 0.0001f)
                osc += growl * (0.75f * std::sin (kTwoPi * 2.0f * p + 0.4f) + 0.3f * std::sin (kTwoPi * 3.0f * p));
            if (drift > 0.0001f)
            {   // detuned twin -> slow analog beating / thickness
                ph2 += inc * 1.0035f; ph2 -= std::floor (ph2);
                osc = osc * (1.0f - 0.22f * drift) + 0.3f * drift * std::sin (kTwoPi * ph2);
            }
            ph += inc; ph -= std::floor (ph);
            if (toneLpHz > 0 && lpHz < 19000.0f) osc = toneLp.lp (osc);
            y += osc * te * toneLevel;
            anyAlive = true;
        }

        // ---------- noise ----------
        float white = rng.bi();
        if (noiseLevel > 0.0001f && ! noiseEnv.done)
        {
            // pink (Paul Kellet economy) and brown
            pinkB0 = 0.99765f * pinkB0 + white * 0.0990460f;
            pinkB1 = 0.96300f * pinkB1 + white * 0.2965164f;
            pinkB2 = 0.57000f * pinkB2 + white * 1.0526913f;
            float pink = (pinkB0 + pinkB1 + pinkB2 + white * 0.1848f) * 0.22f;
            brown = clampf (brown + white * 0.06f, -1.0f, 1.0f) * 0.995f;
            float nz = color < 0.5f ? white + (pink - white) * (color * 2.0f)
                                    : pink + (brown * 2.5f - pink) * ((color - 0.5f) * 2.0f);
            // amplitude: bursts then main envelope
            float ne = noiseEnv.tick();
            float a = ne;
            if (numBursts > 1)
                for (int k = 0; k < numBursts - 1; ++k)
                    if (t >= burstTimes[k]) a = std::max (a, std::exp (-(t - burstTimes[k]) / burstDecay));
            if (--noiseCoefCounter <= 0)
            {
                noiseCoefCounter = 16;
                float c = cutoff * std::exp2 (envAmt * 5.0f * std::max (ne, 0.0f));
                noiseSvf.set (c, 0.5f + reso * 12.0f, sr);
            }
            float lp, bp, hp; noiseSvf.process (nz, lp, bp, hp);
            float filtered = filtType == 0 ? lp : (filtType == 1 ? bp * (1.0f + reso) : hp);
            y += filtered * a * noiseLevel;
            anyAlive = true;
        }
        else if (numBursts > 1 && noiseLevel > 0.0001f && t < burstTimes[numBursts - 1] + 0.05f)
        {
            anyAlive = true;
        }

        // ---------- metal ----------
        if (metalLevel > 0.0001f && ! metalEnv.done)
        {
            float me = metalEnv.tick();
            float sq[6]; float sum = 0;
            for (int k = 0; k < 6; ++k)
            {
                float inc = clampf (metalFreq[k] * metalPitch * pitchRatio * invSr, 0.0f, 0.45f);
                float p = metalPh[k];
                float v = (p < 0.5f ? 1.0f : -1.0f) + polyBlep (p, inc) - polyBlep (std::fmod (p + 0.5f, 1.0f), inc);
                metalPh[k] = p + inc - std::floor (p + inc);
                sq[k] = v; sum += v;
            }
            sum *= (1.0f / 6.0f);
            if (ring > 0.0001f)
            {
                float rm = (sq[0] * sq[1] + sq[2] * sq[3] + sq[4] * sq[5]) * (1.0f / 3.0f);
                sum += ring * (rm - sum);
            }
            if (--metalCoefCounter <= 0)
            {
                metalCoefCounter = 32;
                metalBp.set (metalTone * pitchRatio, 0.7f + metalQ * 8.0f, sr);
                metalHp.set (metalTone * pitchRatio * 0.45f, 0.707f, sr);
            }
            float m = metalHp.hp (metalBp.bp (sum) * (1.0f + metalQ * 2.0f));
            y += m * me * metalLevel * 2.0f;
            anyAlive = true;
        }

        // ---------- body (modal resonators) ----------
        if (bodyOn && bodyLevel > 0.0001f)
        {
            float exc = bodyExcEnv * ((1.0f - bodyExcite) * (ageSamples < 3 ? 1.0f : 0.0f) * 4.0f + bodyExcite * white);
            bodyExcEnv *= bodyExcCoef;
            float b = 0;
            for (int m = 0; m < 2; ++m)
            {
                float yy = bA1[m] * bY1[m] + bA2[m] * bY2[m] + exc * bGain[m];
                bY2[m] = bY1[m]; bY1[m] = yy; b += yy;
            }
            y += b * bodyLevel;
            if (std::abs (b) > 1e-5f || ageSamples < (int) (0.05f * sr)) anyAlive = true;
        }

        out[i] += y * velGain;
        t += invSr; ++ageSamples;
    }

    if (! anyAlive) active = false;
}
} // namespace thudi

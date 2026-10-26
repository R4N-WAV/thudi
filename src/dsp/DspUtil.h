// Thudi — small DSP building blocks (no external dependencies)
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace thudi
{
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;
constexpr double kPiD = 3.14159265358979323846;

inline float dbToGain (float db) { return db <= -59.9f ? 0.0f : std::pow (10.0f, db * 0.05f); }
inline float clampf (float x, float a, float b) { return std::min (b, std::max (a, x)); }
inline float semis (float st) { return std::exp2 (st / 12.0f); }

// Fast xorshift noise, deterministic per seed
struct Rng
{
    uint32_t s = 0x9E3779B9u;
    explicit Rng (uint32_t seed = 1) { setSeed (seed); }
    void setSeed (uint32_t seed) { s = seed ? seed * 2654435761u : 0x9E3779B9u; }
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float uni() { return (float) (next() >> 8) * (1.0f / 16777216.0f); }       // 0..1
    float bi() { return uni() * 2.0f - 1.0f; }                                  // -1..1
};

// Slow random wander for analog drift (smoothed random walk, approx -1..1)
struct Drifter
{
    Rng rng;
    float y = 0, target = 0, coef = 0.0005f;
    int counter = 0, period = 2000;
    void prepare (float sampleRate, float rateHz, uint32_t seed)
    {
        rng.setSeed (seed);
        period = std::max (1, (int) (sampleRate / std::max (0.05f, rateHz)));
        coef = 1.0f - std::exp (-kTwoPi * rateHz / sampleRate);
        target = rng.bi(); y = target * 0.5f; counter = 0;
    }
    float tick()
    {
        if (++counter >= period) { counter = 0; target = rng.bi(); }
        y += coef * (target - y);
        return y;
    }
};

// Topology-preserving state variable filter (Zavalishin)
struct Svf
{
    float ic1 = 0, ic2 = 0, g = 0, k = 1.4f, a1 = 0, a2 = 0, a3 = 0;
    void reset() { ic1 = ic2 = 0; }
    void set (float cutoffHz, float q, float sampleRate)
    {
        cutoffHz = clampf (cutoffHz, 10.0f, sampleRate * 0.49f);
        g = std::tan (kPi * cutoffHz / sampleRate);
        k = 1.0f / std::max (0.05f, q);
        a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    // returns lp, writes bp/hp
    inline void process (float x, float& lp, float& bp, float& hp)
    {
        float v3 = x - ic2;
        float v1 = a1 * ic1 + a2 * v3;
        float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2;
        lp = v2; bp = v1; hp = x - k * v1 - v2;
    }
    inline float lp (float x) { float l, b, h; process (x, l, b, h); return l; }
    inline float hp (float x) { float l, b, h; process (x, l, b, h); return h; }
    inline float bp (float x) { float l, b, h; process (x, l, b, h); return b; }
};

struct OnePole
{
    float z = 0, a = 0;
    void set (float hz, float sr) { a = 1.0f - std::exp (-kTwoPi * clampf (hz, 1.0f, sr * 0.49f) / sr); }
    inline float lp (float x) { z += a * (x - z); return z; }
    void reset() { z = 0; }
};

struct DcBlock
{
    float x1 = 0, y1 = 0, r = 0.9995f;
    void prepare (float sr) { r = 1.0f - (kTwoPi * 8.0f / sr); }
    inline float process (float x) { float y = x - x1 + r * y1; x1 = x; y1 = y; return y; }
    void reset() { x1 = y1 = 0; }
};

// Polynomial anti-aliasing for saw/square edges
inline float polyBlep (float t, float dt)
{
    if (t < dt) { t /= dt; return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

// Envelope shape: x in 0..1 progress through decay, curve -1..1
//  curve -1 : holds then cuts (808 / gated), 0 : natural, +1 : very punchy (fast drop, long tail)
inline float decayShape (float x, float curve)
{
    if (x >= 1.0f) return 0.0f;
    const float k = std::exp2 (2.0f * curve + 1.0f); // 0.5 .. 8
    return std::pow (1.0f - x, k);
}

// Attack-Hold-Decay envelope with release (for gate mode / chokes)
struct Ahd
{
    float attack = 0, hold = 0, decay = 0.1f, curve = 0; // seconds
    float t = 0, dt = 0, relLevel = 0, relT = 0, relTime = 0.04f;
    bool releasing = false, done = true;
    void prepare (float sr) { dt = 1.0f / sr; }
    void start (float a, float h, float d, float c) { attack = a; hold = h; decay = std::max (0.0005f, d); curve = c; t = 0; releasing = false; done = false; }
    void release (float time) { if (done || releasing) return; relLevel = current(); relT = 0; relTime = std::max (0.0005f, time); releasing = true; }
    float current() const
    {
        if (t < 0.0f) return 0.0f;
        if (t < attack) return attack > 0 ? t / attack : 1.0f;
        float u = t - attack;
        if (u < hold) return 1.0f;
        return decayShape ((u - hold) / decay, curve);
    }
    inline float tick()
    {
        if (done) return 0.0f;
        float v;
        if (releasing)
        {
            float x = relT / relTime;
            v = x >= 1.0f ? 0.0f : relLevel * (1.0f - x) * (1.0f - x);
            relT += dt;
            if (x >= 1.0f) done = true;
            return v;
        }
        v = current();
        t += dt;
        if (t > attack + hold + decay) done = true;
        return v;
    }
};

// Fractional delay line (linear interpolation)
struct DelayLine
{
    std::vector<float> buf; int w = 0, mask = 0;
    void prepare (int maxSamples)
    {
        int n = 1; while (n < maxSamples + 4) n <<= 1;
        buf.assign ((size_t) n, 0.0f); mask = n - 1; w = 0;
    }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); }
    inline void push (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }
    inline float read (float delaySamples) const
    {
        float rp = (float) w - 1.0f - delaySamples;
        while (rp < 0) rp += (float) (mask + 1);
        int i0 = (int) rp; float fr = rp - (float) i0;
        float a = buf[(size_t) (i0 & mask)], b = buf[(size_t) ((i0 + 1) & mask)];
        return a + fr * (b - a);
    }
};

// 2x half-band FIR resampler (windowed sinc, linear phase). Cascade for 4x.
struct HalfBand
{
    static constexpr int kTaps = 47; // odd
    float h[kTaps] {};
    std::vector<float> histUp, histDown; int posUp = 0, posDown = 0;
    HalfBand()
    {
        const int m = kTaps / 2;
        double sum = 0;
        for (int n = 0; n < kTaps; ++n)
        {
            int k = n - m;
            double sinc = k == 0 ? 0.5 : std::sin (kPiD * 0.5 * k) / (kPiD * k);
            // Blackman-Harris window
            double w = 0.35875 - 0.48829 * std::cos (2 * kPiD * n / (kTaps - 1)) + 0.14128 * std::cos (4 * kPiD * n / (kTaps - 1)) - 0.01168 * std::cos (6 * kPiD * n / (kTaps - 1));
            h[n] = (float) (sinc * w); sum += h[n];
        }
        for (auto& c : h) c = (float) (c / sum);
        histUp.assign (kTaps, 0.0f); histDown.assign (kTaps, 0.0f);
    }
    void reset() { std::fill (histUp.begin(), histUp.end(), 0.0f); std::fill (histDown.begin(), histDown.end(), 0.0f); posUp = posDown = 0; }
    inline float fir (std::vector<float>& hist, int& pos, float x)
    {
        hist[(size_t) pos] = x;
        float acc = 0; int idx = pos;
        for (int n = 0; n < kTaps; ++n) { acc += h[n] * hist[(size_t) idx]; if (--idx < 0) idx = kTaps - 1; }
        if (++pos >= kTaps) pos = 0;
        return acc;
    }
    // upsample one sample -> two
    inline void up (float x, float& o0, float& o1) { o0 = 2.0f * fir (histUp, posUp, x); o1 = 2.0f * fir (histUp, posUp, 0.0f); }
    // downsample two -> one
    inline float down (float x0, float x1) { fir (histDown, posDown, x0); return fir (histDown, posDown, x1); }
    static constexpr int latencyBaseSamples() { return (kTaps - 1) / 2; } // in the oversampled domain, per stage (up+down => this many base samples)
};

// Asymmetric tube/transformer style waveshaper; bias -> even harmonics
inline float shape (float x, float gain, float bias)
{
    return std::tanh (gain * x + bias) - std::tanh (bias);
}

} // namespace thudi

#include "SampleData.h"
#include <algorithm>
#include <cmath>

namespace thudi
{
std::vector<size_t> chopPoints (const SampleData& s, int numSlices)
{
    numSlices = std::max (1, std::min (64, numSlices));
    const size_t n = s.size();
    std::vector<size_t> out { 0 };
    if (n < 1024 || numSlices == 1) return out;

    // onset strength: positive change of a fast RMS envelope (10 ms hop)
    const size_t hop = std::max<size_t> (64, (size_t) (s.rate * 0.010));
    std::vector<float> env;
    for (size_t i = 0; i + hop <= n; i += hop)
    {
        double acc = 0;
        for (size_t k = 0; k < hop; ++k) acc += (double) s.data[i + k] * s.data[i + k];
        env.push_back ((float) std::sqrt (acc / (double) hop));
    }
    struct Cand { size_t frame; float strength; };
    std::vector<Cand> cands;
    for (size_t f = 3; f + 1 < env.size(); ++f)
    {
        const float prev = (env[f - 1] + env[f - 2] + env[f - 3]) / 3.0f;
        const float rise = env[f] - prev;
        if (rise > 0 && env[f] >= env[f + 1] * 0.8f) cands.push_back ({ f, rise });
    }
    std::sort (cands.begin(), cands.end(), [] (const Cand& a, const Cand& b) { return a.strength > b.strength; });

    // keep the strongest onsets that are at least ~60 ms apart
    const size_t minGap = std::max<size_t> (1, (size_t) (0.06 * s.rate / (double) hop));
    std::vector<size_t> frames;
    for (auto& c : cands)
    {
        if ((int) frames.size() >= numSlices - 1) break;
        if (c.frame * hop < (size_t) (0.03 * s.rate)) continue; // the start is always a slice
        bool ok = true;
        for (auto f : frames) if ((f > c.frame ? f - c.frame : c.frame - f) < minGap) { ok = false; break; }
        if (ok) frames.push_back (c.frame);
    }
    if ((int) frames.size() < numSlices - 1)
    {   // not enough transients (pads, loops of sustained sound): equal slices
        out.clear();
        for (int i = 0; i < numSlices; ++i) out.push_back ((size_t) ((double) n * i / numSlices));
        return out;
    }
    std::sort (frames.begin(), frames.end());
    for (auto f : frames)
    {
        // move back to the quietest point just before the onset (no clicks, keeps the attack)
        size_t pos = f * hop, best = pos;
        float bestAbs = 1e9f;
        const size_t look = std::min (pos, (size_t) (0.012 * s.rate));
        for (size_t k = pos - look; k <= pos && k < n; ++k)
            if (std::abs (s.data[k]) < bestAbs) { bestAbs = std::abs (s.data[k]); best = k; }
        out.push_back (best);
    }
    return out;
}
} // namespace thudi

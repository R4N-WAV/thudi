// Thudi — per-pad character chain: drive -> crush (above crossover) -> width/chorus (mono bass) -> tape -> cuts -> level/pan
#pragma once
#include "DspUtil.h"
#include "ThudiParams.h"

namespace thudi
{
class PadChain
{
public:
    void prepare (float sampleRate, int maxBlock, int oversamplingFactor, uint32_t seed);
    void reset();
    // in: mono (modified in place), outL/outR: written (not added)
    void process (const PadSound& s, float* mono, float* outL, float* outR, int n);
    static int latencySamples (int osFactor);

private:
    float sr = 48000;
    int os = 2;
    HalfBand hb1, hb2a, hb2b; // 2x stage, 4x stage (up/down share filters per stage)
    DcBlock dc;
    std::vector<float> osBuf;

    // crush
    Svf xoA, xoB, xoHA, xoHB; float xoHz = -1; float crushPhase = 0, held = 0;
    // width
    Svf monoA, monoB, monoHA, monoHB; float monoHz = -1;
    DelayLine dly; float lfoL = 0, lfoR = 0.31f; Drifter wobL, wobR;
    // tape
    OnePole tapeL, tapeR; float tapeHz = -1; Rng hiss;
    // cuts
    Svf lcL, lcR, hcL, hcR; float lcHz = -1, hcHz = -1;
    // smoothing
    float gL = 0, gR = 0;

    float driveSample (float x, float gain, float bias, float norm);
};
} // namespace thudi

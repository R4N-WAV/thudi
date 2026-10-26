// Thudi — one drum voice: tone + noise + metal + body layers
#pragma once
#include "DspUtil.h"
#include "ThudiParams.h"

namespace thudi
{
class Voice
{
public:
    void prepare (float sampleRate);
    // pitchSemis: tune + keys offset in semitones (fine handled inside)
    void start (const PadSound& s, float pitchSemis, float velocity, uint32_t seed);
    void glideTo (float pitchSemis, float glideMs);      // legato (808 slides)
    void release (float seconds);                        // gate mode note-off / choke
    void kill() { release (0.004f); }
    // Adds mono output into 'out'. 'live' = current pad values (live edits of levels/filters)
    void render (const PadSound& live, float* out, int numSamples);
    bool isActive() const { return active; }
    bool isReleasing() const { return releasing; }
    int  age() const { return ageSamples; }

private:
    float sr = 48000.0f, invSr = 1.0f / 48000.0f;
    bool active = false, releasing = false;
    int ageSamples = 0;
    float t = 0;            // seconds since start
    float velGain = 1;

    // pitch
    float curSemis = 0, targetSemis = 0, glideCoef = 1;
    float fineSemis = 0;

    // tone
    float ph = 0, ph2 = 0, fmPh = 0;
    Ahd toneEnv; Svf toneLp; float toneLpHz = 20000;
    float sweepAmt = 0, sweepTime = 0.03f, sweepCurve = 0;
    float fmDecay = 0.06f;
    Drifter drifter;

    // noise
    Rng rng;
    float pinkB0 = 0, pinkB1 = 0, pinkB2 = 0, brown = 0;
    Svf noiseSvf; int noiseCoefCounter = 0;
    Ahd noiseEnv;
    int numBursts = 1; float burstTimes[8] {}; float burstDecay = 0.003f;

    // metal
    float metalPh[6] {}; float metalFreq[6] {};
    Svf metalBp, metalHp; Ahd metalEnv; int metalCoefCounter = 0;

    // body (two modal resonators)
    float bY1[2] {}, bY2[2] {}, bA1[2] {}, bA2[2] {}, bGain[2] {};
    float bodyExcEnv = 0, bodyExcCoef = 0; bool bodyOn = false;

    void setupBody (const PadSound& s, float ratio);
};
} // namespace thudi

// Thudi — the 16-pad drum synth engine (host independent)
#pragma once
#include <atomic>
#include <vector>
#include "PadChain.h"
#include "SampleData.h"
#include "Voice.h"

namespace thudi
{
struct NoteEvent
{
    int offset = 0;    // sample offset in block
    int note = 0;
    float velocity = 1;
    bool on = true;
};

// GÆRRO mode master stage: vinyl (wow/flutter, band limit, crackle, hiss, rumble) + sampler grit (12-bit / 26 kHz style)
class GaerroFx
{
public:
    void prepare (double sampleRate, uint32_t seed);
    void process (float* L, float* R, int n, float targetAmount, float dirt, float vinyl, bool withNoiseFloor);
private:
    double sr = 48000;
    float amt = 0, amtCoef = 0.0001f;
    DelayLine dl, dr;
    double wowPh = 0, flutPh = 0;
    Svf hpL, hpR, lpL, lpR; float lpHz = -1;
    float holdL = 0, holdR = 0, holdPh = 0;
    Rng rng;
    float pinkL[3] {}, pinkR[3] {}, rumble = 0, crackEnv = 0, crackSign = 1, crackPan = 0;
    OnePole crackHp;
};

class Engine
{
public:
    static constexpr int kVoicesPerPad = 4;
    static constexpr int kSoloVoices = 8;

    Engine();
    void prepare (double sampleRate, int maxBlockSize, int oversamplingFactor);
    void reset();
    int  latencySamples() const { return PadChain::latencySamples (osFactor); }
    int  oversampling() const { return osFactor; }

    // current sound of each slot (0..15 pads, 16 solo), written by the host wrapper before each block
    PadSound& pad (int i) { return pads[(size_t) i].sound; }
    const PadSound& pad (int i) const { return pads[(size_t) i].sound; }
    void setPadNote (int padIndex, int midiNote) { pads[(size_t) padIndex].note = midiNote; }
    int  padNote (int padIndex) const { return pads[(size_t) padIndex].note; }
    // sample used by a slot whose model is Sample (thread safe; may be called from the message thread)
    void setPadSample (int slot, SamplePtr s);

    float global[kNumGlobalParams] {};

    // UI-thread safe pad trigger (lock-free single producer)
    void queueUiHit (int padIndex, float velocity);
    void queueUiRelease (int padIndex);

    // outputs: array of numChannels pointers; channels are stereo pairs per output bus
    void process (const NoteEvent* events, int numEvents, float* const* outputs, int numChannels, int numSamples);

    // glow of each pad (0..1): follows the envelope of what the pad is playing
    std::atomic<float> padLight[kNumPads];
    std::atomic<float> padActivity[kNumPads]; // (kept for compatibility: peak velocity, decaying)
    std::atomic<int> lastHitPad { -1 };

private:
    struct Pad
    {
        PadSound sound;
        Voice voices[kSoloVoices];
        PadChain chain;
        int note = 36;
        int lastVoice = -1;
        int silentSamples = 1 << 30;
        bool keyHeld = false;
        SamplePtr sample;
    };
    std::vector<Pad> pads;
    double sr = 48000; int maxBlock = 512; int osFactor = 2;
    uint32_t seedCounter = 1234;
    std::vector<float> mono, padL, padR;
    GaerroFx gaerro[kNumOutputs];

    struct UiHit { int pad; float vel; bool on; };
    static constexpr int kFifo = 256;
    UiHit fifo[kFifo]; std::atomic<int> fifoW { 0 }, fifoR { 0 };

    std::vector<int> soloHeld;     // held solo notes (mono legato / glide)

    bool soloOn() const { return global[(int) G::SoloOn] >= 0.5f; }
    int  soloRoot() const { return (int) std::round (global[(int) G::KeysRoot]); }
    void noteOn (int note, float vel);
    void noteOff (int note);
    void triggerPad (int padIndex, float vel, float extraSemis, int depth);
    void releasePad (int padIndex);
    void soloOnNote (int note, float vel);
    void soloOffNote (int note);
    void chokeGroup (int group, int exceptPad);
    void renderSegment (float* const* outputs, int numChannels, int start, int len);
    int  numVoices (int slot) const { return slot == kSoloSlot ? kSoloVoices : kVoicesPerPad; }
    int  freeVoice (int slot);
};
} // namespace thudi

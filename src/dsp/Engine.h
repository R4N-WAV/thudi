// Thudi — the 16-pad drum synth engine (host independent)
#pragma once
#include <atomic>
#include <vector>
#include "PadChain.h"
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

class Engine
{
public:
    static constexpr int kVoicesPerPad = 4;

    Engine();
    void prepare (double sampleRate, int maxBlockSize, int oversamplingFactor);
    void reset();
    int  latencySamples() const { return PadChain::latencySamples (osFactor); }
    int  oversampling() const { return osFactor; }

    // current sound of each pad (written by the host thread wrapper before each block)
    PadSound& pad (int i) { return pads[(size_t) i].sound; }
    const PadSound& pad (int i) const { return pads[(size_t) i].sound; }
    void setPadNote (int padIndex, int midiNote) { pads[(size_t) padIndex].note = midiNote; }
    int  padNote (int padIndex) const { return pads[(size_t) padIndex].note; }

    float global[kNumGlobalParams] {};

    // UI-thread safe pad trigger (lock-free single producer)
    void queueUiHit (int padIndex, float velocity);
    void queueUiRelease (int padIndex);

    // outputs: array of numChannels pointers; channels are stereo pairs per output bus
    void process (const NoteEvent* events, int numEvents, float* const* outputs, int numChannels, int numSamples);

    // activity meters for the GUI (0..1 decaying), updated per block
    std::atomic<float> padActivity[kNumPads];
    // last triggered pad (for GUI follow)
    std::atomic<int> lastHitPad { -1 };

private:
    struct Pad
    {
        PadSound sound;
        Voice voices[kVoicesPerPad];
        PadChain chain;
        int note = 36;
        int lastVoice = -1;
        int silentSamples = 1 << 30;
        bool keyHeld = false;
    };
    std::vector<Pad> pads;
    double sr = 48000; int maxBlock = 512; int osFactor = 2;
    uint32_t seedCounter = 1234;
    std::vector<float> mono, padL, padR;

    struct UiHit { int pad; float vel; bool on; };
    static constexpr int kFifo = 256;
    UiHit fifo[kFifo]; std::atomic<int> fifoW { 0 }, fifoR { 0 };

    int keysPadVoiceNote = -1; // last keys note held (for legato)
    std::vector<int> heldKeys;

    void noteOn (int note, float vel);
    void noteOff (int note);
    void triggerPad (int padIndex, float vel, float extraSemis, int depth, bool fromKeys);
    void releasePad (int padIndex);
    void chokeGroup (int group, int exceptPad);
    void renderSegment (float* const* outputs, int numChannels, int start, int len);
};
} // namespace thudi

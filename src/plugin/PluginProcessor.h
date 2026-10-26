#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Engine.h"
#include "ThudiParams.h"

class PresetManager;

class ThudiProcessor : public juce::AudioProcessor,
                       private juce::AsyncUpdater,
                       private juce::AudioProcessorValueTreeState::Listener
{
public:
    ThudiProcessor();
    ~ThudiProcessor() override;

    // ---- AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Thudi"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- Thudi API (message thread)
    static juce::String padParamId (int pad, thudi::P p);
    static juce::String globalParamId (thudi::G g);
    juce::RangedAudioParameter* padParam (int pad, thudi::P p) const { return padParams[(size_t) pad][(size_t) p]; }
    juce::RangedAudioParameter* globalParam (thudi::G g) const { return globalParams[(size_t) g]; }
    float padValue (int pad, thudi::P p) const { return padRaw[(size_t) pad][(size_t) p]->load(); }
    void setPadValue (int pad, thudi::P p, float plainValue);
    void setGlobalValue (thudi::G g, float plainValue);

    thudi::PadSound getPadSound (int pad) const;
    void setPadSound (int pad, const thudi::PadSound& s);

    juce::String getPadLabel (int pad) const;
    void setPadLabel (int pad, const juce::String& label);
    int getPadNote (int pad) const;
    void setPadNote (int pad, int note);
    juce::String getKitName() const;
    void setKitName (const juce::String& n);

    void hitPad (int pad, float velocity) { engine.queueUiHit (pad, velocity); }
    void releasePadFromUi (int pad) { engine.queueUiRelease (pad); }
    float padActivity (int pad) const { return engine.padActivity[pad].load(); }
    int lastHitPad() const { return engine.lastHitPad.load(); }

    void startMidiLearn (int pad) { midiLearnPad.store (pad); }
    int midiLearnTarget() const { return midiLearnPad.load(); }

    juce::AudioProcessorValueTreeState apvts;
    std::unique_ptr<PresetManager> presets;
    juce::ValueTree meta { "THUDI_META" }; // labels, notes, kit name
    std::function<void()> onMetaChanged;    // GUI refresh hook (message thread)

    int selectedPad = 0; // shared with the editor so it survives editor re-open

private:
    thudi::Engine engine;
    std::array<std::array<juce::RangedAudioParameter*, thudi::kNumPadParams>, thudi::kNumPads> padParams {};
    std::array<std::array<std::atomic<float>*, thudi::kNumPadParams>, thudi::kNumPads> padRaw {};
    std::array<juce::RangedAudioParameter*, thudi::kNumGlobalParams> globalParams {};
    std::array<std::atomic<float>*, thudi::kNumGlobalParams> globalRaw {};

    std::vector<thudi::NoteEvent> events;
    juce::AudioBuffer<float> scratch;
    std::array<float*, thudi::kNumOutputs * 2> scratchPtrs {};
    std::atomic<int> midiLearnPad { -1 }, learnedNote { -1 };
    std::atomic<bool> notesDirty { true };
    std::array<std::atomic<int>, thudi::kNumPads> padNotesAtomic;
    double currentSr = 48000; int currentBlock = 512; int preparedOs = -1;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static BusesProperties makeBuses();
    void parameterChanged (const juce::String& id, float newValue) override;
    void handleAsyncUpdate() override;
    void ensureMeta();
    int oversamplingFactorFromParam() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThudiProcessor)
};

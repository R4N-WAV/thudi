#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "LcdDisplay.h"
#include "PadGrid.h"
#include "PluginProcessor.h"
#include "ThudiLookAndFeel.h"

namespace thudi
{
// Big DATA ENTRY wheel: drag or scroll to step through values / presets
class DataWheel : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    std::function<void (int)> onStep;
private:
    float angle = 0.0f, lastAngle = 0.0f, accum = 0.0f;
    void turn (float delta);
};

class PanelContent : public juce::Component
{
public:
    explicit PanelContent (ThudiProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    std::array<juce::String, 8> knobNames;
    int selectedPad = 0;
    float graffiti = 0.0f, graffitiAlpha = 1.0f;   // GÆRRO overlay animation
    std::function<void()> onPlaqueDoubleClick, onSoloLabelClick;
    static juce::Rectangle<float> soloLabelArea();
private:
    ThudiProcessor& proc;
    void paintFallbackBody (juce::Graphics&);
};
} // namespace thudi

class ThudiEditor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget, private juce::Timer
{
public:
    explicit ThudiEditor (ThudiProcessor&);
    ~ThudiEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

    // used by the headless snapshot tool
    void selectPad (int pad);
    void setPage (int page);
    thudi::LcdDisplay& getLcd() { return lcd; }
    void setSolo (bool on);
    void setGaerro (bool on, bool animate = true);
    void revealGaerroButton (bool animate = true);
    void finishAnimations();   // jump to the end state (snapshots)
    void tick() { timerCallback(); }   // headless: run one UI frame (pad lights, animations)

    // FileDragAndDropTarget: audio files are chopped onto the pads in GÆRRO mode
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    ThudiProcessor& proc;
    thudi::ThudiLookAndFeel lnf;
    thudi::PanelContent content;
    thudi::LcdDisplay lcd;
    thudi::DataWheel wheel;
    juce::Slider masterKnob, driveKnob, vinylKnob, dirtKnob;
    juce::TextButton osButton, enterButton;
    juce::TextButton soloButton, monoButton, glideButton, gaerroButton;
    std::array<juce::TextButton, 4> softKeys;
    std::array<juce::Slider, 8> pageKnobs;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 8> pageAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterAttach, driveAttach, vinylAttach, dirtAttach;
    std::vector<std::unique_ptr<thudi::PadComponent>> pads;
    int lastModel = -1;
    double lastTick = 0;
    float gaerroButtonAnim = 0.0f; bool gaerroButtonShown = false;
    bool lastSolo = false, lastGaerro = false;
    std::unique_ptr<juce::FileChooser> chooser;

    void timerCallback() override;
    void rebuildPage();
    void refreshAll();
    void softKey (int i);
    void enter();
    void wheelStep (int steps);
    void showSaveMenu();
    void askText (const juce::String& title, const juce::String& initial, std::function<void (const juce::String&)> onOk);
    void loadSelectedEntry();
    void updateOsButton();
    void updateModeButtons();
    void layoutModeButtons();
    int  editSlot() const;
    void showSoloMenu();
    void addLibraryMenu (juce::PopupMenu& m, int slot);
    void askChop (const juce::File& f, int firstPad);
    void loadSampleInto (int pad);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThudiEditor)
};

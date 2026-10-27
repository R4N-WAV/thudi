// Thudi — the blue MPC-style LCD: pad editor pages and preset browser
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PresetManager.h"

class ThudiProcessor;

namespace thudi
{
class LcdDisplay : public juce::Component
{
public:
    enum class Mode { Edit, Browse };
    enum class Browse { Kits, Pads };

    explicit LcdDisplay (ThudiProcessor& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    Mode mode = Mode::Edit;
    Browse browse = Browse::Kits;
    int pad = 0, page = 0, focus = 0;
    int browseIndex = 0, browseTop = 0;
    std::vector<PresetManager::Entry> entries;
    juce::String message; int messageTicks = 0; // transient status line

    void refreshEntries();
    void scrollBrowse (int delta);
    juce::StringArray softKeyLabels() const;
    int numPages() const;
    void flash (const juce::String& msg) { message = msg; messageTicks = 120; repaint(); }

    std::function<void (int)> onPageChanged;
    std::function<void (int)> onFocusChanged;
    std::function<void()> onLoadSelected;

    // GÆRRO switch-on: a digital cigarette smoked fast, as a loading screen (seconds; < 0 = off)
    static constexpr float kSmokeLength = 2.6f;
    void startSmoke() { smokeT = 0.0f; smokeSeed = juce::Random::getSystemRandom().nextInt(); repaint(); }
    bool smoking() const { return smokeT >= 0.0f; }
    void stopSmoke() { smokeT = -1.0f; repaint(); }
    void setSmokeTime (float t) { smokeT = t; repaint(); }   // snapshots
    void advance (float dt) { if (smokeT >= 0.0f) { smokeT += dt; if (smokeT > kSmokeLength) smokeT = -1.0f; repaint(); } }

    // set by the editor: the 8 local x centres of the knobs, so the page cells line up nicely
private:
    ThudiProcessor& proc;
    void paintEdit (juce::Graphics&, juce::Rectangle<float> r);
    void paintBrowse (juce::Graphics&, juce::Rectangle<float> r);
    void paintEnvelope (juce::Graphics&, juce::Rectangle<float> r);
    void paintWave (juce::Graphics&, juce::Rectangle<float> r);
    void paintSmoke (juce::Graphics&, juce::Rectangle<float> r);
    juce::Rectangle<float> sideBox() const { return { 378.0f, 54.0f, (float) getWidth() - 384.0f, 92.0f }; }
    bool isSamplePage() const;
    float smokeT = -1.0f; int smokeSeed = 0;
    int dragMarker = -1;   // 0 = start, 1 = end (sample region)
    const void* peaksFor = nullptr; std::vector<std::pair<float, float>> peaks;
    juce::Rectangle<float> tabArea() const;
    juce::Rectangle<float> cellArea (int i) const;
    juce::Rectangle<float> listRow (int i) const;
    static constexpr int kRows = 5;
};
} // namespace thudi

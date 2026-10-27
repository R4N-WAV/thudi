// Thudi — MPC-style pads (play, select, right-click menu)
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class ThudiProcessor;

namespace thudi
{
class PadComponent : public juce::Component
{
public:
    PadComponent (ThudiProcessor& p, int index);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    float lastActivity = 0.0f;
    // backlight 0..1 (follows the envelope of what the pad plays); the editor updates it at 60 Hz
    void setLight (float v, bool gaerro);
    float getLight() const { return light; }
    std::function<void (int)> onSelect;
    std::function<void (int)> onEdit;
    std::function<void (int)> onRename;
    std::function<void (int)> onSavePreset;
    std::function<void()> onChanged;   // any action that changed pad state
    std::function<void()> onSoloMenu;  // right-click while SOLO mode is on
    std::function<void (int)> onLoadSample; // GÆRRO mode: load / chop an audio file from this pad on

private:
    ThudiProcessor& proc;
    const int index;
    bool pressed = false;
    float light = 0.0f;
    bool gaerroLook = false;
    void repaintWithShadow();
    void showMenu();
};
} // namespace thudi

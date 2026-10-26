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
    std::function<void (int)> onSelect;
    std::function<void (int)> onEdit;
    std::function<void (int)> onRename;
    std::function<void (int)> onSavePreset;
    std::function<void()> onChanged;   // any action that changed pad state

private:
    ThudiProcessor& proc;
    const int index;
    bool pressed = false;
    void repaintWithShadow();
    void showMenu();
};
} // namespace thudi

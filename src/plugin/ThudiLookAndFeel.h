#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace thudi
{
namespace colours
{
    const juce::Colour body      { 0xff1c1c1e };
    const juce::Colour bodyLight { 0xff2c2c30 };
    const juce::Colour print     { 0xffc9c6bd };   // silkscreen text on the body
    const juce::Colour printDim  { 0xff8c8a84 };
    const juce::Colour lcdBg     { 0xff1f52d6 };
    const juce::Colour lcdBg2    { 0xff163a9e };
    const juce::Colour lcdInk    { 0xffe8f1ff };
    const juce::Colour lcdDim    { 0xff9db8ff };
    const juce::Colour accentRed { 0xffc8322b };
    const juce::Colour brass     { 0xffc9a65a };
}

class ThudiLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ThudiLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>&, bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu, const juce::String& text, const juce::String& shortcut,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getAlertWindowTitleFont() override;
    juce::Font getAlertWindowMessageFont() override;
    juce::Font getAlertWindowFont() override;
    void drawAlertBox (juce::Graphics&, juce::AlertWindow&, const juce::Rectangle<int>& textArea, juce::TextLayout&) override;
    juce::Slider::SliderLayout getSliderLayout (juce::Slider&) override;
};
} // namespace thudi

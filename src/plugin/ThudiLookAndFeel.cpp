#include "ThudiLookAndFeel.h"
#include "Assets.h"

namespace thudi
{
ThudiLookAndFeel::ThudiLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff17171a));
    setColour (juce::PopupMenu::textColourId, colours::print);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::lcdBg);
    setColour (juce::PopupMenu::highlightedTextColourId, colours::lcdInk);
    setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff1a1a1d));
    setColour (juce::AlertWindow::textColourId, colours::print);
    setColour (juce::AlertWindow::outlineColourId, juce::Colour (0xff3a3a3f));
    setColour (juce::TextEditor::backgroundColourId, colours::lcdBg2);
    setColour (juce::TextEditor::textColourId, colours::lcdInk);
    setColour (juce::TextEditor::highlightColourId, colours::lcdBg.brighter (0.4f));
    setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff3a3a3f));
    setColour (juce::TextEditor::focusedOutlineColourId, colours::lcdDim);
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff9a9a96));
    setColour (juce::TextButton::textColourOffId, juce::Colour (0xff1d1d1f));
    setColour (juce::TextButton::textColourOnId, juce::Colour (0xff1d1d1f));
    setColour (juce::Slider::textBoxTextColourId, colours::lcdInk);
    setColour (juce::Slider::textBoxBackgroundColourId, colours::lcdBg2);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ResizableWindow::backgroundColourId, colours::body);
}

void ThudiLookAndFeel::drawCornerResizer (juce::Graphics& g, int w, int h, bool over, bool dragging)
{
    // a small moulded grip (dots), not the default diagonal lines
    const float a = over || dragging ? 0.45f : 0.18f;
    for (int row = 0; row < 3; ++row)
        for (int col = 2 - row; col < 3; ++col)
        {
            const float x = (float) w - 4.0f - (float) (2 - col) * 4.5f, y = (float) h - 4.0f - (float) row * 4.5f;
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.fillEllipse (x - 1.2f + 0.6f, y - 1.2f + 0.6f, 2.4f, 2.4f);
            g.setColour (colours::print.withAlpha (a));
            g.fillEllipse (x - 1.2f, y - 1.2f, 2.4f, 2.4f);
        }
}

juce::Slider::SliderLayout ThudiLookAndFeel::getSliderLayout (juce::Slider& s)
{
    juce::Slider::SliderLayout l;
    l.sliderBounds = s.getLocalBounds();
    return l;
}

static void drawFallbackKnob (juce::Graphics& g, juce::Rectangle<float> r, bool red)
{
    // skirt
    auto skirt = r.reduced (r.getWidth() * 0.02f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (skirt.translated (0, r.getHeight() * 0.05f));
    juce::ColourGradient sk (juce::Colour (0xff3a3a3e), skirt.getX(), skirt.getY(), juce::Colour (0xff0d0d0f), skirt.getRight(), skirt.getBottom(), false);
    g.setGradientFill (sk); g.fillEllipse (skirt);
    // cap
    auto cap = r.reduced (r.getWidth() * 0.16f);
    juce::Colour c1 = red ? juce::Colour (0xffe0473d) : juce::Colour (0xff4a4a50);
    juce::Colour c2 = red ? juce::Colour (0xff7d1712) : juce::Colour (0xff121214);
    juce::ColourGradient cg (c1, cap.getX() + cap.getWidth() * 0.3f, cap.getY() + cap.getHeight() * 0.2f, c2, cap.getRight(), cap.getBottom(), true);
    g.setGradientFill (cg); g.fillEllipse (cap);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawEllipse (cap.reduced (1.0f), 1.0f);
}

void ThudiLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s)
{
    // component bounds are 1.4x the knob diameter so the printed scale and shadow fit around it
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float d = juce::jmin (r.getWidth(), r.getHeight()) / 1.4f;
    r = r.withSizeKeepingCentre (d, d);
    const bool red = s.getProperties()["knobStyle"].toString() == "red";
    auto img = assets::image (red ? "knob_red.png" : "knob_black.png");
    if (img.isValid())   // renders are framed at 1.25x the knob diameter (room for the shadow)
        g.drawImage (img, r.withSizeKeepingCentre (r.getWidth() * 1.25f, r.getHeight() * 1.25f), juce::RectanglePlacement::stretchToFit);
    else
        drawFallbackKnob (g, r, red);

    // printed scale: 11 silkscreen ticks around the knob (no glowing arcs)
    const float angle = start + pos * (end - start);
    const auto c = r.getCentre();
    const float rad = r.getWidth() * 0.5f;
    if (s.getProperties()["showArc"].toString() != "no")
    {
        g.setColour (colours::print.withAlpha (0.55f));
        for (int k = 0; k <= 10; ++k)
        {
            const float a = start + (end - start) * (float) k / 10.0f;
            const float r0 = rad * 1.10f, r1 = rad * ((k == 0 || k == 5 || k == 10) ? 1.26f : 1.19f);
            g.drawLine ({ c.getPointOnCircumference (r0, a), c.getPointOnCircumference (r1, a) }, (k % 5 == 0) ? 1.3f : 0.9f);
        }
    }
    // pointer line (cream, slightly engraved)
    const float r0 = rad * 0.30f, r1 = rad * 0.66f;
    juce::Line<float> ln (c.getPointOnCircumference (r0, angle), c.getPointOnCircumference (r1, angle));
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawLine (ln.withShortenedStart (0).withShortenedEnd (0), rad * 0.11f);
    g.setColour (red ? juce::Colour (0xfff3e6d8) : juce::Colour (0xffe9e4d6));
    g.drawLine (ln, rad * 0.065f);
}

void ThudiLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat();
    auto img = assets::image (down ? "button_down.png" : "button_up.png");
    if (img.isValid())
    {   // renders carry an 8-unit shadow margin around a 128x32 key
        g.drawImage (img, r.expanded (r.getWidth() * 8.0f / 128.0f, r.getHeight() * 8.0f / 32.0f), juce::RectanglePlacement::stretchToFit);
    }
    else
    {
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (r.translated (0, 2.0f), 4.0f);
        juce::ColourGradient cg (juce::Colour (0xffb5b4ae), r.getX(), r.getY(), juce::Colour (0xff7d7c78), r.getX(), r.getBottom(), false);
        if (down) cg = juce::ColourGradient (juce::Colour (0xff8e8d88), r.getX(), r.getY(), juce::Colour (0xff6c6b67), r.getX(), r.getBottom(), false);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (r.reduced (1.0f).translated (0, down ? 1.0f : 0.0f), 4.0f);
    }
    if (over) { g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRoundedRectangle (r.reduced (2.0f), 4.0f); }
    if (b.getClickingTogglesState())
    {   // status LED set into the key (left side)
        const bool on = b.getToggleState();
        auto led = juce::Rectangle<float> (5.0f, 5.0f).withCentre ({ r.getX() + 10.0f, r.getCentreY() + (down ? 1.0f : 0.0f) });
        const auto onCol = b.getProperties()["ledColour"].isVoid() ? juce::Colour (0xffff3b2a) : juce::Colour::fromString (b.getProperties()["ledColour"].toString());
        if (on)
        {
            g.setGradientFill (juce::ColourGradient (onCol.withAlpha (0.45f), led.getCentreX(), led.getCentreY(), onCol.withAlpha (0.0f), led.getCentreX() + 9.0f, led.getCentreY(), true));
            g.fillEllipse (led.expanded (6.0f));
        }
        g.setColour (juce::Colours::black.withAlpha (0.75f)); g.fillEllipse (led.expanded (1.2f));
        g.setColour (on ? onCol : onCol.darker (0.9f).withMultipliedSaturation (0.5f));
        g.fillEllipse (led);
        g.setColour (juce::Colours::white.withAlpha (on ? 0.55f : 0.12f));
        g.fillEllipse (led.reduced (1.6f).translated (-0.6f, -0.6f));
    }
}

void ThudiLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    g.setFont (assets::labelFont ((float) b.getHeight() * 0.48f));
    g.setColour (juce::Colour (0xff222224).withAlpha (b.isEnabled() ? 0.9f : 0.4f));
    auto area = b.getLocalBounds().translated (0, down ? 1 : 0);
    if (b.getClickingTogglesState()) area = area.withTrimmedLeft (12);
    g.drawFittedText (b.getButtonText(), area, juce::Justification::centred, 1);
}

juce::Font ThudiLookAndFeel::getTextButtonFont (juce::TextButton&, int h) { return assets::labelFont ((float) h * 0.48f); }
juce::Font ThudiLookAndFeel::getPopupMenuFont() { return assets::labelFontMedium (17.0f); }
juce::Font ThudiLookAndFeel::getAlertWindowTitleFont() { return assets::labelFont (20.0f); }
juce::Font ThudiLookAndFeel::getAlertWindowMessageFont() { return assets::labelFontMedium (16.0f); }
juce::Font ThudiLookAndFeel::getAlertWindowFont() { return assets::labelFontMedium (15.0f); }

void ThudiLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (findColour (juce::PopupMenu::backgroundColourId));
    g.setColour (juce::Colour (0xff35353a));
    g.drawRect (0, 0, w, h, 1);
}

void ThudiLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive, bool isHighlighted,
                                          bool isTicked, bool hasSubMenu, const juce::String& text, const juce::String& shortcut,
                                          const juce::Drawable* icon, const juce::Colour* textColour)
{
    if (isSeparator)
    {
        g.setColour (juce::Colour (0xff35353a));
        g.fillRect (area.reduced (8, 0).withHeight (1).withY (area.getCentreY()));
        return;
    }
    juce::LookAndFeel_V4::drawPopupMenuItem (g, area, isSeparator, isActive, isHighlighted, isTicked, hasSubMenu, text, shortcut, icon, textColour);
}

void ThudiLookAndFeel::drawAlertBox (juce::Graphics& g, juce::AlertWindow& w, const juce::Rectangle<int>& textArea, juce::TextLayout& tl)
{
    g.fillAll (findColour (juce::AlertWindow::backgroundColourId));
    g.setColour (findColour (juce::AlertWindow::outlineColourId));
    g.drawRect (w.getLocalBounds(), 1);
    tl.draw (g, textArea.toFloat());
}
} // namespace thudi

#include "LcdDisplay.h"
#include "Assets.h"
#include "Models.h"
#include "ParamFormat.h"
#include "PluginProcessor.h"
#include "ThudiLookAndFeel.h"

namespace thudi
{
LcdDisplay::LcdDisplay (ThudiProcessor& p) : proc (p) {}

int LcdDisplay::numPages() const
{
    return (int) pagesForModel ((int) std::round (proc.padValue (pad, P::Model))).size();
}

void LcdDisplay::refreshEntries()
{
    auto& pm = *proc.presets;
    entries = browse == Browse::Kits ? pm.listKits() : pm.listPads();
    if (browse == Browse::Kits)
    {
        int cur = pm.currentKitIndex();
        if (cur >= 0) browseIndex = cur;
    }
    else
    {   // jump to the category of the selected pad's model
        auto cat = juce::String (modelCategory ((int) std::round (proc.padValue (pad, P::Model))));
        for (size_t i = 0; i < entries.size(); ++i) if (entries[i].category == cat) { browseIndex = (int) i; break; }
    }
    browseIndex = juce::jlimit (0, juce::jmax (0, (int) entries.size() - 1), browseIndex);
    browseTop = juce::jlimit (0, juce::jmax (0, (int) entries.size() - kRows), browseIndex - 2);
    repaint();
}

void LcdDisplay::scrollBrowse (int delta)
{
    if (entries.empty()) return;
    browseIndex = juce::jlimit (0, (int) entries.size() - 1, browseIndex + delta);
    if (browseIndex < browseTop) browseTop = browseIndex;
    if (browseIndex >= browseTop + kRows) browseTop = browseIndex - kRows + 1;
    repaint();
}

juce::StringArray LcdDisplay::softKeyLabels() const
{
    if (mode == Mode::Browse) return { "KITS", "PADS", "LOAD", "EXIT" };
    return { "< PAGE", "PAGE >", "BROWSE", "SAVE" };
}

// ---------------------------------------------------------------- geometry (local, design units 554 x 178)
juce::Rectangle<float> LcdDisplay::tabArea() const { return { 6.0f, 30.0f, getWidth() - 12.0f, 18.0f }; }
juce::Rectangle<float> LcdDisplay::cellArea (int i) const
{
    const float x0 = 6.0f, y0 = 54.0f, cw = 92.0f, ch = 46.0f;
    return { x0 + (float) (i % 4) * cw, y0 + (float) (i / 4) * ch, cw - 4.0f, ch - 4.0f };
}
juce::Rectangle<float> LcdDisplay::listRow (int i) const { return { 6.0f, 30.0f + (float) i * 23.0f, getWidth() - 12.0f, 22.0f }; }

// ---------------------------------------------------------------- painting
void LcdDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    // backlight
    juce::ColourGradient bg (colours::lcdBg.brighter (0.12f), r.getCentreX(), r.getY(), colours::lcdBg2, r.getCentreX(), r.getBottom(), false);
    g.setGradientFill (bg); g.fillRect (r);
    juce::ColourGradient vign (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(), juce::Colours::black.withAlpha (0.25f), r.getX(), r.getY(), true);
    g.setGradientFill (vign); g.fillRect (r);

    g.saveState();
    if (mode == Mode::Edit) paintEdit (g, r); else paintBrowse (g, r);
    g.restoreState();

    // soft-key labels on the bottom line (aligned with the 4 buttons under the LCD)
    auto labels = softKeyLabels();
    g.setFont (assets::lcdFont (19.0f));
    for (int i = 0; i < 4; ++i)
    {
        juce::Rectangle<float> k (2.0f + (float) i * 140.0f, r.getBottom() - 21.0f, 128.0f, 19.0f);
        g.setColour (colours::lcdInk.withAlpha (0.85f));
        g.drawRect (k, 1.0f);
        g.drawText (labels[i], k, juce::Justification::centred);
    }

    // pixel grid + glass
    g.setColour (juce::Colours::black.withAlpha (0.07f));
    for (float x = 0; x < r.getWidth(); x += 3.0f) g.fillRect (x, 0.0f, 0.6f, r.getHeight());
    for (float y = 0; y < r.getHeight(); y += 3.0f) g.fillRect (0.0f, y, r.getWidth(), 0.6f);
    juce::Path glare;
    glare.addQuadrilateral (0, 0, r.getWidth() * 0.55f, 0, r.getWidth() * 0.35f, r.getHeight(), 0, r.getHeight());
    g.setColour (juce::Colours::white.withAlpha (0.035f));
    g.fillPath (glare);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRect (r, 2.0f);
}

static void invText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Justification j, bool inverted)
{
    if (inverted) { g.setColour (colours::lcdInk); g.fillRect (r); g.setColour (colours::lcdBg2); }
    else g.setColour (colours::lcdInk);
    g.drawText (t, r.reduced (3.0f, 0), j, true);
}

void LcdDisplay::paintEdit (juce::Graphics& g, juce::Rectangle<float> r)
{
    const int model = (int) std::round (proc.padValue (pad, P::Model));
    auto pages = pagesForModel (model);
    page = juce::jlimit (0, (int) pages.size() - 1, page);

    // header
    g.setFont (assets::lcdFont (24.0f));
    g.setColour (colours::lcdInk);
    auto head = juce::Rectangle<float> (6.0f, 3.0f, r.getWidth() - 12.0f, 24.0f);
    juce::String left = "PAD " + juce::String (pad + 1).paddedLeft ('0', 2) + "  " + proc.getPadLabel (pad);
    g.drawText (left, head, juce::Justification::centredLeft);
    g.setColour (colours::lcdDim);
    juce::String right = message.isNotEmpty() ? message : (juce::String (modelName (model)).toUpperCase() + "  |  KIT " + proc.getKitName().toUpperCase());
    if (proc.midiLearnTarget() == pad) right = "PLAY A NOTE TO LEARN...";
    g.setColour (message.isNotEmpty() ? colours::lcdInk : colours::lcdDim);
    g.drawText (right, head, juce::Justification::centredRight);

    // tabs
    g.setFont (assets::lcdFont (18.0f));
    auto tabs = tabArea();
    const float tw = tabs.getWidth() / (float) pages.size();
    for (size_t i = 0; i < pages.size(); ++i)
        invText (g, pages[i].name, { tabs.getX() + (float) i * tw, tabs.getY(), tw - 3.0f, tabs.getHeight() }, juce::Justification::centred, (int) i == page);

    // cells
    const auto& knobs = pages[(size_t) page].knobs;
    for (int i = 0; i < 8; ++i)
    {
        auto c = cellArea (i);
        if (i >= (int) knobs.size()) continue;
        const auto& pi = info (knobs[(size_t) i]);
        const bool f = i == focus;
        if (f) { g.setColour (colours::lcdInk); g.fillRect (c); }
        g.setColour (f ? colours::lcdBg2 : colours::lcdDim);
        g.setFont (assets::lcdFont (16.0f));
        g.drawText (juce::String (pi.name).toUpperCase(), c.withHeight (16.0f).reduced (3.0f, 0), juce::Justification::centredLeft, true);
        g.setColour (f ? colours::lcdBg2 : colours::lcdInk);
        g.setFont (assets::lcdFont (23.0f));
        g.drawText (formatValue (pi, proc.padValue (pad, knobs[(size_t) i]), true), c.withTrimmedTop (16.0f).reduced (3.0f, 0), juce::Justification::centredLeft, true);
    }

    paintEnvelope (g, { 378.0f, 54.0f, r.getWidth() - 384.0f, 92.0f });
}

void LcdDisplay::paintEnvelope (juce::Graphics& g, juce::Rectangle<float> r)
{
    const int model = (int) std::round (proc.padValue (pad, P::Model));
    float a, h, d, c; juce::String title;
    auto v = [this] (P p) { return proc.padValue (pad, p); };
    switch ((Model) model)
    {
        case Model::Snare: case Model::Clap: case Model::Perc:
            a = v (P::NoiseAttack); h = 0; d = v (P::NoiseDecay); c = v (P::NoiseCurve); title = "NOISE ENV"; break;
        case Model::ClosedHat: case Model::OpenHat: case Model::Cymbal:
            a = v (P::MetalAttack); h = 0; d = v (P::MetalDecay); c = 0.35f; title = "METAL ENV"; break;
        default:
            a = v (P::ToneAttack); h = v (P::ToneHold); d = v (P::ToneDecay); c = v (P::ToneCurve); title = "AMP ENV"; break;
    }
    g.setColour (colours::lcdInk.withAlpha (0.8f));
    g.drawRect (r, 1.0f);
    g.setFont (assets::lcdFont (15.0f));
    g.setColour (colours::lcdDim);
    const float total = juce::jmax (1.0f, a + h + d * 1.1f);
    g.drawText (title, r.reduced (4.0f, 2.0f), juce::Justification::topLeft);
    g.drawText (total >= 1000.0f ? juce::String (total / 1000.0f, 2) + "s" : juce::String (juce::roundToInt (total)) + "ms",
                r.reduced (4.0f, 2.0f), juce::Justification::topRight);
    auto plot = r.reduced (6.0f, 6.0f).withTrimmedTop (12.0f);
    juce::Path p;
    const int N = 90;
    for (int i = 0; i <= N; ++i)
    {
        const float t = total * (float) i / (float) N;
        float y;
        if (t < a) y = a > 0 ? t / a : 1.0f;
        else if (t < a + h) y = 1.0f;
        else y = decayShape ((t - a - h) / juce::jmax (0.001f, d), c);
        const float px = plot.getX() + plot.getWidth() * (float) i / (float) N;
        const float py = plot.getBottom() - y * plot.getHeight();
        if (i == 0) p.startNewSubPath (px, plot.getBottom()), p.lineTo (px, py); else p.lineTo (px, py);
    }
    g.setColour (colours::lcdInk);
    g.strokePath (p, juce::PathStrokeType (2.0f));
}

void LcdDisplay::paintBrowse (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setFont (assets::lcdFont (24.0f));
    g.setColour (colours::lcdInk);
    auto head = juce::Rectangle<float> (6.0f, 3.0f, r.getWidth() - 12.0f, 24.0f);
    g.drawText (browse == Browse::Kits ? "BROWSE KITS" : "PAD PRESETS -> PAD " + juce::String (pad + 1), head, juce::Justification::centredLeft);
    g.setColour (colours::lcdDim);
    g.drawText (message.isNotEmpty() ? message : juce::String (entries.empty() ? 0 : browseIndex + 1) + "/" + juce::String ((int) entries.size()),
                head, juce::Justification::centredRight);

    g.setFont (assets::lcdFont (21.0f));
    for (int i = 0; i < kRows; ++i)
    {
        const int idx = browseTop + i;
        if (idx >= (int) entries.size()) break;
        const auto& e = entries[(size_t) idx];
        auto row = listRow (i);
        juce::String text = browse == Browse::Pads ? e.category.toUpperCase().paddedRight (' ', 11) + e.name : e.name;
        text << (e.factory ? "" : "   [USER]");
        invText (g, text, row, juce::Justification::centredLeft, idx == browseIndex);
    }
    if (entries.empty())
    {
        g.setColour (colours::lcdDim);
        g.drawText ("NO PRESETS", r, juce::Justification::centred);
    }
}

// ---------------------------------------------------------------- mouse
void LcdDisplay::mouseDown (const juce::MouseEvent& e)
{
    auto pos = e.position;
    if (mode == Mode::Edit)
    {
        auto pages = pagesForModel ((int) std::round (proc.padValue (pad, P::Model)));
        auto tabs = tabArea();
        if (tabs.contains (pos))
        {
            page = juce::jlimit (0, (int) pages.size() - 1, (int) ((pos.x - tabs.getX()) / (tabs.getWidth() / (float) pages.size())));
            if (onPageChanged) onPageChanged (page);
            repaint(); return;
        }
        for (int i = 0; i < 8; ++i)
            if (cellArea (i).contains (pos) && i < (int) pages[(size_t) page].knobs.size())
            {
                focus = i; if (onFocusChanged) onFocusChanged (i); repaint(); return;
            }
    }
    else
    {
        for (int i = 0; i < kRows; ++i)
            if (listRow (i).contains (pos) && browseTop + i < (int) entries.size())
            {
                browseIndex = browseTop + i; repaint(); return;
            }
    }
}

void LcdDisplay::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (mode == Mode::Browse)
        for (int i = 0; i < kRows; ++i)
            if (listRow (i).contains (e.position) && browseTop + i < (int) entries.size())
            {
                browseIndex = browseTop + i;
                if (onLoadSelected) onLoadSelected();
                return;
            }
}

void LcdDisplay::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    if (mode == Mode::Browse) scrollBrowse (w.deltaY > 0 ? -1 : 1);
}
} // namespace thudi

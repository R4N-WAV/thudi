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
    if (mode == Mode::Browse)
        return { browse == Browse::Kits ? "PADS" : "KITS", "LOAD", browse == Browse::Kits ? "SAVE KIT" : (pad == kSoloSlot ? "SAVE SOUND" : "SAVE PAD"), "EXIT" };
    return { "< PAGE", "PAGE >", "BROWSE", "SAVE" };
}

bool LcdDisplay::isSamplePage() const
{
    return mode == Mode::Edit && (int) std::round (proc.padValue (pad, P::Model)) == (int) Model::Sample;
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
    // STN backlight: flat, very slightly uneven (edge-lit from the left)
    juce::ColourGradient bg (colours::lcdBg.brighter (0.05f), r.getX(), r.getCentreY(), colours::lcdBg.darker (0.08f), r.getRight(), r.getCentreY(), false);
    g.setGradientFill (bg); g.fillRect (r);

    if (smoking())
    {
        paintSmoke (g, r);
    }
    else
    {
    g.saveState();
    if (mode == Mode::Edit) paintEdit (g, r); else paintBrowse (g, r);
    g.restoreState();
    }

    // soft-key labels on the bottom line (aligned with the 4 buttons under the LCD)
    auto labels = smoking() ? juce::StringArray { "", "", "", "" } : softKeyLabels();
    g.setFont (assets::lcdFont (19.0f));
    for (int i = 0; i < 4 && ! smoking(); ++i)
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
    // recessed behind the bezel: inner shadow on the top and left edges
    juce::ColourGradient top (juce::Colours::black.withAlpha (0.55f), 0, 0, juce::Colours::transparentBlack, 0, 10.0f, false);
    g.setGradientFill (top); g.fillRect (r.withHeight (10.0f));
    juce::ColourGradient left (juce::Colours::black.withAlpha (0.4f), 0, 0, juce::Colours::transparentBlack, 8.0f, 0, false);
    g.setGradientFill (left); g.fillRect (r.withWidth (8.0f));
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
    juce::String left = pad == kSoloSlot ? "SOLO  " + proc.getPadLabel (pad)
                                         : "PAD " + juce::String (pad + 1).paddedLeft ('0', 2) + "  " + proc.getPadLabel (pad);
    g.drawText (left, head, juce::Justification::centredLeft);
    g.setColour (colours::lcdDim);
    juce::String right = message.isNotEmpty() ? message : (juce::String (modelName (model)).toUpperCase() + "  |  KIT " + proc.getKitName().toUpperCase());
    if (message.isEmpty() && pad == kSoloSlot)
        right = juce::String (proc.padValue (pad, P::Poly) >= 0.5f ? "POLY" : "MONO") + (proc.padValue (pad, P::Glide) > 0.5f ? " GLIDE" : "")
              + "  |  ROOT " + juce::MidiMessage::getMidiNoteName ((int) proc.globalParam (G::KeysRoot)->convertFrom0to1 (proc.globalParam (G::KeysRoot)->getValue()), true, true, 3);
    if (proc.midiLearnTarget() == pad) right = "PLAY A NOTE TO LEARN...";
    g.setColour (message.isNotEmpty() ? colours::lcdInk : colours::lcdDim);
    g.drawText (right, head, juce::Justification::centredRight);

    // tabs
    g.setFont (assets::lcdFont (18.0f));
    auto tabs = tabArea();
    const float tw = tabs.getWidth() / (float) pages.size();
    for (size_t i = 0; i < pages.size(); ++i)
        invText (g, pages[i].name, { tabs.getX() + (float) i * tw, tabs.getY(), tw - 3.0f, tabs.getHeight() }, juce::Justification::centred, (int) i == page);

    if (model == (int) Model::Empty)
    {
        g.setColour (colours::lcdDim);
        g.setFont (assets::lcdFont (26.0f));
        g.drawText ("NO SOUND ON THIS PAD", juce::Rectangle<float> (6.0f, 58.0f, r.getWidth() - 12.0f, 40.0f), juce::Justification::centred);
        g.setFont (assets::lcdFont (19.0f));
        g.drawText (proc.gaerroOn() ? "BROWSE A SOUND, OR DROP AN AUDIO FILE ON THE PADS" : "PRESS BROWSE (OR ENTER) TO PICK A SOUND",
                    juce::Rectangle<float> (6.0f, 100.0f, r.getWidth() - 12.0f, 28.0f), juce::Justification::centred);
        return;
    }

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

    if (model == (int) Model::Sample) paintWave (g, sideBox());
    else paintEnvelope (g, sideBox());
}

void LcdDisplay::paintWave (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (colours::lcdInk.withAlpha (0.8f));
    g.drawRect (r, 1.0f);
    auto smp = proc.getSlotSample (pad);
    g.setFont (assets::lcdFont (15.0f));
    if (! smp || smp->size() == 0)
    {
        g.setColour (colours::lcdDim);
        g.drawText ("DROP AUDIO HERE", r, juce::Justification::centred);
        return;
    }
    auto plot = r.reduced (4.0f, 4.0f).withTrimmedTop (12.0f);
    const int bins = juce::jmax (8, (int) plot.getWidth());
    if (peaksFor != smp.get() || (int) peaks.size() != bins)
    {
        peaksFor = smp.get();
        peaks.assign ((size_t) bins, { 0.0f, 0.0f });
        const size_t n = smp->size();
        for (int b = 0; b < bins; ++b)
        {
            const size_t a = n * (size_t) b / (size_t) bins, e = juce::jmax (a + 1, n * (size_t) (b + 1) / (size_t) bins);
            float lo = 0, hi = 0;
            for (size_t k = a; k < e && k < n; k += 1 + (e - a) / 256) { lo = juce::jmin (lo, smp->data[k]); hi = juce::jmax (hi, smp->data[k]); }
            peaks[(size_t) b] = { lo, hi };
        }
    }
    const float s0 = proc.padValue (pad, P::SmpStart), s1 = proc.padValue (pad, P::SmpEnd);
    const float xa = plot.getX() + plot.getWidth() * juce::jmin (s0, s1), xb = plot.getX() + plot.getWidth() * juce::jmax (s0, s1);
    // selected region shown inverted, like an LCD selection
    g.setColour (colours::lcdInk.withAlpha (0.22f));
    g.fillRect (juce::Rectangle<float> (xa, plot.getY(), juce::jmax (1.0f, xb - xa), plot.getHeight()));
    const float mid = plot.getCentreY(), hh = plot.getHeight() * 0.5f;
    for (int b = 0; b < bins; ++b)
    {
        const float x = plot.getX() + (float) b;
        const bool in = x >= xa && x <= xb;
        g.setColour (in ? colours::lcdInk : colours::lcdDim.withAlpha (0.55f));
        g.fillRect (x, mid - peaks[(size_t) b].second * hh, 1.0f, juce::jmax (1.0f, (peaks[(size_t) b].second - peaks[(size_t) b].first) * hh));
    }
    g.setColour (colours::lcdInk);
    for (float x : { xa, xb })
    {
        g.fillRect (x - 1.0f, plot.getY() - 2.0f, 2.0f, plot.getHeight() + 4.0f);
        juce::Path tri; tri.addTriangle (x - 4.0f, plot.getY() - 3.0f, x + 4.0f, plot.getY() - 3.0f, x, plot.getY() + 3.0f);
        g.fillPath (tri);
    }
    g.setColour (colours::lcdDim);
    juce::String nm (smp->name); nm = nm.toUpperCase().substring (0, 14);
    const double secs = (double) smp->size() / smp->rate * (double) std::abs (s1 - s0);
    g.drawText (nm, r.reduced (4.0f, 1.0f), juce::Justification::topLeft, true);
    g.drawText (juce::String (secs, 2) + "s", r.reduced (4.0f, 1.0f), juce::Justification::topRight);
}

// --------------------------------------------------------------- GÆRRO loading screen: a pixel cigarette, smoked fast
void LcdDisplay::paintSmoke (juce::Graphics& g, juce::Rectangle<float> r)
{
    const float t = smokeT, T = kSmokeLength;
    const float px = 3.0f;    // one LCD dot (matches the pixel grid)
    auto dot = [&] (float x, float y, juce::Colour c) { g.setColour (c); g.fillRect (std::floor (x / px) * px, std::floor (y / px) * px, px, px); };
    auto block = [&] (float x, float y, float w, float h, juce::Colour c)
    {
        g.setColour (c);
        g.fillRect (std::floor (x / px) * px, std::floor (y / px) * px, std::floor (w / px) * px, std::floor (h / px) * px);
    };
    // three quick drags: the ember flares on each pull, the paper burns down, ash drops
    const float burn = juce::jlimit (0.0f, 1.0f, (t - 0.15f) / (T - 0.75f));
    const float pull = std::pow (juce::jmax (0.0f, std::sin (juce::MathConstants<float>::pi * std::fmod (t * 3.0f / (T - 0.6f), 1.0f))), 3.0f);
    const float cy = 92.0f, h = 18.0f;
    const float x0 = 64.0f, filterW = 66.0f, fullLen = 330.0f;
    const float paperEnd = x0 + filterW + (fullLen - filterW) * (1.0f - 0.85f * burn);
    // filter: dotted cork pattern
    block (x0, cy - h / 2, filterW, h, colours::lcdDim);
    for (float x = x0; x < x0 + filterW; x += px * 2)
        for (float y = cy - h / 2 + (std::fmod (x, px * 4) < px * 2 ? 0.0f : px); y < cy + h / 2; y += px * 2) dot (x, y, colours::lcdBg2);
    block (x0 + filterW - px, cy - h / 2, px, h, colours::lcdBg2);
    // paper
    block (x0 + filterW, cy - h / 2, paperEnd - x0 - filterW, h, colours::lcdInk);
    block (x0 + filterW, cy + h / 2 - px, paperEnd - x0 - filterW, px, colours::lcdDim);
    // ash + ember
    const float ashLen = 6.0f + 10.0f * std::fmod (burn * 3.0f, 1.0f);
    block (paperEnd, cy - h / 2 + px, ashLen, h - 2 * px, colours::lcdDim.withAlpha (0.7f));
    for (float y = cy - h / 2 + px; y < cy + h / 2 - px; y += px)
        if (((int) (y / px) + smokeSeed) % 3 == 0) dot (paperEnd + ashLen - px, y, colours::lcdBg2);
    const float glow = 0.55f + 0.45f * pull;
    block (paperEnd - px, cy - h / 2 + px, px * 2, h - 2 * px, colours::lcdInk.withAlpha (glow));
    if (pull > 0.25f)
    {
        g.setColour (colours::lcdInk.withAlpha (0.18f * pull));
        g.fillEllipse (paperEnd - 14.0f, cy - 16.0f, 30.0f, 32.0f);
    }
    // falling ash bits
    juce::Random rnd (smokeSeed);
    for (int i = 0; i < 6; ++i)
    {
        const float st = rnd.nextFloat() * (T - 0.8f), age = t - st;
        if (age < 0.0f || age > 0.6f) continue;
        dot (x0 + filterW + (fullLen - filterW) * (1.0f - 0.85f * juce::jlimit (0.0f, 1.0f, (st - 0.15f) / (T - 0.75f))) + 4.0f,
             cy + h / 2 + age * age * 160.0f, colours::lcdDim);
    }
    // smoke: dots rising from the ember, curling with a slow sine; thicker clouds after each pull (exhale)
    for (int i = 0; i < 90; ++i)
    {
        const float born = rnd.nextFloat() * T, life = 0.9f + rnd.nextFloat() * 0.8f, age = t - born;
        if (age < 0.0f || age > life) continue;
        const float k = age / life;
        const bool exhale = rnd.nextFloat() < 0.4f;
        const float sx = exhale ? x0 - 10.0f : paperEnd + 2.0f;
        const float x = sx + (exhale ? -40.0f * k : 18.0f * k) + 10.0f * std::sin (born * 9.0f + age * 5.0f) * k;
        const float y = (exhale ? cy - 2.0f : cy - h / 2) - age * (exhale ? 26.0f : 60.0f) - (exhale ? 30.0f * k * k : 0.0f);
        if (y < 30.0f) continue;
        const auto c = colours::lcdInk.withAlpha (0.85f * (1.0f - k));
        dot (x, y, c);
        if (k > 0.35f) { dot (x + px, y, c.withMultipliedAlpha (0.6f)); dot (x, y - px, c.withMultipliedAlpha (0.5f)); }
        if (exhale && k > 0.2f) { dot (x - px, y + px, c.withMultipliedAlpha (0.5f)); dot (x + px * 2, y - px, c.withMultipliedAlpha (0.4f)); }
    }
    // text + load bar
    g.setFont (assets::lcdFont (26.0f));
    g.setColour (colours::lcdInk);
    g.drawText (juce::CharPointer_UTF8 ("G\xc3\x86RRO MODE"), juce::Rectangle<float> (6.0f, 4.0f, r.getWidth() - 12.0f, 26.0f), juce::Justification::centredLeft);
    g.setFont (assets::lcdFont (19.0f));
    g.setColour (colours::lcdDim);
    const char* lines[] = { "ROLLING...", "LIGHTING UP...", "DIGGING CRATES...", "WARMING THE NEEDLE...", "READY." };
    g.drawText (lines[juce::jlimit (0, 4, (int) (t / T * 5.0f))], juce::Rectangle<float> (6.0f, 4.0f, r.getWidth() - 12.0f, 26.0f), juce::Justification::centredRight);
    auto bar = juce::Rectangle<float> (64.0f, 140.0f, 330.0f, 12.0f);
    g.setColour (colours::lcdInk.withAlpha (0.8f));
    g.drawRect (bar, 1.0f);
    const float prog = juce::jlimit (0.0f, 1.0f, t / (T - 0.3f));
    for (float x = bar.getX() + 3.0f; x < bar.getX() + 3.0f + (bar.getWidth() - 6.0f) * prog; x += 6.0f)
        g.fillRect (x, bar.getY() + 3.0f, 4.0f, bar.getHeight() - 6.0f);
    g.setColour (colours::lcdDim);
    g.drawText (juce::String (juce::roundToInt (prog * 100.0f)) + "%", juce::Rectangle<float> (bar.getRight() + 8.0f, bar.getY() - 4.0f, 60.0f, 20.0f), juce::Justification::centredLeft);
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
    g.drawText (browse == Browse::Kits ? "BROWSE KITS" : (pad == kSoloSlot ? juce::String ("SOUND LIBRARY -> SOLO") : "PAD PRESETS -> PAD " + juce::String (pad + 1)),
                head, juce::Justification::centredLeft);
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
    dragMarker = -1;
    if (smoking()) return;
    if (isSamplePage() && sideBox().contains (pos) && proc.getSlotSample (pad))
    {
        auto plot = sideBox().reduced (4.0f, 4.0f).withTrimmedTop (12.0f);
        const float s0 = proc.padValue (pad, P::SmpStart), s1 = proc.padValue (pad, P::SmpEnd);
        const float x = (pos.x - plot.getX()) / plot.getWidth();
        dragMarker = std::abs (x - s0) <= std::abs (x - s1) ? 0 : 1;
        if (auto* prm = proc.padParam (pad, dragMarker == 0 ? P::SmpStart : P::SmpEnd)) prm->beginChangeGesture();
        mouseDrag (e);
        return;
    }
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

void LcdDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (dragMarker < 0) return;
    auto plot = sideBox().reduced (4.0f, 4.0f).withTrimmedTop (12.0f);
    const float x = juce::jlimit (0.0f, 1.0f, (e.position.x - plot.getX()) / plot.getWidth());
    const P p = dragMarker == 0 ? P::SmpStart : P::SmpEnd;
    if (auto* prm = proc.padParam (pad, p)) prm->setValueNotifyingHost (prm->convertTo0to1 (x));
    repaint();
}

void LcdDisplay::mouseUp (const juce::MouseEvent&)
{
    if (dragMarker >= 0)
        if (auto* prm = proc.padParam (pad, dragMarker == 0 ? P::SmpStart : P::SmpEnd)) prm->endChangeGesture();
    dragMarker = -1;
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

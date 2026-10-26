#include "PadGrid.h"
#include "Assets.h"
#include "PluginProcessor.h"
#include "PresetManager.h"
#include "ThudiLookAndFeel.h"
#include "ThudiParams.h"

namespace thudi
{
PadComponent::PadComponent (ThudiProcessor& p, int i) : proc (p), index (i)
{
    setRepaintsOnMouseActivity (false);
}

void PadComponent::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto img = assets::image (pressed ? "pad_down.png" : "pad_up.png");
    if (img.isValid())
    {
        // renders are framed at 1.2x the pad size (10% margin on each side for the cast shadow)
        g.drawImage (img, r.expanded (r.getWidth() * 0.1f), juce::RectanglePlacement::stretchToFit);
        return;
    }
    // fallback (no renders embedded): square rubber pad with a soft cast shadow
    auto pr = r.reduced (1.0f).translated (0, pressed ? 1.5f : 0.0f);
    for (int k = 6; k >= 1; --k)
    {
        g.setColour (juce::Colours::black.withAlpha (0.07f));
        g.fillRoundedRectangle (pr.translated ((float) k * 0.6f, (float) k * 1.1f).expanded ((float) k * 0.5f), 4.0f);
    }
    juce::ColourGradient cg (juce::Colour (pressed ? 0xff6b6a66 : 0xff7f7e7a), pr.getX(), pr.getY(),
                             juce::Colour (pressed ? 0xff5a5955 : 0xff676662), pr.getRight(), pr.getBottom(), false);
    g.setGradientFill (cg);
    g.fillRoundedRectangle (pr, 3.5f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawLine (pr.getX() + 3, pr.getY() + 0.8f, pr.getRight() - 3, pr.getY() + 0.8f, 1.2f);
}

void PadComponent::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) { if (onSelect) onSelect (index); showMenu(); return; }
    // velocity from vertical position: top edge = hardest hit
    const float v = juce::jlimit (0.25f, 1.0f, 1.05f - 0.75f * (float) e.y / (float) juce::jmax (1, getHeight()));
    pressed = true; repaint();
    proc.hitPad (index, v);
    if (onSelect) onSelect (index);
}

void PadComponent::mouseUp (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) return;
    pressed = false; repaint();
    proc.releasePadFromUi (index);
}

void PadComponent::showMenu()
{
    auto& pm = *proc.presets;
    juce::PopupMenu m;
    const auto label = proc.getPadLabel (index);
    m.addSectionHeader ("PAD " + juce::String (index + 1) + "  " + label);
    m.addItem ("Edit sound", [this] { if (onEdit) onEdit (index); });
    m.addItem ("Rename...", [this] { if (onRename) onRename (index); });
    m.addSeparator();
    m.addItem ("Copy pad", [this] { proc.presets->copyPad (index); });
    m.addItem ("Paste pad", pm.canPaste(), false, [this] { proc.presets->pastePad (index, true); if (onChanged) onChanged(); });
    m.addItem ("Paste sound only (keep name)", pm.canPaste(), false, [this] { proc.presets->pastePad (index, false); if (onChanged) onChanged(); });

    juce::PopupMenu swap, dup;
    for (int i = 0; i < kNumPads; ++i)
    {
        if (i == index) continue;
        auto name = "Pad " + juce::String (i + 1) + "  " + proc.getPadLabel (i);
        swap.addItem (name, [this, i] { proc.presets->swapPads (index, i); if (onChanged) onChanged(); });
        dup.addItem (name, [this, i] { proc.presets->duplicatePad (index, i); if (onChanged) onChanged(); });
    }
    m.addSubMenu ("Swap with", swap);
    m.addSubMenu ("Copy to", dup);
    m.addSeparator();

    const int model = (int) std::round (proc.padValue (index, P::Model));
    juce::PopupMenu models, init;
    for (int k = 0; k < (int) Model::Count; ++k)
    {
        models.addItem (modelName (k), true, k == model, [this, k] { proc.setPadValue (index, P::Model, (float) k); if (onChanged) onChanged(); });
        init.addItem (modelName (k), [this, k] { proc.presets->initPad (index, k); proc.setPadLabel (index, juce::String (modelName (k)).toUpperCase()); if (onChanged) onChanged(); });
    }
    m.addSubMenu ("Model (editor pages)", models);
    m.addSubMenu ("Init as", init);

    juce::PopupMenu load;
    auto entries = pm.listPads();
    juce::String currentCat;
    juce::PopupMenu catMenu;
    auto flush = [&] { if (currentCat.isNotEmpty()) load.addSubMenu (currentCat, catMenu); catMenu = {}; };
    for (auto& e : entries)
    {
        if (e.category != currentCat) { flush(); currentCat = e.category; }
        catMenu.addItem ((e.factory ? "" : "* ") + e.name, [this, e] { proc.presets->loadPad (index, e); if (onChanged) onChanged(); });
    }
    flush();
    m.addSubMenu ("Load pad preset", load, ! entries.empty());
    m.addItem ("Save pad preset...", [this] { if (onSavePreset) onSavePreset (index); });
    m.addItem ("Randomize sound", [this] { proc.presets->randomizePad (index, 0.25f); if (onChanged) onChanged(); });
    m.addSeparator();

    juce::PopupMenu choke, output, link, mode;
    const int ch = (int) std::round (proc.padValue (index, P::Choke));
    for (int k = 0; k <= 8; ++k)
        choke.addItem (k == 0 ? "Off" : "Group " + juce::String (k), true, ch == k, [this, k] { proc.setPadValue (index, P::Choke, (float) k); });
    const int out = (int) std::round (proc.padValue (index, P::Output));
    for (int k = 0; k < kNumOutputs; ++k)
        output.addItem (k == 0 ? "Main (1-2)" : "Out " + juce::String (k + 1), true, out == k, [this, k] { proc.setPadValue (index, P::Output, (float) k); });
    const int ln = (int) std::round (proc.padValue (index, P::Link));
    link.addItem ("None", true, ln == 0, [this] { proc.setPadValue (index, P::Link, 0.0f); });
    for (int k = 1; k <= kNumPads; ++k)
        if (k - 1 != index)
            link.addItem ("Pad " + juce::String (k) + "  " + proc.getPadLabel (k - 1), true, ln == k, [this, k] { proc.setPadValue (index, P::Link, (float) k); });
    const int pmode = (int) std::round (proc.padValue (index, P::PlayMode));
    mode.addItem ("One-shot", true, pmode == 0, [this] { proc.setPadValue (index, P::PlayMode, 0.0f); });
    mode.addItem ("Gate (note length)", true, pmode == 1, [this] { proc.setPadValue (index, P::PlayMode, 1.0f); });
    m.addSubMenu ("Choke group", choke);
    m.addSubMenu ("Output", output);
    m.addSubMenu ("Link (trigger together)", link);
    m.addSubMenu ("Play mode", mode);

    const int keysPad = (int) std::round (proc.globalParam (G::KeysPad)->convertFrom0to1 (proc.globalParam (G::KeysPad)->getValue()));
    m.addItem ("Play chromatically from keyboard", true, keysPad == index + 1, [this, keysPad]
    {
        proc.setGlobalValue (G::KeysPad, keysPad == index + 1 ? 0.0f : (float) (index + 1));
        if (onChanged) onChanged();
    });
    m.addSeparator();
    const auto noteName = juce::MidiMessage::getMidiNoteName (proc.getPadNote (index), true, true, 3);
    m.addItem ("MIDI learn note (now " + noteName + ")", [this] { proc.startMidiLearn (index); if (onChanged) onChanged(); });
    m.addItem ("Reset note to default", [this] { proc.setPadNote (index, 36 + index); });

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this));
}
} // namespace thudi

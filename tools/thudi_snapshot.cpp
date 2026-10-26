// thudi_snapshot — headless checks of the plugin (processor + editor) and GUI screenshots.
// usage: thudi_snapshot <outdir> [pattern.txt]
#include <juce_audio_formats/juce_audio_formats.h>
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "PresetManager.h"
#include "Assets.h"

using namespace thudi;

static int failures = 0;
static void check (bool ok, const juce::String& what)
{
    std::cout << (ok ? "  [ok]   " : "  [FAIL] ") << what << std::endl;
    if (! ok) ++failures;
}

static float renderNote (ThudiProcessor& p, int note, float seconds, float* peakOut = nullptr)
{
    const int block = 512;
    juce::AudioBuffer<float> buf (p.getTotalNumOutputChannels(), block);
    float peak = 0; bool finite = true;
    const int blocks = (int) (seconds * 48000.0f / block);
    for (int b = 0; b < blocks; ++b)
    {
        juce::MidiBuffer midi;
        if (b == 0) midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 120), 0);
        if (b == 4) midi.addEvent (juce::MidiMessage::noteOff (1, note), 0);
        p.processBlock (buf, midi);
        for (int c = 0; c < buf.getNumChannels(); ++c)
            for (int i = 0; i < block; ++i)
            {
                float v = buf.getSample (c, i);
                if (! std::isfinite (v)) finite = false;
                peak = juce::jmax (peak, std::abs (v));
            }
    }
    if (peakOut) *peakOut = peak;
    return finite ? peak : -1.0f;
}

static void snapshot (juce::Component& c, const juce::File& f, float scale)
{
    auto img = c.createComponentSnapshot (c.getLocalBounds(), true, scale);
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::cout << "  wrote " << f.getFullPathName() << " (" << img.getWidth() << "x" << img.getHeight() << ")" << std::endl;
}

// ---------------------------------------------------------------- setup-wizard artwork (like Anbu's)
static void writePng (const juce::Image& img, const juce::File& f)
{
    f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
}

static juce::Image iconImage (int px)
{
    juce::Image img (juce::Image::ARGB, px, px, true);
    juce::Graphics g (img);
    const float w = (float) px;
    auto disc = juce::Rectangle<float> (w, w).reduced (w * 0.03f);
    juce::ColourGradient body (juce::Colour (0xff34343a), disc.getX(), disc.getY(), juce::Colour (0xff141416), disc.getRight(), disc.getBottom(), false);
    g.setGradientFill (body); g.fillEllipse (disc);
    g.setColour (juce::Colour (0xff4a4a50)); g.drawEllipse (disc.reduced (w * 0.01f), juce::jmax (1.0f, w * 0.02f));
    // Tamil "thu" (first letter of துடி) in brushed silver
    juce::Path p;
    if (auto tf = thudi::assets::typeface ("Kavivanar-Regular.ttf"))
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText (juce::Font (juce::FontOptions (tf).withHeight (100.0f)), juce::String (juce::CharPointer_UTF8 ("\xe0\xae\xa4\xe0\xaf\x81")), 0, 0);
        ga.createPath (p);
    }
    auto target = disc.reduced (w * 0.22f).translated (-w * 0.02f, w * 0.0f);
    p.applyTransform (p.getTransformToScaleToFit (target, true));
    g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillPath (p, juce::AffineTransform::translation (w * 0.01f, w * 0.015f));
    juce::ColourGradient silver (juce::Colour (0xffeeebe3), target.getX(), target.getY(), juce::Colour (0xff9a978f), target.getRight(), target.getBottom(), false);
    g.setGradientFill (silver); g.fillPath (p);
    // red pad LED
    auto led = juce::Rectangle<float> (w * 0.12f, w * 0.12f).withCentre ({ w * 0.76f, w * 0.26f });
    g.setColour (juce::Colour (0x66ff3b2a)); g.fillEllipse (led.expanded (w * 0.04f));
    g.setColour (juce::Colour (0xffff3b2a)); g.fillEllipse (led);
    return img;
}

static void writeIco (const juce::File& f)
{
    const int sizes[] = { 16, 24, 32, 48, 64, 128, 256 };
    std::vector<juce::MemoryBlock> pngs;
    for (int s : sizes) { juce::MemoryOutputStream ms; juce::PNGImageFormat().writeImageToStream (iconImage (s), ms); pngs.push_back (ms.getMemoryBlock()); }
    juce::MemoryOutputStream o;
    o.writeShort (0); o.writeShort (1); o.writeShort ((short) pngs.size());
    int offset = 6 + 16 * (int) pngs.size();
    for (size_t i = 0; i < pngs.size(); ++i)
    {
        const int s = sizes[i];
        o.writeByte ((char) (s >= 256 ? 0 : s)); o.writeByte ((char) (s >= 256 ? 0 : s)); o.writeByte (0); o.writeByte (0);
        o.writeShort (1); o.writeShort (32); o.writeInt ((int) pngs[i].getSize()); o.writeInt (offset);
        offset += (int) pngs[i].getSize();
    }
    for (auto& m : pngs) o.write (m.getData(), m.getSize());
    f.deleteFile(); f.replaceWithData (o.getData(), o.getDataSize());
}

static int makeWizardArt (const juce::File& dir)
{
    auto proc = std::make_unique<ThudiProcessor>();
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc->createEditor());
    ed->setSize (940, 1210);
    if (auto* te = dynamic_cast<ThudiEditor*> (ed.get())) te->selectPad (0);
    const float sc = 2.5f;
    auto panel = ed->createComponentSnapshot (ed->getLocalBounds(), true, sc);
    const juce::Colour bg (0xff161618), red (0xffff3b2a), print (0xffc9c6bd), dim (0xff8c8a84);

    // tall side image 492 x 942: the துடி emblem and the first pad column, fading into the chassis
    {
        const int W = 492, H = 942;
        juce::Image img (juce::Image::RGB, W, H, true);
        juce::Graphics g (img);
        g.fillAll (bg);
        const float x0 = 22.0f, x1 = 302.0f, y0 = 592.0f;
        const float k = (float) W / (x1 - x0);
        const float y1 = y0 + 0.80f * (float) H / k;
        auto src = juce::Rectangle<int> ((int) (x0 * sc), (int) (y0 * sc), (int) ((x1 - x0) * sc), (int) ((y1 - y0) * sc));
        g.drawImage (panel, 0, 0, W, (int) ((y1 - y0) * k), src.getX(), src.getY(), src.getWidth(), src.getHeight());
        const float top = (y1 - y0) * k * 0.55f, bot = (y1 - y0) * k;
        juce::ColourGradient fade (bg.withAlpha (0.0f), 0, top, bg, 0, bot, false);
        g.setGradientFill (fade); g.fillRect (0.0f, top, (float) W, (float) H - top);
        juce::ColourGradient glow (red.withAlpha (0.10f), W * 0.5f, (float) H, red.withAlpha (0.0f), W * 0.5f, H * 0.62f, true);
        g.setGradientFill (glow); g.fillRect (0, 0, W, H);
        auto line = [&] (const juce::String& t, float y, juce::Font f, juce::Colour c)
        { g.setFont (f); g.setColour (c); g.drawText (t, juce::Rectangle<float> (0, y, (float) W, f.getHeight() * 1.3f), juce::Justification::centred); };
        line ("D R U M   S Y N T H E S I Z E R", H * 0.845f, thudi::assets::labelFont (25.0f), print);
        line (juce::String (juce::CharPointer_UTF8 ("Thudi v1  \xc2\xb7  by R4N")), H * 0.895f, thudi::assets::labelFontMedium (28.0f), dim);
        line (juce::String (juce::CharPointer_UTF8 ("V S T 3  \xc2\xb7  6 4 - B I T")), H * 0.94f, thudi::assets::labelFont (21.0f), dim.darker (0.35f));
        writePng (img, dir.getChildFile ("wizard.png"));
    }
    // corner badge 174 x 174: the red MASTER knob in a round window
    {
        const int px = 174; const float cx = 100.0f, cy = 118.0f, r = 70.0f;
        juce::Image img (juce::Image::RGB, px, px, true);
        juce::Graphics g (img);
        g.fillAll (bg);
        juce::Path clip; clip.addEllipse (2.0f, 2.0f, px - 4.0f, px - 4.0f);
        g.reduceClipRegion (clip);
        g.drawImage (panel, 0, 0, px, px, (int) ((cx - r) * sc), (int) ((cy - r) * sc), (int) (2 * r * sc), (int) (2 * r * sc));
        writePng (img, dir.getChildFile ("wizard-small.png"));
    }
    writeIco (dir.getChildFile ("thudi.ico"));
    writePng (iconImage (256), dir.getChildFile ("thudi-icon-256.png"));
    std::cout << "wizard art written to " << dir.getFullPathName() << std::endl;
    ed.reset();
    return 0;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc > 2 && juce::String (argv[1]) == "--wizard-art")
        return makeWizardArt (juce::File (juce::String (argv[2])));
    juce::File out (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    out.createDirectory();

    std::cout << "Thudi headless checks" << std::endl;
    auto proc = std::make_unique<ThudiProcessor>();
    proc->prepareToPlay (48000.0, 512);

    // ---------- presets
    auto kits = proc->presets->listKits();
    check (! kits.empty(), "factory kits embedded (" + juce::String ((int) kits.size()) + ")");
    check (proc->getKitName() == "RHYNO", "default kit is RHYNO (got " + proc->getKitName() + ")");
    auto padPresets = proc->presets->listPads();
    check (padPresets.size() >= 16, "factory pad presets embedded (" + juce::String ((int) padPresets.size()) + ")");

    // ---------- every pad of every kit makes sound
    for (auto& k : kits)
    {
        proc->presets->loadKit (k);
        int silent = 0; float maxPeak = 0;
        for (int pad = 0; pad < kNumPads; ++pad)
        {
            proc->releaseResources(); proc->prepareToPlay (48000.0, 512);
            float peak = 0;
            float r = renderNote (*proc, proc->getPadNote (pad), 1.5f, &peak);
            if (r < 0) check (false, k.name + " pad " + juce::String (pad + 1) + " produced NaN/inf");
            if (peak < 1e-3f) ++silent;
            maxPeak = juce::jmax (maxPeak, peak);
        }
        check (silent == 0, "kit " + k.name + ": all 16 pads sound (silent: " + juce::String (silent) + ", max peak " + juce::String (maxPeak, 2) + ")");
        check (maxPeak < 4.0f, "kit " + k.name + ": no runaway levels");
    }
    proc->presets->loadDefaultKit();

    // ---------- pad actions
    {
        auto& pm = *proc->presets;
        pm.copyPad (0);
        pm.pastePad (15, true);
        auto a = proc->getPadSound (0), b = proc->getPadSound (15);
        bool same = true;
        for (int k = 0; k < kNumPadParams; ++k)
            if (k != (int) P::Output && k != (int) P::Link && std::abs (a.v[(size_t) k] - b.v[(size_t) k]) > 1e-4f) same = false;
        check (same && proc->getPadLabel (15) == proc->getPadLabel (0), "copy / paste pad");
        auto l2 = proc->getPadLabel (2), l3 = proc->getPadLabel (3);
        pm.swapPads (2, 3);
        check (proc->getPadLabel (2) == l3 && proc->getPadLabel (3) == l2, "swap pads");
        pm.swapPads (2, 3);
        proc->setPadLabel (5, "Test Name");
        check (proc->getPadLabel (5) == "TEST NAME", "rename pad");
        check (pm.savePad (1, "ThudiSelfTest 808"), "save pad preset");
        auto list = pm.listPads ("808");
        bool found = false;
        for (auto& e : list) if (e.name == "ThudiSelfTest 808" && ! e.factory) { found = pm.loadPad (9, e); e.file.deleteFile(); }
        check (found, "load saved pad preset");
        check (pm.saveKit ("ThudiSelfTest Kit"), "save kit");
        bool kitFound = false;
        for (auto& e : pm.listKits()) if (e.name == "ThudiSelfTest Kit") { kitFound = pm.loadKit (e); e.file.deleteFile(); }
        check (kitFound, "load saved kit");
        pm.initPad (12, (int) Model::Zap);
        check ((int) std::round (proc->padValue (12, P::Model)) == (int) Model::Zap, "init pad as model");
        pm.randomizePad (12);
        check (renderNote (*proc, proc->getPadNote (12), 1.0f) >= 0.0f, "randomized pad renders finite audio");
    }

    // ---------- state round trip
    {
        proc->presets->loadDefaultKit();
        proc->setPadLabel (7, "ROUNDTRIP");
        proc->setPadNote (7, 60);
        juce::MemoryBlock mb; proc->getStateInformation (mb);
        auto p2 = std::make_unique<ThudiProcessor>();
        p2->setStateInformation (mb.getData(), (int) mb.getSize());
        bool same = true;
        for (int pad = 0; pad < kNumPads; ++pad)
            for (int k = 0; k < kNumPadParams; ++k)
                if (std::abs (proc->padValue (pad, (P) k) - p2->padValue (pad, (P) k)) > 1e-4f) same = false;
        check (same, "state restores all pad parameters");
        check (p2->getPadLabel (7) == "ROUNDTRIP" && p2->getPadNote (7) == 60, "state restores labels and notes");
        proc->presets->loadDefaultKit();
    }

    // ---------- plugin-path render of the test pattern (compare with thudi_render in CI)
    if (argc > 2)
    {
        proc->presets->loadDefaultKit();
        out.getChildFile ("plugin_loaded_rhyno.thudikit").replaceWithText (proc->presets->kitToJson());
        juce::StringArray lines; lines.addLines (juce::File (juce::String (argv[2])).loadFileAsString());
        double bpm = 88; int steps = 32;
        struct Ev { int sample; int note; bool on; float vel; };
        std::vector<Ev> evs;
        for (auto l : lines)
        {
            l = l.trim(); if (l.isEmpty() || l.startsWith ("#")) continue;
            auto t = juce::StringArray::fromTokens (l, " ", "");
            t.removeEmptyStrings();
            if (t[0] == "bpm") { bpm = t[1].getDoubleValue(); continue; }
            if (t[0] == "steps") { steps = t[1].getIntValue(); continue; }
            const double stepSec = 60.0 / bpm / 4.0;
            const int pad = juce::jlimit (1, 16, t[1].getIntValue());
            const double len = t.size() > 3 ? t[3].getDoubleValue() : 0.5;
            const int note = proc->getPadNote (pad - 1);
            evs.push_back ({ (int) (t[0].getDoubleValue() * stepSec * 48000.0), note, true, (float) t[2].getDoubleValue() });
            evs.push_back ({ (int) ((t[0].getDoubleValue() + len) * stepSec * 48000.0), note, false, 0.0f });
        }
        proc->releaseResources(); proc->prepareToPlay (48000.0, 512);
        const int total = (int) ((steps * 60.0 / bpm / 4.0 + 1.0) * 48000.0);
        juce::AudioBuffer<float> all (2, total);
        juce::AudioBuffer<float> buf (proc->getTotalNumOutputChannels(), 512);
        for (int pos = 0; pos < total; pos += 512)
        {
            juce::MidiBuffer midi;
            for (auto& e : evs)
                if (e.sample >= pos && e.sample < pos + 512)
                    midi.addEvent (e.on ? juce::MidiMessage::noteOn (1, e.note, e.vel) : juce::MidiMessage::noteOff (1, e.note), e.sample - pos);
            proc->processBlock (buf, midi);
            const int n = juce::jmin (512, total - pos);
            all.copyFrom (0, pos, buf, 0, 0, n); all.copyFrom (1, pos, buf, 1, 0, n);
        }
        const float peakAll = all.getMagnitude (0, total);
        auto wavFile = out.getChildFile ("plugin_rhyno_pattern.wav");
        juce::AudioBuffer<float> norm (all); if (peakAll > 0) norm.applyGain (0.9f / peakAll); // no clipping in the 24-bit test file
        wavFile.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (wavFile), 48000.0, 2, 24, {}, 0));
        if (w) w->writeFromAudioSampleBuffer (norm, 0, total);
        check (all.getMagnitude (0, total) > 0.05f, "plugin renders the RHYNO pattern (peak " + juce::String (all.getMagnitude (0, total), 2) + ")");
    }

    // ---------- GUI snapshots
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc->createEditor());
        auto* te = dynamic_cast<ThudiEditor*> (ed.get());
        check (te != nullptr, "editor created");
        if (te != nullptr)
        {
            te->setSize (940, 1210);
            te->selectPad (0);
            snapshot (*te, out.getChildFile ("thudi_gui_kick.png"), 1.0f);
            te->selectPad (1); te->setPage (1);
            snapshot (*te, out.getChildFile ("thudi_gui_808_env.png"), 1.0f);
            te->selectPad (4); te->setPage (0);
            snapshot (*te, out.getChildFile ("thudi_gui_hat.png"), 1.0f);
            te->getLcd().mode = LcdDisplay::Mode::Browse; te->getLcd().browse = LcdDisplay::Browse::Pads; te->getLcd().refreshEntries();
            snapshot (*te, out.getChildFile ("thudi_gui_browser.png"), 1.0f);
            te->setSize (677, 871);
            te->getLcd().mode = LcdDisplay::Mode::Edit; te->selectPad (1); te->setPage (2);
            snapshot (*te, out.getChildFile ("thudi_gui_default_size.png"), 1.0f);
        }
        ed.reset();
    }

    std::cout << (failures == 0 ? "ALL CHECKS PASSED" : juce::String (failures) + " CHECK(S) FAILED") << std::endl;
    proc.reset();
    return failures == 0 ? 0 : 1;
}

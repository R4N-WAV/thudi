// thudi_snapshot — headless checks of the plugin (processor + editor) and GUI screenshots.
// usage: thudi_snapshot <outdir> [pattern.txt]
#include <juce_audio_formats/juce_audio_formats.h>
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "PresetManager.h"

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

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
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

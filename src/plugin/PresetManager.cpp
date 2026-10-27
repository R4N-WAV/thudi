#include "PresetManager.h"
#include "BinaryData.h"
#include "KitJson.h"
#include "Models.h"
#include "PluginProcessor.h"

using namespace thudi;

static std::string toStd (const juce::String& s) { return s.toStdString(); }
static juce::String fromStd (const std::string& s) { return juce::String::fromUTF8 (s.c_str(), (int) s.size()); }

PresetManager::PresetManager (ThudiProcessor& p) : proc (p) {}

juce::File PresetManager::userRoot()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("R4N").getChildFile ("Thudi");
}
juce::File PresetManager::kitsDir() { return userRoot().getChildFile ("Kits"); }
juce::File PresetManager::padsDir (const juce::String& category) { return userRoot().getChildFile ("Pads").getChildFile (category); }

juce::StringArray PresetManager::padCategories()
{
    juce::StringArray c;
    for (int m = 0; m < (int) Model::Count; ++m) c.add (modelCategory (m));
    return c;
}

static juce::String resourceText (const juce::String& originalName)
{
    for (int i = 0; i < ThudiBinary::namedResourceListSize; ++i)
        if (juce::String (ThudiBinary::originalFilenames[i]) == originalName)
        {
            int size = 0;
            if (auto* d = ThudiBinary::getNamedResource (ThudiBinary::namedResourceList[i], size))
                return juce::String::fromUTF8 (d, size);
        }
    return {};
}

juce::String PresetManager::readEntry (const Entry& e) const
{
    if (e.factory) return resourceText (e.resource);
    return e.file.loadFileAsString();
}

std::vector<PresetManager::Entry> PresetManager::listKits() const
{
    std::vector<Entry> out;
    out.push_back ({ "Init", {}, true, {}, "__init__" });
    for (int i = 0; i < ThudiBinary::namedResourceListSize; ++i)
    {
        juce::String fn (ThudiBinary::originalFilenames[i]);
        if (fn.equalsIgnoreCase ("Init.thudikit")) continue;
        if (fn.endsWithIgnoreCase (".thudikit"))
            out.push_back ({ fn.upToLastOccurrenceOf (".", false, false), {}, true, {}, fn });
    }
    std::sort (out.begin(), out.end(), [] (const Entry& a, const Entry& b)
               {
                   auto rank = [] (const juce::String& n) { return n == "Init" ? 0 : (n == "RHYNO" ? 1 : 2); };
                   if (rank (a.name) != rank (b.name)) return rank (a.name) < rank (b.name);
                   return a.name.compareNatural (b.name) < 0;
               });
    auto files = kitsDir().findChildFiles (juce::File::findFiles, false, "*.thudikit");
    files.sort();
    for (auto& f : files) out.push_back ({ f.getFileNameWithoutExtension(), {}, false, f, {} });
    return out;
}

std::vector<PresetManager::Entry> PresetManager::listPads (const juce::String& category) const
{
    std::vector<Entry> out;
    // factory pad presets are stored flat as "<Category>__<Name>.thudipad"
    for (int i = 0; i < ThudiBinary::namedResourceListSize; ++i)
    {
        juce::String fn (ThudiBinary::originalFilenames[i]);
        if (! fn.endsWithIgnoreCase (".thudipad")) continue;
        auto cat = fn.upToFirstOccurrenceOf ("__", false, false);
        auto name = fn.fromFirstOccurrenceOf ("__", false, false).upToLastOccurrenceOf (".", false, false);
        if (category.isEmpty() || category == cat) out.push_back ({ name, cat, true, {}, fn });
    }
    for (auto& cat : padCategories())
    {
        if (category.isNotEmpty() && cat != category) continue;
        auto files = padsDir (cat).findChildFiles (juce::File::findFiles, false, "*.thudipad");
        files.sort();
        for (auto& f : files) out.push_back ({ f.getFileNameWithoutExtension(), cat, false, f, {} });
    }
    std::stable_sort (out.begin(), out.end(), [] (const Entry& a, const Entry& b)
                      { return padCategories().indexOf (a.category) < padCategories().indexOf (b.category); });
    return out;
}

int PresetManager::currentKitIndex() const
{
    auto kits = listKits();
    for (size_t i = 0; i < kits.size(); ++i) if (kits[i].name == proc.getKitName()) return (int) i;
    return -1;
}

// ------------------------------------------------------------------ kits
juce::String PresetManager::kitToJson() const
{
    json::Kit kit;
    kit.name = toStd (proc.getKitName());
    for (int g = 0; g < kNumGlobalParams; ++g)
        kit.global[g] = proc.globalParam ((G) g)->convertFrom0to1 (proc.globalParam ((G) g)->getValue());
    for (int p = 0; p < kNumPads; ++p)
    {
        kit.pads[p].label = toStd (proc.getPadLabel (p));
        kit.pads[p].note = proc.getPadNote (p);
        kit.pads[p].sound = proc.getPadSound (p);
        if (auto smp = proc.getSlotSample (p)) kit.pads[p].sample = smp->path;
    }
    kit.solo.label = toStd (proc.getSoloLabel());
    kit.solo.sound = proc.getPadSound (kSoloSlot);
    if (auto smp = proc.getSlotSample (kSoloSlot)) kit.solo.sample = smp->path;
    return fromStd (json::writeKit (kit));
}

void PresetManager::restoreSample (int slot, const std::string& path)
{
    thudi::SamplePtr s;
    if (! path.empty())
    {
        juce::File f (fromStd (path));
        if (! f.existsAsFile()) f = ThudiProcessor::samplesDir().getChildFile (f.getFileName());
        juce::String err;
        if (f.existsAsFile()) s = proc.loadSampleFile (f, err);
        else lastError = "Missing sample " + f.getFileName();
    }
    proc.setSlotSample (slot, s);
}

bool PresetManager::kitFromJson (const juce::String& text)
{
    json::Kit kit;
    if (! json::parseKit (toStd (text), kit)) { lastError = "Could not read kit file"; return false; }
    bool usesSamples = ! kit.solo.sample.empty();
    for (auto& kp : kit.pads) usesSamples |= ! kp.sample.empty();
    for (int g = 0; g < kNumGlobalParams; ++g)
    {
        // GÆRRO mode is a state of the machine, not of the kit; a kit made of chops switches it on
        if ((G) g == G::Gaerro) { if (usesSamples) proc.setGlobalValue (G::Gaerro, 1.0f); continue; }
        proc.setGlobalValue ((G) g, kit.global[g]);
    }
    for (int p = 0; p < kNumPads; ++p)
    {
        proc.setPadSound (p, kit.pads[p].sound);
        proc.meta.getChildWithProperty ("index", p).setProperty ("label", fromStd (kit.pads[p].label), nullptr);
        proc.setPadNote (p, kit.pads[p].note);
        restoreSample (p, kit.pads[p].sample);
    }
    proc.setPadSound (kSoloSlot, kit.solo.sound);
    proc.meta.setProperty ("soloLabel", fromStd (kit.solo.label), nullptr);
    restoreSample (kSoloSlot, kit.solo.sample);
    proc.setKitName (fromStd (kit.name));
    return true;
}

bool PresetManager::loadDefaultKit()
{
    initKit();
    return true;
}

void PresetManager::initKit()
{
    // the machine starts empty: no sound on any pad (the solo slot gets a plain 808 to start from)
    for (int p = 0; p < kNumPads; ++p)
    {
        proc.setPadSound (p, initSoundForModel ((int) Model::Empty));
        proc.meta.getChildWithProperty ("index", p).setProperty ("label", "", nullptr);
        proc.setPadNote (p, 36 + p);
        proc.setSlotSample (p, {});
    }
    auto bass = initSoundForModel ((int) Model::Bass808);
    bass[P::Poly] = 0; bass[P::Glide] = 0; bass[P::PlayMode] = 1;
    proc.setPadSound (kSoloSlot, bass);
    proc.meta.setProperty ("soloLabel", "INIT 808", nullptr);
    proc.setSlotSample (kSoloSlot, {});
    proc.setGlobalValue (G::SoloOn, 0.0f);
    proc.setKitName ("Init");
}

bool PresetManager::loadKit (const Entry& e)
{
    if (e.resource == "__init__") { initKit(); return true; }
    auto text = readEntry (e);
    if (text.isEmpty()) { lastError = "Kit not found"; return false; }
    if (! kitFromJson (text)) return false;
    proc.setKitName (e.name);
    return true;
}

bool PresetManager::saveKit (const juce::String& name)
{
    auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) { lastError = "Empty name"; return false; }
    kitsDir().createDirectory();
    proc.setKitName (clean);
    auto ok = kitsDir().getChildFile (clean + ".thudikit").replaceWithText (kitToJson());
    if (! ok) lastError = "Could not write " + kitsDir().getFullPathName();
    return ok;
}

// ------------------------------------------------------------------ pads
juce::String PresetManager::padToJson (int pad) const
{
    json::KitPad kp;
    kp.label = toStd (proc.getPadLabel (pad));
    kp.sound = proc.getPadSound (pad);
    if (auto smp = proc.getSlotSample (pad)) kp.sample = smp->path;
    return fromStd (json::writePad (kp));
}

bool PresetManager::padFromJson (int pad, const juce::String& text, bool includeLabel)
{
    json::KitPad kp;
    if (! json::parsePad (toStd (text), kp)) { lastError = "Not a Thudi pad"; return false; }
    // keep routing of the target pad (output bus, link, choke) unless the preset changes the sound type completely
    auto current = proc.getPadSound (pad);
    kp.sound[P::Output] = current[P::Output];
    kp.sound[P::Link] = current[P::Link];
    proc.setPadSound (pad, kp.sound);
    if (includeLabel) proc.setPadLabel (pad, fromStd (kp.label));
    restoreSample (pad, kp.sample);
    return true;
}

bool PresetManager::loadPad (int pad, const Entry& e)
{
    auto text = readEntry (e);
    if (text.isEmpty()) { lastError = "Pad preset not found"; return false; }
    return padFromJson (pad, text, true);
}

bool PresetManager::savePad (int pad, const juce::String& name)
{
    auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) { lastError = "Empty name"; return false; }
    const int model = (int) std::round (proc.padValue (pad, P::Model));
    auto dir = padsDir (modelCategory (model));
    dir.createDirectory();
    auto ok = dir.getChildFile (clean + ".thudipad").replaceWithText (padToJson (pad));
    if (! ok) lastError = "Could not write " + dir.getFullPathName();
    return ok;
}

// ------------------------------------------------------------------ actions
void PresetManager::copyPad (int pad)
{
    clipboard = padToJson (pad);
    juce::SystemClipboard::copyTextToClipboard (clipboard);
}

bool PresetManager::canPaste() const
{
    auto sys = juce::SystemClipboard::getTextFromClipboard();
    return clipboard.isNotEmpty() || sys.contains ("\"thudi-pad\"");
}

void PresetManager::pastePad (int pad, bool includeLabel)
{
    auto sys = juce::SystemClipboard::getTextFromClipboard();
    auto text = sys.contains ("\"thudi-pad\"") ? sys : clipboard;
    if (text.isNotEmpty()) padFromJson (pad, text, includeLabel);
}

void PresetManager::swapPads (int a, int b)
{
    if (a == b) return;
    auto sa = proc.getPadSound (a), sb = proc.getPadSound (b);
    auto la = proc.getPadLabel (a), lb = proc.getPadLabel (b);
    proc.setPadSound (a, sb); proc.setPadSound (b, sa);
    proc.setPadLabel (a, lb); proc.setPadLabel (b, la);
    auto xa = proc.getSlotSample (a), xb = proc.getSlotSample (b);
    proc.setSlotSample (a, xb); proc.setSlotSample (b, xa);
}

void PresetManager::duplicatePad (int from, int to)
{
    if (from == to) return;
    proc.setPadSound (to, proc.getPadSound (from));
    proc.setPadLabel (to, proc.getPadLabel (from));
    proc.setSlotSample (to, proc.getSlotSample (from));
}

void PresetManager::initPad (int pad, int model)
{
    auto s = initSoundForModel (model);
    auto current = proc.getPadSound (pad);
    s[P::Output] = current[P::Output];
    proc.setPadSound (pad, s);
    if (model != (int) Model::Sample) proc.setSlotSample (pad, {});
}

void PresetManager::randomizePad (int pad, float amount)
{
    auto s = proc.getPadSound (pad);
    const int model = (int) std::round (s[P::Model]);
    juce::Random rnd;
    // only touch the model's own sound pages (not CHAR/STEREO/PLAY routing)
    auto pages = pagesForModel (model);
    for (size_t pg = 0; pg + 3 < pages.size(); ++pg)
        for (auto p : pages[pg].knobs)
        {
            const auto& pi = info (p);
            if (pi.scale == Scale::Choice || pi.scale == Scale::Int) continue;
            float n = toNormalised (pi, s[p]) + (rnd.nextFloat() * 2.0f - 1.0f) * amount;
            s[p] = fromNormalised (pi, juce::jlimit (0.0f, 1.0f, n));
        }
    s[P::Drive] = fromNormalised (info (P::Drive), juce::jlimit (0.0f, 0.8f, toNormalised (info (P::Drive), s[P::Drive]) + (rnd.nextFloat() - 0.5f) * amount));
    proc.setPadSound (pad, s);
}

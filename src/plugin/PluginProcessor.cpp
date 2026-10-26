#include "PluginProcessor.h"
#include "ParamFormat.h"
#include "PresetManager.h"
#include "PluginEditor.h"

using namespace thudi;

static juce::String twoDigits (int n) { return juce::String (n).paddedLeft ('0', 2); }

juce::String ThudiProcessor::padParamId (int pad, P p) { return "p" + twoDigits (pad + 1) + "_" + juce::String (info (p).id); }
juce::String ThudiProcessor::globalParamId (G g) { return "g_" + juce::String (info (g).id); }

static std::unique_ptr<juce::AudioParameterFloat> makeParam (const juce::String& id, const juce::String& name, const ParamInfo& pi)
{
    juce::NormalisableRange<float> range (pi.min, pi.max,
        [pi] (float, float, float n) { return fromNormalised (pi, n); },
        [pi] (float, float, float v) { return toNormalised (pi, v); },
        [pi] (float, float, float v)
        {
            v = juce::jlimit (pi.min, pi.max, v);
            return (pi.scale == Scale::Int || pi.scale == Scale::Choice) ? std::round (v) : v;
        });
    auto attrs = juce::AudioParameterFloatAttributes()
                     .withStringFromValueFunction ([pi] (float v, int) { return formatValue (pi, v); })
                     .withValueFromStringFunction ([pi] (const juce::String& t) { return parseValue (pi, t); })
                     .withLabel (pi.unit);
    return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, range, pi.def, attrs);
}

juce::AudioProcessorValueTreeState::ParameterLayout ThudiProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int g = 0; g < kNumGlobalParams; ++g)
        layout.add (makeParam (globalParamId ((G) g), juce::String (globalTable()[(size_t) g].name), globalTable()[(size_t) g]));
    for (int pad = 0; pad < kNumPads; ++pad)
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup> ("pad" + twoDigits (pad + 1), "Pad " + juce::String (pad + 1), " | ");
        for (int k = 0; k < kNumPadParams; ++k)
        {
            const auto& pi = paramTable()[(size_t) k];
            group->addChild (makeParam (padParamId (pad, (P) k), "Pad " + juce::String (pad + 1) + " " + pi.name, pi));
        }
        layout.add (std::move (group));
    }
    return layout;
}

juce::AudioProcessor::BusesProperties ThudiProcessor::makeBuses()
{
    auto b = BusesProperties().withOutput ("Main", juce::AudioChannelSet::stereo(), true);
    for (int i = 2; i <= kNumOutputs; ++i)
        b = b.withOutput ("Out " + juce::String (i), juce::AudioChannelSet::stereo(), false);
    return b;
}

ThudiProcessor::ThudiProcessor()
    : juce::AudioProcessor (makeBuses()),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    for (int pad = 0; pad < kNumPads; ++pad)
        for (int k = 0; k < kNumPadParams; ++k)
        {
            auto id = padParamId (pad, (P) k);
            padParams[(size_t) pad][(size_t) k] = apvts.getParameter (id);
            padRaw[(size_t) pad][(size_t) k] = apvts.getRawParameterValue (id);
        }
    for (int g = 0; g < kNumGlobalParams; ++g)
    {
        auto id = globalParamId ((G) g);
        globalParams[(size_t) g] = apvts.getParameter (id);
        globalRaw[(size_t) g] = apvts.getRawParameterValue (id);
    }
    for (int i = 0; i < kNumPads; ++i) padNotesAtomic[(size_t) i].store (36 + i);
    apvts.addParameterListener (globalParamId (G::Oversampling), this);

    events.reserve (2048);
    ensureMeta();
    presets = std::make_unique<PresetManager> (*this);
    presets->loadDefaultKit();
}

ThudiProcessor::~ThudiProcessor()
{
    apvts.removeParameterListener (globalParamId (G::Oversampling), this);
    cancelPendingUpdate();
}

void ThudiProcessor::ensureMeta()
{
    if (! meta.hasProperty ("kitName")) meta.setProperty ("kitName", "Init", nullptr);
    for (int i = 0; i < kNumPads; ++i)
    {
        auto pad = meta.getChildWithProperty ("index", i);
        if (! pad.isValid())
        {
            pad = juce::ValueTree ("PAD");
            pad.setProperty ("index", i, nullptr);
            pad.setProperty ("label", "PAD " + juce::String (i + 1), nullptr);
            pad.setProperty ("note", 36 + i, nullptr);
            meta.appendChild (pad, nullptr);
        }
    }
}

int ThudiProcessor::oversamplingFactorFromParam() const
{
    const int c = (int) std::round (globalRaw[(size_t) G::Oversampling]->load());
    return c <= 0 ? 1 : (c == 1 ? 2 : 4);
}

void ThudiProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSr = sampleRate; currentBlock = juce::jmax (32, samplesPerBlock);
    preparedOs = oversamplingFactorFromParam();
    engine.prepare (sampleRate, currentBlock, preparedOs);
    scratch.setSize (kNumOutputs * 2, currentBlock, false, true, false);
    for (int c = 0; c < kNumOutputs * 2; ++c) scratchPtrs[(size_t) c] = scratch.getWritePointer (c);
    setLatencySamples (engine.latencySamples());
    notesDirty.store (true);
}

bool ThudiProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    for (int i = 1; i < layouts.outputBuses.size(); ++i)
    {
        const auto& set = layouts.outputBuses.getReference (i);
        if (! set.isDisabled() && set != juce::AudioChannelSet::stereo()) return false;
    }
    return layouts.inputBuses.isEmpty();
}

void ThudiProcessor::parameterChanged (const juce::String&, float)
{
    triggerAsyncUpdate(); // oversampling changed -> re-prepare on the message thread
}

void ThudiProcessor::handleAsyncUpdate()
{
    if (oversamplingFactorFromParam() != preparedOs && preparedOs > 0)
    {
        suspendProcessing (true);
        prepareToPlay (currentSr, currentBlock);
        suspendProcessing (false);
    }
    const int learned = learnedNote.exchange (-1);
    if (learned >= 0)
    {
        for (int i = 0; i < kNumPads; ++i)
            meta.getChildWithProperty ("index", i).setProperty ("note", padNotesAtomic[(size_t) i].load(), nullptr);
        if (onMetaChanged) onMetaChanged();
    }
}

void ThudiProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    if (notesDirty.exchange (false))
        for (int i = 0; i < kNumPads; ++i) engine.setPadNote (i, padNotesAtomic[(size_t) i].load());

    for (int pad = 0; pad < kNumPads; ++pad)
    {
        auto& snd = engine.pad (pad);
        for (int k = 0; k < kNumPadParams; ++k) snd.v[(size_t) k] = padRaw[(size_t) pad][(size_t) k]->load();
    }
    for (int g = 0; g < kNumGlobalParams; ++g) engine.global[g] = globalRaw[(size_t) g]->load();

    events.clear();
    for (const auto meta_ : midi)
    {
        const auto m = meta_.getMessage();
        if (m.isNoteOn())
        {
            const int learnPad = midiLearnPad.load();
            if (learnPad >= 0)
            {
                padNotesAtomic[(size_t) learnPad].store (m.getNoteNumber());
                engine.setPadNote (learnPad, m.getNoteNumber());
                midiLearnPad.store (-1);
                learnedNote.store (m.getNoteNumber());
                triggerAsyncUpdate();
            }
            if (events.size() < events.capacity())
                events.push_back ({ meta_.samplePosition, m.getNoteNumber(), m.getFloatVelocity(), true });
        }
        else if (m.isNoteOff())
        {
            if (events.size() < events.capacity())
                events.push_back ({ meta_.samplePosition, m.getNoteNumber(), 0.0f, false });
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            engine.reset();
        }
    }

    // render in chunks that fit the scratch buffer
    auto mainOut = getBusBuffer (buffer, false, 0);
    int done = 0; size_t ev = 0;
    std::vector<thudi::NoteEvent>& evs = events;
    while (done < numSamples)
    {
        const int n = juce::jmin (scratch.getNumSamples(), numSamples - done);
        size_t evStart = ev;
        while (ev < evs.size() && evs[ev].offset < done + n) ++ev;
        // shift offsets into chunk-local time
        for (size_t k = evStart; k < ev; ++k) evs[k].offset = juce::jmax (0, evs[k].offset - done);
        engine.process (evs.data() + evStart, (int) (ev - evStart), scratchPtrs.data(), kNumOutputs * 2, n);

        for (int b = 0; b < kNumOutputs; ++b)
        {
            const float* l = scratchPtrs[(size_t) b * 2];
            const float* r = scratchPtrs[(size_t) b * 2 + 1];
            const bool hasBus = b < getBusCount (false) && getBus (false, b) != nullptr && getBus (false, b)->isEnabled();
            if (b == 0 || ! hasBus)
            {
                mainOut.addFrom (0, done, l, n); mainOut.addFrom (1, done, r, n);
                continue;
            }
            auto out = getBusBuffer (buffer, false, b);
            if (out.getNumChannels() >= 2) { out.copyFrom (0, done, l, n); out.copyFrom (1, done, r, n); }
        }
        done += n;
    }
    midi.clear();
}

thudi::PadSound ThudiProcessor::getPadSound (int pad) const
{
    PadSound s;
    for (int k = 0; k < kNumPadParams; ++k) s.v[(size_t) k] = padRaw[(size_t) pad][(size_t) k]->load();
    return s;
}

void ThudiProcessor::setPadValue (int pad, P p, float plainValue)
{
    auto* prm = padParams[(size_t) pad][(size_t) p];
    prm->setValueNotifyingHost (prm->convertTo0to1 (plainValue));
}

void ThudiProcessor::setGlobalValue (G g, float plainValue)
{
    auto* prm = globalParams[(size_t) g];
    prm->setValueNotifyingHost (prm->convertTo0to1 (plainValue));
}

void ThudiProcessor::setPadSound (int pad, const PadSound& s)
{
    for (int k = 0; k < kNumPadParams; ++k)
        if (std::abs (padValue (pad, (P) k) - s.v[(size_t) k]) > 1e-6f)
            setPadValue (pad, (P) k, s.v[(size_t) k]);
}

juce::String ThudiProcessor::getPadLabel (int pad) const { return meta.getChildWithProperty ("index", pad).getProperty ("label").toString(); }
void ThudiProcessor::setPadLabel (int pad, const juce::String& label)
{
    meta.getChildWithProperty ("index", pad).setProperty ("label", label.toUpperCase().substring (0, 16), nullptr);
    if (onMetaChanged) onMetaChanged();
}
int ThudiProcessor::getPadNote (int pad) const { return (int) meta.getChildWithProperty ("index", pad).getProperty ("note", 36 + pad); }
void ThudiProcessor::setPadNote (int pad, int note)
{
    meta.getChildWithProperty ("index", pad).setProperty ("note", note, nullptr);
    padNotesAtomic[(size_t) pad].store (note); notesDirty.store (true);
    if (onMetaChanged) onMetaChanged();
}
juce::String ThudiProcessor::getKitName() const { return meta.getProperty ("kitName").toString(); }
void ThudiProcessor::setKitName (const juce::String& n) { meta.setProperty ("kitName", n, nullptr); if (onMetaChanged) onMetaChanged(); }

void ThudiProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    auto root = juce::ValueTree ("THUDI_STATE");
    root.setProperty ("version", 1, nullptr);
    root.appendChild (state, nullptr);
    root.appendChild (meta.createCopy(), nullptr);
    if (auto xml = root.createXml()) copyXmlToBinary (*xml, dest);
}

void ThudiProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr) return;
    auto root = juce::ValueTree::fromXml (*xml);
    if (! root.hasType ("THUDI_STATE")) return;
    auto params = root.getChildWithName (apvts.state.getType());
    if (params.isValid()) apvts.replaceState (params);
    auto m = root.getChildWithName ("THUDI_META");
    if (m.isValid()) meta.copyPropertiesAndChildrenFrom (m, nullptr);
    ensureMeta();
    for (int i = 0; i < kNumPads; ++i) padNotesAtomic[(size_t) i].store (getPadNote (i));
    notesDirty.store (true);
    if (onMetaChanged) juce::MessageManager::callAsync ([this] { if (onMetaChanged) onMetaChanged(); });
}

juce::AudioProcessorEditor* ThudiProcessor::createEditor()
{
    return new ThudiEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ThudiProcessor(); }

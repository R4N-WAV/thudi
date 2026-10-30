// Thudi web: the same C++ engine as the VST3, compiled to WebAssembly (wasi-sdk, reactor) for an AudioWorklet.
// Everything the browser page needs to know about parameters and editor pages is exported from here,
// so the web version can never drift from the plugin's tables.
#include <cstdlib>
#include <cstring>
#include <string>
#include "../../src/dsp/Engine.h"
#include "../../src/dsp/Models.h"

using namespace thudi;

#define EXPORT(name) extern "C" __attribute__ ((export_name (#name)))

static Engine* engine = nullptr;
static constexpr int kMaxBlock = 1024;
static float outBuf[kNumOutputs * 2][kMaxBlock];
static float* outPtrs[kNumOutputs * 2];
static float mix[2][kMaxBlock];
static std::vector<NoteEvent> events;
static float lights[kNumPads];
static std::string strBuf;

EXPORT (thudi_init) void thudi_init (float sampleRate, int oversampling)
{
    if (! engine) engine = new Engine();
    for (int c = 0; c < kNumOutputs * 2; ++c) outPtrs[c] = outBuf[c];
    engine->prepare (sampleRate, kMaxBlock, oversampling < 1 ? 1 : oversampling);
    events.reserve (512);
}

EXPORT (thudi_alloc) void* thudi_alloc (int bytes) { return std::malloc ((size_t) bytes); }
EXPORT (thudi_free) void thudi_free (void* p) { std::free (p); }

// parameters: JS writes plain values straight into these arrays
EXPORT (thudi_pad_params) float* thudi_pad_params (int slot) { return engine->pad (slot).v.data(); }
EXPORT (thudi_globals) float* thudi_globals() { return engine->global; }
EXPORT (thudi_set_note) void thudi_set_note (int pad, int note) { engine->setPadNote (pad, note); }

EXPORT (thudi_note) void thudi_note (int note, float vel, int on)
{
    if (events.size() < 512) events.push_back ({ 0, note, vel, on != 0 });
}
EXPORT (thudi_hit) void thudi_hit (int pad, float vel) { engine->queueUiHit (pad, vel); }
EXPORT (thudi_release) void thudi_release (int pad) { engine->queueUiRelease (pad); }
EXPORT (thudi_reset) void thudi_reset() { engine->reset(); }

// renders n frames; every output bus is folded into the stereo mix (a phone has one output)
EXPORT (thudi_process) float* thudi_process (int n)
{
    if (n > kMaxBlock) n = kMaxBlock;
    engine->process (events.data(), (int) events.size(), outPtrs, kNumOutputs * 2, n);
    events.clear();
    for (int i = 0; i < n; ++i)
    {
        float l = 0, r = 0;
        for (int b = 0; b < kNumOutputs; ++b) { l += outBuf[b * 2][i]; r += outBuf[b * 2 + 1][i]; }
        mix[0][i] = l; mix[1][i] = r;
    }
    return &mix[0][0];
}
EXPORT (thudi_mix_stride) int thudi_mix_stride() { return kMaxBlock; }

EXPORT (thudi_lights) float* thudi_lights()
{
    for (int i = 0; i < kNumPads; ++i) lights[i] = engine->padLight[i].load();
    return lights;
}

// samples (GÆRRO): JS copies mono float data into a buffer from thudi_alloc, then hands it over
EXPORT (thudi_set_sample) void thudi_set_sample (int slot, const float* data, int len, float rate)
{
    if (! data || len <= 0) { engine->setPadSample (slot, {}); return; }
    auto sd = std::make_shared<SampleData>();
    sd->data.assign (data, data + len);
    sd->rate = rate;
    engine->setPadSample (slot, sd);
}
// chop points for a mono buffer (same transient finder as the plugin); returns count, writes into out
EXPORT (thudi_chop) int thudi_chop (const float* data, int len, float rate, int slices, int* out)
{
    SampleData sd; sd.data.assign (data, data + len); sd.rate = rate;
    auto pts = chopPoints (sd, slices);
    for (size_t i = 0; i < pts.size(); ++i) out[i] = (int) pts[i];
    return (int) pts.size();
}

// ---------------------------------------------------------------- tables for the page (JSON strings)
static std::string esc (const char* s) { std::string o; for (; *s; ++s) { if (*s == '"' || *s == '\\') o += '\\'; o += *s; } return o; }

EXPORT (thudi_tables) const char* thudi_tables()
{
    std::string j = "{\"pad\":[";
    const char* scales[] = { "lin", "log", "int", "choice" };
    auto add = [&] (const ParamInfo& pi, bool first)
    {
        char num[160];
        std::snprintf (num, sizeof num, "\"min\":%g,\"max\":%g,\"def\":%g,", pi.min, pi.max, pi.def);
        j += std::string (first ? "" : ",") + "{\"id\":\"" + esc (pi.id) + "\",\"name\":\"" + esc (pi.name) + "\"," + num
           + "\"scale\":\"" + scales[(int) pi.scale] + "\",\"unit\":\"" + esc (pi.unit) + "\",\"choices\":\"" + esc (pi.choices) + "\"}";
    };
    for (int k = 0; k < kNumPadParams; ++k) add (paramTable()[(size_t) k], k == 0);
    j += "],\"global\":[";
    for (int k = 0; k < kNumGlobalParams; ++k) add (globalTable()[(size_t) k], k == 0);
    j += "],\"models\":[";
    for (int m = 0; m < (int) Model::Count; ++m)
    {
        j += std::string (m ? "," : "") + "{\"name\":\"" + esc (modelName (m)) + "\",\"pages\":[";
        auto pages = pagesForModel (m);
        for (size_t p = 0; p < pages.size(); ++p)
        {
            j += std::string (p ? "," : "") + "{\"name\":\"" + esc (pages[p].name) + "\",\"knobs\":[";
            for (size_t k = 0; k < pages[p].knobs.size(); ++k) j += (k ? "," : "") + std::to_string ((int) pages[p].knobs[k]);
            j += "]}";
        }
        j += "],\"init\":[";
        auto s = initSoundForModel (m);
        for (int k = 0; k < kNumPadParams; ++k) { char b[32]; std::snprintf (b, sizeof b, "%s%g", k ? "," : "", s.v[(size_t) k]); j += b; }
        j += "]}";
    }
    j += "],\"pads\":" + std::to_string (kNumPads) + ",\"slots\":" + std::to_string (kNumSlots) + ",\"soloSlot\":" + std::to_string (kSoloSlot) + "}";
    strBuf = j;
    return strBuf.c_str();
}

// Thudi (துடி) by R4N — drum synthesizer
// Parameter table shared by the DSP engine, the plugin and the tools.
#pragma once
#include <array>
#include <cmath>
#include <string>

namespace thudi
{
constexpr int kNumPads = 16;
constexpr int kNumOutputs = 16;

enum class Model : int
{
    Kick = 0, Bass808, Snare, Clap, ClosedHat, OpenHat, Tom, Perc, Rim, Cymbal, Zap, Custom, Count
};

inline const char* modelName (int m)
{
    static const char* names[] = { "Kick", "808", "Snare", "Clap", "Closed Hat", "Open Hat",
                                   "Tom", "Perc", "Rim", "Cymbal", "Zap", "Custom" };
    return (m >= 0 && m < (int) Model::Count) ? names[m] : "?";
}

// Category folder used for pad presets
inline const char* modelCategory (int m) { return modelName (m); }

enum class Scale { Linear, Log, Int, Choice };

struct ParamInfo
{
    const char* id;     // short id, unique per pad ("tone_pitch")
    const char* name;   // display name
    float min, max, def;
    Scale scale;
    const char* unit;   // "", "dB", "Hz", "ms", "st", "ct", "%"
    const char* choices; // '|' separated for Scale::Choice
};

// X(enumName, id, name, min, max, def, scale, unit, choices)
#define THUDI_PAD_PARAMS(X) \
    X(Model,        "model",        "Model",          0, 11, 0, Choice, "",   "Kick|808|Snare|Clap|Closed Hat|Open Hat|Tom|Perc|Rim|Cymbal|Zap|Custom") \
    /* ---------------- mix / play ---------------- */ \
    X(Level,        "level",        "Level",        -60, 12, 0, Linear, "dB", "") \
    X(Pan,          "pan",          "Pan",           -1,  1, 0, Linear, "",   "") \
    X(Tune,         "tune",         "Tune",         -24, 24, 0, Linear, "st", "") \
    X(Fine,         "fine",         "Fine",        -100,100, 0, Linear, "ct", "") \
    X(VelSens,      "vel_sens",     "Velocity",       0,  1, 0.6f, Linear, "%", "") \
    X(Choke,        "choke",        "Choke Group",    0,  8, 0, Int,    "",   "") \
    X(Output,       "output",       "Output",         0, 15, 0, Int,    "",   "") \
    X(Link,         "link",         "Link Pad",       0, 16, 0, Int,    "",   "") \
    X(PlayMode,     "play_mode",    "Play Mode",      0,  1, 0, Choice, "",   "One-shot|Gate") \
    X(Poly,         "poly",         "Voicing",        0,  1, 0, Choice, "",   "Mono|Poly") \
    X(Glide,        "glide",        "Glide",          0, 1000, 0, Log,  "ms", "") \
    X(Release,      "release",      "Release",        1, 2000, 40, Log, "ms", "") \
    X(LowCut,       "low_cut",      "Low Cut",       10, 2000, 10, Log, "Hz", "") \
    X(HighCut,      "high_cut",     "High Cut",     500, 22000, 22000, Log, "Hz", "") \
    /* ---------------- tone layer ---------------- */ \
    X(ToneLevel,    "tone_level",   "Tone Level",     0,  1, 1, Linear, "%", "") \
    X(ToneShape,    "tone_shape",   "Wave",           0,  1, 0, Linear, "",  "") \
    X(TonePitch,    "tone_pitch",   "Pitch",         20, 2000, 50, Log, "Hz", "") \
    X(SweepAmt,     "sweep_amt",    "Sweep",          0, 60, 12, Linear, "st", "") \
    X(SweepTime,    "sweep_time",   "Sweep Time",     1, 1000, 30, Log, "ms", "") \
    X(SweepCurve,   "sweep_curve",  "Sweep Curve",   -1,  1, 0, Linear, "",  "") \
    X(Growl,        "growl",        "Growl",          0,  1, 0, Linear, "%", "") \
    X(FmAmt,        "fm_amt",       "FM",             0,  1, 0, Linear, "%", "") \
    X(FmRatio,      "fm_ratio",     "FM Ratio",     0.5f, 16, 1.5f, Log, "",  "") \
    X(FmDecay,      "fm_decay",     "FM Decay",       1, 2000, 60, Log, "ms", "") \
    X(ToneAttack,   "tone_attack",  "Attack",         0, 200, 0, Linear, "ms", "") \
    X(ToneHold,     "tone_hold",    "Hold",           0, 3000, 0, Linear, "ms", "") \
    X(ToneDecay,    "tone_decay",   "Decay",          5, 8000, 400, Log, "ms", "") \
    X(ToneCurve,    "tone_curve",   "Punch",         -1,  1, 0, Linear, "",  "") \
    X(ToneLP,       "tone_lp",      "Roundness",     50, 20000, 20000, Log, "Hz", "") \
    /* ---------------- noise layer ---------------- */ \
    X(NoiseLevel,   "noise_level",  "Noise Level",    0,  1, 0, Linear, "%", "") \
    X(NoiseColor,   "noise_color",  "Color",          0,  1, 0, Linear, "",  "") \
    X(NoiseFilter,  "noise_filter", "Filter",         0,  2, 1, Choice, "",  "LP|BP|HP") \
    X(NoiseCutoff,  "noise_cutoff", "Cutoff",        20, 20000, 2000, Log, "Hz", "") \
    X(NoiseReso,    "noise_reso",   "Resonance",      0,  1, 0.2f, Linear, "%", "") \
    X(NoiseEnvAmt,  "noise_env",    "Filter Env",    -1,  1, 0, Linear, "",  "") \
    X(NoiseAttack,  "noise_attack", "Attack",         0, 200, 0, Linear, "ms", "") \
    X(NoiseDecay,   "noise_decay",  "Decay",          1, 4000, 150, Log, "ms", "") \
    X(NoiseCurve,   "noise_curve",  "Snap",          -1,  1, 0, Linear, "",  "") \
    X(Bursts,       "bursts",       "Bursts",         1,  8, 1, Int,    "",  "") \
    X(BurstSpacing, "burst_space",  "Spacing",        1, 50, 9, Linear, "ms", "") \
    X(BurstDecay,   "burst_decay",  "Burst Decay",  0.5f, 20, 3, Linear, "ms", "") \
    X(BurstRandom,  "burst_rand",   "Humanize",       0,  1, 0.2f, Linear, "%", "") \
    /* ---------------- metal layer ---------------- */ \
    X(MetalLevel,   "metal_level",  "Metal Level",    0,  1, 0, Linear, "%", "") \
    X(MetalSet,     "metal_set",    "Metal Set",      0,  4, 0, Choice, "",  "808|Tight|Bell|Cowbell|Gong") \
    X(MetalPitch,   "metal_pitch",  "Metal Pitch", 0.25f, 4, 1, Log,    "x", "") \
    X(MetalTone,    "metal_tone",   "Tone",        1000, 16000, 7000, Log, "Hz", "") \
    X(MetalQ,       "metal_q",      "Focus",          0,  1, 0.3f, Linear, "%", "") \
    X(MetalRing,    "metal_ring",   "Ring",           0,  1, 0, Linear, "%", "") \
    X(MetalAttack,  "metal_attack", "Attack",         0, 100, 0, Linear, "ms", "") \
    X(MetalDecay,   "metal_decay",  "Decay",          5, 5000, 80, Log, "ms", "") \
    /* ---------------- body / resonator ---------------- */ \
    X(BodyLevel,    "body_level",   "Body Level",     0,  1, 0, Linear, "%", "") \
    X(BodyFreq,     "body_freq",    "Body Pitch",    40, 2000, 200, Log, "Hz", "") \
    X(BodyRatio,    "body_ratio",   "Body Ratio",     1,  3, 1.6f, Linear, "x", "") \
    X(BodyDecay,    "body_decay",   "Body Decay",     5, 3000, 150, Log, "ms", "") \
    X(BodyExcite,   "body_excite",  "Excite",         0,  1, 0.5f, Linear, "",  "") \
    /* ---------------- character ---------------- */ \
    X(Drive,        "drive",        "Drive",          0,  1, 0, Linear, "%", "") \
    X(Asym,         "asym",         "Asymmetry",      0,  1, 0, Linear, "%", "") \
    X(CrushMix,     "crush_mix",    "Crush",          0,  1, 0, Linear, "%", "") \
    X(CrushRate,    "crush_rate",   "Crush Rate",   500, 48000, 48000, Log, "Hz", "") \
    X(CrushBits,    "crush_bits",   "Crush Bits",     2, 16, 16, Linear, "bit", "") \
    X(CrushXover,   "crush_xover",  "Crush Above",   20, 5000, 20, Log, "Hz", "") \
    X(Width,        "width",        "Width",          0,  1, 0, Linear, "%", "") \
    X(ChorusRate,   "chorus_rate",  "Chorus Rate", 0.05f, 8, 0.9f, Log, "Hz", "") \
    X(ChorusDepth,  "chorus_depth", "Chorus Depth",   0,  6, 2.5f, Linear, "ms", "") \
    X(MonoBelow,    "mono_below",   "Mono Below",    20, 1000, 120, Log, "Hz", "") \
    X(Drift,        "drift",        "Analog Drift",   0,  1, 0.2f, Linear, "%", "") \
    X(Tape,         "tape",         "Tape",           0,  1, 0, Linear, "%", "")

enum class P : int
{
#define THUDI_ENUM(e, id, name, mn, mx, df, sc, unit, ch) e,
    THUDI_PAD_PARAMS (THUDI_ENUM)
#undef THUDI_ENUM
    Count
};
constexpr int kNumPadParams = (int) P::Count;

inline const std::array<ParamInfo, kNumPadParams>& paramTable()
{
    static const std::array<ParamInfo, kNumPadParams> t = { {
#define THUDI_INFO(e, id, name, mn, mx, df, sc, unit, ch) ParamInfo { id, name, (float) (mn), (float) (mx), (float) (df), Scale::sc, unit, ch },
        THUDI_PAD_PARAMS (THUDI_INFO)
#undef THUDI_INFO
    } };
    return t;
}

inline const ParamInfo& info (P p) { return paramTable()[(size_t) p]; }

// Global (plugin-wide) parameters
#define THUDI_GLOBAL_PARAMS(X) \
    X(Master,       "master",       "Master",        -60, 6, 0, Linear, "dB", "") \
    X(BusDrive,     "bus_drive",    "Bus Drive",       0, 1, 0.15f, Linear, "%", "") \
    X(Oversampling, "oversampling", "Oversampling",    0, 2, 1, Choice, "", "1x|2x|4x") \
    X(KeysPad,      "keys_pad",     "Keys Pad",        0, 16, 0, Int, "", "") \
    X(KeysRoot,     "keys_root",    "Keys Root",      24, 96, 60, Int, "", "")

enum class G : int
{
#define THUDI_ENUM(e, id, name, mn, mx, df, sc, unit, ch) e,
    THUDI_GLOBAL_PARAMS (THUDI_ENUM)
#undef THUDI_ENUM
    Count
};
constexpr int kNumGlobalParams = (int) G::Count;

inline const std::array<ParamInfo, kNumGlobalParams>& globalTable()
{
    static const std::array<ParamInfo, kNumGlobalParams> t = { {
#define THUDI_INFO(e, id, name, mn, mx, df, sc, unit, ch) ParamInfo { id, name, (float) (mn), (float) (mx), (float) (df), Scale::sc, unit, ch },
        THUDI_GLOBAL_PARAMS (THUDI_INFO)
#undef THUDI_INFO
    } };
    return t;
}
inline const ParamInfo& info (G g) { return globalTable()[(size_t) g]; }

// A full pad sound = one value per parameter (plain/denormalised values)
struct PadSound
{
    std::array<float, kNumPadParams> v {};
    PadSound() { for (int i = 0; i < kNumPadParams; ++i) v[(size_t) i] = paramTable()[(size_t) i].def; }
    float  operator[] (P p) const { return v[(size_t) p]; }
    float& operator[] (P p)       { return v[(size_t) p]; }
};

// Normalised <-> plain helpers (used by GUI and host mapping)
inline float toNormalised (const ParamInfo& pi, float x)
{
    x = std::fmin (pi.max, std::fmax (pi.min, x));
    if (pi.scale == Scale::Log && pi.min <= 0.0f)        // log-like range starting at 0 (e.g. glide): cubic taper
        return std::cbrt ((x - pi.min) / (pi.max - pi.min));
    if (pi.scale == Scale::Log)
        return std::log (x / pi.min) / std::log (pi.max / pi.min);
    return (x - pi.min) / (pi.max - pi.min);
}
inline float fromNormalised (const ParamInfo& pi, float n)
{
    n = std::fmin (1.0f, std::fmax (0.0f, n));
    float x;
    if (pi.scale == Scale::Log && pi.min <= 0.0f) x = pi.min + n * n * n * (pi.max - pi.min);
    else if (pi.scale == Scale::Log)              x = pi.min * std::pow (pi.max / pi.min, n);
    else                                          x = pi.min + n * (pi.max - pi.min);
    if (pi.scale == Scale::Int || pi.scale == Scale::Choice) x = std::round (x);
    return x;
}

} // namespace thudi

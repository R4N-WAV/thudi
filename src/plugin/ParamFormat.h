// Thudi — value <-> text formatting shared by host parameters and the LCD
#pragma once
#include <juce_core/juce_core.h>
#include "ThudiParams.h"

namespace thudi
{
inline juce::StringArray choicesOf (const ParamInfo& pi)
{
    return juce::StringArray::fromTokens (juce::String (pi.choices), "|", "");
}

inline juce::String formatValue (const ParamInfo& pi, float v, bool shortForm = false)
{
    const juce::String id (pi.id);
    if (pi.scale == Scale::Choice)
    {
        auto c = choicesOf (pi);
        int i = juce::jlimit (0, c.size() - 1, (int) std::round (v));
        return c[i];
    }
    if (pi.scale == Scale::Int)
    {
        int i = (int) std::round (v);
        if (id == "choke")    return i == 0 ? "Off" : "Grp " + juce::String (i);
        if (id == "output")   return i == 0 ? "Main" : "Out " + juce::String (i + 1);
        if (id == "link")     return i == 0 ? "None" : "Pad " + juce::String (i);
        if (id == "keys_pad") return i == 0 ? "Off" : "Pad " + juce::String (i);
        if (id == "keys_root") return juce::MidiMessage::getMidiNoteName (i, true, true, 3);
        if (id == "bursts")   return juce::String (i) + (shortForm ? "" : (i == 1 ? " burst" : " bursts"));
        return juce::String (i);
    }
    const juce::String unit (pi.unit);
    auto num = [] (float x, int decimals) { return decimals > 0 ? juce::String (x, decimals) : juce::String (juce::roundToInt (x)); };
    if (unit == "Hz")
        return v >= 1000.0f ? num (v / 1000.0f, v >= 10000.0f ? 1 : 2) + (shortForm ? "k" : " kHz") : num (v, v < 100.0f ? 1 : 0) + (shortForm ? "Hz" : " Hz");
    if (unit == "ms")
        return v >= 1000.0f ? num (v / 1000.0f, 2) + " s" : num (v, v < 10.0f ? 1 : 0) + " ms";
    if (unit == "%")   return juce::String (juce::roundToInt (v * 100.0f)) + "%";
    if (unit == "dB")  return v <= -59.9f ? "-inf" : (v > 0 ? "+" : "") + juce::String (v, 1) + " dB";
    if (unit == "st")  return (v > 0 ? "+" : "") + juce::String (v, 1) + " st";
    if (unit == "ct")  return (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v)) + " ct";
    if (unit == "bit") return juce::String (v, 1) + " bit";
    if (unit == "x")   return juce::String (v, 2) + "x";
    if (id == "pan")
    {
        if (std::abs (v) < 0.01f) return "C";
        return (v < 0 ? "L" : "R") + juce::String (juce::roundToInt (std::abs (v) * 100.0f));
    }
    // curve-type bipolar controls
    return (v > 0 ? "+" : "") + juce::String (v, 2);
}

inline float parseValue (const ParamInfo& pi, const juce::String& text)
{
    auto t = text.trim();
    if (pi.scale == Scale::Choice)
    {
        auto c = choicesOf (pi);
        int i = c.indexOf (t, true);
        return i >= 0 ? (float) i : t.getFloatValue();
    }
    float v = t.retainCharacters ("-.0123456789").getFloatValue();
    if (t.endsWithIgnoreCase ("k")) v *= 1000.0f;
    if (juce::String (pi.unit) == "%") v *= 0.01f;
    if (juce::String (pi.unit) == "ms" && t.endsWithIgnoreCase (" s")) v *= 1000.0f;
    return juce::jlimit (pi.min, pi.max, v);
}
} // namespace thudi

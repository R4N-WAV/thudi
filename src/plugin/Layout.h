// Thudi — panel layout in design units (the Blender scene uses the same numbers: art/blender/layout.json)
#pragma once
#include <juce_graphics/juce_graphics.h>

namespace thudi::layout
{
constexpr float W = 940.0f, H = 1210.0f;
constexpr float renderScale = 2.0f; // Blender renders at 2x design size

inline juce::Point<float> masterKnob()  { return { 100.0f, 118.0f }; }
constexpr float masterKnobR = 44.0f;
inline juce::Point<float> driveKnob()   { return { 100.0f, 238.0f }; }
constexpr float smallKnobR = 32.0f;
inline juce::Rectangle<float> osButton()  { return { 52.0f, 300.0f, 96.0f, 32.0f }; }
inline juce::Rectangle<float> plaque()    { return { 42.0f, 356.0f, 116.0f, 44.0f }; }

inline juce::Rectangle<float> lcdBezel()  { return { 195.0f, 38.0f, 590.0f, 214.0f }; }
inline juce::Rectangle<float> lcdGlass()  { return { 213.0f, 56.0f, 554.0f, 178.0f }; }

inline juce::Point<float> wheel()       { return { 858.0f, 132.0f }; }
constexpr float wheelR = 56.0f;
inline juce::Rectangle<float> enterButton() { return { 812.0f, 210.0f, 92.0f, 32.0f }; }

inline juce::Rectangle<float> softKey (int i) { return { 215.0f + (float) i * 140.0f, 266.0f, 128.0f, 32.0f }; }

constexpr int numKnobs = 8;
inline juce::Point<float> pageKnob (int i) { return { 232.0f + (float) i * 92.5f, 352.0f }; }
constexpr float pageKnobR = 30.0f;

constexpr float padSize = 150.0f, padGapX = 36.666f, padLabelH = 26.0f, padRowGap = 16.0f, padTop = 430.0f, padLeft = 200.0f;
// pad index 0..15, MPC numbering: pad 1 bottom-left, pad 16 top-right
inline juce::Rectangle<float> pad (int index)
{
    const int col = index % 4, rowFromBottom = index / 4, row = 3 - rowFromBottom;
    const float x = padLeft + (float) col * (padSize + padGapX);
    const float y = padTop + (float) row * (padLabelH + padSize + padRowGap) + padLabelH;
    return { x, y, padSize, padSize };
}
inline juce::Rectangle<float> padLabel (int index) { auto p = pad (index); return { p.getX(), p.getY() - padLabelH, p.getWidth(), padLabelH - 5.0f }; }
inline juce::Point<float> padLed (int index) { auto p = pad (index); return { p.getRight() - 5.0f, p.getY() - padLabelH + 6.0f }; }

inline juce::Rectangle<float> logo() { return { 34.0f, 526.0f, 150.0f, 560.0f }; }
} // namespace thudi::layout

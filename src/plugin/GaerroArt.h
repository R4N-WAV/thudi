// Thudi — GÆRRO mode: the graffiti and stickers that get thrown on the machine (art made by tools/make_gaerro_art.py)
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace thudi::gaerro
{
enum class Kind { Tag, Sticker, Tape, Fade };

struct Element
{
    juce::String file;                  // embedded PNG (2x), already rotated
    juce::Rectangle<float> bounds;      // panel design units
    Kind kind = Kind::Sticker;
    float t0 = 0.0f, dur = 0.1f;        // reveal timing, fraction of the whole switch-on animation
    float dirX = 1.0f, dirY = 0.0f;     // spray / tape direction
};

const std::vector<Element>& elements();

// progress: 0 = clean machine, 1 = everything thrown on. alpha fades the whole layer (switching off)
void paintOverlay (juce::Graphics&, float progress, float alpha);

constexpr float kRevealSeconds = 2.6f;  // same length as the LCD loading cigarette
} // namespace thudi::gaerro

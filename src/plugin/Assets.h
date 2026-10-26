// Thudi — embedded images (Blender renders), fonts and the Tamil logo path
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace thudi::assets
{
juce::Image image (const juce::String& fileName);          // e.g. "bg.png"; invalid Image if not embedded
juce::Typeface::Ptr typeface (const juce::String& fileName);
juce::Font lcdFont (float height);       // VT323
juce::Font labelFont (float height);     // Barlow Condensed SemiBold
juce::Font labelFontMedium (float height);
const juce::Path& tamilLogo();           // துடி outline, origin top-left, natural size
}

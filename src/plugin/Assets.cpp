#include "Assets.h"
#include "BinaryData.h"

namespace thudi::assets
{
static const char* findResource (const juce::String& fileName, int& size)
{
    for (int i = 0; i < ThudiBinary::namedResourceListSize; ++i)
        if (fileName == ThudiBinary::originalFilenames[i])
            return ThudiBinary::getNamedResource (ThudiBinary::namedResourceList[i], size);
    size = 0;
    return nullptr;
}

juce::Image image (const juce::String& fileName)
{
    static std::map<juce::String, juce::Image> cache;
    auto it = cache.find (fileName);
    if (it != cache.end()) return it->second;
    int size = 0;
    juce::Image img;
    if (auto* d = findResource (fileName, size))
        img = juce::ImageCache::getFromMemory (d, size);
    cache[fileName] = img;
    return img;
}

juce::Typeface::Ptr typeface (const juce::String& fileName)
{
    static std::map<juce::String, juce::Typeface::Ptr> cache;
    auto it = cache.find (fileName);
    if (it != cache.end()) return it->second;
    int size = 0;
    juce::Typeface::Ptr tf;
    if (auto* d = findResource (fileName, size))
        tf = juce::Typeface::createSystemTypefaceFor (d, (size_t) size);
    cache[fileName] = tf;
    return tf;
}

static juce::Font fontOr (const juce::String& file, float h, const juce::String& fallbackName, bool bold)
{
    if (auto tf = typeface (file))
        return juce::Font (juce::FontOptions (tf).withHeight (h));
    return juce::Font (juce::FontOptions (fallbackName, h, bold ? juce::Font::bold : juce::Font::plain));
}

juce::Font lcdFont (float h)           { return fontOr ("VT323-Regular.ttf", h, juce::Font::getDefaultMonospacedFontName(), false); }
juce::Font labelFont (float h)         { return fontOr ("BarlowCondensed-SemiBold.ttf", h, juce::Font::getDefaultSansSerifFontName(), true); }
juce::Font labelFontMedium (float h)   { return fontOr ("BarlowCondensed-Medium.ttf", h, juce::Font::getDefaultSansSerifFontName(), false); }

const juce::Path& tamilLogo()
{
    static juce::Path p = []
    {
        juce::Path path;
        int size = 0;
        if (auto* d = findResource ("thudi_tamil.path", size))
            path.restoreFromString (juce::String::fromUTF8 (d, size));
        if (path.isEmpty())
        {   // fallback: Latin text outline
            juce::GlyphArrangement ga;
            ga.addLineOfText (juce::Font (juce::FontOptions (120.0f, juce::Font::italic)), "THUDI", 0, 0);
            ga.createPath (path);
            auto b = path.getBounds();
            path.applyTransform (juce::AffineTransform::translation (-b.getX(), -b.getY()));
        }
        return path;
    }();
    return p;
}
} // namespace thudi::assets

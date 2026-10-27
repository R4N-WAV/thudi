#include "GaerroArt.h"
#include "Assets.h"
#include "BinaryData.h"

namespace thudi::gaerro
{
static juce::String resourceText (const juce::String& name)
{
    for (int i = 0; i < ThudiBinary::namedResourceListSize; ++i)
        if (name == ThudiBinary::originalFilenames[i])
        {
            int size = 0;
            if (auto* d = ThudiBinary::getNamedResource (ThudiBinary::namedResourceList[i], size)) return juce::String::fromUTF8 (d, size);
        }
    return {};
}

const std::vector<Element>& elements()
{
    static std::vector<Element> list = []
    {
        std::vector<Element> out;
        auto v = juce::JSON::parse (resourceText ("gaerro_art.json"));
        if (auto* arr = v["elements"].getArray())
            for (auto& e : *arr)
            {
                Element el;
                el.file = e["file"].toString();
                el.bounds = { (float) e["x"], (float) e["y"], (float) e["w"], (float) e["h"] };
                const auto k = e["kind"].toString();
                el.kind = k == "tag" ? Kind::Tag : (k == "tape" ? Kind::Tape : (k == "fade" ? Kind::Fade : Kind::Sticker));
                el.t0 = (float) e["t0"]; el.dur = juce::jmax (0.01f, (float) e["dur"]);
                el.dirX = e.hasProperty ("dx") ? (float) e["dx"] : 1.0f;
                el.dirY = e.hasProperty ("dy") ? (float) e["dy"] : 0.0f;
                out.push_back (el);
            }
        return out;
    }();
    return list;
}

static float easeOut (float x) { x = juce::jlimit (0.0f, 1.0f, x); return 1.0f - (1.0f - x) * (1.0f - x) * (1.0f - x); }

// wipe along the spray / tape direction with a short soft edge (the can moving across)
static void paintWipe (juce::Graphics& g, const juce::Image& img, juce::Rectangle<float> b, float k, float dx, float dy, float soft, float alpha)
{
    const bool horizontal = std::abs (dx) >= std::abs (dy);
    const float len = horizontal ? b.getWidth() : b.getHeight();
    const float edge = len * k;
    const float feather = soft * len;
    auto part = [&] (float from, float to, float opacity)
    {
        if (to <= from) return;
        juce::Rectangle<float> clip;
        if (horizontal) clip = dx >= 0 ? juce::Rectangle<float> (b.getX() + from, b.getY(), to - from, b.getHeight())
                                       : juce::Rectangle<float> (b.getRight() - to, b.getY(), to - from, b.getHeight());
        else            clip = dy >= 0 ? juce::Rectangle<float> (b.getX(), b.getY() + from, b.getWidth(), to - from)
                                       : juce::Rectangle<float> (b.getX(), b.getBottom() - to, b.getWidth(), to - from);
        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (clip.getSmallestIntegerContainer());
        g.setOpacity (opacity * alpha);
        g.drawImage (img, b, juce::RectanglePlacement::stretchToFit);
    };
    part (0.0f, juce::jmin (len, edge - feather), 1.0f);
    if (feather > 0.0f)
        for (int i = 0; i < 4; ++i)
        {
            const float a = edge - feather + feather * (float) i / 4.0f, z = a + feather / 4.0f;
            part (juce::jmax (0.0f, a), juce::jmin (len, z), 0.85f - 0.2f * (float) i);
        }
}

void paintOverlay (juce::Graphics& g, float progress, float alpha)
{
    if (progress <= 0.0f || alpha <= 0.0f) return;
    for (auto& e : elements())
    {
        const float k = (progress - e.t0) / e.dur;
        if (k <= 0.0f) continue;
        auto img = assets::image (e.file);
        if (! img.isValid()) continue;
        juce::Graphics::ScopedSaveState s (g);
        if (k >= 1.0f)
        {
            g.setOpacity (alpha);
            g.drawImage (img, e.bounds, juce::RectanglePlacement::stretchToFit);
        }
        else if (e.kind == Kind::Fade)
        {
            g.setOpacity (alpha * k);
            g.drawImage (img, e.bounds, juce::RectanglePlacement::stretchToFit);
        }
        else if (e.kind == Kind::Sticker)
        {   // slapped on: comes in a little big, lands, presses flat
            const float x = easeOut (k);
            const float sc = 1.0f + 0.22f * (1.0f - x);
            const auto c = e.bounds.getCentre();
            g.setOpacity (alpha * juce::jmin (1.0f, k * 4.0f));
            g.drawImageTransformed (img, juce::AffineTransform::scale (e.bounds.getWidth() / (float) img.getWidth(), e.bounds.getHeight() / (float) img.getHeight())
                                             .translated (e.bounds.getX(), e.bounds.getY())
                                             .scaled (sc, sc, c.x, c.y - 6.0f * (1.0f - x)));
        }
        else
        {
            paintWipe (g, img, e.bounds, e.kind == Kind::Tag ? k * 1.15f : k, e.dirX, e.dirY, e.kind == Kind::Tag ? 0.15f : 0.0f, alpha);
        }
    }
}
} // namespace thudi::gaerro

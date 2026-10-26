// thudi_logo — shapes the Tamil word "துடி" with the Kavivanar font (HarfBuzz via JUCE 8)
// and writes it as an SVG path, so the plugin never depends on runtime Tamil shaping.
// usage: thudi_logo <Kavivanar-Regular.ttf> <out.svg> [preview.png]
#include <juce_graphics/juce_graphics.h>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI* noGui = nullptr; juce::ignoreUnused (noGui);
    if (argc < 3) { std::cerr << "usage: thudi_logo font.ttf out.svg [preview.png]\n"; return 1; }

    juce::MemoryBlock data;
    if (! juce::File (juce::String (argv[1])).loadFileAsData (data)) { std::cerr << "cannot read font\n"; return 2; }
    auto tf = juce::Typeface::createSystemTypefaceFor (data.getData(), data.getSize());
    if (tf == nullptr) { std::cerr << "bad font\n"; return 3; }

    const juce::String word = juce::CharPointer_UTF8 ("\xe0\xae\xa4\xe0\xaf\x81\xe0\xae\x9f\xe0\xae\xbf"); // துடி
    juce::Font font (juce::FontOptions (tf).withHeight (200.0f));

    juce::GlyphArrangement ga;
    ga.addLineOfText (font, word, 0.0f, 0.0f);
    std::cout << "glyphs: " << ga.getNumGlyphs() << "\n";
    for (int i = 0; i < ga.getNumGlyphs(); ++i)
        std::cout << "  glyph x=" << ga.getGlyph (i).getLeft() << " w=" << ga.getGlyph (i).getRight() - ga.getGlyph (i).getLeft() << "\n";

    juce::Path p;
    ga.createPath (p);
    auto b = p.getBounds();
    p.applyTransform (juce::AffineTransform::translation (-b.getX(), -b.getY()));
    b = p.getBounds();
    std::cout << "bounds " << b.toString() << "\n";

    // flattened outlines (for the Blender emblem) + a real SVG
    juce::String svgD, json = "{\"width\": " + juce::String (b.getWidth()) + ", \"height\": " + juce::String (b.getHeight()) + ", \"loops\": [";
    juce::PathFlatteningIterator it (p, {}, 0.15f);
    bool first = true, firstLoop = true;
    juce::String loop;
    auto endLoop = [&]
    {
        if (loop.isNotEmpty()) { json << (firstLoop ? "" : ",") << "[" << loop << "]"; firstLoop = false; }
        loop.clear(); first = true;
    };
    while (it.next())
    {
        if (first) { svgD << "M" << it.x1 << " " << it.y1 << " "; loop << "[" << it.x1 << "," << it.y1 << "]"; first = false; }
        svgD << "L" << it.x2 << " " << it.y2 << " ";
        loop << ",[" << it.x2 << "," << it.y2 << "]";
        if (it.closesSubPath) { svgD << "Z "; endLoop(); }
    }
    endLoop();
    json << "]}";
    juce::String svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 " << b.getWidth() << " " << b.getHeight() << "\">\n"
        << "<!-- Thudi (Tamil) set in Kavivanar (SIL OFL 1.1), shaped with HarfBuzz -->\n"
        << "<path fill=\"#000\" fill-rule=\"evenodd\" d=\"" << svgD.trim() << "\"/>\n</svg>\n";
    juce::File (juce::String (argv[2])).replaceWithText (svg);
    juce::File (juce::String (argv[2])).withFileExtension ("path").replaceWithText (p.toString());
    juce::File (juce::String (argv[2])).withFileExtension ("json").replaceWithText (json);

    if (argc > 3)
    {
        juce::Image img (juce::Image::ARGB, (int) b.getWidth() + 40, (int) b.getHeight() + 40, true);
        juce::Graphics g (img);
        g.fillAll (juce::Colours::white);
        g.setColour (juce::Colours::black);
        g.fillPath (p, juce::AffineTransform::translation (20, 20));
        juce::File out { juce::String (argv[3]) };
        out.deleteFile();
        juce::FileOutputStream os { out };
        juce::PNGImageFormat().writeImageToStream (img, os);
    }
    return 0;
}

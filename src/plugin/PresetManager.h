// Thudi — kit presets (all 16 pads) and pad presets (one sound), factory + user, clipboard actions
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "ThudiParams.h"

class ThudiProcessor;

class PresetManager
{
public:
    struct Entry
    {
        juce::String name, category; // category: "" for kits, model category for pads
        bool factory = false;
        juce::File file;              // user presets
        juce::String resource;        // factory presets (BinaryData original filename)
    };

    explicit PresetManager (ThudiProcessor& p);

    static juce::File userRoot();
    static juce::File kitsDir();
    static juce::File padsDir (const juce::String& category);

    std::vector<Entry> listKits() const;
    std::vector<Entry> listPads (const juce::String& category = {}) const; // empty = all
    static juce::StringArray padCategories();

    bool loadDefaultKit();
    void initKit();               // empty machine
    bool loadKit (const Entry&);
    bool saveKit (const juce::String& name);
    bool loadPad (int pad, const Entry&);
    bool savePad (int pad, const juce::String& name);
    int  currentKitIndex() const;

    juce::String kitToJson() const;
    bool kitFromJson (const juce::String& json);
    juce::String padToJson (int pad) const;
    bool padFromJson (int pad, const juce::String& json, bool includeLabel);

    // pad actions (right-click menu)
    void copyPad (int pad);
    bool canPaste() const;
    void pastePad (int pad, bool includeLabel);
    void swapPads (int a, int b);
    void duplicatePad (int from, int to);
    void initPad (int pad, int model);
    void randomizePad (int pad, float amount = 0.3f);

    juce::String lastError;

private:
    ThudiProcessor& proc;
    juce::String clipboard;
    juce::String readEntry (const Entry&) const;
    void restoreSample (int slot, const std::string& path);
};

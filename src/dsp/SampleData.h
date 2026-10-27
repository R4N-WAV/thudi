// Thudi — an audio file loaded for gærro-mode chopping (mono, original sample rate)
#pragma once
#include <memory>
#include <string>
#include <vector>

namespace thudi
{
struct SampleData
{
    std::vector<float> data;   // mono
    double rate = 48000.0;
    std::string name;          // file name shown on the LCD
    std::string path;          // where it was loaded from (kept with the kit)
    size_t size() const { return data.size(); }
};
using SamplePtr = std::shared_ptr<const SampleData>;

// Transient-based chop points (sample indices, ascending, first is 0). Falls back to equal slices.
std::vector<size_t> chopPoints (const SampleData& s, int numSlices);
} // namespace thudi

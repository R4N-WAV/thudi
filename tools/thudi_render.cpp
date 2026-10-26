// thudi_render — offline renderer for kits (used for tests, preset design and CI checks)
// usage: thudi_render <kit.thudikit> <pattern.txt> <out.wav> [samplerate] [os]
// pattern file:  "bpm 88" / "steps 32" / lines: <step> <pad 1-16 | note:N> <velocity 0-1> [length_steps]
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include "../src/dsp/Engine.h"
#include "../src/dsp/KitJson.h"

using namespace thudi;

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    if (! f) { std::cerr << "cannot write " << path << "\n"; return; }
    const uint32_t n = (uint32_t) L.size(), dataBytes = n * 2 * 4;
    auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); w32 (36 + dataBytes); std::fwrite ("WAVE", 1, 4, f);
    std::fwrite ("fmt ", 1, 4, f); w32 (16); w16 (3); w16 (2); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 8); w16 (8); w16 (32);
    std::fwrite ("data", 1, 4, f); w32 (dataBytes);
    for (uint32_t i = 0; i < n; ++i) { std::fwrite (&L[i], 4, 1, f); std::fwrite (&R[i], 4, 1, f); }
    std::fclose (f);
}

int main (int argc, char** argv)
{
    if (argc < 4) { std::cerr << "usage: thudi_render kit pattern out.wav [sr] [os]\n"; return 1; }
    const int sr = argc > 4 ? std::atoi (argv[4]) : 48000;
    const int os = argc > 5 ? std::atoi (argv[5]) : 2;

    std::ifstream kf (argv[1]); std::stringstream ks; ks << kf.rdbuf();
    json::Kit kit;
    if (! json::parseKit (ks.str(), kit)) { std::cerr << "bad kit json\n"; return 2; }

    Engine eng;
    eng.prepare (sr, 256, os);
    for (int p = 0; p < kNumPads; ++p) { eng.pad (p) = kit.pads[p].sound; eng.setPadNote (p, kit.pads[p].note); }
    for (int g = 0; g < kNumGlobalParams; ++g) eng.global[g] = kit.global[g];

    float bpm = 88; int steps = 32;
    struct Ev { double t; int note; float vel; bool on; };
    std::vector<Ev> evs;
    std::ifstream pf (argv[2]); std::string line;
    while (std::getline (pf, line))
    {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ls (line); std::string a; ls >> a;
        if (a == "bpm") { ls >> bpm; continue; }
        if (a == "steps") { ls >> steps; continue; }
        double step = std::atof (a.c_str()); std::string padS; float vel = 1, len = 0.5f;
        ls >> padS >> vel; ls >> len;
        int note;
        if (padS.rfind ("note:", 0) == 0) note = std::atoi (padS.c_str() + 5);
        else note = kit.pads[std::max (0, std::min (15, std::atoi (padS.c_str()) - 1))].note;
        const double stepSec = 60.0 / bpm / 4.0;
        evs.push_back ({ step * stepSec, note, vel, true });
        evs.push_back ({ (step + std::max (0.05f, len)) * stepSec, note, 0.0f, false });
    }
    std::sort (evs.begin(), evs.end(), [] (const Ev& x, const Ev& y) { return x.t < y.t || (x.t == y.t && ! x.on && y.on); });

    const double total = steps * 60.0 / bpm / 4.0 + 1.0;
    const int N = (int) (total * sr);
    std::vector<float> L ((size_t) N, 0.0f), R ((size_t) N, 0.0f);
    const int block = 256;
    std::vector<float> bufs (32 * block);
    float* chans[32]; for (int c = 0; c < 32; ++c) chans[c] = bufs.data() + c * block;
    size_t ei = 0;
    const int lat = eng.latencySamples();
    for (int pos = 0; pos < N; pos += block)
    {
        int n = std::min (block, N - pos);
        std::vector<NoteEvent> blockEv;
        while (ei < evs.size() && (int) (evs[ei].t * sr) < pos + n)
        {
            blockEv.push_back ({ std::max (0, (int) (evs[ei].t * sr) - pos), evs[ei].note, evs[ei].vel, evs[ei].on });
            ++ei;
        }
        eng.process (blockEv.data(), (int) blockEv.size(), chans, 32, n);
        for (int i = 0; i < n; ++i)
        {
            float l = 0, r = 0;
            for (int b = 0; b < 16; ++b) { l += chans[b * 2][i]; r += chans[b * 2 + 1][i]; }
            int o = pos + i - lat;
            if (o >= 0) { L[(size_t) o] = l; R[(size_t) o] = r; }
        }
    }
    writeWav (argv[3], L, R, sr);
    return 0;
}

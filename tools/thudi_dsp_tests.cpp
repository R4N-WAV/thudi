// thudi_dsp_tests — checks of the engine features that have no reference recording
#include <cmath>
#include <cstdio>
#include <memory>
#include "../src/dsp/Engine.h"
#include "../src/dsp/Models.h"

using namespace thudi;
static int fails = 0;
static void check (bool ok, const char* what) { std::printf ("  [%s] %s\n", ok ? "ok" : "FAIL", what); if (! ok) ++fails; }

struct Rig
{
    Engine e; float buf[32][256]; float* ch[32];
    Rig() { for (int c = 0; c < 32; ++c) ch[c] = buf[c]; e.prepare (48000, 256, 2); }
    float block (std::vector<NoteEvent> ev = {})
    {
        e.process (ev.data(), (int) ev.size(), ch, 32, 256);
        float pk = 0; for (int c = 0; c < 2; ++c) for (int i = 0; i < 256; ++i) { if (! std::isfinite (buf[c][i])) return -1; pk = std::max (pk, std::abs (buf[c][i])); }
        return pk;
    }
};

int main()
{
    std::printf ("Thudi DSP tests\n");
    {   // empty pads are silent, a kick lights its pad and the glow follows its envelope
        Rig r;
        for (int i = 0; i < kNumPads; ++i) r.e.pad (i) = initSoundForModel ((int) Model::Empty);
        float pk = r.block ({ { 0, 36, 1.0f, true } });
        check (pk == 0.0f, "empty pad is silent");
        r.e.pad (0) = initSoundForModel ((int) Model::Kick);
        r.block ({ { 0, 36, 1.0f, true } });
        const float l0 = r.e.padLight[0].load();
        for (int b = 0; b < 40; ++b) r.block();          // ~210 ms later
        const float l1 = r.e.padLight[0].load();
        for (int b = 0; b < 200; ++b) r.block();
        const float l2 = r.e.padLight[0].load();
        std::printf ("    light: %.3f -> %.3f -> %.3f\n", l0, l1, l2);
        check (l0 > 0.5f && l1 < l0 && l2 < 0.01f, "pad glow rises on the hit and fades with the decay");
        check (r.e.padLight[1].load() == 0.0f, "other pads stay dark");
    }
    {   // solo mode: every note plays the same sound, transposed; mono legato glide keeps one voice
        Rig r;
        r.e.global[(int) G::SoloOn] = 1;
        auto s = initSoundForModel ((int) Model::Bass808);
        s[P::Poly] = 0; s[P::Glide] = 80; s[P::PlayMode] = 1;
        r.e.pad (kSoloSlot) = s;
        float pk1 = r.block ({ { 0, 60, 1.0f, true } });
        for (int b = 0; b < 10; ++b) r.block();
        float pk2 = r.block ({ { 0, 67, 1.0f, true } });
        check (pk1 > 0.05f && pk2 > 0.05f, "solo notes sound");
        check (r.e.padLight[7].load() > 0.1f && r.e.padLight[0].load() < 0.01f, "solo glow moves to the pad of the note (C3 + 7 -> pad 8)");
        r.block ({ { 0, 67, 0.0f, false } });
        float held = 0; for (int b = 0; b < 5; ++b) held = std::max (held, r.block());
        check (held > 0.05f, "mono legato: releasing the top note glides back, the sound keeps going");
        r.block ({ { 0, 60, 0.0f, false } });
        float tail = 0; for (int b = 0; b < 40; ++b) tail = r.block();
        check (tail < 0.01f, "gate release when the last note is let go");
        float pad16 = r.block ({ { 0, 40, 1.0f, true } });
        check (pad16 > 0.01f, "solo mode ignores pad mapping (any note plays)");
    }
    {   // samples: a 1 s saw, chopped and played back
        Rig r;
        auto sd = std::make_shared<SampleData>();
        sd->rate = 44100;
        for (int i = 0; i < 44100; ++i) sd->data.push_back (((i % 200) / 100.0f - 1.0f) * ((i / 11025) % 2 ? 1.0f : 0.1f));
        auto pts = chopPoints (*sd, 4);
        std::printf ("    chop points:"); for (auto p : pts) std::printf (" %zu", p); std::printf ("\n");
        check (pts.size() == 4 && pts[0] == 0, "chop gives 4 slices");
        auto s = initSoundForModel ((int) Model::Sample);
        s[P::SmpStart] = 0.25f; s[P::SmpEnd] = 0.5f;
        r.e.pad (0) = s; r.e.setPadSample (0, sd);
        float pk = r.block ({ { 0, 36, 1.0f, true } });
        int blocks = 1; while (r.block() > 1e-4f && blocks < 400) ++blocks;
        const float secs = blocks * 256.0f / 48000.0f;
        std::printf ("    slice plays %.3f s (expected ~0.25 s)\n", secs);
        check (pk > 0.05f && secs > 0.2f && secs < 0.35f, "sample slice plays its region");
        s[P::SmpSpeed] = 2.0f; r.e.pad (0) = s;
        r.block ({ { 0, 36, 1.0f, true } }); blocks = 1; while (r.block() > 1e-4f && blocks < 400) ++blocks;
        check (blocks * 256.0f / 48000.0f < 0.18f, "speed 2x halves the slice");
    }
    {   // gærro: noise floor even in silence, nothing blows up
        Rig r;
        r.e.global[(int) G::Gaerro] = 1; r.e.global[(int) G::Vinyl] = 1;
        float mx = 0; for (int b = 0; b < 400; ++b) { float p = r.block(); if (p < 0) { mx = -1; break; } mx = std::max (mx, p); }
        std::printf ("    vinyl floor peak %.4f\n", mx);
        check (mx > 0.0005f && mx < 0.5f, "vinyl noise floor in gærro mode");
        r.e.global[(int) G::Gaerro] = 0;
        for (int b = 0; b < 600; ++b) r.block();
        check (r.block() < 1e-4f, "back to silence when gærro is off");
    }
    std::printf ("%s\n", fails ? "SOME DSP TESTS FAILED" : "ALL DSP TESTS PASSED");
    return fails ? 1 : 0;
}

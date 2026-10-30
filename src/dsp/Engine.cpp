#include "Engine.h"
#include <algorithm>
#include <cstring>

// sample pointers are swapped from the message thread; the web build (WASI, single-threaded worklet) has no atomics
#if defined(__wasi__)
 #define THUDI_SAMPLE_STORE(dst, v) (dst) = (v)
 #define THUDI_SAMPLE_LOAD(src) (src)
#else
 #define THUDI_SAMPLE_STORE(dst, v) std::atomic_store (&(dst), v)
 #define THUDI_SAMPLE_LOAD(src) std::atomic_load (&(src))
#endif

namespace thudi
{
// ===================================================================== GÆRRO stage
void GaerroFx::prepare (double sampleRate, uint32_t seed)
{
    sr = sampleRate;
    dl.prepare ((int) (0.012 * sr)); dr.prepare ((int) (0.012 * sr));
    hpL.set (32.0f, 0.6f, (float) sr); hpR.set (32.0f, 0.6f, (float) sr);
    lpHz = -1; rng.setSeed (seed);
    crackHp.set (900.0f, (float) sr);
    amtCoef = 1.0f - std::exp (-1.0f / (0.6f * (float) sr)); // ~0.6 s cross-fade when toggled
}

void GaerroFx::process (float* L, float* R, int n, float target, float dirt, float vinyl, bool noiseFloor)
{
    if (amt < 1e-4f && target < 1e-4f) { amt = 0; return; }
    const float lp = 16500.0f - 7500.0f * vinyl - 2500.0f * dirt;
    if (std::abs (lp - lpHz) > 1.0f) { lpL.set (lp, 0.6f, (float) sr); lpR.set (lp, 0.6f, (float) sr); lpHz = lp; }
    const double holdRate = (26040.0 - 10000.0 * dirt) / sr;         // sampler rate (SP style)
    const float q = std::exp2 (12.0f - 3.0f * dirt - 1.0f);          // 12 -> 9 bits
    const float sat = 1.0f + dirt * 2.2f, satN = 1.0f / std::tanh (sat);
    const float hiss = 0.0022f * (0.4f + vinyl), rum = 0.006f * vinyl;
    const float crackRate = (float) ((2.0 + 14.0 * vinyl) / sr);
    const double wowInc = 0.55 / sr, flutInc = 5.7 / sr;
    for (int i = 0; i < n; ++i)
    {
        amt += amtCoef * (target - amt);
        const float xl = L[i], xr = R[i];
        // wow & flutter (pitch wobble of a slightly warped record)
        wowPh += wowInc; if (wowPh >= 1) wowPh -= 1;
        flutPh += flutInc; if (flutPh >= 1) flutPh -= 1;
        const float d = (float) ((0.003 + vinyl * (0.00055 * std::sin (6.2831853 * wowPh) + 0.00006 * std::sin (6.2831853 * flutPh))) * sr);
        dl.push (xl); dr.push (xr);
        float yl = dl.read (d), yr = dr.read (d);
        // groove band limit
        yl = lpL.lp (hpL.hp (yl)); yr = lpR.lp (hpR.hp (yr));
        // sampler grit: zero-order hold + bit depth
        holdPh += (float) holdRate;
        if (holdPh >= 1.0f) { holdPh -= std::floor (holdPh); holdL = std::round (yl * q) / q; holdR = std::round (yr * q) / q; }
        yl = yl + (0.35f + 0.6f * dirt) * (holdL - yl);
        yr = yr + (0.35f + 0.6f * dirt) * (holdR - yr);
        yl = std::tanh (yl * sat) * satN; yr = std::tanh (yr * sat) * satN;
        // noise floor: hiss, rumble, crackle
        if (noiseFloor && vinyl > 0.001f)
        {
            const float wl = rng.bi(), wr = rng.bi();
            pinkL[0] = 0.99765f * pinkL[0] + wl * 0.099046f; pinkL[1] = 0.963f * pinkL[1] + wl * 0.2965164f; pinkL[2] = 0.57f * pinkL[2] + wl * 1.0526913f;
            pinkR[0] = 0.99765f * pinkR[0] + wr * 0.099046f; pinkR[1] = 0.963f * pinkR[1] + wr * 0.2965164f; pinkR[2] = 0.57f * pinkR[2] + wr * 1.0526913f;
            rumble += 0.0025f * (rng.bi() - rumble);
            if (rng.uni() < crackRate) { const float u = rng.uni(); crackEnv = 0.025f + 0.22f * u * u * u; crackSign = rng.bi() > 0 ? 1.0f : -1.0f; crackPan = rng.bi() * 0.6f; }
            const float c = crackEnv * crackSign * (rng.uni() * 0.6f + 0.4f);
            crackEnv *= 0.92f;
            const float nl = hiss * (pinkL[0] + pinkL[1] + pinkL[2]) * 0.25f + rum * rumble + vinyl * c * (1.0f - crackPan) * 0.5f;
            const float nr = hiss * (pinkR[0] + pinkR[1] + pinkR[2]) * 0.25f + rum * rumble + vinyl * c * (1.0f + crackPan) * 0.5f;
            yl += nl; yr += nr;
        }
        L[i] = xl + amt * (yl - xl);
        R[i] = xr + amt * (yr - xr);
    }
}

// ===================================================================== Engine
Engine::Engine()
{
    pads.resize (kNumSlots);
    for (int i = 0; i < kNumSlots; ++i) pads[(size_t) i].note = i < kNumPads ? 36 + i : -1;
    for (int i = 0; i < kNumPads; ++i) { padActivity[i].store (0.0f); padLight[i].store (0.0f); }
    for (int g = 0; g < kNumGlobalParams; ++g) global[g] = globalTable()[(size_t) g].def;
}

void Engine::prepare (double sampleRate, int maxBlockSize, int oversamplingFactor)
{
    sr = sampleRate; maxBlock = std::max (16, maxBlockSize); osFactor = oversamplingFactor;
    seedCounter = 1234; // reproducible renders after every prepare
    mono.assign ((size_t) maxBlock, 0.0f); padL.assign ((size_t) maxBlock, 0.0f); padR.assign ((size_t) maxBlock, 0.0f);
    for (int i = 0; i < kNumSlots; ++i)
    {
        auto& p = pads[(size_t) i];
        for (auto& v : p.voices) v.prepare ((float) sr);
        p.chain.prepare ((float) sr, maxBlock, osFactor, (uint32_t) (i + 1));
        p.silentSamples = 1 << 30;
    }
    for (int b = 0; b < kNumOutputs; ++b) gaerro[b].prepare (sr, (uint32_t) (101 + b));
}

void Engine::reset()
{
    for (auto& p : pads)
    {
        for (auto& v : p.voices) v.release (0.001f);
        p.chain.reset(); p.silentSamples = 1 << 30;
    }
    soloHeld.clear();
}

void Engine::setPadSample (int slot, SamplePtr s)
{
    if (slot < 0 || slot >= kNumSlots) return;
    THUDI_SAMPLE_STORE (pads[(size_t) slot].sample, std::move (s));
}

void Engine::queueUiHit (int padIndex, float velocity)
{
    int w = fifoW.load (std::memory_order_relaxed);
    int nw = (w + 1) % kFifo;
    if (nw == fifoR.load (std::memory_order_acquire)) return; // full
    fifo[w] = { padIndex, velocity, true };
    fifoW.store (nw, std::memory_order_release);
}

void Engine::queueUiRelease (int padIndex)
{
    int w = fifoW.load (std::memory_order_relaxed);
    int nw = (w + 1) % kFifo;
    if (nw == fifoR.load (std::memory_order_acquire)) return;
    fifo[w] = { padIndex, 0.0f, false };
    fifoW.store (nw, std::memory_order_release);
}

void Engine::chokeGroup (int group, int exceptPad)
{
    if (group <= 0) return;
    for (int i = 0; i < kNumPads; ++i)
    {
        if (i == exceptPad) continue;
        auto& p = pads[(size_t) i];
        if ((int) std::round (p.sound[P::Choke]) == group)
            for (int k = 0; k < numVoices (i); ++k) if (p.voices[k].isActive()) p.voices[k].kill();
    }
}

int Engine::freeVoice (int slotIdx)
{
    auto& p = pads[(size_t) slotIdx];
    const int nv = numVoices (slotIdx);
    for (int k = 0; k < nv; ++k) if (! p.voices[k].isActive()) return k;
    int oldest = 0;
    for (int k = 1; k < nv; ++k) if (p.voices[k].age() > p.voices[oldest].age()) oldest = k;
    return oldest;
}

void Engine::triggerPad (int idx, float vel, float extraSemis, int depth)
{
    if (idx < 0 || idx >= kNumPads || depth > 2) return;
    auto& p = pads[(size_t) idx];
    const auto& s = p.sound;
    if ((int) std::round (s[P::Model]) == (int) Model::Empty) return;
    const float semi = s[P::Tune] + extraSemis;
    const bool mono = (int) std::round (s[P::Poly]) == 0;

    chokeGroup ((int) std::round (s[P::Choke]), idx);
    if (mono) for (int k = 0; k < numVoices (idx); ++k) if (p.voices[k].isActive()) p.voices[k].kill();
    const int slot = freeVoice (idx);
    p.voices[slot].start (s, semi, vel, seedCounter++, THUDI_SAMPLE_LOAD (p.sample));
    p.voices[slot].lightPad = idx;
    p.lastVoice = slot;
    p.silentSamples = 0;
    p.keyHeld = true;
    padActivity[idx].store (std::max (padActivity[idx].load(), vel));
    lastHitPad.store (idx);

    const int link = (int) std::round (s[P::Link]);
    if (link >= 1 && link <= kNumPads && link - 1 != idx)
        triggerPad (link - 1, vel, 0.0f, depth + 1);
}

void Engine::releasePad (int idx)
{
    auto relIfGate = [this] (int i)
    {
        auto& p = pads[(size_t) i];
        if ((int) std::round (p.sound[P::PlayMode]) != 1) return;
        const float rel = p.sound[P::Release] * 0.001f;
        const float r = (int) std::round (p.sound[P::Model]) == (int) Model::Sample ? std::max (0.002f, p.sound[P::SmpRelease] * 0.001f) : rel;
        for (int k = 0; k < numVoices (i); ++k)
            if (p.voices[k].isActive() && ! p.voices[k].isReleasing()) p.voices[k].release (r);
    };
    pads[(size_t) idx].keyHeld = false;
    relIfGate (idx);
    const int link = (int) std::round (pads[(size_t) idx].sound[P::Link]);
    if (link >= 1 && link <= kNumPads && link - 1 != idx) relIfGate (link - 1);
}

// ---------------------------------------------------------------- solo mode: one sound across every note
void Engine::soloOnNote (int note, float vel)
{
    auto& p = pads[(size_t) kSoloSlot];
    const auto& s = p.sound;
    if ((int) std::round (s[P::Model]) == (int) Model::Empty) return;
    const int rel = note - soloRoot();
    const float semi = s[P::Tune] + (float) rel;
    const bool mono = (int) std::round (s[P::Poly]) == 0;
    const float glide = s[P::Glide];
    const int light = ((rel % 16) + 16) % 16;
    soloHeld.push_back (note);

    if (mono && glide > 0.5f && soloHeld.size() > 1 && p.lastVoice >= 0 && p.voices[p.lastVoice].isActive() && ! p.voices[p.lastVoice].isReleasing())
    {
        p.voices[p.lastVoice].glideTo (semi, glide);   // legato: the sound keeps going, only the pitch moves
        p.voices[p.lastVoice].lightPad = light;
    }
    else
    {
        if (mono) for (int k = 0; k < kSoloVoices; ++k) if (p.voices[k].isActive()) p.voices[k].kill();
        const int slot = freeVoice (kSoloSlot);
        p.voices[slot].start (s, semi, vel, seedCounter++, THUDI_SAMPLE_LOAD (p.sample));
        p.voices[slot].lightPad = light;
        p.lastVoice = slot;
    }
    p.silentSamples = 0;
    padActivity[light].store (std::max (padActivity[light].load(), vel));
}

void Engine::soloOffNote (int note)
{
    auto it = std::find (soloHeld.begin(), soloHeld.end(), note);
    if (it == soloHeld.end()) return;
    soloHeld.erase (it);
    auto& p = pads[(size_t) kSoloSlot];
    const auto& s = p.sound;
    const bool mono = (int) std::round (s[P::Poly]) == 0;
    const bool gate = (int) std::round (s[P::PlayMode]) == 1;
    if (mono)
    {
        if (! soloHeld.empty() && s[P::Glide] > 0.5f && p.lastVoice >= 0 && p.voices[p.lastVoice].isActive())
        {   // back to the note still held
            const int rel = soloHeld.back() - soloRoot();
            p.voices[p.lastVoice].glideTo (s[P::Tune] + (float) rel, s[P::Glide]);
            p.voices[p.lastVoice].lightPad = ((rel % 16) + 16) % 16;
        }
        else if (soloHeld.empty() && gate && p.lastVoice >= 0)
            p.voices[p.lastVoice].release (s[P::Release] * 0.001f);
    }
    else if (gate)
    {
        // poly: release the voice that plays this note (match by its light pad)
        const int light = (((note - soloRoot()) % 16) + 16) % 16;
        for (int k = 0; k < kSoloVoices; ++k)
            if (p.voices[k].isActive() && ! p.voices[k].isReleasing() && p.voices[k].lightPad == light) { p.voices[k].release (s[P::Release] * 0.001f); break; }
    }
}

void Engine::noteOn (int note, float vel)
{
    if (soloOn()) { soloOnNote (note, vel); return; }
    for (int i = 0; i < kNumPads; ++i)
        if (pads[(size_t) i].note == note) triggerPad (i, vel, 0.0f, 0);
}

void Engine::noteOff (int note)
{
    if (soloOn()) { soloOffNote (note); return; }
    for (int i = 0; i < kNumPads; ++i)
        if (pads[(size_t) i].note == note) releasePad (i);
}

void Engine::renderSegment (float* const* outputs, int numChannels, int start, int len)
{
    const int numBuses = std::max (1, numChannels / 2);
    for (int i = 0; i < kNumSlots; ++i)
    {
        auto& p = pads[(size_t) i];
        const int nv = numVoices (i);
        bool anyActive = false;
        for (int k = 0; k < nv; ++k) anyActive |= p.voices[k].isActive();
        if (! anyActive && p.silentSamples > (int) (0.06 * sr)) continue;

        std::fill (mono.begin(), mono.begin() + len, 0.0f);
        for (int k = 0; k < nv; ++k) if (p.voices[k].isActive()) p.voices[k].render (p.sound, mono.data(), len);

        p.chain.process (p.sound, mono.data(), padL.data(), padR.data(), len);

        float peak = 0;
        for (int k = 0; k < len; ++k) peak = std::max (peak, std::abs (padL[(size_t) k]) + std::abs (padR[(size_t) k]));
        if (anyActive || peak > 1e-6f) p.silentSamples = anyActive ? 0 : p.silentSamples + len;
        else p.silentSamples += len;

        int bus = (int) std::round (p.sound[P::Output]);
        if (bus < 0 || bus >= numBuses) bus = 0;
        float* L = outputs[bus * 2] + start;
        float* R = outputs[bus * 2 + 1] + start;
        for (int k = 0; k < len; ++k) { L[k] += padL[(size_t) k]; R[k] += padR[(size_t) k]; }
    }
}

void Engine::process (const NoteEvent* events, int numEvents, float* const* outputs, int numChannels, int numSamples)
{
    for (int c = 0; c < numChannels; ++c) std::memset (outputs[c], 0, sizeof (float) * (size_t) numSamples);

    // UI hits at block start (in solo mode the pads play the solo sound, one semitone apart)
    while (fifoR.load (std::memory_order_relaxed) != fifoW.load (std::memory_order_acquire))
    {
        int r = fifoR.load (std::memory_order_relaxed);
        auto h = fifo[r];
        if (soloOn()) { if (h.on) soloOnNote (soloRoot() + h.pad, h.vel); else soloOffNote (soloRoot() + h.pad); }
        else if (h.on) triggerPad (h.pad, h.vel, 0.0f, 0); else releasePad (h.pad);
        fifoR.store ((r + 1) % kFifo, std::memory_order_release);
    }

    int pos = 0, e = 0;
    while (pos < numSamples)
    {
        while (e < numEvents && events[e].offset <= pos)
        {
            if (events[e].on && events[e].velocity > 0) noteOn (events[e].note, events[e].velocity);
            else noteOff (events[e].note);
            ++e;
        }
        int next = numSamples;
        if (e < numEvents) next = std::min (numSamples, std::max (pos + 1, events[e].offset));
        int len = next - pos;
        while (len > 0)
        {
            int chunk = std::min (len, maxBlock);
            renderSegment (outputs, numChannels, pos, chunk);
            pos += chunk; len -= chunk;
        }
    }

    // GÆRRO stage (main bus carries the record's noise floor)
    const float gTarget = global[(int) G::Gaerro] >= 0.5f ? 1.0f : 0.0f;
    const int numBuses = std::max (1, numChannels / 2);
    for (int b = 0; b < numBuses && b < kNumOutputs; ++b)
        gaerro[b].process (outputs[b * 2], outputs[b * 2 + 1], numSamples, gTarget, global[(int) G::GaerroDirt], global[(int) G::Vinyl], b == 0);

    // master + bus drive
    const float master = dbToGain (global[(int) G::Master]);
    const float bd = global[(int) G::BusDrive];
    const float g = 1.0f + bd * 3.0f, ng = 1.0f / std::tanh (g);
    for (int c = 0; c < numChannels; ++c)
    {
        float* o = outputs[c];
        if (bd > 0.001f) for (int k = 0; k < numSamples; ++k) o[k] = std::tanh (o[k] * g) * ng * master;
        else for (int k = 0; k < numSamples; ++k) o[k] *= master;
    }

    // pad glow = envelope of what each pad is playing right now
    float light[kNumPads] {};
    for (int i = 0; i < kNumSlots; ++i)
        for (int k = 0; k < numVoices (i); ++k)
        {
            const auto& v = pads[(size_t) i].voices[k];
            if (v.isActive() && v.lightPad >= 0 && v.lightPad < kNumPads) light[v.lightPad] = std::max (light[v.lightPad], v.light());
        }
    for (int i = 0; i < kNumPads; ++i) padLight[i].store (std::min (1.0f, light[i]));

    const float dec = std::exp (-(float) numSamples / (float) (0.15 * sr));
    for (int i = 0; i < kNumPads; ++i) padActivity[i].store (padActivity[i].load() * dec);
}
} // namespace thudi

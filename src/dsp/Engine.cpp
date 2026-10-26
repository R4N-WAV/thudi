#include "Engine.h"
#include <algorithm>
#include <cstring>

namespace thudi
{
Engine::Engine()
{
    pads.resize (kNumPads);
    for (int i = 0; i < kNumPads; ++i) { pads[(size_t) i].note = 36 + i; padActivity[i].store (0.0f); }
    for (int g = 0; g < kNumGlobalParams; ++g) global[g] = globalTable()[(size_t) g].def;
}

void Engine::prepare (double sampleRate, int maxBlockSize, int oversamplingFactor)
{
    sr = sampleRate; maxBlock = std::max (16, maxBlockSize); osFactor = oversamplingFactor;
    seedCounter = 1234; // reproducible renders after every prepare
    mono.assign ((size_t) maxBlock, 0.0f); padL.assign ((size_t) maxBlock, 0.0f); padR.assign ((size_t) maxBlock, 0.0f);
    for (int i = 0; i < kNumPads; ++i)
    {
        auto& p = pads[(size_t) i];
        for (auto& v : p.voices) v.prepare ((float) sr);
        p.chain.prepare ((float) sr, maxBlock, osFactor, (uint32_t) (i + 1));
        p.silentSamples = 1 << 30;
    }
}

void Engine::reset()
{
    for (auto& p : pads)
    {
        for (auto& v : p.voices) v.release (0.001f);
        p.chain.reset(); p.silentSamples = 1 << 30;
    }
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
            for (auto& v : p.voices) if (v.isActive()) v.kill();
    }
}

void Engine::triggerPad (int idx, float vel, float extraSemis, int depth, bool fromKeys)
{
    if (idx < 0 || idx >= kNumPads || depth > 2) return;
    auto& p = pads[(size_t) idx];
    const auto& s = p.sound;
    const float semi = s[P::Tune] + extraSemis;
    const bool mono = (int) std::round (s[P::Poly]) == 0;
    const float glide = s[P::Glide];

    chokeGroup ((int) std::round (s[P::Choke]), idx);

    // legato glide (mono pad, voice still sounding, glide > 0, only for keys playing)
    if (mono && glide > 0.5f && fromKeys && p.lastVoice >= 0 && p.voices[p.lastVoice].isActive() && ! p.voices[p.lastVoice].isReleasing())
    {
        p.voices[p.lastVoice].glideTo (semi, glide);
    }
    else
    {
        int slot = -1;
        if (mono)
        {
            for (auto& v : p.voices) if (v.isActive()) v.kill();
        }
        for (int k = 0; k < kVoicesPerPad; ++k) if (! p.voices[k].isActive()) { slot = k; break; }
        if (slot < 0)
        {   // steal oldest
            int oldest = 0;
            for (int k = 1; k < kVoicesPerPad; ++k) if (p.voices[k].age() > p.voices[oldest].age()) oldest = k;
            slot = oldest;
        }
        p.voices[slot].start (s, semi, vel, seedCounter++);
        p.lastVoice = slot;
    }
    p.silentSamples = 0;
    p.keyHeld = true;
    padActivity[idx].store (std::max (padActivity[idx].load(), vel));
    lastHitPad.store (idx);

    const int link = (int) std::round (s[P::Link]);
    if (link >= 1 && link <= kNumPads && link - 1 != idx)
        triggerPad (link - 1, vel, fromKeys ? extraSemis : 0.0f, depth + 1, fromKeys);
}

void Engine::releasePad (int idx)
{
    auto relIfGate = [this] (int i)
    {
        auto& p = pads[(size_t) i];
        if ((int) std::round (p.sound[P::PlayMode]) != 1) return;
        const float rel = p.sound[P::Release] * 0.001f;
        for (auto& v : p.voices) if (v.isActive() && ! v.isReleasing()) v.release (rel);
    };
    pads[(size_t) idx].keyHeld = false;
    relIfGate (idx);
    const int link = (int) std::round (pads[(size_t) idx].sound[P::Link]);
    if (link >= 1 && link <= kNumPads && link - 1 != idx) relIfGate (link - 1);
}

void Engine::noteOn (int note, float vel)
{
    bool mapped = false;
    for (int i = 0; i < kNumPads; ++i)
        if (pads[(size_t) i].note == note) { triggerPad (i, vel, 0.0f, 0, false); mapped = true; }
    if (mapped) return;

    const int keysPad = (int) std::round (global[(int) G::KeysPad]);
    if (keysPad >= 1 && keysPad <= kNumPads)
    {
        const float semi = (float) (note - (int) std::round (global[(int) G::KeysRoot]));
        heldKeys.push_back (note);
        triggerPad (keysPad - 1, vel, semi, 0, true);
    }
}

void Engine::noteOff (int note)
{
    for (int i = 0; i < kNumPads; ++i)
        if (pads[(size_t) i].note == note) releasePad (i);

    const int keysPad = (int) std::round (global[(int) G::KeysPad]);
    if (keysPad >= 1 && keysPad <= kNumPads)
    {
        auto it = std::find (heldKeys.begin(), heldKeys.end(), note);
        if (it != heldKeys.end())
        {
            heldKeys.erase (it);
            auto& p = pads[(size_t) keysPad - 1];
            if (heldKeys.empty()) releasePad (keysPad - 1);
            else if ((int) std::round (p.sound[P::Poly]) == 0 && p.sound[P::Glide] > 0.5f && p.lastVoice >= 0)
            {   // glide back to the previous held key
                const float semi = p.sound[P::Tune] + (float) (heldKeys.back() - (int) std::round (global[(int) G::KeysRoot]));
                p.voices[p.lastVoice].glideTo (semi, p.sound[P::Glide]);
            }
        }
    }
}

void Engine::renderSegment (float* const* outputs, int numChannels, int start, int len)
{
    const int numBuses = std::max (1, numChannels / 2);
    for (int i = 0; i < kNumPads; ++i)
    {
        auto& p = pads[(size_t) i];
        bool anyActive = false;
        for (auto& v : p.voices) anyActive |= v.isActive();
        if (! anyActive && p.silentSamples > (int) (0.06 * sr)) continue;

        std::fill (mono.begin(), mono.begin() + len, 0.0f);
        for (auto& v : p.voices) if (v.isActive()) v.render (p.sound, mono.data(), len);

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

    // UI hits at block start
    while (fifoR.load (std::memory_order_relaxed) != fifoW.load (std::memory_order_acquire))
    {
        int r = fifoR.load (std::memory_order_relaxed);
        auto h = fifo[r];
        if (h.on) triggerPad (h.pad, h.vel, 0.0f, 0, false); else releasePad (h.pad);
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

    // activity decay
    const float dec = std::exp (-(float) numSamples / (float) (0.15 * sr));
    for (int i = 0; i < kNumPads; ++i) padActivity[i].store (padActivity[i].load() * dec);
}
} // namespace thudi

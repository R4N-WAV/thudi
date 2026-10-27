// Thudi — per-model editor pages (8 knobs each) and "init" templates
#pragma once
#include <string>
#include <utility>
#include <vector>
#include "ThudiParams.h"

namespace thudi
{
struct Page
{
    const char* name;
    std::vector<P> knobs; // up to 8
};

inline std::vector<Page> pagesForModel (int model)
{
    using V = std::vector<P>;
    std::vector<Page> pages;
    switch ((Model) model)
    {
        case Model::Kick:
            pages.push_back ({ "BODY",   V { P::TonePitch, P::SweepAmt, P::SweepTime, P::SweepCurve, P::ToneDecay, P::ToneCurve, P::ToneLP, P::ToneHold } });
            pages.push_back ({ "ATTACK", V { P::ToneAttack, P::ToneShape, P::FmAmt, P::FmRatio, P::FmDecay, P::NoiseLevel, P::NoiseCutoff, P::NoiseDecay } });
            break;
        case Model::Bass808:
            pages.push_back ({ "TONE",   V { P::TonePitch, P::Growl, P::ToneShape, P::SweepAmt, P::SweepTime, P::ToneLP, P::FmAmt, P::FmRatio } });
            pages.push_back ({ "ENV",    V { P::ToneAttack, P::ToneHold, P::ToneDecay, P::ToneCurve, P::Release, P::Glide, P::PlayMode, P::Poly } });
            break;
        case Model::Snare:
            pages.push_back ({ "TONE",   V { P::ToneLevel, P::TonePitch, P::SweepAmt, P::ToneDecay, P::BodyLevel, P::BodyFreq, P::BodyRatio, P::BodyDecay } });
            pages.push_back ({ "SNAPPY", V { P::NoiseLevel, P::NoiseColor, P::NoiseCutoff, P::NoiseReso, P::NoiseEnvAmt, P::NoiseDecay, P::NoiseCurve, P::NoiseFilter } });
            break;
        case Model::Clap:
            pages.push_back ({ "BURST",  V { P::Bursts, P::BurstSpacing, P::BurstDecay, P::BurstRandom, P::NoiseLevel, P::NoiseDecay, P::NoiseCurve, P::NoiseAttack } });
            pages.push_back ({ "FILTER", V { P::NoiseFilter, P::NoiseCutoff, P::NoiseReso, P::NoiseEnvAmt, P::NoiseColor, P::ToneLevel, P::TonePitch, P::ToneDecay } });
            break;
        case Model::ClosedHat:
        case Model::OpenHat:
            pages.push_back ({ "METAL",  V { P::MetalLevel, P::MetalSet, P::MetalPitch, P::MetalTone, P::MetalQ, P::MetalRing, P::MetalDecay, P::MetalAttack } });
            pages.push_back ({ "NOISE",  V { P::NoiseLevel, P::NoiseFilter, P::NoiseCutoff, P::NoiseReso, P::NoiseDecay, P::NoiseCurve, P::NoiseColor, P::Choke } });
            break;
        case Model::Tom:
            pages.push_back ({ "TONE",   V { P::TonePitch, P::SweepAmt, P::SweepTime, P::SweepCurve, P::ToneDecay, P::ToneCurve, P::ToneShape, P::Growl } });
            pages.push_back ({ "SKIN",   V { P::BodyLevel, P::BodyFreq, P::BodyRatio, P::BodyDecay, P::BodyExcite, P::NoiseLevel, P::NoiseCutoff, P::NoiseDecay } });
            break;
        case Model::Perc:
            pages.push_back ({ "TONE",   V { P::ToneLevel, P::TonePitch, P::SweepAmt, P::SweepTime, P::ToneDecay, P::FmAmt, P::FmRatio, P::MetalLevel } });
            pages.push_back ({ "NOISE",  V { P::NoiseLevel, P::NoiseFilter, P::NoiseCutoff, P::NoiseReso, P::NoiseDecay, P::Bursts, P::BurstSpacing, P::BurstDecay } });
            break;
        case Model::Rim:
            pages.push_back ({ "CLICK",  V { P::ToneLevel, P::TonePitch, P::SweepAmt, P::ToneDecay, P::BodyLevel, P::BodyFreq, P::BodyRatio, P::BodyDecay } });
            pages.push_back ({ "NOISE",  V { P::NoiseLevel, P::NoiseFilter, P::NoiseCutoff, P::NoiseReso, P::NoiseDecay, P::NoiseCurve, P::BodyExcite, P::ToneShape } });
            break;
        case Model::Cymbal:
            pages.push_back ({ "METAL",  V { P::MetalLevel, P::MetalSet, P::MetalPitch, P::MetalTone, P::MetalQ, P::MetalRing, P::MetalAttack, P::MetalDecay } });
            pages.push_back ({ "WASH",   V { P::NoiseLevel, P::NoiseFilter, P::NoiseCutoff, P::NoiseReso, P::NoiseAttack, P::NoiseDecay, P::NoiseCurve, P::NoiseColor } });
            break;
        case Model::Zap:
            pages.push_back ({ "SWEEP",  V { P::TonePitch, P::SweepAmt, P::SweepTime, P::SweepCurve, P::ToneDecay, P::ToneCurve, P::ToneShape, P::Growl } });
            pages.push_back ({ "FM",     V { P::FmAmt, P::FmRatio, P::FmDecay, P::NoiseLevel, P::NoiseCutoff, P::NoiseDecay, P::ToneAttack, P::ToneLP } });
            break;
        case Model::Sample:
            pages.push_back ({ "CHOP",   V { P::SmpStart, P::SmpEnd, P::Tune, P::SmpSpeed, P::SmpAttack, P::SmpRelease, P::SmpCutoff, P::SmpReso } });
            pages.push_back ({ "TRIG",   V { P::SmpReverse, P::PlayMode, P::Poly, P::Fine, P::VelSens, P::Release, P::Choke, P::Level } });
            break;
        case Model::Empty:
            return { { "EMPTY", V {} } };
        case Model::Custom:
        case Model::Count:
        default:
            pages.push_back ({ "TONE",   V { P::ToneLevel, P::TonePitch, P::SweepAmt, P::SweepTime, P::ToneDecay, P::ToneCurve, P::Growl, P::FmAmt } });
            pages.push_back ({ "NOISE",  V { P::NoiseLevel, P::NoiseFilter, P::NoiseCutoff, P::NoiseReso, P::NoiseDecay, P::Bursts, P::BurstSpacing, P::BurstDecay } });
            pages.push_back ({ "METAL",  V { P::MetalLevel, P::MetalSet, P::MetalPitch, P::MetalTone, P::MetalQ, P::MetalRing, P::MetalDecay, P::BodyLevel } });
            break;
    }
    pages.push_back ({ "CHAR",   V { P::Drive, P::Asym, P::CrushMix, P::CrushRate, P::CrushBits, P::CrushXover, P::Tape, P::Drift } });
    pages.push_back ({ "STEREO", V { P::Width, P::ChorusRate, P::ChorusDepth, P::MonoBelow, P::Pan, P::LowCut, P::HighCut, P::Level } });
    pages.push_back ({ "PLAY",   V { P::Tune, P::Fine, P::VelSens, P::PlayMode, P::Release, P::Choke, P::Link, P::Output } });
    return pages;
}

// "Init" sound for each model (applied on top of the parameter defaults)
inline PadSound initSoundForModel (int model)
{
    PadSound s;
    auto set = [&] (P p, float v) { s[p] = v; };
    set (P::Model, (float) model);
    switch ((Model) model)
    {
        case Model::Kick:
            set (P::TonePitch, 55); set (P::SweepAmt, 18); set (P::SweepTime, 25); set (P::SweepCurve, 0.25f);
            set (P::ToneDecay, 350); set (P::ToneCurve, 0.5f); set (P::ToneLP, 1200); set (P::Drive, 0.2f); break;
        case Model::Bass808:
            set (P::TonePitch, 49); set (P::SweepAmt, 5); set (P::SweepTime, 40); set (P::Growl, 0.4f);
            set (P::ToneAttack, 3); set (P::ToneHold, 120); set (P::ToneDecay, 900); set (P::ToneCurve, -0.4f);
            set (P::PlayMode, 1); set (P::Poly, 0); set (P::Glide, 60); set (P::Drive, 0.3f); set (P::Asym, 0.4f); break;
        case Model::Snare:
            set (P::ToneLevel, 0.6f); set (P::TonePitch, 200); set (P::SweepAmt, 5); set (P::SweepTime, 12); set (P::ToneDecay, 140);
            set (P::NoiseLevel, 0.8f); set (P::NoiseFilter, 1); set (P::NoiseCutoff, 2500); set (P::NoiseDecay, 220); set (P::NoiseCurve, 0.3f);
            set (P::BodyLevel, 0.3f); set (P::BodyFreq, 180); set (P::BodyDecay, 120); break;
        case Model::Clap:
            set (P::ToneLevel, 0); set (P::NoiseLevel, 1); set (P::NoiseFilter, 1); set (P::NoiseCutoff, 1500); set (P::NoiseReso, 0.2f);
            set (P::Bursts, 4); set (P::BurstSpacing, 8.5f); set (P::BurstDecay, 4); set (P::NoiseDecay, 120); set (P::NoiseCurve, 0.35f); break;
        case Model::ClosedHat:
            set (P::ToneLevel, 0); set (P::MetalLevel, 0.9f); set (P::MetalTone, 7500); set (P::MetalDecay, 60);
            set (P::NoiseLevel, 0.3f); set (P::NoiseFilter, 2); set (P::NoiseCutoff, 6500); set (P::NoiseDecay, 40); set (P::Choke, 1); break;
        case Model::OpenHat:
            set (P::ToneLevel, 0); set (P::MetalLevel, 0.9f); set (P::MetalTone, 7500); set (P::MetalDecay, 380);
            set (P::NoiseLevel, 0.3f); set (P::NoiseFilter, 2); set (P::NoiseCutoff, 6500); set (P::NoiseDecay, 300); set (P::Choke, 1); break;
        case Model::Tom:
            set (P::TonePitch, 130); set (P::SweepAmt, 7); set (P::SweepTime, 60); set (P::ToneDecay, 450); set (P::ToneCurve, 0.3f);
            set (P::BodyLevel, 0.25f); set (P::BodyFreq, 130); set (P::BodyDecay, 280); break;
        case Model::Perc:
            set (P::ToneLevel, 0.4f); set (P::TonePitch, 600); set (P::ToneDecay, 60);
            set (P::NoiseLevel, 0.8f); set (P::NoiseFilter, 1); set (P::NoiseCutoff, 2600); set (P::NoiseReso, 0.3f); set (P::NoiseDecay, 60); break;
        case Model::Rim:
            set (P::ToneLevel, 0.6f); set (P::TonePitch, 1700); set (P::ToneDecay, 30); set (P::ToneCurve, 0.5f);
            set (P::NoiseLevel, 0.4f); set (P::NoiseFilter, 1); set (P::NoiseCutoff, 3200); set (P::NoiseDecay, 18);
            set (P::BodyLevel, 0.4f); set (P::BodyFreq, 480); set (P::BodyDecay, 60); break;
        case Model::Cymbal:
            set (P::ToneLevel, 0); set (P::MetalLevel, 0.8f); set (P::MetalPitch, 1.3f); set (P::MetalRing, 0.4f); set (P::MetalTone, 7500);
            set (P::MetalDecay, 1800); set (P::NoiseLevel, 0.35f); set (P::NoiseFilter, 2); set (P::NoiseCutoff, 5000); set (P::NoiseDecay, 1500); break;
        case Model::Zap:
            set (P::TonePitch, 200); set (P::SweepAmt, 48); set (P::SweepTime, 110); set (P::SweepCurve, -0.2f); set (P::ToneDecay, 160);
            set (P::FmAmt, 0.2f); set (P::FmRatio, 2); break;
        case Model::Sample:
            set (P::ToneLevel, 0); set (P::Poly, 1); set (P::Drift, 0); set (P::VelSens, 0.3f); break;
        case Model::Empty:
            set (P::ToneLevel, 0); set (P::NoiseLevel, 0); set (P::MetalLevel, 0); set (P::BodyLevel, 0); set (P::Drift, 0); break;
        default: break;
    }
    return s;
}
} // namespace thudi

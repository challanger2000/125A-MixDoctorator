#pragma once
#include "MixDoctoratorIPC.h"

namespace MixDoctorator::Analysis {

enum class RoleFamily {
    Unknown=0,
    RhythmLow,
    RhythmMid,
    RhythmHigh,
    Bass,
    Guitar,
    Vocal,
    KeysSynth,
    Pad
};

inline RoleFamily roleFamily(IPC::Role role) noexcept {
    switch(role){
        case IPC::Role::Kick:
            return RoleFamily::RhythmLow;
        case IPC::Role::Snare:
        case IPC::Role::Toms:
        case IPC::Role::Percussion:
        case IPC::Role::Drums:
            return RoleFamily::RhythmMid;
        case IPC::Role::Cymbals:
            return RoleFamily::RhythmHigh;
        case IPC::Role::Bass:
            return RoleFamily::Bass;
        case IPC::Role::ElectricGuitar:
        case IPC::Role::AcousticGuitar:
            return RoleFamily::Guitar;
        case IPC::Role::LeadVocal:
        case IPC::Role::BackingVocal:
            return RoleFamily::Vocal;
        case IPC::Role::PianoKeys:
        case IPC::Role::Synth:
            return RoleFamily::KeysSynth;
        case IPC::Role::Pad:
            return RoleFamily::Pad;
        default:
            return RoleFamily::Unknown;
    }
}

inline bool isLowEndRole(IPC::Role role) noexcept {
    const auto family=roleFamily(role);
    return family==RoleFamily::RhythmLow ||
           family==RoleFamily::Bass;
}

inline bool isTransientRole(IPC::Role role) noexcept {
    const auto family=roleFamily(role);
    return family==RoleFamily::RhythmLow ||
           family==RoleFamily::RhythmMid ||
           family==RoleFamily::RhythmHigh;
}

inline bool isHarmonicRole(IPC::Role role) noexcept {
    const auto family=roleFamily(role);
    return family==RoleFamily::Guitar ||
           family==RoleFamily::Vocal ||
           family==RoleFamily::KeysSynth ||
           family==RoleFamily::Pad;
}

} // namespace MixDoctorator::Analysis

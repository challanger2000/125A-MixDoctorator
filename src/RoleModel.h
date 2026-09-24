#pragma once
#include "MixDoctoratorIPC.h"
#include <algorithm>

namespace MixDoctorator::Analysis {

constexpr int kRolePairCount=
    (IPC::kRoleCount*(IPC::kRoleCount-1))/2;

struct RoleCountSummary {
    int sensors{0};
    int roles{0};
};

inline RoleCountSummary summarizeRoleCounts(
    const int* counts) noexcept {

    RoleCountSummary out;

    if(!counts)
        return out;

    for(int i=0;i<IPC::kRoleCount;++i){
        const int count=
            std::clamp(
                counts[i],
                0,
                IPC::kSensorSlotCount);

        if(count>0)
            ++out.roles;

        out.sensors+=count;
    }

    out.sensors=
        std::clamp(
            out.sensors,
            0,
            IPC::kSensorSlotCount);

    return out;
}

inline IPC::Role roleFromIndex(int index) noexcept {
    if(index<0 || index>=IPC::kRoleCount)
        return IPC::Role::Unknown;
    return static_cast<IPC::Role>(index+1);
}

inline int roleToIndex(IPC::Role role) noexcept {
    const int value=static_cast<int>(role);
    return (value>=1 && value<=IPC::kRoleCount)
        ? value-1
        : -1;
}

inline int encodeRolePair(IPC::Role first,IPC::Role second) noexcept {
    int a=roleToIndex(first);
    int b=roleToIndex(second);
    if(a<0 || b<0 || a==b)
        return -1;
    if(a>b){
        const int t=a;
        a=b;
        b=t;
    }

    int index=0;
    for(int i=0;i<a;++i)
        index+=IPC::kRoleCount-i-1;

    index+=b-a-1;
    return index;
}

inline bool decodeRolePair(
    int pairIndex,
    IPC::Role& first,
    IPC::Role& second) noexcept {

    if(pairIndex<0 || pairIndex>=kRolePairCount){
        first=IPC::Role::Unknown;
        second=IPC::Role::Unknown;
        return false;
    }

    int remaining=pairIndex;
    for(int a=0;a<IPC::kRoleCount-1;++a){
        const int row=IPC::kRoleCount-a-1;
        if(remaining<row){
            first=roleFromIndex(a);
            second=roleFromIndex(a+1+remaining);
            return true;
        }
        remaining-=row;
    }

    first=IPC::Role::Unknown;
    second=IPC::Role::Unknown;
    return false;
}

inline int roleDisplayRank(IPC::Role role) noexcept {
    switch(role){
        case IPC::Role::Kick: return 10;
        case IPC::Role::Bass: return 20;
        case IPC::Role::Snare: return 30;
        case IPC::Role::Toms: return 40;
        case IPC::Role::Drums: return 50;
        case IPC::Role::Cymbals: return 60;
        case IPC::Role::Percussion: return 70;
        case IPC::Role::ElectricGuitar: return 80;
        case IPC::Role::AcousticGuitar: return 90;
        case IPC::Role::LeadVocal: return 100;
        case IPC::Role::BackingVocal: return 110;
        case IPC::Role::PianoKeys: return 120;
        case IPC::Role::Synth: return 130;
        case IPC::Role::Pad: return 140;
        default: return 1000;
    }
}

inline void orderRolePairForDisplay(
    IPC::Role& first,
    IPC::Role& second) noexcept {

    if(roleDisplayRank(second)<
       roleDisplayRank(first)){
        const auto tmp=first;
        first=second;
        second=tmp;
    }
}

inline const char* roleName(IPC::Role role) noexcept {
    switch(role){
        case IPC::Role::Drums: return "SCHLAGZEUG";
        case IPC::Role::Bass: return "BASS";
        case IPC::Role::ElectricGuitar: return "E-GITARRE";
        case IPC::Role::Kick: return "KICK";
        case IPC::Role::Snare: return "SNARE";
        case IPC::Role::Toms: return "TOMS";
        case IPC::Role::Cymbals: return "BECKEN / HI-HAT";
        case IPC::Role::Percussion: return "PERCUSSION";
        case IPC::Role::AcousticGuitar: return "AKUSTIKGITARRE";
        case IPC::Role::LeadVocal: return "HAUPTGESANG";
        case IPC::Role::BackingVocal: return "HINTERGRUNDGESANG";
        case IPC::Role::PianoKeys: return "PIANO / KEYS";
        case IPC::Role::Synth: return "SYNTH";
        case IPC::Role::Pad: return "PAD";
        default: return "UNKNOWN";
    }
}

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

inline bool isDrumSubRole(IPC::Role role) noexcept {
    return role==IPC::Role::Kick ||
           role==IPC::Role::Snare ||
           role==IPC::Role::Toms ||
           role==IPC::Role::Cymbals ||
           role==IPC::Role::Percussion;
}

inline bool rolesComparableForCoach(
    IPC::Role first,
    IPC::Role second) noexcept {

    if(first==IPC::Role::Unknown ||
       second==IPC::Role::Unknown ||
       first==second)
        return false;

    // A generic drum bus/loop may contain the component track. Comparing
    // DRUMS directly against KICK/SNARE/TOMS/CYMBALS/PERCUSSION can therefore
    // create a high-confidence self-correlation false positive.
    if((first==IPC::Role::Drums &&
        isDrumSubRole(second)) ||
       (second==IPC::Role::Drums &&
        isDrumSubRole(first)))
        return false;

    return true;
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

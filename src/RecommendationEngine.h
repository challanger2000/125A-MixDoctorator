#pragma once
#include "MixDoctoratorIPC.h"
#include "RoleModel.h"
#include <algorithm>
#include <cmath>

namespace MixDoctorator::Analysis {

enum class RecommendationKind {
    None=0,
    LowEndOwnership,
    LowMidCleanup,
    MidSeparation,
    PresenceSeparation,
    TopEndSeparation,
    AttackSeparation
};

enum class RecommendationContext {
    Generic=0,
    KickBass,
    VocalVsHarmonic,
    CymbalVsHarmonic,
    RhythmVsHarmonic,
    BassVsHarmonic
};

struct Recommendation {
    bool valid{false};
    RecommendationKind kind{RecommendationKind::None};
    RecommendationContext context{RecommendationContext::Generic};
    IPC::Role first{IPC::Role::Unknown};
    IPC::Role second{IPC::Role::Unknown};
    int band{0};
    double score{0.0};
    double confidence{0.0};
    double dominance{0.0};
};

inline RecommendationContext recommendationContext(
    IPC::Role first,
    IPC::Role second) noexcept {

    const bool kickBass=
        (first==IPC::Role::Kick &&
         second==IPC::Role::Bass) ||
        (first==IPC::Role::Bass &&
         second==IPC::Role::Kick);

    if(kickBass)
        return RecommendationContext::KickBass;

    const auto a=roleFamily(first);
    const auto b=roleFamily(second);

    const bool vocalA=a==RoleFamily::Vocal;
    const bool vocalB=b==RoleFamily::Vocal;
    const bool harmonicA=isHarmonicRole(first);
    const bool harmonicB=isHarmonicRole(second);

    if((vocalA && harmonicB && !vocalB) ||
       (vocalB && harmonicA && !vocalA))
        return RecommendationContext::VocalVsHarmonic;

    const bool cymbalA=first==IPC::Role::Cymbals;
    const bool cymbalB=second==IPC::Role::Cymbals;

    if((cymbalA && harmonicB) ||
       (cymbalB && harmonicA))
        return RecommendationContext::CymbalVsHarmonic;

    const bool rhythmA=isTransientRole(first);
    const bool rhythmB=isTransientRole(second);

    if((rhythmA && harmonicB) ||
       (rhythmB && harmonicA))
        return RecommendationContext::RhythmVsHarmonic;

    const bool bassA=a==RoleFamily::Bass;
    const bool bassB=b==RoleFamily::Bass;

    if((bassA && harmonicB) ||
       (bassB && harmonicA))
        return RecommendationContext::BassVsHarmonic;

    return RecommendationContext::Generic;
}

inline int recommendationActionCode(
    RecommendationContext context) noexcept {
    return static_cast<int>(context);
}

inline Recommendation makeRecommendation(
    IPC::Role first,
    IPC::Role second,
    int band,
    double masking,
    double confidence,
    double dominance,
    double transientCompetition) noexcept {

    Recommendation out;
    out.first=first;
    out.second=second;
    out.context=
        recommendationContext(
            first,
            second);
    out.band=std::clamp(band,0,IPC::kBandCount-1);

    const double safeMasking=
        std::isfinite(masking)
        ? masking
        : 0.0;

    const double safeConfidence=
        std::isfinite(confidence)
        ? confidence
        : 0.0;

    const double safeDominance=
        std::isfinite(dominance)
        ? dominance
        : 0.0;

    const double safeTransientCompetition=
        std::isfinite(transientCompetition)
        ? transientCompetition
        : 0.0;

    out.score=std::clamp(safeMasking,0.0,1.0);
    out.confidence=std::clamp(safeConfidence,0.0,1.0);
    out.dominance=std::clamp(safeDominance,-1.0,1.0);

    if(first==IPC::Role::Unknown ||
       second==IPC::Role::Unknown ||
       first==second ||
       out.score<0.05 ||
       out.confidence<0.12)
        return out;

    const bool transientPair=
        isTransientRole(first) ||
        isTransientRole(second);

    if(transientPair &&
       safeTransientCompetition>=0.35 &&
       out.band>=2 &&
       out.band<=7){
        out.kind=RecommendationKind::AttackSeparation;
    }else if(out.band<=1){
        out.kind=RecommendationKind::LowEndOwnership;
    }else if(out.band<=3){
        out.kind=RecommendationKind::LowMidCleanup;
    }else if(out.band<=5){
        out.kind=RecommendationKind::MidSeparation;
    }else if(out.band==6){
        out.kind=RecommendationKind::PresenceSeparation;
    }else{
        out.kind=RecommendationKind::TopEndSeparation;
    }

    // Low-end ownership is especially meaningful for kick/bass style pairs,
    // but remains valid for other sources when measured evidence supports it.
    if(out.kind==RecommendationKind::LowEndOwnership &&
       (isLowEndRole(first) || isLowEndRole(second)))
        out.score=std::min(1.0,out.score*1.08);

    out.valid=true;
    return out;
}

inline int recommendationCode(RecommendationKind kind) noexcept {
    return static_cast<int>(kind);
}

} // namespace MixDoctorator::Analysis

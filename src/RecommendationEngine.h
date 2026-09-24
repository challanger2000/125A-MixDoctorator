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
    None=0,
    Generic,
    KickBass,
    VocalVsHarmonic,
    CymbalVsHarmonic,
    RhythmVsHarmonic,
    BassVsHarmonic
};

struct Recommendation {
    bool valid{false};
    RecommendationKind kind{RecommendationKind::None};
    RecommendationContext context{RecommendationContext::None};
    IPC::Role first{IPC::Role::Unknown};
    IPC::Role second{IPC::Role::Unknown};
    IPC::Role adjustRole{IPC::Role::Unknown};
    int band{0};
    double score{0.0};
    double confidence{0.0};
    double dominance{0.0};
};

inline IPC::Role recommendedAdjustmentRole(
    IPC::Role first,
    IPC::Role second,
    RecommendationContext context,
    double dominance) noexcept {

    if(first==IPC::Role::Unknown ||
       second==IPC::Role::Unknown ||
       first==second)
        return IPC::Role::Unknown;

    // Lead-vocal intelligibility is usually the more useful reference point
    // for a beginner-facing first check. Backing vocals are the safer first
    // place to inspect when the pair is lead vs backing vocal.
    if((first==IPC::Role::LeadVocal &&
        second==IPC::Role::BackingVocal))
        return IPC::Role::BackingVocal;

    if((second==IPC::Role::LeadVocal &&
        first==IPC::Role::BackingVocal))
        return IPC::Role::BackingVocal;

    if(context==RecommendationContext::VocalVsHarmonic){
        const bool firstVocal=
            roleFamily(first)==RoleFamily::Vocal;
        const bool secondVocal=
            roleFamily(second)==RoleFamily::Vocal;

        if(firstVocal && !secondVocal)
            return second;

        if(secondVocal && !firstVocal)
            return first;
    }

    // When bass conflicts with unnecessary lows from a harmonic source,
    // checking the harmonic source first is a conservative beginner move.
    if(context==RecommendationContext::BassVsHarmonic){
        if(roleFamily(first)==RoleFamily::Bass &&
           isHarmonicRole(second))
            return second;

        if(roleFamily(second)==RoleFamily::Bass &&
           isHarmonicRole(first))
            return first;
    }

    // Kick/bass ownership is musical/contextual. Do not pretend the analyzer
    // knows which one should lead solely from level dominance.
    if(context==RecommendationContext::KickBass)
        return IPC::Role::Unknown;

    const double safeDominance=
        std::isfinite(dominance)
        ? dominance
        : 0.0;

    if(safeDominance>0.20)
        return first;

    if(safeDominance<-0.20)
        return second;

    return IPC::Role::Unknown;
}

inline RecommendationContext recommendationContext(
    IPC::Role first,
    IPC::Role second,
    RecommendationKind kind) noexcept {

    const bool kickBass=
        (first==IPC::Role::Kick &&
         second==IPC::Role::Bass) ||
        (first==IPC::Role::Bass &&
         second==IPC::Role::Kick);

    if(kickBass &&
       (kind==RecommendationKind::LowEndOwnership ||
        kind==RecommendationKind::LowMidCleanup))
        return RecommendationContext::KickBass;

    const auto a=roleFamily(first);
    const auto b=roleFamily(second);

    const bool vocalA=a==RoleFamily::Vocal;
    const bool vocalB=b==RoleFamily::Vocal;
    const bool leadVocalA=
        first==IPC::Role::LeadVocal;
    const bool leadVocalB=
        second==IPC::Role::LeadVocal;
    const bool harmonicA=isHarmonicRole(first);
    const bool harmonicB=isHarmonicRole(second);

    if((leadVocalA && harmonicB && !vocalB) ||
       (leadVocalB && harmonicA && !vocalA))
        return RecommendationContext::VocalVsHarmonic;

    const bool cymbalA=first==IPC::Role::Cymbals;
    const bool cymbalB=second==IPC::Role::Cymbals;

    if((kind==RecommendationKind::TopEndSeparation ||
        kind==RecommendationKind::PresenceSeparation) &&
       ((cymbalA && harmonicB) ||
        (cymbalB && harmonicA)))
        return RecommendationContext::CymbalVsHarmonic;

    const bool rhythmA=isTransientRole(first);
    const bool rhythmB=isTransientRole(second);

    if(kind==RecommendationKind::AttackSeparation &&
       ((rhythmA && harmonicB) ||
        (rhythmB && harmonicA)))
        return RecommendationContext::RhythmVsHarmonic;

    const bool bassA=a==RoleFamily::Bass;
    const bool bassB=b==RoleFamily::Bass;

    if((kind==RecommendationKind::LowEndOwnership ||
        kind==RecommendationKind::LowMidCleanup) &&
       ((bassA && harmonicB) ||
        (bassB && harmonicA)))
        return RecommendationContext::BassVsHarmonic;

    return RecommendationContext::Generic;
}

inline int recommendationActionCode(
    RecommendationContext context) noexcept {
    return static_cast<int>(context);
}

inline int recommendationActionCode(
    const Recommendation& recommendation) noexcept {

    if(!recommendation.valid ||
       recommendation.confidence<0.30)
        return 0;

    return recommendationActionCode(
        recommendation.context);
}

constexpr int kRecommendationTargetCodeCount=
    IPC::kRoleCount+1;

constexpr int kRecommendationBandCodeCount=
    IPC::kBandCount;

inline int recommendationBandCode(
    const Recommendation& recommendation) noexcept {

    if(!recommendation.valid)
        return 0;

    return
        std::clamp(
            recommendation.band,
            0,
            IPC::kBandCount-1)+1;
}

inline int recommendationTargetCode(
    const Recommendation& recommendation) noexcept {

    if(!recommendation.valid ||
       recommendation.confidence<0.30)
        return 0;

    if(recommendation.adjustRole==
       IPC::Role::Unknown)
        return 1;

    const int role=
        static_cast<int>(
            recommendation.adjustRole);

    if(role<1 || role>IPC::kRoleCount)
        return 1;

    return role+1;
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

    if(!rolesComparableForCoach(
           first,
           second) ||
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

    out.context=
        recommendationContext(
            first,
            second,
            out.kind);

    // Source-specific action hints are intentionally stricter than merely
    // showing a finding. Early hints may identify a pair/range, but should not
    // yet tell a beginner which source to change first.
    if(out.confidence>=0.30){
        out.adjustRole=
            recommendedAdjustmentRole(
                first,
                second,
                out.context,
                out.dominance);
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

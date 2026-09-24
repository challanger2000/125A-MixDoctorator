#pragma once
#include "MixDoctoratorIPC.h"
#include "RoleModel.h"
#include <algorithm>

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

struct Recommendation {
    bool valid{false};
    RecommendationKind kind{RecommendationKind::None};
    IPC::Role first{IPC::Role::Unknown};
    IPC::Role second{IPC::Role::Unknown};
    int band{0};
    double score{0.0};
    double confidence{0.0};
    double dominance{0.0};
};

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
    out.score=std::clamp(masking,0.0,1.0);
    out.confidence=std::clamp(confidence,0.0,1.0);
    out.dominance=std::clamp(dominance,-1.0,1.0);

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
       transientCompetition>=0.35 &&
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

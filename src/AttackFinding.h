#pragma once
#include <algorithm>
#include <cmath>

namespace MixDoctorator::Analysis {

constexpr double kCoachAttackMinimumScore=0.35;
constexpr double kCoachAttackMinimumObservation=2.5;

struct AttackFinding {
    int pair{-1};
    double score{0.0};
};

inline AttackFinding chooseAttackFindingCount(
    const double* scores,
    const double* observedSeconds,
    int count,
    double minimumScore=0.22,
    double minimumObservation=2.0) noexcept {

    AttackFinding out;

    if(!scores ||
       !observedSeconds ||
       count<=0)
        return out;

    minimumScore=
        std::clamp(
            std::isfinite(minimumScore)
            ? minimumScore
            : 0.22,
            0.0,
            1.0);

    minimumObservation=
        std::max(
            0.0,
            std::isfinite(minimumObservation)
            ? minimumObservation
            : 2.0);

    for(int i=0;i<count;++i){
        if(!std::isfinite(
               observedSeconds[i]) ||
           observedSeconds[i]<
               minimumObservation)
            continue;

        if(!std::isfinite(scores[i]))
            continue;

        const double score=
            std::clamp(
                scores[i],
                0.0,
                1.0);

        if(score>=minimumScore &&
           (out.pair<0 ||
            score>out.score)){

            out.pair=i;
            out.score=score;
        }
    }

    return out;
}

inline AttackFinding chooseAttackFinding(
    const double* scores,
    const double* observedSeconds,
    double minimumScore=0.22,
    double minimumObservation=2.0) noexcept {

    return chooseAttackFindingCount(
        scores,
        observedSeconds,
        3,
        minimumScore,
        minimumObservation);
}

inline AttackFinding chooseStableAttackFindingCount(
    const double* scores,
    const double* observedSeconds,
    int count,
    int heldPair,
    double minimumScore=kCoachAttackMinimumScore,
    double minimumObservation=kCoachAttackMinimumObservation,
    double switchMargin=0.05) noexcept {

    if(!scores ||
       !observedSeconds ||
       count<=0)
        return {};

    minimumScore=
        std::clamp(
            std::isfinite(minimumScore)
            ? minimumScore
            : kCoachAttackMinimumScore,
            0.0,
            1.0);

    minimumObservation=
        std::max(
            0.0,
            std::isfinite(minimumObservation)
            ? minimumObservation
            : kCoachAttackMinimumObservation);

    switchMargin=
        std::max(
            0.0,
            std::isfinite(switchMargin)
            ? switchMargin
            : 0.05);

    auto eligible=[&](int i) noexcept {
        return
            i>=0 &&
            i<count &&
            std::isfinite(scores[i]) &&
            std::isfinite(observedSeconds[i]) &&
            observedSeconds[i]>=minimumObservation &&
            scores[i]>=minimumScore;
    };

    if(eligible(heldPair)){
        AttackFinding out{
            heldPair,
            std::clamp(
                scores[heldPair],
                0.0,
                1.0)
        };

        for(int i=0;i<count;++i){
            if(!eligible(i))
                continue;

            const double score=
                std::clamp(
                    scores[i],
                    0.0,
                    1.0);

            if(score>
               out.score+
               switchMargin){
                out.pair=i;
                out.score=score;
            }
        }

        return out;
    }

    return chooseAttackFindingCount(
        scores,
        observedSeconds,
        count,
        minimumScore,
        minimumObservation);
}

} // namespace MixDoctorator::Analysis

#pragma once
#include <algorithm>

namespace MixDoctorator::Analysis {

struct AttackFinding {
    int pair{-1};
    double score{0.0};
};

inline AttackFinding chooseAttackFinding(
    const double* scores,
    const double* observedSeconds,
    double minimumScore=0.22,
    double minimumObservation=2.0) noexcept {

    AttackFinding out;

    if(!scores ||
       !observedSeconds)
        return out;

    for(int i=0;i<3;++i){
        if(observedSeconds[i]<
           minimumObservation)
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

} // namespace MixDoctorator::Analysis

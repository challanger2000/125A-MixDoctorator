#pragma once
#include <algorithm>
#include <cmath>

namespace MixDoctorator::Analysis {

struct FindingCandidate {
    double observedSeconds{0.0};
    double masking{0.0};
    double confidence{0.0};
};

inline bool findingEligible(
    const FindingCandidate& c) noexcept {

    if(!std::isfinite(c.observedSeconds) ||
       !std::isfinite(c.masking) ||
       !std::isfinite(c.confidence))
        return false;

    return
        c.observedSeconds>=2.5 &&
        c.masking>=0.14 &&
        c.confidence>=0.12;
}

inline double findingRank(
    const FindingCandidate& c) noexcept {

    if(!findingEligible(c))
        return 0.0;

    return
        std::clamp(c.masking,0.0,1.0) *
        (0.60+
         0.40*
         std::clamp(c.confidence,0.0,1.0));
}

template<typename Getter>
inline int chooseStableFinding(
    int count,
    int held,
    Getter getter,
    double switchMargin=0.04) noexcept {

    if(count<=0)
        return -1;

    switchMargin=
        std::isfinite(switchMargin)
        ? std::max(0.0,switchMargin)
        : 0.04;

    if(held>=0 && held<count){
        const auto current=getter(held);

        if(findingEligible(current)){
            int best=held;
            double bestRank=findingRank(current);

            for(int i=0;i<count;++i){
                const auto candidate=getter(i);

                if(!findingEligible(candidate))
                    continue;

                const double rank=
                    findingRank(candidate);

                if(rank>bestRank+switchMargin){
                    best=i;
                    bestRank=rank;
                }
            }

            return best;
        }
    }

    int best=-1;
    double bestRank=0.14;

    for(int i=0;i<count;++i){
        const auto candidate=getter(i);

        if(!findingEligible(candidate))
            continue;

        const double rank=
            findingRank(candidate);

        if(rank>bestRank){
            best=i;
            bestRank=rank;
        }
    }

    return best;
}

} // namespace MixDoctorator::Analysis

#pragma once
#include <algorithm>

namespace MixDoctorator::Analysis {

// Returns one of 16 stable recommendation codes. The code selects explanatory
// text in the controller; it is not itself a psychoacoustic score.
inline int coachAdviceCode(
    int pairIndex,
    int bandIndex,
    double dominance) noexcept {

    const int side=
        dominance>0.20
        ? 1
        : dominance<-0.20
            ? -1
            : 0;

    if(pairIndex<0 || pairIndex>2)
        return 0;

    bandIndex=std::clamp(bandIndex,0,8);

    if(pairIndex==0){
        if(bandIndex<=1){
            if(side>0) return 1;
            if(side<0) return 2;
            return 3;
        }
        if(bandIndex<=3) return 4;
        return 5;
    }

    if(pairIndex==1){
        if(bandIndex<=2){
            if(side>0) return 6;
            if(side<0) return 7;
            return 8;
        }
        if(bandIndex<=5) return 9;
        return 10;
    }

    if(bandIndex<=2)
        return 11;

    if(bandIndex<=6){
        if(side>0) return 12;
        if(side<0) return 13;
        return 14;
    }

    return 15;
}

} // namespace MixDoctorator::Analysis

#pragma once
#include "MaskingModel.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace MixDoctorator::Analysis {

struct NeighborMaskingMetrics {
    PairMetrics direct{};
    PairMetrics spread{};
    double addedRisk{0.0};
};

// Experimental QA model only. It broadens each normalized band profile into
// adjacent bands with conservative weights, then compares the broadened
// profiles using the existing masking model. This is intentionally isolated
// from production until false-positive behaviour is measured.
inline NeighborMaskingMetrics evaluatePairWithNeighborSpread(
    double rmsDbA,
    double activityA,
    const double* bandsA,
    double rmsDbB,
    double activityB,
    const double* bandsB,
    double lowerWeight=0.12,
    double upperWeight=0.20) noexcept {

    NeighborMaskingMetrics out;
    out.direct=evaluatePair(
        rmsDbA,activityA,bandsA,
        rmsDbB,activityB,bandsB);

    if(!bandsA || !bandsB){
        out.spread=out.direct;
        return out;
    }

    lowerWeight=
        std::clamp(
            std::isfinite(lowerWeight)
                ? lowerWeight
                : 0.12,
            0.0,
            0.35);

    upperWeight=
        std::clamp(
            std::isfinite(upperWeight)
                ? upperWeight
                : 0.20,
            0.0,
            0.35);

    auto oneSidedSpread=[&](
        const double* source){

        std::array<double,kBandCount> result{};

        for(int i=0;i<kBandCount;++i){
            const double value=
                std::clamp(
                    std::isfinite(source[i])
                        ? source[i]
                        : 0.0,
                    0.0,
                    1.0);

            result[i]+=value;

            if(i>0)
                result[i-1]+=
                    value*lowerWeight;

            if(i+1<kBandCount)
                result[i+1]+=
                    value*upperWeight;
        }

        for(double& value:result)
            value=std::clamp(
                value,
                0.0,
                1.0);

        return result;
    };

    // Spread only one side at a time, then keep only the extra adjacent-band
    // risk over the direct model. Broadening both profiles simultaneously
    // creates artificial two-band interactions through overlapping tails.
    const auto spreadA=
        oneSidedSpread(bandsA);

    const auto spreadB=
        oneSidedSpread(bandsB);

    const auto aSpreadVsB=
        evaluatePair(
            rmsDbA,activityA,spreadA.data(),
            rmsDbB,activityB,bandsB);

    const auto aVsBSpread=
        evaluatePair(
            rmsDbA,activityA,bandsA,
            rmsDbB,activityB,spreadB.data());

    out.spread=out.direct;

    double addedMasking=0.0;

    for(int i=0;i<kBandCount;++i){
        const double extraA=
            std::max(
                0.0,
                aSpreadVsB.bandRisk[i]-
                out.direct.bandRisk[i]);

        const double extraB=
            std::max(
                0.0,
                aVsBSpread.bandRisk[i]-
                out.direct.bandRisk[i]);

        out.spread.bandRisk[i]=
            std::clamp(
                out.direct.bandRisk[i]+
                extraA+
                extraB,
                0.0,
                1.0);

        addedMasking+=
            extraA+
            extraB;
    }

    out.addedRisk=
        std::clamp(
            addedMasking,
            0.0,
            1.0);

    out.spread.masking=
        std::clamp(
            out.direct.masking+
            out.addedRisk,
            0.0,
            1.0);

    const double extraOverlapA=
        std::max(
            0.0,
            aSpreadVsB.overlap-
            out.direct.overlap);

    const double extraOverlapB=
        std::max(
            0.0,
            aVsBSpread.overlap-
            out.direct.overlap);

    out.spread.overlap=
        std::clamp(
            out.direct.overlap+
            extraOverlapA+
            extraOverlapB,
            0.0,
            1.0);

    int bestBand=0;
    for(int i=1;i<kBandCount;++i)
        if(out.spread.bandRisk[i]>
           out.spread.bandRisk[bestBand])
            bestBand=i;

    out.spread.dominantBand=
        bestBand;

    if(out.direct.masking>1.0e-12){
        out.spread.dominance=
            out.direct.dominance;
    }else if(aSpreadVsB.masking>=
            aVsBSpread.masking){
        out.spread.dominance=
            aSpreadVsB.dominance;
    }else{
        out.spread.dominance=
            aVsBSpread.dominance;
    }

    return out;
}

} // namespace MixDoctorator::Analysis

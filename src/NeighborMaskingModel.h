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

    auto spreadProfile=[&](
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

        double total=0.0;
        for(double value:result)
            total+=value;

        if(total>1.0e-20)
            for(double& value:result)
                value/=total;

        return result;
    };

    const auto spreadA=
        spreadProfile(bandsA);

    const auto spreadB=
        spreadProfile(bandsB);

    out.spread=evaluatePair(
        rmsDbA,activityA,spreadA.data(),
        rmsDbB,activityB,spreadB.data());

    out.addedRisk=
        std::clamp(
            out.spread.masking-
            out.direct.masking,
            0.0,
            1.0);

    return out;
}

} // namespace MixDoctorator::Analysis

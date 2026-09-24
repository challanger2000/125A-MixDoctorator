#pragma once
#include "NeighborMaskingModel.h"
#include <algorithm>
#include <cmath>

namespace MixDoctorator::Analysis {

// Production-facing masking candidate for the beginner Coach.
//
// Direct same-band masking remains the primary measurement. Adjacent-band
// coupling is only allowed to add risk where direct overlap leaves room for
// it, which prevents broad spectra from being counted twice.
inline PairMetrics evaluateCoachMasking(
    double rmsDbA,
    double activityA,
    const double* bandsA,
    double rmsDbB,
    double activityB,
    const double* bandsB) noexcept {

    const auto neighbor=
        evaluatePairWithNeighborSpread(
            rmsDbA,
            activityA,
            bandsA,
            rmsDbB,
            activityB,
            bandsB);

    PairMetrics out=
        neighbor.direct;

    const double directOverlap=
        std::clamp(
            std::isfinite(
                neighbor.direct.overlap)
                ? neighbor.direct.overlap
                : 0.0,
            0.0,
            1.0);

    const double roomForNeighbor=
        1.0-
        directOverlap;

    const double safeAdded=
        std::clamp(
            std::isfinite(
                neighbor.addedRisk)
                ? neighbor.addedRisk
                : 0.0,
            0.0,
            1.0);

    const double boundedAdded=
        std::min(
            0.18,
            safeAdded);

    const double effectiveAdded=
        boundedAdded*
        roomForNeighbor;

    const double rawAdded=
        std::max(
            1.0e-20,
            safeAdded);

    const double scale=
        effectiveAdded/
        rawAdded;

    out.masking=
        std::clamp(
            neighbor.direct.masking+
            effectiveAdded,
            0.0,
            1.0);

    const double spreadOverlapDelta=
        std::max(
            0.0,
            neighbor.spread.overlap-
            neighbor.direct.overlap);

    out.overlap=
        std::clamp(
            neighbor.direct.overlap+
            spreadOverlapDelta*
            scale,
            0.0,
            1.0);

    for(int i=0;i<kBandCount;++i){
        const double delta=
            std::max(
                0.0,
                neighbor.spread.bandRisk[i]-
                neighbor.direct.bandRisk[i]);

        out.bandRisk[i]=
            std::clamp(
                neighbor.direct.bandRisk[i]+
                delta*scale,
                0.0,
                1.0);
    }

    int bestBand=0;

    for(int i=1;i<kBandCount;++i)
        if(out.bandRisk[i]>
           out.bandRisk[bestBand])
            bestBand=i;

    out.dominantBand=
        bestBand;

    // Direct evidence wins when present. For a pure adjacent-band finding,
    // use the symmetric neighbour model's source dominance.
    out.dominance=
        neighbor.direct.masking>1.0e-12
        ? neighbor.direct.dominance
        : neighbor.spread.dominance;

    return out;
}

} // namespace MixDoctorator::Analysis

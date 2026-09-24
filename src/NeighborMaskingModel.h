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

// Experimental QA model only. It adds conservative psychoacoustic coupling
// between directly adjacent measured bands while leaving the original band
// levels intact. This is intentionally isolated from production until
// false-positive behaviour is measured.
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

    rmsDbA=
        std::isfinite(rmsDbA)
        ? rmsDbA
        : -120.0;

    rmsDbB=
        std::isfinite(rmsDbB)
        ? rmsDbB
        : -120.0;

    activityA=
        std::isfinite(activityA)
        ? activityA
        : 0.0;

    activityB=
        std::isfinite(activityB)
        ? activityB
        : 0.0;

    const double jointActivity=
        std::sqrt(
            std::clamp(activityA,0.0,1.0) *
            std::clamp(activityB,0.0,1.0));

    const double activityFactor=
        0.25+
        0.75*jointActivity;

    // Neighbour weights model psychoacoustic coupling, not additional
    // physical band energy. Keeping them outside the dB calculation avoids
    // a false non-monotonic response where a larger source-level gap could
    // accidentally become "more similar" after spread attenuation.
    const double coupling=
        0.5*
        (lowerWeight+upperWeight);

    auto fraction=[](
        double value) noexcept {

        return std::clamp(
            std::isfinite(value)
                ? value
                : 0.0,
            0.0,
            1.0);
    };

    struct CrossRisk {
        double risk{0.0};
        double overlap{0.0};
        double dominance{0.0};
    };

    auto crossRisk=[&](
        double af,
        double bf) noexcept {

        CrossRisk x;

        af=fraction(af);
        bf=fraction(bf);

        const double common=
            std::min(af,bf);

        if(common<=0.0 ||
           coupling<=0.0)
            return x;

        const double aBandDb=
            rmsDbA+
            10.0*std::log10(
                std::max(
                    af,
                    1.0e-12));

        const double bBandDb=
            rmsDbB+
            10.0*std::log10(
                std::max(
                    bf,
                    1.0e-12));

        const double gap=
            std::abs(
                aBandDb-
                bBandDb);

        const double levelSimilarity=
            std::exp(
                -gap/6.0);

        x.overlap=
            common*
            coupling;

        x.risk=
            x.overlap*
            levelSimilarity*
            activityFactor;

        x.dominance=
            std::clamp(
                (aBandDb-bBandDb)/
                12.0,
                -1.0,
                1.0);

        return x;
    };

    out.spread=out.direct;

    double addedMasking=0.0;
    double addedOverlap=0.0;
    double strongestNeighborRisk=0.0;
    double strongestNeighborDominance=0.0;

    for(int i=0;
        i+1<kBandCount;
        ++i){

        // A lower band against B upper band.
        const auto forward=
            crossRisk(
                bandsA[i],
                bandsB[i+1]);

        // A upper band against B lower band.
        const auto reverse=
            crossRisk(
                bandsA[i+1],
                bandsB[i]);

        const double pairRisk=
            forward.risk+
            reverse.risk;

        const double pairOverlap=
            forward.overlap+
            reverse.overlap;

        addedMasking+=
            pairRisk;

        addedOverlap+=
            pairOverlap;

        // Split adjacent interaction over the two participating display bands.
        const double halfRisk=
            0.5*
            pairRisk;

        out.spread.bandRisk[i]=
            std::clamp(
                out.spread.bandRisk[i]+
                halfRisk,
                0.0,
                1.0);

        out.spread.bandRisk[i+1]=
            std::clamp(
                out.spread.bandRisk[i+1]+
                halfRisk,
                0.0,
                1.0);

        if(forward.risk>
           strongestNeighborRisk){
            strongestNeighborRisk=
                forward.risk;
            strongestNeighborDominance=
                forward.dominance;
        }

        if(reverse.risk>
           strongestNeighborRisk){
            strongestNeighborRisk=
                reverse.risk;
            strongestNeighborDominance=
                reverse.dominance;
        }
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

    out.spread.overlap=
        std::clamp(
            out.direct.overlap+
            addedOverlap,
            0.0,
            1.0);

    int bestBand=0;

    for(int i=1;
        i<kBandCount;
        ++i)
        if(out.spread.bandRisk[i]>
           out.spread.bandRisk[bestBand])
            bestBand=i;

    out.spread.dominantBand=
        bestBand;

    if(out.direct.masking>1.0e-12)
        out.spread.dominance=
            out.direct.dominance;
    else
        out.spread.dominance=
            strongestNeighborDominance;

    return out;
}

} // namespace MixDoctorator::Analysis

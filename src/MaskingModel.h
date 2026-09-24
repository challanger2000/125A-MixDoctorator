#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace MixDoctorator::Analysis {

constexpr int kBandCount=9;

struct PairMetrics {
    double overlap{0.0};
    double masking{0.0};
    std::array<double,kBandCount> bandRisk{};
    int dominantBand{0};
    double dominance{0.0};
};

inline PairMetrics evaluatePair(
    double rmsDbA,
    double activityA,
    const double* bandsA,
    double rmsDbB,
    double activityB,
    const double* bandsB) noexcept {

    PairMetrics m;

    if(!bandsA || !bandsB)
        return m;

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

    double bestRisk=-1.0;

    for(int i=0;i<kBandCount;++i){
        const double aRaw=
            std::isfinite(bandsA[i])
            ? bandsA[i]
            : 0.0;

        const double bRaw=
            std::isfinite(bandsB[i])
            ? bandsB[i]
            : 0.0;

        const double af=
            std::clamp(aRaw,0.0,1.0);

        const double bf=
            std::clamp(bRaw,0.0,1.0);

        const double common=
            std::min(af,bf);

        m.overlap+=common;

        const double aBandDb=
            rmsDbA +
            10.0*std::log10(
                std::max(af,1.0e-12));

        const double bBandDb=
            rmsDbB +
            10.0*std::log10(
                std::max(bf,1.0e-12));

        const double gap=
            std::abs(aBandDb-bBandDb);

        const double levelSimilarity=
            std::exp(-gap/6.0);

        // Near-silent material must not accumulate a confident masking
        // finding merely because two normalized spectra look alike. Keep a
        // small floor for continuity, but let real joint activity dominate.
        const double activityFactor=
            0.04+
            0.96*jointActivity;

        const double risk=
            common *
            levelSimilarity *
            activityFactor;

        m.bandRisk[i]=risk;
        m.masking+=risk;

        if(risk>bestRisk){
            bestRisk=risk;
            m.dominantBand=i;
        }
    }

    m.overlap=
        std::clamp(
            m.overlap,
            0.0,
            1.0);

    m.masking=
        std::clamp(
            m.masking,
            0.0,
            1.0);

    const int i=
        m.dominantBand;

    const double af=
        std::clamp(
            std::isfinite(bandsA[i])
                ? bandsA[i]
                : 0.0,
            0.0,
            1.0);

    const double bf=
        std::clamp(
            std::isfinite(bandsB[i])
                ? bandsB[i]
                : 0.0,
            0.0,
            1.0);

    const double aBandDb=
        rmsDbA +
        10.0*std::log10(
            std::max(af,1.0e-12));

    const double bBandDb=
        rmsDbB +
        10.0*std::log10(
            std::max(bf,1.0e-12));

    m.dominance=
        std::clamp(
            (aBandDb-bBandDb)/12.0,
            -1.0,
            1.0);

    return m;
}

} // namespace MixDoctorator::Analysis

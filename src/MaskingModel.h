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

    const double jointActivity=
        std::sqrt(
            std::clamp(activityA,0.0,1.0) *
            std::clamp(activityB,0.0,1.0));

    double bestRisk=-1.0;

    for(int i=0;i<kBandCount;++i){
        const double af=
            std::clamp(bandsA[i],0.0,1.0);

        const double bf=
            std::clamp(bandsB[i],0.0,1.0);

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

        const double risk=
            common *
            levelSimilarity *
            (0.25+0.75*jointActivity);

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
            bandsA[i],
            0.0,
            1.0);

    const double bf=
        std::clamp(
            bandsB[i],
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

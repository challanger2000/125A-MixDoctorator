#pragma once
#include <algorithm>
#include <cmath>

namespace MixDoctorator::Analysis {

inline double transientCompetition(
    double transientA,
    double transientB,
    double rmsDbA,
    double rmsDbB,
    double activityA,
    double activityB) noexcept {

    const double jointTransient=
        std::sqrt(
            std::clamp(transientA,0.0,1.0) *
            std::clamp(transientB,0.0,1.0));

    const double levelGap=
        std::abs(rmsDbA-rmsDbB);

    const double levelSimilarity=
        std::exp(-levelGap/8.0);

    const double jointActivity=
        std::sqrt(
            std::clamp(activityA,0.0,1.0) *
            std::clamp(activityB,0.0,1.0));

    return std::clamp(
        jointTransient *
        levelSimilarity *
        (0.30+0.70*jointActivity),
        0.0,
        1.0);
}

} // namespace MixDoctorator::Analysis

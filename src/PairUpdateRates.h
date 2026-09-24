#pragma once
#include <algorithm>
#include <cmath>

namespace MixDoctorator::Analysis {

struct PairUpdateRates {
    double dt{0.0};
    double dominanceAlpha{0.0};
    double overlapUpAlpha{0.0};
    double overlapDownAlpha{0.0};
    double maskingUpAlpha{0.0};
    double maskingDownAlpha{0.0};
    double transientUpAlpha{0.0};
    double transientDownAlpha{0.0};
    double bandAlpha{0.0};
    double confidenceAlpha{0.0};
};

inline PairUpdateRates makePairUpdateRates(
    int numSamples,
    double sampleRate) noexcept {

    const double safeRate=
        (std::isfinite(sampleRate) &&
         sampleRate>8000.0)
        ? sampleRate
        : 44100.0;

    const double dt=
        std::clamp(
            static_cast<double>(
                std::max(1,numSamples))/
            safeRate,
            0.0001,
            0.25);

    auto alpha=[dt](double tau){
        return 1.0-
            std::exp(
                -dt/
                tau);
    };

    PairUpdateRates r;
    r.dt=dt;
    r.dominanceAlpha=alpha(1.8);
    r.overlapUpAlpha=alpha(0.60);
    r.overlapDownAlpha=alpha(2.5);
    r.maskingUpAlpha=alpha(0.85);
    r.maskingDownAlpha=alpha(2.4);
    r.transientUpAlpha=alpha(0.12);
    r.transientDownAlpha=alpha(0.90);
    r.bandAlpha=alpha(1.5);
    r.confidenceAlpha=alpha(2.5);
    return r;
}

} // namespace MixDoctorator::Analysis

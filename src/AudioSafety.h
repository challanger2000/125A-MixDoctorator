#pragma once
#include <cmath>

namespace MixDoctorator::Analysis {

// Finite audio passes through unchanged. Non-finite samples are replaced with
// silence so an analysis plug-in cannot propagate NaN/Inf through the host.
inline double sanitizeAudioSample(
    double sample) noexcept {

    return std::isfinite(sample)
        ? sample
        : 0.0;
}

} // namespace MixDoctorator::Analysis

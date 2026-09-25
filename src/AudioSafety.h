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

template<typename T>
inline double readAudioSample(
    const T* buffer,
    int index) noexcept {

    if(!buffer || index<0)
        return 0.0;

    return sanitizeAudioSample(
        static_cast<double>(
            buffer[index]));
}

} // namespace MixDoctorator::Analysis

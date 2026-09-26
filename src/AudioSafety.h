#pragma once
#include <algorithm>
#include <cmath>
#include <limits>

namespace MixDoctorator::Analysis {

// Finite audio passes through unchanged. Non-finite samples are replaced with
// silence so an analysis plug-in cannot propagate NaN/Inf through the host.
inline double sanitizeAudioSample(
    double sample) noexcept {

    return std::isfinite(sample)
        ? sample
        : 0.0;
}

// Analysis arithmetic squares samples and the spectral path accumulates many
// FFT-bin powers. Clamp only the internal analysis copy far beyond any
// meaningful audio amplitude so finite pathological inputs cannot overflow
// to Inf/NaN. Audio pass-through remains bit-for-bit unchanged for finite
// samples.
inline double analysisSampleLimit() noexcept {
    static const double limit=
        std::sqrt(
            std::numeric_limits<double>::
                max())/
        1.0e12;

    return limit;
}

inline double sanitizeAnalysisSample(
    double sample) noexcept {

    sample=sanitizeAudioSample(sample);

    const double limit=
        analysisSampleLimit();

    return std::clamp(
        sample,
        -limit,
        limit);
}

inline double analysisSamplePower(
    double sample) noexcept {

    const double safe=
        sanitizeAnalysisSample(sample);

    return safe*safe;
}

inline double analysisStereoPower(
    double left,
    double right) noexcept {

    return
        0.5*
        analysisSamplePower(left)+
        0.5*
        analysisSamplePower(right);
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

template<typename T>
inline void copySanitizedAudioBlock(
    const T* input,
    T* output,
    int numSamples) noexcept {

    if(!output ||
       numSamples<=0)
        return;

    for(int i=0;
        i<numSamples;
        ++i)
        output[i]=
            static_cast<T>(
                readAudioSample(
                    input,
                    i));
}

} // namespace MixDoctorator::Analysis

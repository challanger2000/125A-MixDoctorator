#pragma once
#include "TimingModel.h"
#include <cstdint>

namespace MixDoctorator::Analysis {

inline bool ipcResponseMatchesContext(
    int responseSession,
    std::uint64_t responseGeneration,
    int currentSession,
    std::uint64_t currentGeneration) noexcept {

    return
        responseSession==currentSession &&
        responseGeneration==currentGeneration;
}

inline bool ipcResponseIsFresh(
    int responseSession,
    std::uint64_t responseGeneration,
    std::int64_t responseSamplePosition,
    int currentSession,
    std::uint64_t currentGeneration,
    std::int64_t currentSamplePosition,
    int numSamples) noexcept {

    return
        ipcResponseMatchesContext(
            responseSession,
            responseGeneration,
            currentSession,
            currentGeneration) &&
        samplePositionsCoherent(
            currentSamplePosition,
            responseSamplePosition,
            responseSamplePosition,
            numSamples);
}

} // namespace MixDoctorator::Analysis

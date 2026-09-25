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

    if(!ipcResponseMatchesContext(
           responseSession,
           responseGeneration,
           currentSession,
           currentGeneration))
        return false;

    // If the Brain has a valid host timeline, a worker response without a
    // sample position cannot be proven current and must not drive diagnosis.
    if(currentSamplePosition>=0 &&
       responseSamplePosition<0)
        return false;

    return
        samplePositionsCoherent(
            currentSamplePosition,
            responseSamplePosition,
            responseSamplePosition,
            numSamples);
}

} // namespace MixDoctorator::Analysis

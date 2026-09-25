#pragma once
#include <algorithm>
#include <cstdint>

namespace MixDoctorator::Analysis {

inline bool shouldResetAnalysis(
    bool playing,
    bool wasPlaying,
    bool cycleActive,
    std::int64_t currentSample,
    std::int64_t lastSample,
    std::int64_t rewindTolerance) noexcept {

    const bool restarted=
        playing &&
        !wasPlaying;

    const auto tolerance=
        std::max<std::int64_t>(
            0,
            rewindTolerance);

    const bool havePositions=
        currentSample>=0 &&
        lastSample>=0;

    const bool jumpedBackward=
        havePositions &&
        !cycleActive &&
        currentSample+
            tolerance<
        lastSample;

    // A large forward relocation is never a normal process-block advance.
    // Reset even while cycle is enabled: a user seek inside a loop should not
    // carry findings from the previous location into the new section.
    const bool jumpedForward=
        havePositions &&
        currentSample>
            lastSample+
            tolerance;

    return
        restarted ||
        jumpedBackward ||
        jumpedForward;
}

} // namespace MixDoctorator::Analysis

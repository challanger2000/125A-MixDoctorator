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

    const bool jumpedBackward=
        !cycleActive &&
        currentSample>=0 &&
        lastSample>=0 &&
        currentSample+
            std::max<std::int64_t>(
                0,
                rewindTolerance)<
        lastSample;

    return
        restarted ||
        jumpedBackward;
}

} // namespace MixDoctorator::Analysis

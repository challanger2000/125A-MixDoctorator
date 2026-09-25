#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>

namespace MixDoctorator::Analysis {

inline bool samplePositionsCoherent(
    std::int64_t current,
    std::int64_t a,
    std::int64_t b,
    std::int64_t blockSize) noexcept {

    const std::int64_t tolerance=
        std::max<std::int64_t>(
            4096,
            std::max<std::int64_t>(
                1,
                blockSize)*
            4);

    // If the host position is unavailable, retain heartbeat-based fallback.
    // When both sensor positions are known we can still reject an obviously
    // incoherent pair.
    if(current<0){
        if(a>=0 && b>=0)
            return
                std::llabs(a-b)<=
                tolerance;

        return true;
    }

    // With a valid host position, mixed timing certainty is unsafe: one
    // source can be proven current while the other cannot be aligned.
    if((a<0)!=(b<0))
        return false;

    // If neither sensor exposes a sample position, fall back to heartbeat
    // freshness instead of disabling analysis entirely.
    if(a<0 && b<0)
        return true;

    return
        std::llabs(
            a-current)<=
            tolerance &&
        std::llabs(
            b-current)<=
            tolerance &&
        std::llabs(
            a-b)<=
            tolerance;
}

} // namespace MixDoctorator::Analysis

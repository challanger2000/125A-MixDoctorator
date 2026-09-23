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

    if(current<0 ||
       a<0 ||
       b<0)
        return true;

    const std::int64_t tolerance=
        std::max<std::int64_t>(
            4096,
            std::max<std::int64_t>(
                1,
                blockSize)*
            4);

    return
        std::llabs(
            a-current)<=
            tolerance &&
        std::llabs(
            b-current)<=
            tolerance;
}

} // namespace MixDoctorator::Analysis

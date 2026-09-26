#pragma once

#include <cstddef>
#include <cstdint>

namespace MixDoctorator::Analysis {

inline std::uint64_t nextNonZeroGeneration(
    std::uint64_t current) noexcept {

    ++current;

    if(current==0)
        current=1;

    return current;
}

template<std::size_t N>
inline void invalidatePublishedValues(
    double (&values)[N]) noexcept {

    for(double& value:values)
        value=-1.0;
}

} // namespace MixDoctorator::Analysis

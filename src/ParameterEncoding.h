#pragma once
#include <algorithm>
#include <cmath>

namespace MixDoctorator::Analysis {

inline double encodeDiscreteCode(
    int code,
    int maximumCode) noexcept {

    if(maximumCode<=0)
        return 0.0;

    return
        static_cast<double>(
            std::clamp(
                code,
                0,
                maximumCode))/
        static_cast<double>(
            maximumCode);
}

inline int decodeDiscreteCode(
    double normalized,
    int maximumCode) noexcept {

    if(maximumCode<=0 ||
       !std::isfinite(normalized))
        return 0;

    return
        std::clamp(
            static_cast<int>(
                std::lround(
                    std::clamp(
                        normalized,
                        0.0,
                        1.0)*
                    static_cast<double>(
                        maximumCode))),
            0,
            maximumCode);
}

} // namespace MixDoctorator::Analysis

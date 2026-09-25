#pragma once

#include <atomic>
#include <cstdint>
#include <random>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace MixDoctorator::Analysis {

inline std::uint64_t makeRuntimeInstanceId() noexcept {
    static std::atomic<std::uint64_t> counter{1};

    std::uint64_t randomPart=0;

    try{
        std::random_device rd;

        randomPart=
            (static_cast<std::uint64_t>(rd())<<32) ^
            static_cast<std::uint64_t>(rd());
    }catch(...){
        randomPart=0;
    }

    const std::uint64_t serial=
        counter.fetch_add(
            1,
            std::memory_order_relaxed);

#ifdef _WIN32
    const std::uint64_t processPart=
        static_cast<std::uint64_t>(
            GetCurrentProcessId())<<32;

    const std::uint64_t timePart=
        static_cast<std::uint64_t>(
            GetTickCount64());
#else
    const std::uint64_t processPart=0;
    const std::uint64_t timePart=0;
#endif

    std::uint64_t id=
        randomPart ^
        processPart ^
        timePart ^
        (serial*0x9E3779B97F4A7C15ull);

    if(id==0)
        id=serial ? serial : 1;

    return id;
}

} // namespace MixDoctorator::Analysis

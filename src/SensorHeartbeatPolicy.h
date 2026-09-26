#pragma once
#include <cstdint>

namespace MixDoctorator::Analysis {

constexpr std::uint64_t kSensorConnectedTimeoutMs=1500u;
constexpr std::uint64_t kSensorReclaimTimeoutMs=2000u;

inline bool sensorHeartbeatConnected(
    std::uint64_t nowMs,
    std::uint64_t heartbeatMs,
    bool active,
    std::uint64_t instanceId) noexcept {

    return
        instanceId!=0 &&
        active &&
        nowMs>=heartbeatMs &&
        (nowMs-heartbeatMs)<kSensorConnectedTimeoutMs;
}

inline bool sensorHeartbeatReclaimable(
    std::uint64_t nowMs,
    std::uint64_t heartbeatMs,
    std::uint64_t instanceId) noexcept {

    return
        instanceId==0 ||
        (nowMs>=heartbeatMs &&
         (nowMs-heartbeatMs)>kSensorReclaimTimeoutMs);
}

} // namespace MixDoctorator::Analysis

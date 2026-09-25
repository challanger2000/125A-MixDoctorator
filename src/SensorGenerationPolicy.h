#pragma once
#include <cstdint>

namespace MixDoctorator::Analysis {

inline bool sensorPacketGenerationCurrent(
    std::uint64_t packetGeneration,
    std::uint64_t currentGeneration) noexcept {

    return
        packetGeneration!=0 &&
        packetGeneration==currentGeneration;
}

inline bool sensorPublicationNeedsRelease(
    int lastPublishedSession,
    std::uint64_t lastPublishedGeneration,
    std::uint64_t currentGeneration) noexcept {

    return
        lastPublishedSession>=0 &&
        lastPublishedGeneration!=0 &&
        lastPublishedGeneration!=currentGeneration;
}

} // namespace MixDoctorator::Analysis

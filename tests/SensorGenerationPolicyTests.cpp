#include "../src/SensorGenerationPolicy.h"
#include <cassert>
#include <cstdint>
#include <iostream>

using namespace MixDoctorator::Analysis;

int main(){
    constexpr std::uint64_t current=12;

    assert(
        sensorPacketGenerationCurrent(
            current,
            current));

    assert(
        !sensorPacketGenerationCurrent(
            current-1,
            current));

    assert(
        !sensorPacketGenerationCurrent(
            0,
            current));

    assert(
        sensorPublicationNeedsRelease(
            3,
            current-1,
            current));

    assert(
        !sensorPublicationNeedsRelease(
            3,
            current,
            current));

    assert(
        !sensorPublicationNeedsRelease(
            -1,
            current-1,
            current));

    assert(
        !sensorPublicationNeedsRelease(
            3,
            0,
            current));

    // Repeated processing stop/start cycles advance generation. Every
    // previously queued packet must remain invalid after each reset.
    {
        std::uint64_t generation=100;

        for(int cycle=0;cycle<64;++cycle){
            const auto previous=
                generation;

            ++generation;

            assert(
                sensorPublicationNeedsRelease(
                    2,
                    previous,
                    generation));

            assert(
                !sensorPacketGenerationCurrent(
                    previous,
                    generation));

            assert(
                sensorPacketGenerationCurrent(
                    generation,
                    generation));
        }
    }

    std::cout
        << "Sensor generation policy tests passed\n";

    return 0;
}

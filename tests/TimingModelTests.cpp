#include "../src/TimingModel.h"
#include "../src/MixDoctoratorIPC.h"

#include <cassert>
#include <iostream>

using MixDoctorator::Analysis::
    samplePositionsCoherent;

int main(){
    using MixDoctorator::IPC::clampSession;

    assert(clampSession(-5)==0);
    assert(clampSession(0)==0);
    assert(clampSession(7)==7);
    assert(clampSession(99)==7);

    assert(
        samplePositionsCoherent(
            100000,
            100000,
            100256,
            256));

    assert(
        samplePositionsCoherent(
            100000,
            96000,
            100000,
            256));

    assert(
        !samplePositionsCoherent(
            100000,
            90000,
            100000,
            256));

    // Both can individually fit the host tolerance while still being too far
    // apart from each other. That must not count as a coherent pair.
    assert(
        !samplePositionsCoherent(
            100000,
            96000,
            104000,
            256));

    // Unknown host positions fall back to heartbeat freshness, but if both
    // sensor positions are known they must still agree with each other.
    assert(
        samplePositionsCoherent(
            -1,
            100000,
            100000,
            256));

    assert(
        !samplePositionsCoherent(
            -1,
            100000,
            110000,
            256));

    // Mixed known/unknown sensor timing must not be paired when the host has
    // a valid transport position.
    assert(
        !samplePositionsCoherent(
            100000,
            100000,
            -1,
            256));

    // If neither sensor exposes positions, heartbeat freshness remains the
    // fallback path.
    assert(
        samplePositionsCoherent(
            100000,
            -1,
            -1,
            256));

    std::cout
        << "TimingModel tests passed\n";

    return 0;
}

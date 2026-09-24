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

    // Unknown host positions fall back to heartbeat freshness.
    assert(
        samplePositionsCoherent(
            -1,
            100000,
            100000,
            256));

    std::cout
        << "TimingModel tests passed\n";

    return 0;
}

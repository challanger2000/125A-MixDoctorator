#include "../src/TimingModel.h"

#include <cassert>
#include <iostream>

using MixDoctorator::Analysis::
    samplePositionsCoherent;

int main(){
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

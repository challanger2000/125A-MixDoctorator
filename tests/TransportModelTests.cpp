#include "../src/TransportModel.h"

#include <cassert>
#include <iostream>

using MixDoctorator::Analysis::shouldResetAnalysis;

int main(){
    // Starting playback starts a fresh measurement.
    assert(
        shouldResetAnalysis(
            true,false,false,
            1000,1000,4096));

    // Normal forward playback within the coherence tolerance does not reset.
    assert(
        !shouldResetAnalysis(
            true,true,false,
            10256,10000,4096));

    // Small normal process-block advance does not reset.
    assert(
        !shouldResetAnalysis(
            true,true,false,
            10256,10000,4096));

    // Large forward seek resets stale findings.
    assert(
        shouldResetAnalysis(
            true,true,false,
            30000,10000,4096));

    // Forward user seek also resets while cycle is enabled.
    assert(
        shouldResetAnalysis(
            true,true,true,
            30000,10000,4096));

    // Manual rewind outside a cycle resets.
    assert(
        shouldResetAnalysis(
            true,true,false,
            1000,20000,4096));

    // A loop wrap must NOT reset accumulated analysis.
    assert(
        !shouldResetAnalysis(
            true,true,true,
            1000,20000,4096));

    // Stopping freezes; stop itself is not a reset.
    assert(
        !shouldResetAnalysis(
            false,true,false,
            20000,20000,4096));

    std::cout
        << "TransportModel tests passed\n";

    return 0;
}

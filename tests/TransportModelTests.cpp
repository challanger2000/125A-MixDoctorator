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

    // Repeated loop wraps must NOT reset accumulated analysis. The host
    // acceptance plan requires at least four cycle passes.
    {
        std::int64_t lastSample=20000;

        for(int wrap=0;wrap<4;++wrap){
            const std::int64_t currentSample=
                1000+
                static_cast<std::int64_t>(
                    wrap)*128;

            assert(
                !shouldResetAnalysis(
                    true,
                    true,
                    true,
                    currentSample,
                    lastSample,
                    4096));

            lastSample=20000;
        }
    }

    // Stopping freezes; stop itself is not a reset.
    assert(
        !shouldResetAnalysis(
            false,true,false,
            20000,20000,4096));

    // Repeated stop/start cycles: stopping must freeze without resetting,
    // while every restart starts a fresh measurement.
    for(int cycle=0;cycle<64;++cycle){
        const std::int64_t position=
            20000+
            static_cast<std::int64_t>(cycle)*256;

        assert(
            !shouldResetAnalysis(
                false,
                true,
                false,
                position,
                position,
                4096));

        assert(
            shouldResetAnalysis(
                true,
                false,
                false,
                position,
                position,
                4096));
    }

    std::cout
        << "TransportModel tests passed\n";

    return 0;
}

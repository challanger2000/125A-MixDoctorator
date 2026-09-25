#include "../src/PairUpdateRates.h"
#include "../src/PairDynamics.h"
#include "../src/FindingRanking.h"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace MixDoctorator::Analysis;

using State=
    PairDynamicsState;

static void step(
    State& s,
    bool active,
    double maskingTarget,
    int numSamples,
    double sampleRate){

    const auto rates=
        makePairUpdateRates(
            numSamples,
            sampleRate);

    PairMeasurement measurement;
    measurement.active=active;
    measurement.masking=
        active
        ? maskingTarget
        : 0.0;

    // Keep the dominant band stable so this timing test isolates the same
    // confidence/eligibility dynamics used by the production Brain.
    if(active)
        measurement.bandRisk[4]=maskingTarget;

    updatePairDynamics(
        measurement,
        s,
        rates);
}

static bool eligible(const State& s){
    return findingEligible(
        FindingCandidate{
            s.observedSeconds,
            s.masking,
            s.confidence
        });
}

struct TimingResult {
    double firstEligible{-1.0};
    double clearsAfter{-1.0};
};

static TimingResult runScenario(
    double sampleRate,
    int block){

    const double blockSeconds=
        static_cast<double>(block)/
        sampleRate;

    auto runFor=[
        sampleRate,
        block,
        blockSeconds
    ](
        State& s,
        double seconds,
        bool active,
        double maskingTarget,
        bool requireIneligible){

        const int blocks=
            static_cast<int>(
                std::ceil(
                    seconds/
                    blockSeconds));

        for(int i=0;i<blocks;++i){
            step(
                s,
                active,
                maskingTarget,
                block,
                sampleRate);

            if(requireIneligible)
                assert(!eligible(s));
        }
    };

    State s;
    s.dominantBand=4;

    // Short collision must not become a stable Coach finding.
    runFor(
        s,
        0.5,
        true,
        0.85,
        false);

    assert(!eligible(s));

    // Quiet time after the short burst must keep it ineligible.
    runFor(
        s,
        3.0,
        false,
        0.0,
        true);

    TimingResult result;

    const int attackBlocks=
        static_cast<int>(
            std::ceil(
                12.0/
                blockSeconds));

    for(int i=0;i<attackBlocks;++i){
        step(
            s,
            true,
            0.50,
            block,
            sampleRate);

        if(result.firstEligible<0.0 &&
           eligible(s))
            result.firstEligible=
                static_cast<double>(i+1)*
                blockSeconds;
    }

    assert(result.firstEligible>=2.5);
    assert(result.firstEligible<8.0);

    const int clearBlocks=
        static_cast<int>(
            std::ceil(
                12.0/
                blockSeconds));

    for(int i=0;i<clearBlocks;++i){
        step(
            s,
            false,
            0.0,
            block,
            sampleRate);

        if(!eligible(s)){
            result.clearsAfter=
                static_cast<double>(i+1)*
                blockSeconds;
            break;
        }
    }

    assert(result.clearsAfter>0.0);
    assert(result.clearsAfter<3.5);

    return result;
}

int main(){
    const double sampleRates[]{
        44100.0,
        48000.0,
        96000.0,
        192000.0
    };

    const int blocks[]{
        64,
        128,
        256,
        512,
        1024
    };

    const auto reference=
        runScenario(
            48000.0,
            256);

    double worstAttackDrift=0.0;
    double worstClearDrift=0.0;

    for(double sampleRate:sampleRates){
        for(int block:blocks){
            const auto result=
                runScenario(
                    sampleRate,
                    block);

            worstAttackDrift=
                std::max(
                    worstAttackDrift,
                    std::abs(
                        result.firstEligible-
                        reference.firstEligible));

            worstClearDrift=
                std::max(
                    worstClearDrift,
                    std::abs(
                        result.clearsAfter-
                        reference.clearsAfter));
        }
    }

    // Host buffer size and sample rate must not materially alter what the
    // beginner sees. Allow only small quantization error from block boundaries.
    assert(worstAttackDrift<0.08);
    assert(worstClearDrift<0.12);

    std::cout
        << "Temporal finding QA: first eligible="
        << reference.firstEligible
        << " s, clears after="
        << reference.clearsAfter
        << " s, worst attack drift="
        << worstAttackDrift
        << " s, worst clear drift="
        << worstClearDrift
        << " s\n";

    return 0;
}

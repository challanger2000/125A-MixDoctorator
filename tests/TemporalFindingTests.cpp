#include "../src/PairUpdateRates.h"
#include "../src/FindingRanking.h"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace MixDoctorator::Analysis;

struct State {
    double masking{0.0};
    double confidence{0.0};
    double observedSeconds{0.0};
    int dominantBand{4};
};

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

    const double dt=
        rates.dt;

    if(active)
        s.observedSeconds=
            std::min(
                45.0,
                s.observedSeconds+dt);
    else
        s.observedSeconds=
            std::max(
                0.0,
                s.observedSeconds-dt*0.20);

    const double alpha=
        maskingTarget>s.masking
        ? rates.maskingUpAlpha
        : rates.maskingDownAlpha;

    s.masking+=
        alpha*
        (maskingTarget-s.masking);

    const double timeConfidence=
        std::clamp(
            s.observedSeconds/10.0,
            0.0,
            1.0);

    const double riskConfidence=
        std::clamp(
            (s.masking-0.08)/0.34,
            0.0,
            1.0);

    const double targetConfidence=
        timeConfidence*
        (
            0.60*riskConfidence+
            0.40*1.0
        );

    s.confidence+=
        rates.confidenceAlpha*
        (targetConfidence-
         s.confidence);
}

static bool eligible(const State& s){
    return findingEligible(
        FindingCandidate{
            s.observedSeconds,
            s.masking,
            s.confidence
        });
}

int main(){
    constexpr double sampleRate=48000.0;
    constexpr int block=256;
    const int blocksPerSecond=
        static_cast<int>(
            std::round(
                sampleRate/
                static_cast<double>(block)));

    State s;

    // 0.5 s strong collision must not become a stable Coach finding.
    for(int i=0;i<blocksPerSecond/2;++i)
        step(s,true,0.85,block,sampleRate);

    assert(!eligible(s));

    // Follow with 3 s quiet/inactive time. The short burst must decay away
    // and remain ineligible throughout.
    for(int i=0;i<blocksPerSecond*3;++i){
        step(s,false,0.0,block,sampleRate);
        assert(!eligible(s));
    }

    // A genuinely persistent problem should eventually become eligible.
    double firstEligibleSeconds=-1.0;

    for(int i=0;i<blocksPerSecond*12;++i){
        step(s,true,0.50,block,sampleRate);

        if(firstEligibleSeconds<0.0 &&
           eligible(s))
            firstEligibleSeconds=
                static_cast<double>(i+1)/
                static_cast<double>(
                    blocksPerSecond);
    }

    assert(firstEligibleSeconds>=2.5);
    assert(firstEligibleSeconds<8.0);

    // Once the problem disappears, it should not remain eligible for an
    // excessive period. Measure the real hold time rather than assuming.
    double clearSeconds=-1.0;

    for(int i=0;i<blocksPerSecond*12;++i){
        step(s,false,0.0,block,sampleRate);

        if(!eligible(s)){
            clearSeconds=
                static_cast<double>(i+1)/
                static_cast<double>(
                    blocksPerSecond);
            break;
        }
    }

    assert(clearSeconds>0.0);
    assert(clearSeconds<3.5);

    std::cout
        << "Temporal finding QA: first eligible="
        << firstEligibleSeconds
        << " s, clears after="
        << clearSeconds
        << " s\n";

    return 0;
}

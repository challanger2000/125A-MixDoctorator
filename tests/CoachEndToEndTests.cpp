#include "../src/SpectralAnalyzer.h"
#include "../src/PairMeasurement.h"
#include "../src/RecommendationEngine.h"
#include "../src/TransientModel.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

using namespace MixDoctorator;
using namespace MixDoctorator::Analysis;

namespace {
constexpr double kPi=
    3.14159265358979323846;

std::array<double,IPC::kBandCount>
analyzeTone(
    double sampleRate,
    double hz){

    SpectralAnalyzer left;
    SpectralAnalyzer right;

    left.prepare(sampleRate);
    right.prepare(sampleRate);

    const int samples=
        static_cast<int>(
            sampleRate*0.75);

    for(int n=0;n<samples;++n){
        const double x=
            std::sin(
                2.0*kPi*
                hz*
                static_cast<double>(n)/
                sampleRate);

        left.push(x);
        right.push(x);
    }

    return combineStereoFractions(
        left,
        right);
}

double measureBurstTransient(
    double sampleRate,
    double hz,
    double amplitude){

    TransientDetector detector;
    detector.prepare(sampleRate);

    const int silenceSamples=
        static_cast<int>(
            sampleRate*0.20);

    for(int n=0;n<silenceSamples;++n)
        detector.pushPower(0.0);

    const int burstSamples=
        static_cast<int>(
            sampleRate*0.03);

    double peakTransient=0.0;

    for(int n=0;n<burstSamples;++n){
        const double x=
            amplitude*
            std::sin(
                2.0*kPi*
                hz*
                static_cast<double>(n)/
                sampleRate);

        detector.pushPower(x*x);

        peakTransient=
            std::max(
                peakTransient,
                detector.value());
    }

    return peakTransient;
}

IPC::Snapshot makeSnapshot(
    IPC::Role role,
    const std::array<double,IPC::kBandCount>& bands,
    double rmsDb,
    std::int64_t samplePosition){

    IPC::Snapshot s;
    s.instanceId=
        static_cast<std::uint64_t>(
            static_cast<int>(role));
    s.role=role;
    s.connected=true;
    s.samplePosition=samplePosition;
    s.rmsDb=rmsDb;
    s.peakDb=rmsDb+3.0;
    s.activity=
        std::clamp(
            (rmsDb+60.0)/60.0,
            0.0,
            1.0);
    s.transient=0.10;

    for(int i=0;i<IPC::kBandCount;++i)
        s.bands[i]=bands[
            static_cast<std::size_t>(i)];

    return s;
}

int strongestRiskBand(
    const PairMeasurement& m){

    int best=0;

    for(int i=1;i<IPC::kBandCount;++i)
        if(m.bandRisk[
               static_cast<std::size_t>(i)]>
           m.bandRisk[
               static_cast<std::size_t>(best)])
            best=i;

    return best;
}

Recommendation evaluateScenario(
    IPC::Role aRole,
    double aHz,
    double aDb,
    IPC::Role bRole,
    double bHz,
    double bDb,
    double sampleRate,
    PairMeasurement* measured=nullptr,
    double transientA=0.10,
    double transientB=0.10){

    constexpr std::int64_t pos=
        480000;

    const auto aBands=
        analyzeTone(sampleRate,aHz);

    const auto bBands=
        analyzeTone(sampleRate,bHz);

    auto a=
        makeSnapshot(
            aRole,
            aBands,
            aDb,
            pos);

    auto b=
        makeSnapshot(
            bRole,
            bBands,
            bDb,
            pos);

    a.transient=transientA;
    b.transient=transientB;

    const auto m=
        measurePair(
            a,b,pos,256,true);

    if(measured)
        *measured=m;

    if(!m.active)
        return {};

    const int band=
        strongestRiskBand(m);

    return makeRecommendation(
        aRole,
        bRole,
        band,
        m.masking,
        0.75,
        m.dominance,
        m.transientCompetition);
}
}

int main(){
    // Kick and bass sharing the low end must survive the complete analyzer ->
    // pair measurement -> recommendation path as a low-end ownership issue.
    {
        PairMeasurement m;
        const auto rec=evaluateScenario(
            IPC::Role::Kick,80.0,-12.0,
            IPC::Role::Bass,120.0,-13.0,
            48000.0,
            &m);

        assert(m.active);
        assert(m.masking>=0.14);
        assert(rec.valid);
        assert(rec.kind==
               RecommendationKind::LowEndOwnership);
        assert(rec.context==
               RecommendationContext::KickBass);
        assert(rec.adjustRole==
               IPC::Role::Unknown);
    }

    // Same scenario at 192 kHz should lead to the same musical conclusion.
    {
        PairMeasurement m;
        const auto rec=evaluateScenario(
            IPC::Role::Kick,80.0,-12.0,
            IPC::Role::Bass,120.0,-13.0,
            192000.0,
            &m);

        assert(m.active);
        assert(m.masking>=0.14);
        assert(rec.valid);
        assert(rec.kind==
               RecommendationKind::LowEndOwnership);
        assert(rec.context==
               RecommendationContext::KickBass);
    }

    // Vocal and synth occupying the same presence region should produce the
    // beginner-safe vocal-vs-harmonic context and inspect the synth first.
    {
        PairMeasurement m;
        const auto rec=evaluateScenario(
            IPC::Role::LeadVocal,3500.0,-16.0,
            IPC::Role::Synth,3500.0,-17.0,
            48000.0,
            &m);

        assert(m.active);
        assert(m.masking>=0.14);
        assert(rec.valid);
        assert(rec.kind==
               RecommendationKind::PresenceSeparation);
        assert(rec.context==
               RecommendationContext::VocalVsHarmonic);
        assert(rec.adjustRole==
               IPC::Role::Synth);
    }

    // Cymbals and guitar sharing the top end should retain the conservative
    // cymbal-vs-harmonic context rather than blindly telling the user to cut
    // cymbals.
    {
        PairMeasurement m;
        const auto rec=evaluateScenario(
            IPC::Role::Cymbals,7500.0,-18.0,
            IPC::Role::ElectricGuitar,7500.0,-19.0,
            48000.0,
            &m);

        assert(m.active);
        assert(m.masking>=0.14);
        assert(rec.valid);
        assert(rec.kind==
               RecommendationKind::TopEndSeparation);
        assert(rec.context==
               RecommendationContext::CymbalVsHarmonic);
    }

    // Closely neighbouring but not identical presence bands may still be a
    // legitimate masking hint through the conservative neighbour model.
    {
        PairMeasurement m;
        const auto rec=evaluateScenario(
            IPC::Role::LeadVocal,3500.0,-16.0,
            IPC::Role::Pad,5000.0,-16.0,
            48000.0,
            &m);

        assert(m.active);
        assert(m.masking>0.05);
        assert(rec.valid);
        assert(rec.context==
               RecommendationContext::VocalVsHarmonic);
    }

    // Snare and guitar with strong transient competition in the upper mids
    // should become an attack-separation recommendation instead of generic EQ.
    {
        PairMeasurement m;
        const double snareTransient=
            measureBurstTransient(
                48000.0,
                1800.0,
                0.95);

        const double guitarTransient=
            measureBurstTransient(
                48000.0,
                1800.0,
                0.75);

        assert(snareTransient>0.35);
        assert(guitarTransient>0.35);

        const auto rec=evaluateScenario(
            IPC::Role::Snare,1800.0,-14.0,
            IPC::Role::ElectricGuitar,1800.0,-14.5,
            48000.0,
            &m,
            snareTransient,
            guitarTransient);

        assert(m.active);
        assert(m.transientCompetition>=0.35);
        assert(rec.valid);
        assert(rec.kind==
               RecommendationKind::AttackSeparation);
        assert(rec.context==
               RecommendationContext::RhythmVsHarmonic);
    }

    // Clearly separated sources must not turn into a recommendation merely
    // because both are active and well above the Sensor activity threshold.
    {
        PairMeasurement m;
        const auto rec=evaluateScenario(
            IPC::Role::Bass,120.0,-12.0,
            IPC::Role::LeadVocal,3500.0,-14.0,
            48000.0,
            &m);

        assert(m.active);
        assert(m.masking<0.14);
        assert(!rec.valid);
    }

    std::cout
        << "End-to-end Coach scenarios passed\n";

    return 0;
}

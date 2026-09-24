#include "../src/MaskingModel.h"
#include "../src/RecommendationEngine.h"
#include <array>
#include <cassert>
#include <iostream>

int main(){
    using namespace MixDoctorator;
    using namespace MixDoctorator::Analysis;

    auto oneBand=[](int band){
        std::array<double,IPC::kBandCount> v{};
        if(band>=0 && band<IPC::kBandCount)
            v[static_cast<std::size_t>(band)]=1.0;
        return v;
    };

    {
        const auto kick=oneBand(1);
        const auto bass=oneBand(1);

        const auto pair=evaluatePair(
            -12.0,0.9,kick.data(),
            -13.0,0.9,bass.data());

        assert(pair.dominantBand==1);
        assert(pair.masking>0.5);

        const auto rec=makeRecommendation(
            IPC::Role::Kick,
            IPC::Role::Bass,
            pair.dominantBand,
            pair.masking,
            0.75,
            pair.dominance,
            0.10);

        assert(rec.valid);
        assert(rec.kind==RecommendationKind::LowEndOwnership);
        assert(rec.context==RecommendationContext::KickBass);
    }

    {
        const auto vocal=oneBand(6);
        const auto synth=oneBand(6);

        const auto pair=evaluatePair(
            -16.0,0.8,vocal.data(),
            -17.0,0.8,synth.data());

        assert(pair.dominantBand==6);
        assert(pair.masking>0.4);

        const auto rec=makeRecommendation(
            IPC::Role::LeadVocal,
            IPC::Role::Synth,
            pair.dominantBand,
            pair.masking,
            0.70,
            pair.dominance,
            0.05);

        assert(rec.valid);
        assert(rec.kind==RecommendationKind::PresenceSeparation);
        assert(rec.context==RecommendationContext::VocalVsHarmonic);
    }

    {
        const auto cymbals=oneBand(8);
        const auto guitar=oneBand(8);

        const auto pair=evaluatePair(
            -18.0,0.7,cymbals.data(),
            -19.0,0.8,guitar.data());

        assert(pair.dominantBand==8);

        const auto rec=makeRecommendation(
            IPC::Role::Cymbals,
            IPC::Role::ElectricGuitar,
            pair.dominantBand,
            pair.masking,
            0.65,
            pair.dominance,
            0.05);

        assert(rec.valid);
        assert(rec.kind==RecommendationKind::TopEndSeparation);
        assert(rec.context==RecommendationContext::CymbalVsHarmonic);
    }

    {
        const auto bass=oneBand(1);
        const auto vocal=oneBand(6);

        const auto pair=evaluatePair(
            -12.0,0.9,bass.data(),
            -12.0,0.9,vocal.data());

        assert(pair.masking<0.01);

        const auto rec=makeRecommendation(
            IPC::Role::Bass,
            IPC::Role::LeadVocal,
            pair.dominantBand,
            pair.masking,
            0.90,
            pair.dominance,
            0.0);

        assert(!rec.valid);
    }

    {
        std::array<double,IPC::kBandCount> snare{};
        std::array<double,IPC::kBandCount> guitar{};
        snare[5]=0.65;
        snare[6]=0.35;
        guitar[5]=0.60;
        guitar[6]=0.40;

        const auto pair=evaluatePair(
            -14.0,0.9,snare.data(),
            -14.5,0.9,guitar.data());

        const auto rec=makeRecommendation(
            IPC::Role::Snare,
            IPC::Role::ElectricGuitar,
            pair.dominantBand,
            pair.masking,
            0.80,
            pair.dominance,
            0.65);

        assert(rec.valid);
        assert(rec.kind==RecommendationKind::AttackSeparation);
        assert(rec.context==RecommendationContext::RhythmVsHarmonic);
    }

    std::cout
        << "Recommendation scenario integration tests passed\n";

    return 0;
}

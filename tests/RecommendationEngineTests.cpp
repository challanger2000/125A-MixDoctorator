#include "../src/RecommendationEngine.h"
#include <cassert>
#include <iostream>

int main(){
    using namespace MixDoctorator;
    using namespace MixDoctorator::Analysis;

    auto r=makeRecommendation(
        IPC::Role::Kick,
        IPC::Role::Bass,
        1,0.40,0.70,0.2,0.10);
    assert(r.valid);
    assert(r.kind==RecommendationKind::LowEndOwnership);
    assert(r.context==RecommendationContext::KickBass);
    assert(r.score>0.40);

    r=makeRecommendation(
        IPC::Role::Snare,
        IPC::Role::ElectricGuitar,
        6,0.35,0.65,-0.1,0.55);
    assert(r.valid);
    assert(r.kind==RecommendationKind::AttackSeparation);
    assert(r.context==RecommendationContext::RhythmVsHarmonic);

    r=makeRecommendation(
        IPC::Role::LeadVocal,
        IPC::Role::Synth,
        6,0.30,0.50,0.0,0.05);
    assert(r.valid);
    assert(r.kind==RecommendationKind::PresenceSeparation);
    assert(r.context==RecommendationContext::VocalVsHarmonic);

    r=makeRecommendation(
        IPC::Role::Cymbals,
        IPC::Role::ElectricGuitar,
        8,0.25,0.50,0.0,0.05);
    assert(r.valid);
    assert(r.kind==RecommendationKind::TopEndSeparation);
    assert(r.context==RecommendationContext::CymbalVsHarmonic);

    r=makeRecommendation(
        IPC::Role::Bass,
        IPC::Role::Pad,
        2,0.25,0.10,0.0,0.0);
    assert(!r.valid);
    assert(r.context==RecommendationContext::None);

    r=makeRecommendation(
        IPC::Role::Bass,
        IPC::Role::Pad,
        3,0.30,0.60,0.0,0.0);
    assert(r.valid);
    assert(r.context==RecommendationContext::BassVsHarmonic);

    assert(recommendationActionCode(
        RecommendationContext::Generic)==1);
    assert(recommendationActionCode(
        RecommendationContext::BassVsHarmonic)==6);

    std::cout << "Recommendation engine tests passed\n";
    return 0;
}

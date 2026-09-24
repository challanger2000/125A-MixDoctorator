#include "../src/RecommendationEngine.h"
#include "../src/RoleModel.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main(){
    using namespace MixDoctorator;
    using namespace MixDoctorator::Analysis;

    int checked=0;

    for(int a=0;a<IPC::kRoleCount;++a){
        for(int b=a+1;b<IPC::kRoleCount;++b){
            const auto first=roleFromIndex(a);
            const auto second=roleFromIndex(b);

            const int encoded=encodeRolePair(first,second);
            assert(encoded>=0);
            assert(encoded<kRolePairCount);
            assert(encodeRolePair(second,first)==encoded);

            IPC::Role decodedA=IPC::Role::Unknown;
            IPC::Role decodedB=IPC::Role::Unknown;
            assert(decodeRolePair(encoded,decodedA,decodedB));
            assert(decodedA==first);
            assert(decodedB==second);

            for(int band=0;band<IPC::kBandCount;++band){
                const auto normal=makeRecommendation(
                    first,second,band,
                    0.35,0.70,0.15,0.10);

                assert(normal.valid);
                assert(normal.band==band);
                assert(normal.score>=0.0 && normal.score<=1.0);
                assert(normal.confidence>=0.0 && normal.confidence<=1.0);
                assert(normal.dominance>=-1.0 && normal.dominance<=1.0);
                assert(recommendationCode(normal.kind)>=1);
                assert(recommendationCode(normal.kind)<=6);
                assert(recommendationActionCode(normal.context)>=0);
                assert(recommendationActionCode(normal.context)<=5);

                if(band<=1)
                    assert(normal.kind==RecommendationKind::LowEndOwnership);
                else if(band<=3)
                    assert(normal.kind==RecommendationKind::LowMidCleanup);
                else if(band<=5)
                    assert(normal.kind==RecommendationKind::MidSeparation);
                else if(band==6)
                    assert(normal.kind==RecommendationKind::PresenceSeparation);
                else
                    assert(normal.kind==RecommendationKind::TopEndSeparation);

                const auto mirrored=makeRecommendation(
                    second,first,band,
                    0.35,0.70,-0.15,0.10);

                assert(mirrored.valid);
                assert(mirrored.kind==normal.kind);
                assert(mirrored.context==normal.context);
                assert(std::abs(mirrored.score-normal.score)<1.0e-12);

                ++checked;
            }

            if(isTransientRole(first) || isTransientRole(second)){
                const auto attack=makeRecommendation(
                    first,second,4,
                    0.35,0.70,0.0,0.60);
                assert(attack.valid);
                assert(attack.kind==RecommendationKind::AttackSeparation);
            }
        }
    }

    assert(checked==kRolePairCount*IPC::kBandCount);
    assert(checked==819);

    auto invalid=makeRecommendation(
        IPC::Role::Unknown,
        IPC::Role::Bass,
        1,0.5,0.8,0.0,0.0);
    assert(!invalid.valid);

    invalid=makeRecommendation(
        IPC::Role::Bass,
        IPC::Role::Bass,
        1,0.5,0.8,0.0,0.0);
    assert(!invalid.valid);

    invalid=makeRecommendation(
        IPC::Role::Kick,
        IPC::Role::Bass,
        1,0.049,0.8,0.0,0.0);
    assert(!invalid.valid);

    invalid=makeRecommendation(
        IPC::Role::Kick,
        IPC::Role::Bass,
        1,0.5,0.119,0.0,0.0);
    assert(!invalid.valid);

    const double nan=std::numeric_limits<double>::quiet_NaN();
    const double inf=std::numeric_limits<double>::infinity();

    auto pathological=makeRecommendation(
        IPC::Role::Kick,
        IPC::Role::Bass,
        99,nan,inf,nan,inf);

    assert(pathological.band==8);
    assert(std::isfinite(pathological.score));
    assert(std::isfinite(pathological.confidence));
    assert(std::isfinite(pathological.dominance));

    IPC::Role displayA=IPC::Role::Bass;
    IPC::Role displayB=IPC::Role::Kick;
    orderRolePairForDisplay(displayA,displayB);
    assert(displayA==IPC::Role::Kick);
    assert(displayB==IPC::Role::Bass);

    std::cout
        << "Recommendation matrix tests passed: "
        << checked
        << " role/band combinations\n";

    return 0;
}

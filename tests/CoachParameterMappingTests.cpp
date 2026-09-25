#include "../src/ParameterEncoding.h"
#include "../src/RecommendationEngine.h"
#include "../src/CoachModel.h"
#include "../src/RoleModel.h"
#include <cassert>
#include <iostream>

using namespace MixDoctorator::Analysis;

int main(){
    static_assert(
        kRecommendationKindCodeCount==
        static_cast<int>(
            RecommendationKind::AttackSeparation));

    static_assert(
        kRecommendationContextCodeCount==
        static_cast<int>(
            RecommendationContext::BassVsHarmonic));

    static_assert(kCoachEvidenceCodeCount==3);

    for(int code=0;
        code<=kRecommendationKindCodeCount;
        ++code)
        assert(
            decodeDiscreteCode(
                encodeDiscreteCode(
                    code,
                    kRecommendationKindCodeCount),
                kRecommendationKindCodeCount)==code);

    for(int code=0;
        code<=kRecommendationContextCodeCount;
        ++code)
        assert(
            decodeDiscreteCode(
                encodeDiscreteCode(
                    code,
                    kRecommendationContextCodeCount),
                kRecommendationContextCodeCount)==code);

    for(int code=0;
        code<=kCoachEvidenceCodeCount;
        ++code)
        assert(
            decodeDiscreteCode(
                encodeDiscreteCode(
                    code,
                    kCoachEvidenceCodeCount),
                kCoachEvidenceCodeCount)==code);

    for(int code=0;
        code<=kRecommendationBandCodeCount;
        ++code)
        assert(
            decodeDiscreteCode(
                encodeDiscreteCode(
                    code,
                    kRecommendationBandCodeCount),
                kRecommendationBandCodeCount)==code);

    for(int code=0;
        code<=kRecommendationTargetCodeCount;
        ++code)
        assert(
            decodeDiscreteCode(
                encodeDiscreteCode(
                    code,
                    kRecommendationTargetCodeCount),
                kRecommendationTargetCodeCount)==code);

    for(int code=0;
        code<=kRolePairCount;
        ++code)
        assert(
            decodeDiscreteCode(
                encodeDiscreteCode(
                    code,
                    kRolePairCount),
                kRolePairCount)==code);

    std::cout
        << "Coach parameter mapping tests passed\n";

    return 0;
}

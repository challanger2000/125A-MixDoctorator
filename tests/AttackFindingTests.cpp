#include "../src/AttackFinding.h"

#include <cassert>
#include <iostream>
#include <limits>

using MixDoctorator::Analysis::chooseAttackFinding;
using MixDoctorator::Analysis::chooseAttackFindingCount;

int main(){
    {
        const double scores[3]{0.10,0.18,0.20};
        const double observed[3]{10.0,10.0,10.0};

        const auto finding=
            chooseAttackFinding(
                scores,
                observed);

        assert(finding.pair==-1);
        assert(finding.score==0.0);
    }

    {
        const double scores[3]{0.31,0.62,0.44};
        const double observed[3]{10.0,10.0,10.0};

        const auto finding=
            chooseAttackFinding(
                scores,
                observed);

        assert(finding.pair==1);
        assert(finding.score>0.61);
    }

    {
        const double scores[3]{0.70,0.55,0.40};
        const double observed[3]{0.5,8.0,8.0};

        const auto finding=
            chooseAttackFinding(
                scores,
                observed);

        assert(finding.pair==1);
    }

    {
        const double scores[5]{
            0.10,0.75,0.40,0.90,0.65
        };
        const double observed[5]{
            10.0,10.0,10.0,0.5,10.0
        };

        const auto finding=
            chooseAttackFindingCount(
                scores,
                observed,
                5,
                0.30,
                2.0);

        assert(finding.pair==1);
        assert(finding.score>0.74);
    }

    // Coach-specific threshold must not surface a borderline transient hint
    // before the stable beginner-facing window is reached.
    {
        const double scores[3]{0.34,0.35,0.80};
        const double observed[3]{10.0,2.5,2.49};

        const auto finding=
            chooseAttackFindingCount(
                scores,
                observed,
                3,
                MixDoctorator::Analysis::kCoachAttackMinimumScore,
                MixDoctorator::Analysis::kCoachAttackMinimumObservation);

        assert(finding.pair==1);
        assert(finding.score==0.35);
    }

    // Held Coach attack finding should not flap to a nearly equal challenger.
    {
        const double scores[3]{0.50,0.53,0.20};
        const double observed[3]{8.0,8.0,8.0};

        auto finding=
            MixDoctorator::Analysis::chooseStableAttackFindingCount(
                scores,
                observed,
                3,
                0);

        assert(finding.pair==0);

        const double stronger[3]{0.50,0.57,0.20};

        finding=
            MixDoctorator::Analysis::chooseStableAttackFindingCount(
                stronger,
                observed,
                3,
                0);

        assert(finding.pair==1);
    }

    // If the held pair becomes ineligible, the strongest valid pair takes over.
    {
        const double scores[3]{0.20,0.48,0.42};
        const double observed[3]{8.0,8.0,8.0};

        const auto finding=
            MixDoctorator::Analysis::chooseStableAttackFindingCount(
                scores,
                observed,
                3,
                0);

        assert(finding.pair==1);
    }

    {
        const double nan=
            std::numeric_limits<double>::
            quiet_NaN();

        const double inf=
            std::numeric_limits<double>::
            infinity();

        const double scores[4]{
            nan,inf,0.45,0.30
        };
        const double observed[4]{
            10.0,10.0,nan,10.0
        };

        const auto finding=
            chooseAttackFindingCount(
                scores,
                observed,
                4,
                0.22,
                2.0);

        assert(finding.pair==3);
        assert(finding.score==0.30);
    }

    std::cout
        << "AttackFinding tests passed\n";

    return 0;
}

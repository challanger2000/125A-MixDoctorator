#include "../src/AttackFinding.h"

#include <cassert>
#include <iostream>

using MixDoctorator::Analysis::chooseAttackFinding;

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

    std::cout
        << "AttackFinding tests passed\n";

    return 0;
}

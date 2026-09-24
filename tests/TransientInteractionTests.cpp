#include "../src/TransientInteraction.h"

#include <cassert>
#include <iostream>

using MixDoctorator::Analysis::transientCompetition;

int main(){
    const double strong=
        transientCompetition(
            0.85,0.80,
            -18.0,-18.0,
            1.0,1.0);

    assert(strong>0.75);

    const double weakSecond=
        transientCompetition(
            0.85,0.05,
            -18.0,-18.0,
            1.0,1.0);

    assert(weakSecond<strong*0.35);

    const double levelSeparated=
        transientCompetition(
            0.85,0.80,
            -18.0,-36.0,
            1.0,1.0);

    assert(levelSeparated<strong*0.20);

    const double lowActivity=
        transientCompetition(
            0.85,0.80,
            -18.0,-18.0,
            0.10,0.10);

    assert(lowActivity<strong*0.50);

    std::cout
        << "TransientInteraction tests passed\n";

    return 0;
}

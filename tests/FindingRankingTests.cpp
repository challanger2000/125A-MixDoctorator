#include "../src/FindingRanking.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main(){
    using MixDoctorator::Analysis::FindingCandidate;
    using MixDoctorator::Analysis::chooseStableFinding;
    using MixDoctorator::Analysis::findingEligible;
    using MixDoctorator::Analysis::findingRank;

    std::array<FindingCandidate,6> c{};

    auto choose=[&](int held,double margin=0.04){
        return chooseStableFinding(
            static_cast<int>(c.size()),
            held,
            [&](int i){ return c[static_cast<std::size_t>(i)]; },
            margin);
    };

    assert(choose(-1)==-1);

    c[0]={2.49,0.50,0.80};
    assert(!findingEligible(c[0]));
    assert(choose(-1)==-1);

    c[0]={3.0,0.20,0.40};
    assert(findingEligible(c[0]));
    assert(findingRank(c[0])>0.0);
    assert(choose(-1)==0);

    c[1]={4.0,0.25,0.50};
    assert(choose(-1)==1);

    // A held finding should not flap to a merely tiny improvement.
    c[0]={4.0,0.30,0.60};
    c[1]={4.0,0.31,0.60};
    assert(choose(0)==0);

    // A clearly better challenger must take over.
    c[1]={4.0,0.55,0.80};
    assert(choose(0)==1);

    // If held becomes ineligible, best current finding is selected.
    c[1]={1.0,0.55,0.80};
    assert(choose(1)==0);

    // Many candidates: strongest stable one wins.
    c[2]={8.0,0.35,0.70};
    c[3]={8.0,0.42,0.80};
    c[4]={8.0,0.28,0.95};
    c[5]={8.0,0.50,0.90};
    assert(choose(-1)==5);

    // Non-finite data is never eligible.
    const double nan=std::numeric_limits<double>::quiet_NaN();
    c[5]={8.0,nan,0.90};
    assert(!findingEligible(c[5]));
    assert(choose(-1)==3);

    // Negative or non-finite switch margins are sanitized. Isolate this
    // scenario so stronger candidates from the previous test cannot leak in.
    c.fill(FindingCandidate{});
    c[0]={8.0,0.30,0.60};
    c[3]={8.0,0.31,0.60};
    assert(choose(0,-1.0)==3);
    assert(choose(0,nan)==0);

    std::cout << "Finding ranking tests passed\n";
    return 0;
}

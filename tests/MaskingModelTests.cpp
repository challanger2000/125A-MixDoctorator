#include "../src/MaskingModel.h"

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

using MixDoctorator::Analysis::evaluatePair;
using MixDoctorator::Analysis::kBandCount;

static std::array<double,kBandCount>
oneBand(int index,double amount=1.0){
    std::array<double,kBandCount> bands{};
    bands[index]=amount;
    return bands;
}

int main(){
    const auto a=oneBand(3);
    const auto b=oneBand(3);

    const auto same=
        evaluatePair(
            -18.0,1.0,a.data(),
            -18.0,1.0,b.data());

    assert(same.overlap>0.95);
    assert(same.masking>0.95);
    assert(same.dominantBand==3);
    assert(std::abs(same.dominance)<0.01);

    const auto gap20=
        evaluatePair(
            -18.0,1.0,a.data(),
            -38.0,1.0,b.data());

    assert(gap20.overlap>0.95);
    assert(gap20.masking<0.10);
    assert(gap20.dominance>0.95);

    const auto separateBand=oneBand(6);

    const auto separate=
        evaluatePair(
            -18.0,1.0,a.data(),
            -18.0,1.0,separateBand.data());

    assert(separate.overlap<0.01);
    assert(separate.masking<0.01);

    const auto inactive=
        evaluatePair(
            -18.0,1.0,a.data(),
            -18.0,0.0,b.data());

    assert(inactive.masking<
           same.masking*0.40);

    std::array<double,kBandCount> broadA{};
    std::array<double,kBandCount> broadB{};

    for(int i=2;i<=5;++i){
        broadA[i]=0.25;
        broadB[i]=0.25;
    }

    const auto broad=
        evaluatePair(
            -18.0,1.0,broadA.data(),
            -18.0,1.0,broadB.data());

    assert(broad.masking>0.90);

    std::array<double,kBandCount> d1{};
    std::array<double,kBandCount> d2{};

    d1[2]=0.15;
    d1[5]=0.60;
    d2[2]=0.15;
    d2[5]=0.50;

    const auto dominant=
        evaluatePair(
            -18.0,1.0,d1.data(),
            -18.0,1.0,d2.data());

    assert(dominant.dominantBand==5);

    std::cout
        << "MaskingModel tests passed\n";

    return 0;
}

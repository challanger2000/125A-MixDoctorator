#include "../src/NeighborMaskingModel.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
    using namespace MixDoctorator::Analysis;

    auto oneBand=[](int index){
        std::array<double,kBandCount> v{};
        v[index]=1.0;
        return v;
    };

    // Identical spectra must remain strong and bounded.
    {
        const auto a=oneBand(4);
        const auto b=oneBand(4);

        const auto m=evaluatePairWithNeighborSpread(
            -18.0,1.0,a.data(),
            -18.0,1.0,b.data());

        assert(m.direct.masking>0.95);
        assert(m.spread.masking>0.90);
        assert(m.spread.masking<=1.0);
    }

    // Immediately adjacent bands are the intended target: direct masking is
    // near zero, while conservative spreading should reveal some interaction.
    {
        const auto a=oneBand(4);
        const auto b=oneBand(5);

        const auto m=evaluatePairWithNeighborSpread(
            -18.0,1.0,a.data(),
            -18.0,1.0,b.data());

        assert(m.direct.masking<0.01);
        assert(m.spread.masking>0.05);
        assert(m.spread.masking<0.35);
        assert(m.addedRisk>0.05);
    }

    // Two-band separation should remain essentially clear. This is the main
    // false-positive guard for the experimental spread.
    {
        const auto a=oneBand(3);
        const auto b=oneBand(5);

        const auto m=evaluatePairWithNeighborSpread(
            -18.0,1.0,a.data(),
            -18.0,1.0,b.data());

        assert(m.direct.masking<0.01);
        assert(m.spread.masking<0.04);
    }

    // A large level difference should still suppress neighbour interaction.
    {
        const auto a=oneBand(4);
        const auto b=oneBand(5);

        const auto close=evaluatePairWithNeighborSpread(
            -18.0,1.0,a.data(),
            -18.0,1.0,b.data());

        const auto far=evaluatePairWithNeighborSpread(
            -18.0,1.0,a.data(),
            -36.0,1.0,b.data());

        assert(far.spread.masking<
               close.spread.masking*0.20);
    }

    // Swapping source order must preserve total risk.
    {
        const auto a=oneBand(2);
        const auto b=oneBand(3);

        const auto ab=evaluatePairWithNeighborSpread(
            -15.0,0.8,a.data(),
            -17.0,0.7,b.data());

        const auto ba=evaluatePairWithNeighborSpread(
            -17.0,0.7,b.data(),
            -15.0,0.8,a.data());

        assert(std::abs(
            ab.spread.masking-
            ba.spread.masking)<1.0e-12);
    }

    std::cout
        << "Neighbor masking experiment tests passed\n";
    return 0;
}

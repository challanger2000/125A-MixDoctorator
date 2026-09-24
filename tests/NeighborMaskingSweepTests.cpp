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

    double maxAdjacent=0.0;
    double maxDistance2=0.0;
    double maxDistance3Plus=0.0;

    for(int a=0;a<kBandCount;++a){
        for(int b=0;b<kBandCount;++b){
            if(a==b)
                continue;

            const auto pa=oneBand(a);
            const auto pb=oneBand(b);

            const auto m=
                evaluatePairWithNeighborSpread(
                    -18.0,1.0,pa.data(),
                    -18.0,1.0,pb.data());

            const int distance=
                std::abs(a-b);

            assert(std::isfinite(m.spread.masking));
            assert(m.spread.masking>=0.0);
            assert(m.spread.masking<=1.0);

            if(distance==1)
                maxAdjacent=
                    std::max(
                        maxAdjacent,
                        m.spread.masking);
            else if(distance==2)
                maxDistance2=
                    std::max(
                        maxDistance2,
                        m.spread.masking);
            else
                maxDistance3Plus=
                    std::max(
                        maxDistance3Plus,
                        m.spread.masking);
        }
    }

    assert(maxAdjacent>0.05);
    assert(maxAdjacent<0.35);
    assert(maxDistance2<1.0e-12);
    assert(maxDistance3Plus<0.001);

    // Level-gap sweep for adjacent bands must be monotonically decreasing.
    const auto a=oneBand(4);
    const auto b=oneBand(5);

    const double gaps[]{0.0,3.0,6.0,12.0,18.0,24.0};
    double previous=2.0;

    for(double gap:gaps){
        const auto m=
            evaluatePairWithNeighborSpread(
                -18.0,1.0,a.data(),
                -18.0-gap,1.0,b.data());

        assert(m.spread.masking<previous);
        previous=m.spread.masking;
    }

    std::cout
        << "Neighbor masking matrix: adjacent="
        << maxAdjacent
        << " distance2="
        << maxDistance2
        << " distance3+="
        << maxDistance3Plus
        << "\n";

    return 0;
}

#include "../src/ResetStateModel.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>

int main(){
    using namespace MixDoctorator::Analysis;

    assert(nextNonZeroGeneration(1)==2);
    assert(nextNonZeroGeneration(41)==42);
    assert(
        nextNonZeroGeneration(
            std::numeric_limits<
                std::uint64_t>::max())==1);

    double published[51];

    for(int i=0;i<51;++i)
        published[i]=
            static_cast<double>(i)/
            50.0;

    invalidatePublishedValues(
        published);

    for(double value:published)
        assert(value==-1.0);

    // Repeated resets must keep forcing a republish sentinel.
    for(int cycle=0;cycle<64;++cycle){
        for(int i=0;i<51;++i)
            published[i]=0.5;

        invalidatePublishedValues(
            published);

        for(double value:published)
            assert(value==-1.0);
    }

    std::cout
        << "Brain reset state tests passed\n";

    return 0;
}

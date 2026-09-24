#include "../src/TransientModel.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using MixDoctorator::Analysis::TransientDetector;

int main(){
    constexpr double sr=48000.0;

    TransientDetector detector;
    detector.prepare(sr);

    for(int i=0;i<static_cast<int>(sr);++i)
        detector.pushPower(0.25);

    assert(detector.value()<0.12);

    detector.prepare(sr);

    for(int i=0;i<static_cast<int>(sr*0.25);++i)
        detector.pushPower(0.0);

    double maxTransient=0.0;

    for(int i=0;i<static_cast<int>(sr*0.03);++i){
        detector.pushPower(0.8);
        maxTransient=
            std::max(
                maxTransient,
                detector.value());
    }

    assert(maxTransient>0.45);

    for(int i=0;i<static_cast<int>(sr*0.50);++i)
        detector.pushPower(0.8);

    assert(detector.value()<0.20);

    std::cout
        << "TransientModel tests passed\n";

    return 0;
}

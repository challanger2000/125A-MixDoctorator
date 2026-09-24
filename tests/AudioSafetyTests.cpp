#include "../src/AudioSafety.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main(){
    using MixDoctorator::Analysis::
        sanitizeAudioSample;

    const double finiteValues[]={
        0.0,
        -0.0,
        1.0,
        -1.0,
        0.125,
        -0.875,
        1.0e-300
    };

    for(double value:finiteValues){
        const double out=
            sanitizeAudioSample(value);

        assert(std::isfinite(out));
        assert(out==value);
    }

    assert(
        sanitizeAudioSample(
            std::numeric_limits<double>::
                quiet_NaN())==0.0);

    assert(
        sanitizeAudioSample(
            std::numeric_limits<double>::
                infinity())==0.0);

    assert(
        sanitizeAudioSample(
            -std::numeric_limits<double>::
                infinity())==0.0);

    std::cout
        << "AudioSafety tests passed\n";

    return 0;
}

#include "../src/AudioSafety.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main(){
    using MixDoctorator::Analysis::
        sanitizeAudioSample;
    using MixDoctorator::Analysis::
        readAudioSample;

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

    const float floatBuffer[]{
        0.25f,
        -0.5f,
        std::numeric_limits<float>::
            quiet_NaN()
    };

    assert(std::abs(
        readAudioSample(
            floatBuffer,0)-0.25)<1.0e-7);

    assert(std::abs(
        readAudioSample(
            floatBuffer,1)+0.5)<1.0e-7);

    assert(
        readAudioSample(
            floatBuffer,2)==0.0);

    assert(
        readAudioSample<float>(
            nullptr,0)==0.0);

    assert(
        readAudioSample(
            floatBuffer,-1)==0.0);

    std::cout
        << "AudioSafety tests passed\n";

    return 0;
}

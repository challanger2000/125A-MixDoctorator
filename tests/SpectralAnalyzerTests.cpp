#include "../src/SpectralAnalyzer.h"

#include <cassert>
#include <cmath>
#include <iostream>

using MixDoctorator::Analysis::SpectralAnalyzer;
using MixDoctorator::Analysis::combineStereoFractions;

namespace {
constexpr double kPi=
    3.14159265358979323846;

int expectedBand(double hz){
    if(hz<80.0) return 0;
    if(hz<160.0) return 1;
    if(hz<300.0) return 2;
    if(hz<600.0) return 3;
    if(hz<1200.0) return 4;
    if(hz<2500.0) return 5;
    if(hz<5000.0) return 6;
    if(hz<10000.0) return 7;
    return 8;
}

int strongestBand(
    const std::array<double,9>& bands){

    int strongest=0;

    for(int i=1;i<9;++i)
        if(bands[i]>
           bands[strongest])
            strongest=i;

    return strongest;
}
}

int main(){
    constexpr double sampleRate=
        48000.0;

    const double frequencies[]={
        50.0,
        120.0,
        220.0,
        430.0,
        850.0,
        1800.0,
        3500.0,
        7500.0,
        14000.0
    };

    for(double hz:frequencies){
        SpectralAnalyzer left;
        SpectralAnalyzer right;

        left.prepare(
            sampleRate);

        right.prepare(
            sampleRate);

        for(int n=0;
            n<24000;
            ++n){

            const double x=
                std::sin(
                    2.0*kPi*
                    hz*
                    static_cast<double>(n)/
                    sampleRate);

            left.push(x);
            right.push(x);
        }

        const auto bands=
            combineStereoFractions(
                left,
                right);

        assert(
            strongestBand(bands)==
            expectedBand(hz));
    }

    // Anti-phase stereo must not disappear from analysis.
    SpectralAnalyzer left;
    SpectralAnalyzer right;

    left.prepare(sampleRate);
    right.prepare(sampleRate);

    constexpr double antiPhaseHz=
        3500.0;

    for(int n=0;
        n<24000;
        ++n){

        const double x=
            std::sin(
                2.0*kPi*
                antiPhaseHz*
                static_cast<double>(n)/
                sampleRate);

        left.push(x);
        right.push(-x);
    }

    const auto antiPhaseBands=
        combineStereoFractions(
            left,
            right);

    assert(
        strongestBand(
            antiPhaseBands)==6);

    assert(
        antiPhaseBands[6]>0.95);

    std::cout
        << "SpectralAnalyzer tests passed\n";

    return 0;
}

#include "../src/SpectralAnalyzer.h"
#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>

int main(){
    using MixDoctorator::Analysis::SpectralAnalyzer;

    constexpr double sampleRate=192000.0;
    constexpr double seconds=2.0;
    const int samples=
        static_cast<int>(sampleRate*seconds);

    SpectralAnalyzer left;
    SpectralAnalyzer right;
    left.prepare(sampleRate);
    right.prepare(sampleRate);

    constexpr double pi=
        3.14159265358979323846;

    const auto start=
        std::chrono::steady_clock::now();

    for(int i=0;i<samples;++i){
        const double t=
            static_cast<double>(i)/
            sampleRate;

        const double l=
            0.15*std::sin(2.0*pi*80.0*t)+
            0.08*std::sin(2.0*pi*3500.0*t);

        const double r=
            0.15*std::sin(2.0*pi*120.0*t)+
            0.08*std::sin(2.0*pi*7500.0*t);

        left.push(l);
        right.push(r);
    }

    const auto end=
        std::chrono::steady_clock::now();

    const double elapsedMs=
        std::chrono::duration<double,std::milli>(
            end-start).count();

    // Catastrophic-regression guard only. Two channels of 192 kHz analysis
    // must remain comfortably faster than real time on the CI runner.
    assert(elapsedMs<1500.0);

    double energy=0.0;
    for(double v:left.energy())
        energy+=v;
    for(double v:right.energy())
        energy+=v;

    assert(std::isfinite(energy));
    assert(energy>0.0);

    std::cout
        << "Spectral realtime smoke: "
        << elapsedMs
        << " ms for "
        << seconds
        << " s stereo at "
        << sampleRate
        << " Hz\n";

    return 0;
}

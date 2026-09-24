#include "../src/SpectralAnalyzer.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

using MixDoctorator::Analysis::SpectralAnalyzer;
using MixDoctorator::Analysis::combineStereoFractions;

static std::array<double,SpectralAnalyzer::kBandCount>
measureTone(double sampleRate,double frequency){
    SpectralAnalyzer left;
    SpectralAnalyzer right;
    left.prepare(sampleRate);
    right.prepare(sampleRate);

    const int samples=
        static_cast<int>(
            std::ceil(sampleRate*1.5));

    constexpr double pi=
        3.14159265358979323846;

    for(int i=0;i<samples;++i){
        const double x=
            0.2*
            std::sin(
                2.0*pi*frequency*
                static_cast<double>(i)/
                sampleRate);

        left.push(x);
        right.push(x);
    }

    return combineStereoFractions(left,right);
}

static int dominantBand(
    const std::array<double,SpectralAnalyzer::kBandCount>& bands){
    int best=0;
    for(int i=1;i<SpectralAnalyzer::kBandCount;++i)
        if(bands[i]>bands[best])
            best=i;
    return best;
}

int main(){
    const double sampleRates[]{44100.0,48000.0,96000.0,192000.0};
    const double frequencies[]{50.0,120.0,220.0,430.0,850.0,1800.0,3500.0,7500.0,14000.0};

    double worstCentreShare=1.0;
    double worstRateDrift=0.0;

    for(int band=0;band<SpectralAnalyzer::kBandCount;++band){
        std::array<double,4> centreShare{};

        for(int r=0;r<4;++r){
            const auto measured=
                measureTone(
                    sampleRates[r],
                    frequencies[band]);

            const int dominant=
                dominantBand(measured);

            // A tone at a declared analysis-band centre should resolve to that
            // band or, at worst, one direct neighbour.
            assert(std::abs(dominant-band)<=1);

            centreShare[r]=measured[band];
            worstCentreShare=
                std::min(
                    worstCentreShare,
                    centreShare[r]);
        }

        for(int a=0;a<4;++a)
            for(int b=a+1;b<4;++b)
                worstRateDrift=
                    std::max(
                        worstRateDrift,
                        std::abs(
                            centreShare[a]-
                            centreShare[b]));
    }

    // Low-end spot checks are intentionally stricter: these are central to
    // kick/bass coaching and should not migrate wildly with sample rate.
    for(double frequency : {50.0,80.0,120.0,160.0}){
        const auto a=measureTone(44100.0,frequency);
        const auto b=measureTone(96000.0,frequency);
        const auto c192=measureTone(192000.0,frequency);

        double distance96=0.0;
        double distance192=0.0;
        for(int i=0;i<SpectralAnalyzer::kBandCount;++i){
            distance96+=std::abs(a[i]-b[i]);
            distance192+=std::abs(a[i]-c192[i]);
        }

        std::cout
            << "Low-end " << frequency
            << " Hz: drift 44.1->96k=" << distance96
            << " drift 44.1->192k=" << distance192
            << "\n";

        // Total-variation style bound. Large values mean the same tone would
        // look materially different to the Coach merely because of sample rate.
        assert(distance96<0.70);
        assert(distance192<0.85);
    }

    assert(worstCentreShare>0.18);
    assert(worstRateDrift<0.45);

    std::cout
        << "Spectral sample-rate accuracy: worst centre share="
        << worstCentreShare
        << " worst rate drift="
        << worstRateDrift
        << "\n";

    return 0;
}

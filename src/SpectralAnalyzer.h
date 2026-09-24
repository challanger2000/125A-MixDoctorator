#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>

namespace MixDoctorator::Analysis {

constexpr int kSpectralBandCount=9;

struct SmoothBandPosition {
    int lower{0};
    int upper{0};
    double upperWeight{0.0};
};

inline SmoothBandPosition smoothBandPosition(
    double hz) noexcept {

    static constexpr double centres[kSpectralBandCount]{
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

    if(!std::isfinite(hz) ||
       hz<=centres[0])
        return {0,0,0.0};

    if(hz>=centres[kSpectralBandCount-1])
        return {
            kSpectralBandCount-1,
            kSpectralBandCount-1,
            0.0
        };

    for(int i=0;
        i<kSpectralBandCount-1;
        ++i){

        if(hz>centres[i+1])
            continue;

        const double lo=
            std::log(centres[i]);

        const double hi=
            std::log(centres[i+1]);

        const double x=
            std::log(hz);

        return {
            i,
            i+1,
            std::clamp(
                (x-lo)/(hi-lo),
                0.0,
                1.0)
        };
    }

    return {
        kSpectralBandCount-1,
        kSpectralBandCount-1,
        0.0
    };
}

inline void distributeSmoothBandEnergy(
    std::array<double,kSpectralBandCount>& bands,
    double hz,
    double power) noexcept {

    if(!std::isfinite(power) ||
       power<=0.0)
        return;

    const auto position=
        smoothBandPosition(hz);

    bands[position.lower]+=
        power*
        (1.0-position.upperWeight);

    if(position.upper!=position.lower)
        bands[position.upper]+=
            power*
            position.upperWeight;
}

class SpectralAnalyzer {
public:
    static constexpr int kFftSize=1024;
    static constexpr int kHopSize=512;
    static constexpr int kBandCount=kSpectralBandCount;

    void prepare(double sampleRate) noexcept {
        sampleRate_=
            (std::isfinite(sampleRate) && sampleRate>8000.0)
            ? sampleRate
            : 44100.0;

        write_=0;
        sinceFft_=0;
        filled_=0;

        time_.fill(0.0);
        energy_.fill(0.0);
        prepareBandMap();
    }

    void push(double x) noexcept {
        if(!std::isfinite(x))
            x=0.0;

        time_[write_]=x;
        write_=(write_+1)%kFftSize;
        filled_=std::min(filled_+1,kFftSize);
        ++sinceFft_;

        if(filled_==kFftSize &&
           sinceFft_>=kHopSize){
            sinceFft_=0;
            analyze();
        }
    }

    const std::array<double,kBandCount>&
    energy() const noexcept {
        return energy_;
    }

private:
    static constexpr double kPi=
        3.1415926535897932384626433832795;

    std::array<double,kFftSize> time_{};
    std::array<std::complex<double>,kFftSize> fft_{};
    std::array<double,kBandCount> energy_{};
    std::array<int,kFftSize/2+1> bandLower_{};
    std::array<int,kFftSize/2+1> bandUpper_{};
    std::array<double,kFftSize/2+1> bandUpperWeight_{};

    double sampleRate_{44100.0};
    int write_{0};
    int sinceFft_{0};
    int filled_{0};

    void prepareBandMap() noexcept {
        for(int bin=1;
            bin<=kFftSize/2;
            ++bin){

            const double hz=
                static_cast<double>(bin)*
                sampleRate_/
                static_cast<double>(
                    kFftSize);

            const auto position=
                smoothBandPosition(hz);

            bandLower_[bin]=
                position.lower;

            bandUpper_[bin]=
                position.upper;

            bandUpperWeight_[bin]=
                position.upperWeight;
        }
    }

    void fftInPlace() noexcept {
        for(int i=1,j=0;i<kFftSize;++i){
            int bit=kFftSize>>1;

            for(;j&bit;bit>>=1)
                j^=bit;

            j^=bit;

            if(i<j)
                std::swap(
                    fft_[i],
                    fft_[j]);
        }

        for(int len=2;
            len<=kFftSize;
            len<<=1){

            const double angle=
                -2.0*kPi/
                static_cast<double>(len);

            const std::complex<double> wlen(
                std::cos(angle),
                std::sin(angle));

            for(int i=0;
                i<kFftSize;
                i+=len){

                std::complex<double> w(
                    1.0,
                    0.0);

                for(int j=0;
                    j<len/2;
                    ++j){

                    const auto u=
                        fft_[i+j];

                    const auto v=
                        fft_[i+j+len/2]*
                        w;

                    fft_[i+j]=u+v;
                    fft_[i+j+len/2]=u-v;

                    w*=wlen;
                }
            }
        }
    }

    void analyze() noexcept {
        for(int i=0;i<kFftSize;++i){
            const int index=
                (write_+i)%kFftSize;

            const double window=
                0.5-
                0.5*
                std::cos(
                    2.0*kPi*
                    static_cast<double>(i)/
                    static_cast<double>(
                        kFftSize-1));

            fft_[i]=
                std::complex<double>(
                    time_[index]*window,
                    0.0);
        }

        fftInPlace();

        std::array<double,kBandCount> raw{};

        for(int bin=1;
            bin<=kFftSize/2;
            ++bin){

            const double hz=
                static_cast<double>(bin)*
                sampleRate_/
                static_cast<double>(
                    kFftSize);

            const double power=
                std::norm(
                    fft_[bin]);

            const int lower=
                bandLower_[bin];

            const int upper=
                bandUpper_[bin];

            const double upperWeight=
                bandUpperWeight_[bin];

            raw[lower]+=
                power*
                (1.0-upperWeight);

            if(upper!=lower)
                raw[upper]+=
                    power*
                    upperWeight;
        }

        constexpr double kSmooth=0.35;

        for(int i=0;
            i<kBandCount;
            ++i){

            energy_[i]+=
                kSmooth*
                (raw[i]-energy_[i]);
        }
    }
};

inline std::array<double,SpectralAnalyzer::kBandCount>
combineStereoFractions(
    const SpectralAnalyzer& left,
    const SpectralAnalyzer& right) noexcept {

    std::array<double,SpectralAnalyzer::kBandCount> out{};
    double total=0.0;

    for(int i=0;
        i<SpectralAnalyzer::kBandCount;
        ++i){

        out[i]=
            left.energy()[i]+
            right.energy()[i];

        total+=out[i];
    }

    if(total>1.0e-20)
        for(double& value:out)
            value/=total;

    return out;
}

} // namespace MixDoctorator::Analysis

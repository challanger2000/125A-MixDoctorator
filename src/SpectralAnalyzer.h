#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>

namespace MixDoctorator::Analysis {

class SpectralAnalyzer {
public:
    static constexpr int kFftSize=1024;
    static constexpr int kHopSize=512;
    static constexpr int kBandCount=9;

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
    }

    void push(double x) noexcept {
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

    double sampleRate_{44100.0};
    int write_{0};
    int sinceFft_{0};
    int filled_{0};

    static int bandFor(double hz) noexcept {
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

            raw[
                bandFor(hz)]
                +=std::norm(
                    fft_[bin]);
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

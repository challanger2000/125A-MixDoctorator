#pragma once
#include <algorithm>
#include <cmath>

namespace MixDoctorator::Analysis {

class TransientDetector {
public:
    void prepare(double sampleRate) noexcept {
        const double sr=
            (std::isfinite(sampleRate) && sampleRate>8000.0)
            ? sampleRate
            : 44100.0;

        fastAlpha_=1.0-std::exp(-1.0/(0.006*sr));
        slowAlpha_=1.0-std::exp(-1.0/(0.090*sr));
        attackAlpha_=1.0-std::exp(-1.0/(0.012*sr));
        releaseAlpha_=1.0-std::exp(-1.0/(0.120*sr));

        reset();
    }

    void reset() noexcept {
        fastPower_=0.0;
        slowPower_=0.0;
        value_=0.0;
    }

    void pushPower(double power) noexcept {
        power=std::max(0.0,power);

        fastPower_+=fastAlpha_*(power-fastPower_);
        slowPower_+=slowAlpha_*(power-slowPower_);

        double target=0.0;

        if(power>1.0e-12){
            const double ratioDb=
                10.0*
                std::log10(
                    (fastPower_+1.0e-15)/
                    (slowPower_+1.0e-15));

            target=
                std::clamp(
                    (ratioDb-1.5)/10.0,
                    0.0,
                    1.0);
        }

        const double alpha=
            target>value_
            ? attackAlpha_
            : releaseAlpha_;

        value_+=alpha*(target-value_);
    }

    double value() const noexcept {
        return std::clamp(value_,0.0,1.0);
    }

private:
    double fastAlpha_{0.0};
    double slowAlpha_{0.0};
    double attackAlpha_{0.0};
    double releaseAlpha_{0.0};

    double fastPower_{0.0};
    double slowPower_{0.0};
    double value_{0.0};
};

} // namespace MixDoctorator::Analysis

#include "../src/SpectralAnalyzer.h"
#include "../src/TransientModel.h"
#include "../src/MaskingModel.h"
#include "../src/RoleAggregate.h"

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main(){
    using namespace MixDoctorator::Analysis;

    SpectralAnalyzer left;
    SpectralAnalyzer right;
    left.prepare(48000.0);
    right.prepare(48000.0);

    const double bad[]={
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()
    };

    for(int i=0;i<4096;++i){
        left.push(bad[i%3]);
        right.push(bad[(i+1)%3]);
    }

    const auto bands=
        combineStereoFractions(left,right);

    double total=0.0;
    for(double v:bands){
        assert(std::isfinite(v));
        assert(v>=0.0);
        total+=v;
    }
    assert(total<=1.000001);

    // Silence, denormals and extreme finite input must keep the complete
    // analyzer path finite and bounded.
    {
        SpectralAnalyzer silenceLeft;
        SpectralAnalyzer silenceRight;
        silenceLeft.prepare(48000.0);
        silenceRight.prepare(48000.0);

        for(int i=0;i<4096;++i){
            silenceLeft.push(0.0);
            silenceRight.push(0.0);
        }

        const auto silenceBands=
            combineStereoFractions(
                silenceLeft,
                silenceRight);

        double silenceTotal=0.0;

        for(double v:silenceBands){
            assert(std::isfinite(v));
            assert(v>=0.0);
            silenceTotal+=v;
        }

        assert(silenceTotal==0.0);
    }

    {
        SpectralAnalyzer extremeLeft;
        SpectralAnalyzer extremeRight;
        extremeLeft.prepare(48000.0);
        extremeRight.prepare(48000.0);

        const double denormal=
            std::numeric_limits<double>::
                denorm_min();

        const double large=
            std::sqrt(
                std::numeric_limits<double>::
                    max())*
            1.0e-6;

        for(int i=0;i<4096;++i){
            const double a=
                (i&1)
                ? denormal
                : -denormal;

            const double b=
                (i&1)
                ? large
                : -large;

            extremeLeft.push(a);
            extremeRight.push(b);
        }

        const auto extremeBands=
            combineStereoFractions(
                extremeLeft,
                extremeRight);

        double extremeTotal=0.0;

        for(double v:extremeBands){
            assert(std::isfinite(v));
            assert(v>=0.0);
            extremeTotal+=v;
        }

        assert(std::isfinite(extremeTotal));
        assert(extremeTotal<=1.000001);
    }

    TransientDetector detector;
    detector.prepare(48000.0);

    for(double v:bad)
        detector.pushPower(v);

    assert(std::isfinite(detector.value()));
    assert(detector.value()>=0.0);
    assert(detector.value()<=1.0);

    double brokenBands[9]{};
    brokenBands[2]=
        std::numeric_limits<double>::quiet_NaN();
    brokenBands[4]=
        std::numeric_limits<double>::infinity();

    const auto pair=
        evaluatePair(
            std::numeric_limits<double>::quiet_NaN(),
            std::numeric_limits<double>::infinity(),
            brokenBands,
            -18.0,
            0.8,
            brokenBands);

    assert(std::isfinite(pair.overlap));
    assert(std::isfinite(pair.masking));
    assert(std::isfinite(pair.dominance));

    RoleAggregate aggregate;
    aggregate.reset();
    aggregate.add(
        -18.0,
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::quiet_NaN(),
        brokenBands);

    const auto aggregateResult=
        aggregate.result();

    assert(aggregateResult.valid);
    assert(std::isfinite(aggregateResult.rmsDb));
    assert(std::isfinite(aggregateResult.peakDb));
    assert(std::isfinite(aggregateResult.activity));
    assert(std::isfinite(aggregateResult.transient));

    std::cout << "AnalysisSafety tests passed\n";
    return 0;
}

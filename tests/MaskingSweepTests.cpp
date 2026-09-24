#include "../src/MaskingModel.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main(){
    using namespace MixDoctorator::Analysis;

    std::array<double,kBandCount> band{};
    band[4]=1.0;

    const double gaps[]{0.0,3.0,6.0,12.0,18.0,24.0};
    double previous=2.0;

    for(double gap:gaps){
        const auto m=evaluatePair(
            -18.0,1.0,band.data(),
            -18.0-gap,1.0,band.data());

        assert(std::isfinite(m.masking));
        assert(m.masking>=0.0 && m.masking<=1.0);
        assert(m.masking<previous);
        previous=m.masking;
    }

    const auto full=evaluatePair(
        -18.0,1.0,band.data(),
        -18.0,1.0,band.data());

    const auto half=evaluatePair(
        -18.0,0.5,band.data(),
        -18.0,0.5,band.data());

    const auto low=evaluatePair(
        -18.0,0.1,band.data(),
        -18.0,0.1,band.data());

    assert(full.masking>half.masking);
    assert(half.masking>low.masking);

    // Near-silent equal spectra must not cross the Coach finding threshold.
    const auto nearSilent=evaluatePair(
        -54.0,0.10,band.data(),
        -54.0,0.10,band.data());

    assert(nearSilent.masking<0.14);

    const auto ab=evaluatePair(
        -12.0,0.8,band.data(),
        -18.0,0.7,band.data());

    const auto ba=evaluatePair(
        -18.0,0.7,band.data(),
        -12.0,0.8,band.data());

    assert(std::abs(ab.overlap-ba.overlap)<1.0e-12);
    assert(std::abs(ab.masking-ba.masking)<1.0e-12);
    assert(std::abs(ab.dominance+ba.dominance)<1.0e-12);

    const double nan=
        std::numeric_limits<double>::quiet_NaN();
    const double inf=
        std::numeric_limits<double>::infinity();

    std::array<double,kBandCount> pathological{};
    pathological[4]=nan;
    pathological[5]=inf;
    pathological[6]=-1.0;
    pathological[7]=2.0;

    const auto safe=evaluatePair(
        nan,nan,pathological.data(),
        inf,inf,pathological.data());

    assert(std::isfinite(safe.overlap));
    assert(std::isfinite(safe.masking));
    assert(std::isfinite(safe.dominance));
    assert(safe.overlap>=0.0 && safe.overlap<=1.0);
    assert(safe.masking>=0.0 && safe.masking<=1.0);
    assert(safe.dominance>=-1.0 && safe.dominance<=1.0);

    std::cout << "Masking sweep tests passed\n";
    return 0;
}

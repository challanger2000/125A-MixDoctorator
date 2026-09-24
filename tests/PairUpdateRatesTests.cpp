#include "../src/PairUpdateRates.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main(){
    using MixDoctorator::Analysis::makePairUpdateRates;

    const auto r=
        makePairUpdateRates(
            256,
            48000.0);

    const double dt=256.0/48000.0;

    assert(std::abs(r.dt-dt)<1.0e-12);
    assert(std::abs(
        r.dominanceAlpha-
        (1.0-std::exp(-dt/1.8)))<
        1.0e-12);
    assert(std::abs(
        r.overlapUpAlpha-
        (1.0-std::exp(-dt/0.60)))<
        1.0e-12);
    assert(std::abs(
        r.overlapDownAlpha-
        (1.0-std::exp(-dt/2.5)))<
        1.0e-12);
    assert(std::abs(
        r.maskingUpAlpha-
        (1.0-std::exp(-dt/0.85)))<
        1.0e-12);
    assert(std::abs(
        r.maskingDownAlpha-
        (1.0-std::exp(-dt/2.4)))<
        1.0e-12);
    assert(std::abs(
        r.transientUpAlpha-
        (1.0-std::exp(-dt/0.12)))<
        1.0e-12);
    assert(std::abs(
        r.transientDownAlpha-
        (1.0-std::exp(-dt/0.90)))<
        1.0e-12);
    assert(std::abs(
        r.bandAlpha-
        (1.0-std::exp(-dt/1.5)))<
        1.0e-12);
    assert(std::abs(
        r.confidenceAlpha-
        (1.0-std::exp(-dt/2.5)))<
        1.0e-12);

    const auto small=
        makePairUpdateRates(
            32,
            48000.0);

    const auto large=
        makePairUpdateRates(
            1024,
            48000.0);

    assert(large.dt>small.dt);
    assert(large.overlapUpAlpha>
           small.overlapUpAlpha);
    assert(large.maskingUpAlpha>
           small.maskingUpAlpha);
    assert(large.transientUpAlpha>
           small.transientUpAlpha);

    const double nan=
        std::numeric_limits<double>::quiet_NaN();

    const auto safe=
        makePairUpdateRates(
            -10,
            nan);

    assert(std::isfinite(safe.dt));
    assert(safe.dt>=0.0001);
    assert(safe.dt<=0.25);
    assert(std::isfinite(safe.confidenceAlpha));

    const auto clamped=
        makePairUpdateRates(
            1000000,
            8001.0);

    assert(std::abs(clamped.dt-0.25)<1.0e-12);

    std::cout
        << "Pair update rate tests passed\n";

    return 0;
}

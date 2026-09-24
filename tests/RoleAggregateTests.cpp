#include "../src/RoleAggregate.h"

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

using MixDoctorator::Analysis::RoleAggregate;

int main(){
    std::array<double,9> a{};
    std::array<double,9> b{};

    a[2]=1.0;
    b[6]=1.0;

    RoleAggregate aggregate;
    aggregate.reset();

    aggregate.add(
        -18.0,
        -6.0,
        0.8,
        0.2,
        a.data());

    aggregate.add(
        -18.0,
        -8.0,
        0.4,
        0.7,
        b.data());

    const auto result=
        aggregate.result();

    assert(result.valid);
    assert(result.count==2);

    // Two equal-power sources add about +3.0103 dB.
    assert(
        std::abs(
            result.rmsDb-
            (-14.9897))<
        0.02);

    assert(
        std::abs(
            result.activity-
            0.6)<
        0.01);

    assert(
        std::abs(
            result.transient-
            0.7)<
        0.001);

    assert(
        std::abs(
            result.bands[2]-
            0.5)<
        0.01);

    assert(
        std::abs(
            result.bands[6]-
            0.5)<
        0.01);

    assert(
        std::abs(
            result.peakDb-
            (-6.0))<
        0.001);

    std::cout
        << "RoleAggregate tests passed\n";

    return 0;
}

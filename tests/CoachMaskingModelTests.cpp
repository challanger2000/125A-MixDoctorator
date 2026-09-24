#include "../src/CoachMaskingModel.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main(){
    using namespace MixDoctorator::Analysis;

    auto oneBand=[](int index){
        std::array<double,kBandCount> v{};
        v[index]=1.0;
        return v;
    };

    // Identical same-band content remains direct/full strength.
    {
        const auto a=oneBand(4);
        const auto b=oneBand(4);

        const auto m=evaluateCoachMasking(
            -18.0,1.0,a.data(),
            -18.0,1.0,b.data());

        assert(m.masking>0.95);
        assert(m.masking<=1.0);
        assert(m.overlap>0.95);
    }

    // Equal-level immediately adjacent bands are intentionally just strong
    // enough to become a possible stable Coach finding.
    {
        const auto a=oneBand(4);
        const auto b=oneBand(5);

        const auto m=evaluateCoachMasking(
            -18.0,1.0,a.data(),
            -18.0,1.0,b.data());

        assert(m.masking>0.14);
        assert(m.masking<0.20);
        assert(m.overlap>0.14);
        assert(m.overlap<0.20);
    }

    // A modest 3 dB level gap suppresses the pure neighbour case below the
    // current 0.14 finding threshold.
    {
        const auto a=oneBand(4);
        const auto b=oneBand(5);

        const auto m=evaluateCoachMasking(
            -18.0,1.0,a.data(),
            -21.0,1.0,b.data());

        assert(m.masking<0.14);
    }

    // Two-band separation must remain clear.
    {
        const auto a=oneBand(3);
        const auto b=oneBand(5);

        const auto m=evaluateCoachMasking(
            -18.0,1.0,a.data(),
            -18.0,1.0,b.data());

        assert(m.masking<1.0e-12);
        assert(m.overlap<1.0e-12);
    }

    // Existing direct overlap leaves less room for neighbour contribution.
    {
        std::array<double,kBandCount> a{};
        std::array<double,kBandCount> b{};

        a[4]=0.60;
        a[5]=0.40;

        b[4]=0.60;
        b[6]=0.40;

        const auto direct=evaluatePair(
            -18.0,1.0,a.data(),
            -18.0,1.0,b.data());

        const auto coach=evaluateCoachMasking(
            -18.0,1.0,a.data(),
            -18.0,1.0,b.data());

        assert(coach.masking>=direct.masking);
        assert(coach.masking-direct.masking<0.10);
        assert(coach.masking<=1.0);
    }

    // Source-order symmetry is mandatory.
    {
        const auto a=oneBand(2);
        const auto b=oneBand(3);

        const auto ab=evaluateCoachMasking(
            -15.0,0.8,a.data(),
            -17.0,0.7,b.data());

        const auto ba=evaluateCoachMasking(
            -17.0,0.7,b.data(),
            -15.0,0.8,a.data());

        assert(std::abs(ab.masking-ba.masking)<1.0e-12);
        assert(std::abs(ab.overlap-ba.overlap)<1.0e-12);
    }

    // Non-finite values must never escape the model.
    {
        const double nan=std::numeric_limits<double>::quiet_NaN();
        const double inf=std::numeric_limits<double>::infinity();

        std::array<double,kBandCount> a{};
        std::array<double,kBandCount> b{};
        a[4]=nan;
        b[5]=inf;

        const auto m=evaluateCoachMasking(
            nan,nan,a.data(),
            inf,inf,b.data());

        assert(std::isfinite(m.masking));
        assert(std::isfinite(m.overlap));
        assert(std::isfinite(m.dominance));
        assert(m.masking>=0.0 && m.masking<=1.0);
        assert(m.overlap>=0.0 && m.overlap<=1.0);
    }

    std::cout
        << "Coach masking model tests passed\n";

    return 0;
}

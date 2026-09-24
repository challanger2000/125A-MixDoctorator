#include "../src/CoachMaskingModel.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
    using namespace MixDoctorator::Analysis;

    std::array<double,kBandCount> narrow{};
    narrow[4]=1.0;

    std::array<double,kBandCount> broad{};
    for(double& v:broad)
        v=1.0/static_cast<double>(kBandCount);

    std::array<double,kBandCount> tilted{};
    double total=0.0;
    for(int i=0;i<kBandCount;++i){
        tilted[i]=static_cast<double>(i+1);
        total+=tilted[i];
    }
    for(double& v:tilted)
        v/=total;

    const std::array<const double*,3> shapes{
        narrow.data(),
        broad.data(),
        tilted.data()
    };

    const double activities[]{0.0,0.02,0.05,0.10,0.25,0.50,0.80,1.0};
    const double gaps[]{0.0,3.0,6.0,12.0,18.0,24.0};

    double maxNearSilent=0.0;
    double maxLargeGap=0.0;

    for(const double* shape:shapes){
        double previousActivity=-1.0;

        for(double activity:activities){
            const auto m=evaluateCoachMasking(
                -18.0,activity,shape,
                -18.0,activity,shape);

            assert(std::isfinite(m.masking));
            assert(m.masking>=0.0 && m.masking<=1.0);

            if(previousActivity>=0.0)
                assert(m.masking>=previousActivity-1.0e-12);

            previousActivity=m.masking;

            if(activity<=0.10)
                maxNearSilent=std::max(
                    maxNearSilent,
                    m.masking);
        }

        double previousGap=2.0;

        for(double gap:gaps){
            const auto m=evaluateCoachMasking(
                -18.0,1.0,shape,
                -18.0-gap,1.0,shape);

            assert(std::isfinite(m.masking));
            assert(m.masking<=previousGap+1.0e-12);
            previousGap=m.masking;

            if(gap>=18.0)
                maxLargeGap=std::max(
                    maxLargeGap,
                    m.masking);
        }
    }

    // The Coach's current stable-finding threshold is 0.14. Very quiet
    // material must not cross it solely because normalized spectra match.
    assert(maxNearSilent<0.14);

    // Very large level gaps should be comfortably below the finding threshold
    // even for perfectly matching normalized spectra.
    assert(maxLargeGap<0.10);

    std::cout
        << "False-positive surface: max near-silent="
        << maxNearSilent
        << " max >=18 dB gap="
        << maxLargeGap
        << "\n";

    return 0;
}

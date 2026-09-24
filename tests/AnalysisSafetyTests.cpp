#include "../src/SpectralAnalyzer.h"
#include "../src/TransientModel.h"

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

    TransientDetector detector;
    detector.prepare(48000.0);

    for(double v:bad)
        detector.pushPower(v);

    assert(std::isfinite(detector.value()));
    assert(detector.value()>=0.0);
    assert(detector.value()<=1.0);

    std::cout << "AnalysisSafety tests passed\n";
    return 0;
}

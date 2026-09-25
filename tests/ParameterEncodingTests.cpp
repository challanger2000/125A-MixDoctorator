#include "../src/ParameterEncoding.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

using namespace MixDoctorator::Analysis;

int main(){
    for(int maximum:
        {1,3,6,9,15,91}){

        for(int code=0;
            code<=maximum;
            ++code){

            const double normalized=
                encodeDiscreteCode(
                    code,
                    maximum);

            assert(normalized>=0.0);
            assert(normalized<=1.0);

            assert(
                decodeDiscreteCode(
                    normalized,
                    maximum)==code);
        }
    }

    assert(encodeDiscreteCode(-5,6)==0.0);
    assert(encodeDiscreteCode(99,6)==1.0);
    assert(decodeDiscreteCode(-1.0,6)==0);
    assert(decodeDiscreteCode(2.0,6)==6);
    assert(
        decodeDiscreteCode(
            std::numeric_limits<double>::
                quiet_NaN(),
            6)==0);

    assert(encodeDiscreteCode(5,0)==0.0);
    assert(decodeDiscreteCode(0.75,0)==0);

    std::cout
        << "Parameter encoding tests passed\n";

    return 0;
}

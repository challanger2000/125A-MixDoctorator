#include "../src/PrimaryFinding.h"
#include <cassert>
#include <iostream>

int main(){
    using MixDoctorator::Analysis::selectPrimaryFinding;

    const auto empty=selectPrimaryFinding(-1,-1);
    assert(empty.pair==-1 && !empty.current);

    const auto current=selectPrimaryFinding(0,1);
    assert(current.pair==0 && current.current);

    const auto fallback=selectPrimaryFinding(-1,1);
    assert(fallback.pair==1 && !fallback.current);

    const auto invalid=selectPrimaryFinding(3,3);
    assert(invalid.pair==-1 && !invalid.current);

    const auto invalidCurrent=selectPrimaryFinding(3,2);
    assert(invalidCurrent.pair==2 && !invalidCurrent.current);

    std::cout << "PrimaryFinding tests passed\n";
    return 0;
}

#include "../src/CoachModel.h"
#include <cassert>
#include <iostream>

int main(){
    using MixDoctorator::Analysis::coachAdviceCode;

    assert(coachAdviceCode(-1,0,0.0)==0);
    assert(coachAdviceCode(3,0,0.0)==0);

    assert(coachAdviceCode(0,0,0.3)==1);
    assert(coachAdviceCode(0,1,-0.3)==2);
    assert(coachAdviceCode(0,1,0.0)==3);
    assert(coachAdviceCode(0,3,0.0)==4);
    assert(coachAdviceCode(0,8,0.0)==5);

    assert(coachAdviceCode(1,1,0.3)==6);
    assert(coachAdviceCode(1,2,-0.3)==7);
    assert(coachAdviceCode(1,2,0.0)==8);
    assert(coachAdviceCode(1,5,0.0)==9);
    assert(coachAdviceCode(1,8,0.0)==10);

    assert(coachAdviceCode(2,0,0.0)==11);
    assert(coachAdviceCode(2,4,0.3)==12);
    assert(coachAdviceCode(2,6,-0.3)==13);
    assert(coachAdviceCode(2,6,0.0)==14);
    assert(coachAdviceCode(2,8,0.0)==15);

    std::cout << "Coach model tests passed\n";
    return 0;
}

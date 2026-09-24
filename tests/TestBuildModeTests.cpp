#include <iostream>

#ifdef NDEBUG
#error "NDEBUG must be undefined for MixDoctorator regression tests"
#endif

int main(){
    std::cout
        << "Release regression assertions are active\n";
    return 0;
}

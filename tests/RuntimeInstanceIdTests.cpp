#include "../src/RuntimeInstanceId.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <unordered_set>

int main(){
    using MixDoctorator::Analysis::makeRuntimeInstanceId;

    std::unordered_set<std::uint64_t> ids;
    ids.reserve(4096);

    for(int i=0;i<4096;++i){
        const auto id=
            makeRuntimeInstanceId();

        assert(id!=0);
        assert(ids.insert(id).second);
    }

    std::cout
        << "Runtime instance ID tests passed: "
        << ids.size()
        << " unique IDs\n";

    return 0;
}

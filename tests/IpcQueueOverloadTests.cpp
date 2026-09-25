#include "../src/SpscQueue.h"
#include <cassert>
#include <cstdint>
#include <iostream>

struct Request {
    std::uint64_t sequence{0};
};

int main(){
    using Queue=
        MixDoctorator::Realtime::
        SpscQueue<Request,64>;

    Queue q;

    for(std::uint64_t i=1;i<=64;++i)
        assert(q.push({i}));

    // The realtime producer never blocks when the worker is stalled.
    assert(!q.push({65}));

    Request r;
    std::uint64_t newest=0;
    int drained=0;

    while(q.pop(r)){
        newest=r.sequence;
        ++drained;
    }

    assert(drained==64);
    assert(newest==64);

    // As soon as the worker has drained the backlog, the next realtime frame
    // becomes available immediately; there is no persistent lockout.
    assert(q.push({66}));
    assert(q.pop(r));
    assert(r.sequence==66);

    std::cout
        << "IPC queue overload QA: capacity="
        << drained
        << " newest backlog="
        << newest
        << " next accepted="
        << r.sequence
        << "\n";

    return 0;
}

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
    assert(q.drainNewest(r));
    assert(r.sequence==64);

    // As soon as the worker has drained the backlog, the next realtime frame
    // becomes available immediately; there is no persistent lockout.
    assert(q.push({66}));
    assert(q.drainNewest(r));
    assert(r.sequence==66);

    std::cout
        << "IPC queue overload QA: capacity=64"
        << " newest backlog=64"
        << " next accepted="
        << r.sequence
        << "\n";

    return 0;
}

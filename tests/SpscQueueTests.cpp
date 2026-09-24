#include "../src/SpscQueue.h"

#include <atomic>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <thread>

struct Packet {
    std::uint64_t sequence{0};
    double value{0.0};
};

int main(){
    using Queue=
        MixDoctorator::Realtime::
        SpscQueue<Packet,8>;

    Queue queue;

    for(std::uint64_t i=0;i<8;++i)
        assert(queue.push({i,static_cast<double>(i)}));

    assert(!queue.push({99,99.0}));

    for(std::uint64_t i=0;i<8;++i){
        Packet p;
        assert(queue.pop(p));
        assert(p.sequence==i);
        assert(p.value==static_cast<double>(i));
    }

    Packet empty;
    assert(!queue.pop(empty));

    Queue concurrent;
    constexpr std::uint64_t kCount=100000;
    std::atomic<bool> done{false};

    std::thread producer([&]{
        for(std::uint64_t i=0;i<kCount;){
            if(concurrent.push(
                   {i,static_cast<double>(i)}))
                ++i;
        }

        done.store(true,std::memory_order_release);
    });

    std::uint64_t expected=0;

    while(!done.load(std::memory_order_acquire) ||
          expected<kCount){

        Packet p;

        if(!concurrent.pop(p))
            continue;

        assert(p.sequence==expected);
        assert(p.value==
               static_cast<double>(expected));
        ++expected;
    }

    producer.join();
    assert(expected==kCount);

    std::cout << "SPSC queue tests passed\n";
    return 0;
}

#include "../src/MixDoctoratorIPC.h"

#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <thread>

int main(){
#ifdef _WIN32
    using namespace MixDoctorator::IPC;

    SharedMemory writerMemory;
    SharedMemory readerMemory;
    assert(writerMemory.open());
    assert(readerMemory.open());

    constexpr int session=6;
    constexpr std::uint64_t id=
        0x125A0000CC771234ull;

    int cachedSlot=-1;
    double initialBands[kBandCount]{};
    initialBands[0]=1.0;

    assert(
        writerMemory.publish(
            session,
            id,
            cachedSlot,
            Role::Drums,
            1,
            -10.001,
            -1.001,
            0.01,
            0.01,
            initialBands));

    assert(cachedSlot>=0);

    std::atomic<bool> finished{false};
    std::atomic<int> coherentReads{0};

    std::thread writer([&]{
        for(std::int64_t n=2;n<=20000;++n){
            double bands[kBandCount]{};
            bands[0]=static_cast<double>(n);
            bands[1]=static_cast<double>(n)+0.25;

            const bool ok=
                writerMemory.publish(
                    session,
                    id,
                    cachedSlot,
                    Role::Drums,
                    n,
                    -10.0-
                        static_cast<double>(n)*0.001,
                    -1.0-
                        static_cast<double>(n)*0.001,
                    static_cast<double>(n%100)/100.0,
                    static_cast<double>(n%50)/50.0,
                    bands);

            assert(ok);
        }

        finished.store(
            true,
            std::memory_order_release);
    });

    while(!finished.load(
              std::memory_order_acquire)){

        Snapshot s;
        const auto now=
            static_cast<std::uint64_t>(
                GetTickCount64());

        if(!readerMemory.readSlot(
               session,
               cachedSlot,
               s,
               now))
            continue;

        if(s.instanceId!=id)
            continue;

        const auto n=
            s.samplePosition;

        if(n<1)
            continue;

        const double expectedRms=
            -10.0-
            static_cast<double>(n)*0.001;

        const double expectedPeak=
            -1.0-
            static_cast<double>(n)*0.001;

        assert(s.role==Role::Drums);
        assert(std::abs(
            s.rmsDb-expectedRms)<1.0e-12);
        assert(std::abs(
            s.peakDb-expectedPeak)<1.0e-12);
        assert(std::abs(
            s.bands[0]-
            static_cast<double>(n))<1.0e-12);

        if(n>=2)
            assert(std::abs(
                s.bands[1]-
                (static_cast<double>(n)+0.25))<
                1.0e-12);

        ++coherentReads;
    }

    writer.join();

    assert(coherentReads.load()>0);

    // Same instance ID published in another session must not leak here.
    int otherSlot=-1;
    double otherBands[kBandCount]{};
    otherBands[2]=1.0;

    assert(
        writerMemory.publish(
            5,
            id+1,
            otherSlot,
            Role::Bass,
            100,
            -20.0,
            -3.0,
            0.5,
            0.2,
            otherBands));

    bool leaked=false;
    const auto now=
        static_cast<std::uint64_t>(
            GetTickCount64());

    for(int i=0;i<kSensorSlotCount;++i){
        Snapshot s;
        if(readerMemory.readSlot(
               session,
               i,
               s,
               now) &&
           s.instanceId==id+1)
            leaked=true;
    }

    assert(!leaked);

    assert(
        dbFromAmplitude(
            std::numeric_limits<double>::
                quiet_NaN())==-180.0);

    std::cout
        << "IPC concurrency tests passed\n";
#else
    std::cout
        << "IPC concurrency tests skipped on non-Windows\n";
#endif

    return 0;
}

#include "../src/PairMeasurement.h"
#include "../src/RoleModel.h"

#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>

int main(){
    using namespace MixDoctorator;
    using namespace MixDoctorator::Analysis;

    std::array<IPC::Snapshot,IPC::kRoleCount> roles{};

    for(int i=0;i<IPC::kRoleCount;++i){
        auto& s=roles[static_cast<std::size_t>(i)];
        s.role=roleFromIndex(i);
        s.connected=true;
        s.samplePosition=48000;
        s.rmsDb=-12.0-static_cast<double>(i%5);
        s.peakDb=s.rmsDb+6.0;
        s.activity=0.65+0.02*static_cast<double>(i%4);
        s.transient=0.15+0.05*static_cast<double>(i%6);

        double total=0.0;
        for(int b=0;b<IPC::kBandCount;++b){
            const double value=
                0.2+
                static_cast<double>(
                    ((i+1)*(b+3))%11);
            s.bands[b]=value;
            total+=value;
        }

        for(int b=0;b<IPC::kBandCount;++b)
            s.bands[b]/=total;
    }

    constexpr int frames=300;
    volatile double sink=0.0;

    const auto start=
        std::chrono::steady_clock::now();

    for(int frame=0;frame<frames;++frame){
        for(int a=0;a<IPC::kRoleCount-1;++a){
            for(int b=a+1;b<IPC::kRoleCount;++b){
                const auto first=roleFromIndex(a);
                const auto second=roleFromIndex(b);

                if(!rolesComparableForCoach(
                       first,
                       second))
                    continue;

                const auto m=measurePair(
                    roles[static_cast<std::size_t>(a)],
                    roles[static_cast<std::size_t>(b)],
                    48000,
                    128,
                    true);

                sink+=
                    m.masking+
                    m.overlap+
                    m.transientCompetition;
            }
        }
    }

    const auto end=
        std::chrono::steady_clock::now();

    const double elapsedMs=
        std::chrono::duration<double,std::milli>(
            end-start).count();

    const double perFrameMs=
        elapsedMs/
        static_cast<double>(frames);

    assert(std::isfinite(
        static_cast<double>(sink)));

    // This is deliberately generous and only guards catastrophic regressions.
    // The worker may drop stale requests, so it does not need audio-block-rate
    // throughput. Staying below 20 ms for a complete 14-role analysis frame
    // preserves at least a useful 50 Hz analysis cadence on the CI runner.
    assert(perFrameMs<20.0);

    std::cout
        << "Coach performance smoke test: "
        << perFrameMs
        << " ms per 14-role frame, "
        << kRolePairCount
        << " possible pairs\n";

    return 0;
}

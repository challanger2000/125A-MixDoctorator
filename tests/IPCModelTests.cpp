#include "../src/MixDoctoratorIPC.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

int main(){
#ifdef _WIN32
    using namespace MixDoctorator::IPC;

    SharedMemory first;
    SharedMemory second;

    assert(first.open());
    assert(second.open());

    std::array<double,kBandCount> bands{};
    bands[3]=1.0;

    int slotA=-1;
    int slotB=-1;

    constexpr std::uint64_t idA=
        0x125A0000000000A1ull;

    constexpr std::uint64_t idB=
        0x125A0000000000B2ull;

    assert(
        first.publish(
            7,
            idA,
            slotA,
            Role::ElectricGuitar,
            100000,
            -18.0,
            -6.0,
            0.8,
            0.3,
            bands.data()));

    assert(
        second.publish(
            7,
            idB,
            slotB,
            Role::ElectricGuitar,
            100000,
            -20.0,
            -8.0,
            0.7,
            0.5,
            bands.data()));

    assert(slotA>=0);
    assert(slotB>=0);
    assert(slotA!=slotB);

    const int originalSlotA=
        slotA;

    assert(
        first.publish(
            7,
            idA,
            slotA,
            Role::ElectricGuitar,
            100256,
            -17.0,
            -5.0,
            0.9,
            0.4,
            bands.data()));

    assert(
        slotA==
        originalSlotA);

    bool foundA=false;
    bool foundB=false;
    int guitarCount=0;

    for(int i=0;
        i<kSensorSlotCount;
        ++i){

        Snapshot snapshot;

        if(!first.readSlot(
               7,
               i,
               snapshot))
            continue;

        if(!snapshot.connected ||
           snapshot.role!=
               Role::ElectricGuitar)
            continue;

        ++guitarCount;

        if(snapshot.instanceId==idA)
            foundA=true;

        if(snapshot.instanceId==idB)
            foundB=true;
    }

    assert(foundA);
    assert(foundB);
    assert(guitarCount>=2);

    std::cout
        << "IPCModel tests passed\n";
#else
    std::cout
        << "IPCModel tests skipped on non-Windows\n";
#endif

    return 0;
}

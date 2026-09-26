#include "../src/SensorHeartbeatPolicy.h"

#include <cassert>
#include <cstdint>
#include <iostream>

int main(){
    using namespace MixDoctorator::Analysis;

    constexpr std::uint64_t now=10000;
    constexpr std::uint64_t id=0x125A;

    assert(sensorHeartbeatConnected(now,now,true,id));
    assert(sensorHeartbeatConnected(now,now-1499,true,id));
    assert(!sensorHeartbeatConnected(now,now-1500,true,id));
    assert(!sensorHeartbeatConnected(now,now-1499,false,id));
    assert(!sensorHeartbeatConnected(now,now-1499,true,0));
    assert(!sensorHeartbeatConnected(now,now+1,true,id));

    assert(!sensorHeartbeatReclaimable(now,now,id));
    assert(!sensorHeartbeatReclaimable(now,now-2000,id));
    assert(sensorHeartbeatReclaimable(now,now-2001,id));
    assert(sensorHeartbeatReclaimable(now,now,0));
    assert(!sensorHeartbeatReclaimable(now,now+1,id));

    // Intentional grace window: disconnected for diagnosis, but not yet
    // reclaimable by another writer.
    assert(!sensorHeartbeatConnected(now,now-1750,true,id));
    assert(!sensorHeartbeatReclaimable(now,now-1750,id));

    std::cout << "Sensor heartbeat policy tests passed\n";
    return 0;
}

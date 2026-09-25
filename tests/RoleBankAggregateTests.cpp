#include "../src/RoleBankAggregate.h"
#include "../src/PairMeasurement.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
    using namespace MixDoctorator;
    using namespace MixDoctorator::Analysis;

    constexpr std::int64_t now=100000;
    constexpr int block=256;

    auto snapshot=[](
        IPC::Role role,
        std::int64_t position,
        double rmsDb,
        int band,
        std::uint64_t heartbeat){

        IPC::Snapshot s;
        s.connected=true;
        s.role=role;
        s.samplePosition=position;
        s.rmsDb=rmsDb;
        s.peakDb=rmsDb+6.0;
        s.activity=0.8;
        s.transient=0.25;
        s.heartbeatMs=heartbeat;

        if(band>=0 &&
           band<IPC::kBandCount)
            s.bands[band]=1.0;

        return s;
    };

    RoleBankAggregate bank;
    bank.reset();

    auto kickA=snapshot(
        IPC::Role::Kick,
        now,
        -18.0,
        1,
        100);

    auto kickB=snapshot(
        IPC::Role::Kick,
        now+128,
        -18.0,
        1,
        120);

    auto bass=snapshot(
        IPC::Role::Bass,
        now,
        -12.0,
        1,
        130);

    bank.add(kickA,now,block);
    bank.add(kickB,now,block);
    bank.add(bass,now,block);

    IPC::Snapshot out;
    int count=0;

    const int kickIndex=
        roleToIndex(
            IPC::Role::Kick);

    assert(bank.result(
        kickIndex,
        now,
        out,
        count));

    assert(count==2);
    assert(out.connected);
    assert(out.role==IPC::Role::Kick);
    assert(out.aggregateCount==2);
    assert(out.heartbeatMs==120);
    assert(out.samplePosition==now);

    // Two equal -18 dB power sources sum to about -14.9897 dB.
    assert(std::abs(out.rmsDb-(-14.9897))<0.02);
    assert(out.bands[1]>0.999);

    const int bassIndex=
        roleToIndex(
            IPC::Role::Bass);

    assert(bank.result(
        bassIndex,
        now,
        out,
        count));

    assert(count==1);
    assert(std::abs(out.rmsDb-(-12.0))<0.001);

    // A connected but incoherent snapshot contributes to live count but not
    // to the coherent aggregate payload.
    auto staleKick=snapshot(
        IPC::Role::Kick,
        now+50000,
        -6.0,
        8,
        140);

    bank.add(staleKick,now,block);

    assert(bank.result(
        kickIndex,
        now,
        out,
        count));

    assert(count==3);
    assert(out.aggregateCount==2);
    assert(out.bands[1]>0.999);
    assert(out.bands[8]<0.001);

    // If a role has only incoherent connected sensors, it reports the live
    // count but has no valid coherent analysis snapshot.
    RoleBankAggregate incoherentOnly;
    incoherentOnly.reset();

    auto vocal=snapshot(
        IPC::Role::LeadVocal,
        now+50000,
        -10.0,
        6,
        200);

    incoherentOnly.add(
        vocal,
        now,
        block);

    const int vocalIndex=
        roleToIndex(
            IPC::Role::LeadVocal);

    assert(!incoherentOnly.result(
        vocalIndex,
        now,
        out,
        count));

    assert(count==1);
    assert(!out.connected);

    // Unknown Sensor timing must not be promoted to the Brain's known host
    // position through aggregation.
    RoleBankAggregate unknownTiming;
    unknownTiming.reset();

    auto unknownPos=snapshot(
        IPC::Role::Synth,
        -1,
        -12.0,
        6,
        210);

    unknownTiming.add(
        unknownPos,
        now,
        block);

    const int synthIndex=
        roleToIndex(
            IPC::Role::Synth);

    assert(!unknownTiming.result(
        synthIndex,
        now,
        out,
        count));

    assert(count==1);

    // If the Brain also has no host position, heartbeat freshness remains the
    // intentional fallback and the source may participate.
    RoleBankAggregate heartbeatFallback;
    heartbeatFallback.reset();
    heartbeatFallback.add(
        unknownPos,
        -1,
        block);

    assert(heartbeatFallback.result(
        synthIndex,
        -1,
        out,
        count));

    assert(count==1);
    assert(out.samplePosition==-1);

    // Disconnected snapshots never count.
    vocal.connected=false;
    RoleBankAggregate disconnected;
    disconnected.reset();
    disconnected.add(vocal,now,block);

    assert(!disconnected.result(
        vocalIndex,
        now,
        out,
        count));

    assert(count==0);

    // Opposite-edge contributors can each be close enough to the Brain while
    // still being too far apart for a valid cross-role comparison.
    RoleBankAggregate edgeTiming;
    edgeTiming.reset();

    auto edgeKick=snapshot(
        IPC::Role::Kick,
        now-4096,
        -12.0,
        1,
        250);

    auto edgeBass=snapshot(
        IPC::Role::Bass,
        now+4096,
        -12.0,
        1,
        251);

    edgeTiming.add(
        edgeKick,
        now,
        block);

    edgeTiming.add(
        edgeBass,
        now,
        block);

    IPC::Snapshot edgeKickOut;
    IPC::Snapshot edgeBassOut;
    int edgeKickCount=0;
    int edgeBassCount=0;

    assert(edgeTiming.result(
        roleToIndex(IPC::Role::Kick),
        now,
        edgeKickOut,
        edgeKickCount));

    assert(edgeTiming.result(
        roleToIndex(IPC::Role::Bass),
        now,
        edgeBassOut,
        edgeBassCount));

    assert(edgeKickOut.samplePositionMin==now-4096);
    assert(edgeKickOut.samplePositionMax==now-4096);
    assert(edgeBassOut.samplePositionMin==now+4096);
    assert(edgeBassOut.samplePositionMax==now+4096);

    const auto edgePair=
        measurePair(
            edgeKickOut,
            edgeBassOut,
            now,
            block,
            true);

    assert(!edgePair.active);
    assert(edgePair.masking==0.0);

    // Every declared role must route to its own aggregate slot.
    RoleBankAggregate allRoles;
    allRoles.reset();

    for(int i=0;i<IPC::kRoleCount;++i){
        auto s=snapshot(
            roleFromIndex(i),
            now,
            -20.0,
            i%IPC::kBandCount,
            static_cast<std::uint64_t>(300+i));

        allRoles.add(
            s,
            now,
            block);
    }

    for(int i=0;i<IPC::kRoleCount;++i){
        assert(allRoles.result(
            i,
            now,
            out,
            count));

        assert(count==1);
        assert(out.role==roleFromIndex(i));
        assert(out.aggregateCount==1);
        assert(out.bands[
            i%IPC::kBandCount]>0.999);
    }

    std::cout
        << "Role bank aggregate tests passed\n";

    return 0;
}

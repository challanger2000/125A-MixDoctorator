#pragma once
#include "MixDoctoratorIPC.h"
#include "RoleAggregate.h"
#include "RoleModel.h"
#include "TimingModel.h"

#include <algorithm>
#include <cstdint>

namespace MixDoctorator::Analysis {

class RoleBankAggregate {
public:
    void reset() noexcept {
        for(int i=0;i<IPC::kRoleCount;++i){
            aggregates_[i].reset();
            connectedCount_[i]=0;
            freshest_[i]=0;
        }
    }

    void add(
        const IPC::Snapshot& source,
        std::int64_t currentSamplePosition,
        int numSamples) noexcept {

        if(!source.connected)
            return;

        const int roleIndex=
            roleToIndex(
                source.role);

        if(roleIndex<0 ||
           roleIndex>=IPC::kRoleCount)
            return;

        ++connectedCount_[roleIndex];

        if(!samplePositionsCoherent(
               currentSamplePosition,
               source.samplePosition,
               source.samplePosition,
               numSamples))
            return;

        aggregates_[roleIndex].add(
            source.rmsDb,
            source.peakDb,
            source.activity,
            source.transient,
            source.bands);

        freshest_[roleIndex]=
            std::max(
                freshest_[roleIndex],
                source.heartbeatMs);
    }

    bool result(
        int roleIndex,
        std::int64_t currentSamplePosition,
        IPC::Snapshot& out,
        int& connectedCount) const noexcept {

        connectedCount=0;
        out=IPC::Snapshot{};

        if(roleIndex<0 ||
           roleIndex>=IPC::kRoleCount)
            return false;

        connectedCount=
            connectedCount_[roleIndex];

        const auto aggregate=
            aggregates_[roleIndex].result();

        if(!aggregate.valid)
            return false;

        out.role=
            roleFromIndex(roleIndex);
        out.connected=true;
        out.heartbeatMs=
            freshest_[roleIndex];
        out.samplePosition=
            currentSamplePosition;
        out.rmsDb=
            aggregate.rmsDb;
        out.peakDb=
            aggregate.peakDb;
        out.activity=
            aggregate.activity;
        out.transient=
            aggregate.transient;
        out.aggregateCount=
            connectedCount;

        for(int band=0;
            band<IPC::kBandCount;
            ++band)
            out.bands[band]=
                aggregate.bands[band];

        return true;
    }

private:
    RoleAggregate aggregates_[IPC::kRoleCount]{};
    int connectedCount_[IPC::kRoleCount]{};
    std::uint64_t freshest_[IPC::kRoleCount]{};
};

} // namespace MixDoctorator::Analysis

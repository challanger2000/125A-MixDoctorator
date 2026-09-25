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
            contributorCount_[i]=0;
            earliestPosition_[i]=-1;
            latestPosition_[i]=-1;
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

        // Do not let a source with unknown timing inherit the Brain's current
        // position through aggregation. If the Brain has a valid transport
        // position, each contributing Sensor must provide one as well.
        if(currentSamplePosition>=0 &&
           source.samplePosition<0)
            return;

        if(!samplePositionsCoherent(
               currentSamplePosition,
               source.samplePosition,
               source.samplePosition,
               numSamples))
            return;

        const bool accepted=
            aggregates_[roleIndex].add(
                source.rmsDb,
                source.peakDb,
                source.activity,
                source.transient,
                source.bands);

        if(!accepted)
            return;

        ++contributorCount_[roleIndex];

        if(source.samplePosition>=0){
            auto& earliest=
                earliestPosition_[roleIndex];
            auto& latest=
                latestPosition_[roleIndex];

            earliest=
                earliest<0
                ? source.samplePosition
                : std::min(
                    earliest,
                    source.samplePosition);

            latest=
                latest<0
                ? source.samplePosition
                : std::max(
                    latest,
                    source.samplePosition);
        }

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
        out.samplePositionMin=
            earliestPosition_[roleIndex];
        out.samplePositionMax=
            latestPosition_[roleIndex];
        out.rmsDb=
            aggregate.rmsDb;
        out.peakDb=
            aggregate.peakDb;
        out.activity=
            aggregate.activity;
        out.transient=
            aggregate.transient;
        out.aggregateCount=
            contributorCount_[roleIndex];

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
    int contributorCount_[IPC::kRoleCount]{};
    std::int64_t earliestPosition_[IPC::kRoleCount]{};
    std::int64_t latestPosition_[IPC::kRoleCount]{};
    std::uint64_t freshest_[IPC::kRoleCount]{};
};

} // namespace MixDoctorator::Analysis

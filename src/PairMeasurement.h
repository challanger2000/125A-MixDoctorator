#pragma once
#include "MixDoctoratorIPC.h"
#include "MaskingModel.h"
#include "CoachMaskingModel.h"
#include "TimingModel.h"
#include "TransientInteraction.h"
#include <array>
#include <cmath>

namespace MixDoctorator::Analysis {

struct PairMeasurement {
    bool active{false};
    double overlap{0.0};
    double masking{0.0};
    double transientCompetition{0.0};
    double dominance{0.0};
    std::array<double,IPC::kBandCount> bandRisk{};
};

inline PairMeasurement measurePair(
    const IPC::Snapshot& a,
    const IPC::Snapshot& b,
    std::int64_t currentSamplePosition,
    int numSamples,
    bool coachMasking) noexcept {

    PairMeasurement out;

    const bool timeCoherent=
        samplePositionsCoherent(
            currentSamplePosition,
            a.samplePosition,
            b.samplePosition,
            numSamples);

    out.active=
        a.connected &&
        b.connected &&
        timeCoherent &&
        std::isfinite(a.rmsDb) &&
        std::isfinite(b.rmsDb) &&
        a.rmsDb>-55.0 &&
        b.rmsDb>-55.0;

    if(!out.active)
        return out;

    const auto metrics=
        coachMasking
        ? evaluateCoachMasking(
            a.rmsDb,
            a.activity,
            a.bands,
            b.rmsDb,
            b.activity,
            b.bands)
        : evaluatePair(
            a.rmsDb,
            a.activity,
            a.bands,
            b.rmsDb,
            b.activity,
            b.bands);

    out.overlap=
        metrics.overlap;

    out.masking=
        metrics.masking;

    out.dominance=
        metrics.dominance;

    out.transientCompetition=
        transientCompetition(
            a.transient,
            b.transient,
            a.rmsDb,
            b.rmsDb,
            a.activity,
            b.activity);

    for(int i=0;i<IPC::kBandCount;++i)
        out.bandRisk[
            static_cast<std::size_t>(i)]=
            metrics.bandRisk[
                static_cast<std::size_t>(i)];

    return out;
}

} // namespace MixDoctorator::Analysis

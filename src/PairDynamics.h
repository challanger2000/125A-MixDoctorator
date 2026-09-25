#pragma once
#include "MixDoctoratorIPC.h"
#include "PairMeasurement.h"
#include "PairUpdateRates.h"
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace MixDoctorator::Analysis {

struct PairDynamicsState {
    double overlap{0.0};
    double masking{0.0};
    double bandRisk[IPC::kBandCount]{};
    double observedSeconds{0.0};
    double dominance{0.0};
    double confidence{0.0};
    double transientCompetition{0.0};
    int dominantBand{0};
};

inline void updatePairDynamics(
    const PairMeasurement& measurement,
    PairDynamicsState& state,
    const PairUpdateRates& rates) noexcept {

    const double dt=rates.dt;

    const double overlapTarget=
        measurement.active
        ? measurement.overlap
        : 0.0;

    const double maskingTarget=
        measurement.active
        ? measurement.masking
        : 0.0;

    const double transientCompetitionTarget=
        measurement.active
        ? measurement.transientCompetition
        : 0.0;

    if(measurement.active){
        state.observedSeconds=
            std::min(
                45.0,
                state.observedSeconds+dt);

        state.dominance+=
            rates.dominanceAlpha*
            (measurement.dominance-
             state.dominance);
    }else{
        state.observedSeconds=
            std::max(
                0.0,
                state.observedSeconds-dt*0.20);

        state.dominance+=
            rates.dominanceAlpha*
            (0.0-state.dominance);
    }

    const double overlapAlpha=
        overlapTarget>state.overlap
        ? rates.overlapUpAlpha
        : rates.overlapDownAlpha;

    const double maskingAlpha=
        maskingTarget>state.masking
        ? rates.maskingUpAlpha
        : rates.maskingDownAlpha;

    state.overlap+=
        overlapAlpha*
        (overlapTarget-state.overlap);

    state.masking+=
        maskingAlpha*
        (maskingTarget-state.masking);

    const double transientAlpha=
        transientCompetitionTarget>
        state.transientCompetition
        ? rates.transientUpAlpha
        : rates.transientDownAlpha;

    state.transientCompetition+=
        transientAlpha*
        (transientCompetitionTarget-
         state.transientCompetition);

    for(int i=0;i<IPC::kBandCount;++i){
        const double target=
            measurement.active
            ? measurement.bandRisk[
                static_cast<std::size_t>(i)]
            : 0.0;

        state.bandRisk[i]+=
            rates.bandAlpha*
            (target-state.bandRisk[i]);
    }

    int stableBest=0;

    for(int i=1;i<IPC::kBandCount;++i)
        if(state.bandRisk[i]>
           state.bandRisk[stableBest])
            stableBest=i;

    const bool sameBand=
        stableBest==state.dominantBand;

    state.dominantBand=stableBest;

    const double timeConfidence=
        std::clamp(
            state.observedSeconds/10.0,
            0.0,
            1.0);

    const double riskConfidence=
        std::clamp(
            (state.masking-0.08)/0.34,
            0.0,
            1.0);

    const double bandStability=
        sameBand ? 1.0 : 0.40;

    const double targetConfidence=
        timeConfidence*
        (
            0.60*riskConfidence+
            0.40*bandStability
        );

    state.confidence+=
        rates.confidenceAlpha*
        (targetConfidence-state.confidence);
}

} // namespace MixDoctorator::Analysis

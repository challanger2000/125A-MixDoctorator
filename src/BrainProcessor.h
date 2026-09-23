#pragma once
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "MixDoctoratorIPC.h"

namespace MixDoctorator::Brain {

class Processor final : public Steinberg::Vst::AudioEffect {
public:
    Processor();

    static Steinberg::FUnknown* createInstance(void*) {
        return static_cast<Steinberg::Vst::IAudioProcessor*>(new Processor());
    }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown*) override;
    Steinberg::tresult PLUGIN_API setBusArrangements(
        Steinberg::Vst::SpeakerArrangement*, Steinberg::int32,
        Steinberg::Vst::SpeakerArrangement*, Steinberg::int32) override;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32) override;
    Steinberg::tresult PLUGIN_API setupProcessing(
        Steinberg::Vst::ProcessSetup&) override;
    Steinberg::tresult PLUGIN_API setProcessing(Steinberg::TBool) override;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData&) override;

private:
    struct PairState {
        double score {0.0};
        double bands[IPC::kBandCount] {0.0,0.0,0.0,0.0,0.0};
        int dominantBand {0};
    };

    IPC::SharedMemory ipc_;
    double sampleRate_ {44100.0};
    PairState pairStates_[3] {};

    double last_[18] {
        -1,-1,-1,-1,-1,-1,
        -1,-1,-1,
        -1,-1,-1,
        -1,-1,-1,
        -1,-1,-1
    };

    void publishParam(
        Steinberg::Vst::ProcessData&,
        Steinberg::Vst::ParamID,
        double,
        int);

    void updatePair(
        const IPC::Snapshot&,
        const IPC::Snapshot&,
        PairState&,
        Steinberg::int32) noexcept;

    static double severityFromScore(double) noexcept;
};

} // namespace MixDoctorator::Brain

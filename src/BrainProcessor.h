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
        double observedSeconds {0.0};
        double dominance {0.0};
        double confidence {0.0};
        int dominantBand {0};
        int previousBand {0};
    };

    struct SessionFinding {
        int pair {-1};
        int band {0};
        double score {0.0};
        double confidence {0.0};
    };

    IPC::SharedMemory ipc_;
    double sampleRate_ {44100.0};
    PairState pairStates_[3] {};
    SessionFinding sessionFinding_ {};
    int heldTopPair_ {-1};

    double last_[24] {
        -1,-1,-1,-1,-1,-1,
        -1,-1,-1,
        -1,-1,-1,
        -1,-1,-1,
        -1,-1,-1,-1,-1,-1,
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

    void updateSessionFinding() noexcept;

    static double severityFromState(const PairState&) noexcept;
    static int adviceFor(int pairIndex,int bandIndex,double dominance) noexcept;
    static double dominanceParam(double dominance) noexcept;
    int chooseTopPair() noexcept;
};

} // namespace MixDoctorator::Brain

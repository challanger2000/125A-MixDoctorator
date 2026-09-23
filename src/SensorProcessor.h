#pragma once
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "MixDoctoratorIPC.h"

namespace MixDoctorator::Sensor {

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
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup&) override;
    Steinberg::tresult PLUGIN_API setProcessing(Steinberg::TBool) override;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData&) override;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream*) override;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream*) override;

private:
    void readParameters(Steinberg::Vst::IParameterChanges*);
    void prepareBands(double sampleRate) noexcept;
    void analyzeSample(double x, double* energy) noexcept;

    IPC::Role role_{IPC::Role::Drums};
    IPC::SharedMemory ipc_;

    double lpState_[4]{0.0,0.0,0.0,0.0};
    double lpCoeff_[4]{0.0,0.0,0.0,0.0};
    double smoothedBands_[IPC::kBandCount]{0.2,0.2,0.2,0.2,0.2};
};

} // namespace MixDoctorator::Sensor

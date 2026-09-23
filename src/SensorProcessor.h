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
    Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement*,Steinberg::int32,
        Steinberg::Vst::SpeakerArrangement*,Steinberg::int32) override;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32) override;
    Steinberg::tresult PLUGIN_API setProcessing(Steinberg::TBool) override;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData&) override;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream*) override;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream*) override;
private:
    void readParameters(Steinberg::Vst::IParameterChanges*);
    IPC::Role role_{IPC::Role::Drums};
    IPC::SharedMemory ipc_;
};
}

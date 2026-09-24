#pragma once
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "MixDoctoratorIPC.h"
#include "SpectralAnalyzer.h"
#include "TransientModel.h"

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

    void analyzeStereoSample(double left,double right) noexcept {
        analyzerLeft_.push(left);
        analyzerRight_.push(right);

        const double power=
            0.5*
            (left*left+
             right*right);

        transientDetector_.pushPower(
            power);
    }

private:
    void readParameters(Steinberg::Vst::IParameterChanges*);

    IPC::Role role_{IPC::Role::Drums};
    int session_{0};

    IPC::SharedMemory ipc_;
    Analysis::SpectralAnalyzer analyzerLeft_;
    Analysis::SpectralAnalyzer analyzerRight_;
    Analysis::TransientDetector transientDetector_;
    double sampleRate_{44100.0};
};

} // namespace MixDoctorator::Sensor

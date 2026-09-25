#pragma once
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "MixDoctoratorIPC.h"
#include "SpectralAnalyzer.h"
#include "TransientModel.h"
#include "SpscQueue.h"
#include <atomic>
#include <cstdint>
#include <thread>

namespace MixDoctorator::Sensor {

class Processor final : public Steinberg::Vst::AudioEffect {
public:
    Processor();
    ~Processor() override;

    static Steinberg::FUnknown* createInstance(void*) {
        return static_cast<Steinberg::Vst::IAudioProcessor*>(new Processor());
    }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown*) override;
    Steinberg::tresult PLUGIN_API terminate() override;
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
    struct AnalysisPacket {
        std::uint64_t generation{0};
        int session{0};
        IPC::Role role{IPC::Role::Unknown};
        std::int64_t samplePosition{-1};
        double rmsDb{-180.0};
        double peakDb{-180.0};
        double activity{0.0};
        double transient{0.0};
        double bands[IPC::kBandCount]{};
    };

    void readParameters(Steinberg::Vst::IParameterChanges*);
    void resetAnalysisMeters() noexcept;
    void startIpcWorker();
    void stopIpcWorker() noexcept;
    void ipcWorkerLoop() noexcept;

    IPC::Role role_{IPC::Role::Drums};
    int session_{0};
    std::uint64_t instanceId_{0};
    int cachedSlot_[IPC::kSessionCount]{
        -1,-1,-1,-1,-1,-1,-1,-1
    };
    int lastPublishedSession_{-1};
    std::uint64_t lastPublishedGeneration_{0};
    std::atomic<std::uint64_t> configGeneration_{1};

    IPC::SharedMemory ipc_;
    Realtime::SpscQueue<AnalysisPacket,64> ipcQueue_;
    std::atomic<bool> ipcWorkerRunning_{false};
    std::thread ipcWorker_;
    bool ipcReady_{false};

    Analysis::SpectralAnalyzer analyzerLeft_;
    Analysis::SpectralAnalyzer analyzerRight_;
    Analysis::TransientDetector transientDetector_;
    double sampleRate_{44100.0};
};

} // namespace MixDoctorator::Sensor

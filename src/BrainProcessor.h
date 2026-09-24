#pragma once
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "MixDoctoratorIPC.h"
#include "SpscQueue.h"
#include "RoleModel.h"
#include "PairUpdateRates.h"
#include <atomic>
#include <cstdint>
#include <thread>

namespace MixDoctorator::Brain {

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
    Steinberg::tresult PLUGIN_API setupProcessing(
        Steinberg::Vst::ProcessSetup&) override;
    Steinberg::tresult PLUGIN_API setProcessing(Steinberg::TBool) override;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData&) override;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream*) override;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream*) override;

private:
    struct PairState {
        double overlap{0.0};
        double masking{0.0};
        double bandRisk[IPC::kBandCount]{};
        double observedSeconds{0.0};
        double dominance{0.0};
        double confidence{0.0};
        double transientCompetition{0.0};
        int dominantBand{0};
    };

    struct SessionFinding {
        int pair{-1};
        int band{0};
        double score{0.0};
        double confidence{0.0};
        double dominance{0.0};
    };

    struct IpcRequest {
        int session{0};
        std::uint64_t generation{0};
        std::int64_t samplePosition{-1};
        Steinberg::int32 numSamples{0};
    };

    struct IpcResponse {
        int session{0};
        std::uint64_t generation{0};
        std::int64_t samplePosition{-1};
        IPC::Snapshot drums{};
        IPC::Snapshot bass{};
        IPC::Snapshot guitar{};
        IPC::Snapshot roles[IPC::kRoleCount]{};
        bool roleOk[IPC::kRoleCount]{};
        int roleCount[IPC::kRoleCount]{};
        int drumsCount{0};
        int bassCount{0};
        int guitarCount{0};
        bool drumsOk{false};
        bool bassOk{false};
        bool guitarOk{false};
    };

    IPC::SharedMemory ipc_;
    Realtime::SpscQueue<IpcRequest,64> ipcRequests_;
    Realtime::SpscQueue<IpcResponse,64> ipcResponses_;
    std::atomic<bool> ipcWorkerRunning_{false};
    std::thread ipcWorker_;
    IpcResponse latestIpc_{};
    bool haveLatestIpc_{false};
    bool ipcReady_{false};
    std::uint64_t ipcGeneration_{1};
    double sampleRate_{44100.0};
    PairState pairStates_[3]{};
    PairState coachPairStates_[Analysis::kRolePairCount]{};
    SessionFinding sessionFinding_{};
    int heldTopPair_{-1};
    int heldCoachPair_{-1};
    int session_{0};
    bool wasPlaying_{false};
    std::int64_t lastProjectSample_{-1};

    double last_[49]{
        -1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,
        -1,-1,-1,-1,
        -1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,
        -1,-1,-1,
        -1,-1,-1,
        -1,-1,-1,
        -1,-1,-1,
        -1,-1,-1,
        -1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1
    };

    void publishParam(
        Steinberg::Vst::ProcessData&,
        Steinberg::Vst::ParamID,
        double,
        int);

    void readAllRoleAggregates(
        int,
        std::int64_t,
        Steinberg::int32,
        std::uint64_t,
        IpcResponse&) noexcept;

    void startIpcWorker();
    void stopIpcWorker() noexcept;
    void ipcWorkerLoop() noexcept;

    void updatePair(
        const IPC::Snapshot&,
        const IPC::Snapshot&,
        PairState&,
        const Analysis::PairUpdateRates&,
        bool,
        Steinberg::int32,
        std::int64_t) noexcept;

    void readParameters(Steinberg::Vst::IParameterChanges*) noexcept;
    void updateSessionFinding() noexcept;
    void resetAnalysisState() noexcept;

    static double severityFromState(const PairState&) noexcept;
    static double dominanceParam(double dominance) noexcept;
    int chooseTopPair() noexcept;
    int chooseCoachPair() noexcept;
    int chooseCoachAttackPair() noexcept;
};

} // namespace MixDoctorator::Brain

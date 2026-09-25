#include "BrainProcessor.h"
#include "BrainIDs.h"
#include "MaskingModel.h"
#include "TimingModel.h"
#include "IpcResponsePolicy.h"
#include "RoleAggregate.h"
#include "RoleBankAggregate.h"
#include "TransientInteraction.h"
#include "AttackFinding.h"
#include "TransportModel.h"
#include "PrimaryFinding.h"
#include "AudioSafety.h"
#include "CoachModel.h"
#include "CoachMaskingModel.h"
#include "RecommendationEngine.h"
#include "FindingRanking.h"
#include "PairUpdateRates.h"
#include "RoleModel.h"
#include "ParameterEncoding.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/vstspeaker.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>
#include <type_traits>

namespace MixDoctorator::Brain {
static_assert(
    Analysis::kBandCount==IPC::kBandCount,
    "Brain masking band count must match IPC");

static_assert(
    Analysis::kAggregateBandCount==IPC::kBandCount,
    "Brain aggregate band count must match IPC");

using namespace Steinberg;
using namespace Steinberg::Vst;

Processor::Processor(){
    setControllerClass(kControllerUID);
}

Processor::~Processor(){
    stopIpcWorker();
}

void Processor::resetAnalysisState() noexcept{
    for(auto& p:pairStates_)
        p=PairState{};

    for(auto& p:coachPairStates_)
        p=PairState{};

    sessionFinding_=SessionFinding{};
    heldTopPair_=-1;
    heldCoachPair_=-1;

    ++ipcGeneration_;
    if(ipcGeneration_==0)
        ipcGeneration_=1;

    haveLatestIpc_=false;
    latestIpc_=IpcResponse{};

    for(double& value:last_)
        value=-1.0;
}

tresult PLUGIN_API Processor::initialize(FUnknown* c){
    const auto r=AudioEffect::initialize(c);

    if(r!=kResultOk)
        return r;

    addAudioInput(
        STR16("Stereo In"),
        SpeakerArr::kStereo);

    addAudioOutput(
        STR16("Stereo Out"),
        SpeakerArr::kStereo);

    ipcReady_=ipc_.open();

    if(ipcReady_)
        startIpcWorker();

    return kResultOk;
}

tresult PLUGIN_API Processor::terminate(){
    stopIpcWorker();
    ipc_.close();
    ipcReady_=false;
    return AudioEffect::terminate();
}

void Processor::startIpcWorker(){
    if(ipcWorkerRunning_.exchange(
           true,
           std::memory_order_acq_rel))
        return;

    try{
        ipcWorker_=
            std::thread(
                [this]{
                    ipcWorkerLoop();
                });
    }catch(...){
        ipcWorkerRunning_.store(
            false,
            std::memory_order_release);
        ipcReady_=false;
    }
}

void Processor::stopIpcWorker() noexcept{
    ipcWorkerRunning_.store(
        false,
        std::memory_order_release);

    if(ipcWorker_.joinable())
        ipcWorker_.join();
}

void Processor::ipcWorkerLoop() noexcept{
    while(ipcWorkerRunning_.load(
              std::memory_order_acquire)){

        IpcRequest newest;

        if(!ipcRequests_.drainNewest(newest)){
            std::this_thread::sleep_for(
                std::chrono::milliseconds(1));
            continue;
        }

        const std::uint64_t nowMs=
#ifdef _WIN32
            static_cast<std::uint64_t>(
                GetTickCount64());
#else
            0;
#endif

        IpcResponse response;
        response.session=
            IPC::clampSession(
                newest.session);
        response.generation=
            newest.generation;
        response.samplePosition=
            newest.samplePosition;

        readAllRoleAggregates(
            response.session,
            newest.samplePosition,
            newest.numSamples,
            nowMs,
            response);

        response.drums=
            response.roles[
                Analysis::roleToIndex(
                    IPC::Role::Drums)];
        response.bass=
            response.roles[
                Analysis::roleToIndex(
                    IPC::Role::Bass)];
        response.guitar=
            response.roles[
                Analysis::roleToIndex(
                    IPC::Role::ElectricGuitar)];

        response.drumsOk=
            response.roleOk[
                Analysis::roleToIndex(
                    IPC::Role::Drums)];
        response.bassOk=
            response.roleOk[
                Analysis::roleToIndex(
                    IPC::Role::Bass)];
        response.guitarOk=
            response.roleOk[
                Analysis::roleToIndex(
                    IPC::Role::ElectricGuitar)];

        response.drumsCount=
            response.roleCount[
                Analysis::roleToIndex(
                    IPC::Role::Drums)];
        response.bassCount=
            response.roleCount[
                Analysis::roleToIndex(
                    IPC::Role::Bass)];
        response.guitarCount=
            response.roleCount[
                Analysis::roleToIndex(
                    IPC::Role::ElectricGuitar)];

        // Heavy Coach pair measurement lives on the worker thread. The audio
        // thread only smooths these precomputed targets.
        for(int a=0;a<IPC::kRoleCount-1;++a){
            for(int b=a+1;b<IPC::kRoleCount;++b){
                const auto first=
                    Analysis::roleFromIndex(a);
                const auto second=
                    Analysis::roleFromIndex(b);

                const int pairIndex=
                    Analysis::encodeRolePair(
                        first,
                        second);

                if(pairIndex<0 ||
                   !Analysis::rolesComparableForCoach(
                       first,
                       second))
                    continue;

                response.coachMeasurements[pairIndex]=
                    Analysis::measurePair(
                        response.roles[a],
                        response.roles[b],
                        newest.samplePosition,
                        newest.numSamples,
                        true);
            }
        }

        // Worker-to-audio queue is also bounded. The audio thread drains to
        // the newest available response each block.
        ipcResponses_.push(response);
    }
}

tresult PLUGIN_API Processor::setBusArrangements(
    SpeakerArrangement* in,int32 ni,
    SpeakerArrangement* out,int32 no){

    if(ni==1 &&
       no==1 &&
       ((in[0]==SpeakerArr::kStereo &&
         out[0]==SpeakerArr::kStereo) ||
        (in[0]==SpeakerArr::kMono &&
         out[0]==SpeakerArr::kMono)))
        return AudioEffect::setBusArrangements(
            in,ni,out,no);

    return kResultFalse;
}

tresult PLUGIN_API Processor::canProcessSampleSize(int32 s){
    return (s==kSample32 || s==kSample64)
        ? kResultTrue
        : kResultFalse;
}

tresult PLUGIN_API Processor::setupProcessing(
    ProcessSetup& setup){

    sampleRate_=
        (std::isfinite(setup.sampleRate) &&
         setup.sampleRate>8000.0)
        ? setup.sampleRate
        : 44100.0;

    return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API Processor::setProcessing(TBool state){
    if(state){
        resetAnalysisState();
        wasPlaying_=false;
        lastProjectSample_=-1;
    }

    AudioEffect::setProcessing(state);
    return kResultTrue;
}

void Processor::readParameters(
    IParameterChanges* changes) noexcept{

    if(!changes)
        return;

    for(int32 i=0;i<changes->getParameterCount();++i){
        auto* q=changes->getParameterData(i);

        if(!q ||
           q->getPointCount()<=0 ||
           q->getParameterId()!=kSession)
            continue;

        int32 off=0;
        ParamValue v=0.0;

        if(q->getPoint(
               q->getPointCount()-1,
               off,
               v)!=kResultTrue)
            continue;

        const int newSession=
            std::clamp(
                static_cast<int>(
                    std::lround(v*7.0)),
                0,
                7);

        if(newSession!=session_){
            session_=newSession;
            resetAnalysisState();
        }
    }
}

void Processor::publishParam(
    ProcessData& data,
    Steinberg::Vst::ParamID id,
    double v,
    int slot){

    v=std::clamp(v,0.0,1.0);

    if(std::abs(v-last_[slot])<0.0005 ||
       !data.outputParameterChanges)
        return;

    int32 index=0;

    auto* q=
        data.outputParameterChanges->
        addParameterData(id,index);

    if(!q)
        return;

    int32 point=0;

    if(q->addPoint(0,v,point)==kResultTrue)
        last_[slot]=v;
}

void Processor::readAllRoleAggregates(
    int session,
    std::int64_t currentSamplePosition,
    int32 numSamples,
    std::uint64_t nowMs,
    IpcResponse& response) noexcept {

    Analysis::RoleBankAggregate bank;
    bank.reset();

    // Read shared memory once. RoleBankAggregate handles role routing,
    // connected counts and sample-position coherence.
    for(int slot=0;
        slot<IPC::kSensorSlotCount;
        ++slot){

        IPC::Snapshot source;

        if(!ipc_.readSlot(
               session,
               slot,
               source,
               nowMs))
            continue;

        bank.add(
            source,
            currentSamplePosition,
            numSamples);
    }

    for(int i=0;i<IPC::kRoleCount;++i){
        response.roleOk[i]=
            bank.result(
                i,
                currentSamplePosition,
                response.roles[i],
                response.roleCount[i]);
    }
}

void Processor::updatePair(
    const IPC::Snapshot& a,
    const IPC::Snapshot& b,
    PairState& state,
    const Analysis::PairUpdateRates& rates,
    bool coachMasking,
    int32 numSamples,
    std::int64_t currentSamplePosition) noexcept{

    const auto measurement=
        Analysis::measurePair(
            a,
            b,
            currentSamplePosition,
            numSamples,
            coachMasking);

    applyPairMeasurement(
        measurement,
        state,
        rates);
}

void Processor::applyPairMeasurement(
    const Analysis::PairMeasurement& measurement,
    PairState& state,
    const Analysis::PairUpdateRates& rates) noexcept{

    Analysis::updatePairDynamics(
        measurement,
        state,
        rates);
}

double Processor::severityFromState(
    const PairState& state) noexcept{

    if(state.observedSeconds<2.5)
        return 0.0;

    if(state.masking<0.14)
        return 0.25;

    if(state.masking<0.27)
        return 0.50;

    if(state.masking<0.42)
        return 0.75;

    return 1.0;
}

double Processor::dominanceParam(
    double d) noexcept{

    if(d>0.20)
        return 1.0;

    if(d<-0.20)
        return 0.0;

    return 0.5;
}

int Processor::chooseTopPair() noexcept{
    const int selected=
        Analysis::chooseStableFinding(
            3,
            heldTopPair_,
            [this](int i){
                const auto& state=pairStates_[i];
                return Analysis::FindingCandidate{
                    state.observedSeconds,
                    state.masking,
                    state.confidence
                };
            });

    heldTopPair_=selected;
    return selected;
}

int Processor::chooseCoachPair() noexcept{
    const int selected=
        Analysis::chooseStableFinding(
            Analysis::kRolePairCount,
            heldCoachPair_,
            [this](int i){
                IPC::Role first=IPC::Role::Unknown;
                IPC::Role second=IPC::Role::Unknown;

                if(!Analysis::decodeRolePair(
                       i,
                       first,
                       second) ||
                   !Analysis::rolesComparableForCoach(
                       first,
                       second))
                    return Analysis::FindingCandidate{};

                const auto& state=coachPairStates_[i];
                return Analysis::FindingCandidate{
                    state.observedSeconds,
                    state.masking,
                    state.confidence
                };
            });

    heldCoachPair_=selected;
    return selected;
}

int Processor::chooseCoachAttackPair() noexcept{
    double scores[Analysis::kRolePairCount]{};
    double observed[Analysis::kRolePairCount]{};

    for(int i=0;
        i<Analysis::kRolePairCount;
        ++i){

        IPC::Role first=IPC::Role::Unknown;
        IPC::Role second=IPC::Role::Unknown;

        const bool comparable=
            Analysis::decodeRolePair(
                i,
                first,
                second) &&
            Analysis::rolesComparableForCoach(
                first,
                second);

        if(!comparable){
            scores[i]=-1.0;
            observed[i]=0.0;
            continue;
        }

        scores[i]=
            coachPairStates_[i].
            transientCompetition;

        observed[i]=
            coachPairStates_[i].
            observedSeconds;
    }

    return Analysis::chooseAttackFindingCount(
        scores,
        observed,
        Analysis::kRolePairCount,
        0.30,
        2.0).pair;
}

void Processor::updateSessionFinding() noexcept{
    for(int i=0;i<3;++i){
        const auto& p=
            pairStates_[i];

        if(p.observedSeconds<5.0 ||
           p.confidence<0.30 ||
           p.masking<0.16)
            continue;

        const double rank=
            p.masking *
            (0.60+
             0.40*p.confidence);

        const double storedRank=
            sessionFinding_.score *
            (0.60+
             0.40*
             sessionFinding_.confidence);

        if(sessionFinding_.pair<0 ||
           rank>storedRank+0.012){

            sessionFinding_.pair=i;
            sessionFinding_.band=
                p.dominantBand;
            sessionFinding_.score=
                p.masking;
            sessionFinding_.confidence=
                p.confidence;
            sessionFinding_.dominance=
                p.dominance;
        }
    }
}

template<typename T>
static void pass(
    AudioBusBuffers& input,
    AudioBusBuffers& output,
    int32 n){

    const int32 ch=
        std::min(
            input.numChannels,
            output.numChannels);

    T** in=nullptr;
    T** out=nullptr;

    if constexpr(
        std::is_same_v<T,float>){
        in=input.channelBuffers32;
        out=output.channelBuffers32;
    }else{
        in=input.channelBuffers64;
        out=output.channelBuffers64;
    }

    for(int32 c=0;c<ch;++c){
        if(!out[c])
            continue;

        for(int32 i=0;i<n;++i){
            const double raw=
                in[c]
                ? static_cast<double>(
                    in[c][i])
                : 0.0;

            out[c][i]=
                static_cast<T>(
                    Analysis::
                    sanitizeAudioSample(
                        raw));
        }
    }
}

tresult PLUGIN_API Processor::process(
    ProcessData& data){

    readParameters(
        data.inputParameterChanges);

    if(data.numInputs>0 &&
       data.numOutputs>0 &&
       data.numSamples>0){

        if(data.symbolicSampleSize==kSample64)
            pass<double>(
                data.inputs[0],
                data.outputs[0],
                data.numSamples);
        else
            pass<float>(
                data.inputs[0],
                data.outputs[0],
                data.numSamples);
    }

    const bool hasProcessContext=
        data.processContext!=nullptr;

    const bool playing=
        !hasProcessContext ||
        ((data.processContext->state &
          ProcessContext::kPlaying)!=0);

    const bool cycleActive=
        hasProcessContext &&
        ((data.processContext->state &
          ProcessContext::kCycleActive)!=0);

    const std::int64_t currentSamplePosition=
        hasProcessContext
        ? static_cast<std::int64_t>(
            data.processContext->
            projectTimeSamples)
        : -1;

    const std::int64_t rewindTolerance=
        std::max<std::int64_t>(
            4096,
            static_cast<std::int64_t>(
                std::max<int32>(
                    1,
                    data.numSamples))*
            4);

    if(Analysis::shouldResetAnalysis(
           playing,
           wasPlaying_,
           cycleActive,
           currentSamplePosition,
           lastProjectSample_,
           rewindTolerance))
        resetAnalysisState();

    wasPlaying_=playing;

    if(currentSamplePosition>=0)
        lastProjectSample_=
            currentSamplePosition;

    if(ipcReady_){
        const IpcRequest request{
            session_,
            ipcGeneration_,
            currentSamplePosition,
            data.numSamples
        };

        ipcRequests_.push(request);
    }

    IpcResponse response;
    while(ipcResponses_.pop(response)){
        latestIpc_=response;
        haveLatestIpc_=true;
    }

    const bool responseForSession=
        haveLatestIpc_ &&
        Analysis::ipcResponseMatchesContext(
            latestIpc_.session,
            latestIpc_.generation,
            session_,
            ipcGeneration_);

    const bool responseFresh=
        haveLatestIpc_ &&
        Analysis::ipcResponseIsFresh(
            latestIpc_.session,
            latestIpc_.generation,
            latestIpc_.samplePosition,
            session_,
            ipcGeneration_,
            currentSamplePosition,
            data.numSamples);

    IPC::Snapshot drums,bass,guitar;
    int drumsCount=0;
    int bassCount=0;
    int guitarCount=0;
    bool drumsOk=false;
    bool bassOk=false;
    bool guitarOk=false;

    if(responseFresh){
        drumsCount=latestIpc_.drumsCount;
        bassCount=latestIpc_.bassCount;
        guitarCount=latestIpc_.guitarCount;
        drums=latestIpc_.drums;
        bass=latestIpc_.bass;
        guitar=latestIpc_.guitar;
        drumsOk=latestIpc_.drumsOk;
        bassOk=latestIpc_.bassOk;
        guitarOk=latestIpc_.guitarOk;
    }

    auto level=[](
        const IPC::Snapshot& s,
        bool ok){

        return ok
            ? std::clamp(
                (s.rmsDb+60.0)/60.0,
                0.0,
                1.0)
            : 0.0;
    };

    publishParam(
        data,
        kDrumsConnected,
        drumsCount>0?1.0:0.0,
        0);

    publishParam(
        data,
        kBassConnected,
        bassCount>0?1.0:0.0,
        2);

    publishParam(
        data,
        kGuitarConnected,
        guitarCount>0?1.0:0.0,
        4);

    publishParam(
        data,
        kDrumsCount,
        drumsCount>0
            ? std::clamp(
                static_cast<double>(drumsCount)/
                static_cast<double>(IPC::kSensorSlotCount),
                0.0,
                1.0)
            : 0.0,
        30);

    publishParam(
        data,
        kBassCount,
        bassCount>0
            ? std::clamp(
                static_cast<double>(bassCount)/
                static_cast<double>(IPC::kSensorSlotCount),
                0.0,
                1.0)
            : 0.0,
        31);

    publishParam(
        data,
        kGuitarCount,
        guitarCount>0
            ? std::clamp(
                static_cast<double>(guitarCount)/
                static_cast<double>(IPC::kSensorSlotCount),
                0.0,
                1.0)
            : 0.0,
        32);

    const auto overallSummary=
        Analysis::summarizeRoleCounts(
            responseFresh
            ? latestIpc_.roleCount
            : nullptr);

    publishParam(
        data,
        kAllSensorCount,
        static_cast<double>(
            overallSummary.sensors)/
        static_cast<double>(
            IPC::kSensorSlotCount),
        49);

    publishParam(
        data,
        kAllRoleCount,
        static_cast<double>(
            overallSummary.roles)/
        static_cast<double>(
            IPC::kRoleCount),
        50);

    // When the transport is stopped, keep the last measured diagnosis
    // visible instead of decaying live values to silence.
    if(hasProcessContext && !playing)
        return kResultOk;

    publishParam(
        data,
        kDrumsLevel,
        level(drums,drumsOk),
        1);

    publishParam(
        data,
        kBassLevel,
        level(bass,bassOk),
        3);

    publishParam(
        data,
        kGuitarLevel,
        level(guitar,guitarOk),
        5);

    // Do not decay accumulated evidence merely because the IPC worker is
    // one response behind (especially across a cycle wrap). A fresh response
    // with a genuinely disconnected source still drives the normal decay path.
    if(responseFresh){
        const auto pairRates=
            Analysis::makePairUpdateRates(
                data.numSamples,
                sampleRate_);

        updatePair(
            drums,bass,
            pairStates_[0],
            pairRates,
            false,
            data.numSamples,
            currentSamplePosition);

        updatePair(
            bass,guitar,
            pairStates_[1],
            pairRates,
            false,
            data.numSamples,
            currentSamplePosition);

        updatePair(
            drums,guitar,
            pairStates_[2],
            pairRates,
            false,
            data.numSamples,
            currentSamplePosition);

        for(int a=0;a<IPC::kRoleCount-1;++a){
            for(int b=a+1;b<IPC::kRoleCount;++b){
                const auto first=
                    Analysis::roleFromIndex(a);
                const auto second=
                    Analysis::roleFromIndex(b);

                const int pairIndex=
                    Analysis::encodeRolePair(
                        first,second);

                if(pairIndex<0 ||
                   !Analysis::rolesComparableForCoach(
                       first,
                       second))
                    continue;

                auto& state=
                    coachPairStates_[pairIndex];

                bool bandsIdle=true;

                for(int band=0;
                    band<IPC::kBandCount;
                    ++band){
                    if(std::abs(
                           state.bandRisk[band])>
                       1.0e-6){
                        bandsIdle=false;
                        break;
                    }
                }

                const bool stateIdle=
                    state.observedSeconds<=1.0e-6 &&
                    std::abs(state.overlap)<=1.0e-6 &&
                    std::abs(state.masking)<=1.0e-6 &&
                    std::abs(state.dominance)<=1.0e-6 &&
                    std::abs(
                        state.transientCompetition)<=1.0e-6 &&
                    std::abs(state.confidence)<=1.0e-6 &&
                    bandsIdle;

                const bool bothConnected=
                    latestIpc_.roles[a].connected &&
                    latestIpc_.roles[b].connected;

                if(!bothConnected &&
                   stateIdle){
                    state=PairState{};
                    continue;
                }

                applyPairMeasurement(
                    latestIpc_.
                        coachMeasurements[
                            pairIndex],
                    state,
                    pairRates);
            }
        }
    }else if(responseForSession){
        // A response outside the coherence grace is materially stale, not
        // merely one worker frame behind. Let old evidence decay instead of
        // freezing a diagnosis indefinitely if IPC stalls.
        const auto pairRates=
            Analysis::makePairUpdateRates(
                data.numSamples,
                sampleRate_);

        const Analysis::PairMeasurement inactive{};

        for(auto& state:pairStates_)
            applyPairMeasurement(
                inactive,
                state,
                pairRates);

        for(auto& state:coachPairStates_)
            applyPairMeasurement(
                inactive,
                state,
                pairRates);
    }

    publishParam(data,kDrumsBassOverlap,pairStates_[0].overlap,6);
    publishParam(data,kDrumsBassMasking,pairStates_[0].masking,7);
    publishParam(data,kDrumsBassBand,static_cast<double>(pairStates_[0].dominantBand)/8.0,8);
    publishParam(data,kDrumsBassStatus,severityFromState(pairStates_[0]),9);

    publishParam(data,kBassGuitarOverlap,pairStates_[1].overlap,10);
    publishParam(data,kBassGuitarMasking,pairStates_[1].masking,11);
    publishParam(data,kBassGuitarBand,static_cast<double>(pairStates_[1].dominantBand)/8.0,12);
    publishParam(data,kBassGuitarStatus,severityFromState(pairStates_[1]),13);

    publishParam(data,kDrumsGuitarOverlap,pairStates_[2].overlap,14);
    publishParam(data,kDrumsGuitarMasking,pairStates_[2].masking,15);
    publishParam(data,kDrumsGuitarBand,static_cast<double>(pairStates_[2].dominantBand)/8.0,16);
    publishParam(data,kDrumsGuitarStatus,severityFromState(pairStates_[2]),17);

    const int best=
        chooseTopPair();

    updateSessionFinding();

    // The session entry is a previously observed finding, not a current
    // masking measurement. Only use it when there is no eligible current one.
    const auto selection=
        Analysis::selectPrimaryFinding(
            best,
            sessionFinding_.pair);

    const int displayPair=
        selection.pair;

    const bool useCurrent=
        selection.current;

    const int displayBand=
        displayPair<0
        ? 0
        : useCurrent
            ? pairStates_[best].dominantBand
            : sessionFinding_.band;

    const double displayDominance=
        displayPair<0
        ? 0.0
        : useCurrent
            ? pairStates_[best].dominance
            : sessionFinding_.dominance;

    const double pairValue=
        displayPair<0
        ? 0.0
        : static_cast<double>(displayPair+1)/3.0;

    const double scoreValue=
        displayPair<0
        ? 0.0
        : useCurrent
            ? pairStates_[best].masking
            : sessionFinding_.score;

    const double bandValue=
        displayPair<0
        ? 0.0
        : static_cast<double>(
            displayBand)/8.0;

    const double dominanceValue=
        displayPair<0
        ? 0.5
        : dominanceParam(
            displayDominance);

    const double confidenceValue=
        displayPair<0
        ? 0.0
        : useCurrent
            ? pairStates_[best].confidence
            : sessionFinding_.confidence;

    const int advice=
        displayPair<0
        ? 0
        : Analysis::coachAdviceCode(
            displayPair,
            displayBand,
            displayDominance);

    const double adviceValue=
        static_cast<double>(advice)/15.0;

    publishParam(data,kTopPair,pairValue,18);
    publishParam(data,kTopScore,scoreValue,19);
    publishParam(data,kTopBand,bandValue,20);
    publishParam(data,kTopAdvice,adviceValue,21);
    publishParam(data,kTopDominance,dominanceValue,22);
    publishParam(data,kTopConfidence,confidenceValue,23);

    const int coachPair=
        chooseCoachPair();

    Analysis::Recommendation recommendation;

    if(coachPair>=0){
        IPC::Role first=IPC::Role::Unknown;
        IPC::Role second=IPC::Role::Unknown;

        if(Analysis::decodeRolePair(
               coachPair,
               first,
               second)){

            const auto& state=
                coachPairStates_[coachPair];

            recommendation=
                Analysis::makeRecommendation(
                    first,
                    second,
                    state.dominantBand,
                    state.masking,
                    state.confidence,
                    state.dominance,
                    state.transientCompetition);
        }
    }

    const double coachAdviceValue=
        recommendation.valid
        ? Analysis::encodeDiscreteCode(
            Analysis::recommendationCode(
                recommendation.kind),
            6)
        : 0.0;

    const double coachPairValue=
        recommendation.valid
        ? Analysis::encodeDiscreteCode(
            coachPair+1,
            Analysis::kRolePairCount)
        : 0.0;

    const double coachBandValue=
        Analysis::encodeDiscreteCode(
            Analysis::recommendationBandCode(
                recommendation),
            Analysis::
            kRecommendationBandCodeCount);

    const double coachConfidence=
        recommendation.valid
        ? recommendation.confidence
        : 0.0;

    const double coachActionValue=
        recommendation.valid
        ? Analysis::encodeDiscreteCode(
            Analysis::recommendationActionCode(
                recommendation),
            6)
        : 0.0;

    const int coachTargetCode=
        Analysis::recommendationTargetCode(
            recommendation);

    const double coachTargetValue=
        Analysis::encodeDiscreteCode(
            coachTargetCode,
            Analysis::
            kRecommendationTargetCodeCount);

    publishParam(data,kCoachHeadline,coachAdviceValue,39);
    publishParam(data,kCoachAction,coachActionValue,40);
    publishParam(data,kCoachListen,coachAdviceValue,41);
    publishParam(data,kCoachReason,coachAdviceValue,42);

    const double evidenceValue=
        Analysis::encodeDiscreteCode(
            Analysis::coachEvidenceBand(
                coachConfidence),
            3);

    publishParam(data,kCoachEvidence,evidenceValue,43);
    publishParam(data,kCoachPair,coachPairValue,44);
    publishParam(data,kCoachBand,coachBandValue,45);
    publishParam(data,kCoachTarget,coachTargetValue,48);

    const double sessionPair=
        (sessionFinding_.pair<0)
        ? 0.0
        : static_cast<double>(
            sessionFinding_.pair+1)/3.0;

    const double sessionScore=
        (sessionFinding_.pair<0)
        ? 0.0
        : sessionFinding_.score;

    const double sessionBand=
        (sessionFinding_.pair<0)
        ? 0.0
        : static_cast<double>(
            sessionFinding_.band)/8.0;

    publishParam(data,kSessionPair,sessionPair,24);
    publishParam(data,kSessionScore,sessionScore,25);
    publishParam(data,kSessionBand,sessionBand,26);

    publishParam(
        data,
        kDrumsTransient,
        drumsOk
            ? std::clamp(drums.transient,0.0,1.0)
            : 0.0,
        27);

    publishParam(
        data,
        kBassTransient,
        bassOk
            ? std::clamp(bass.transient,0.0,1.0)
            : 0.0,
        28);

    publishParam(
        data,
        kGuitarTransient,
        guitarOk
            ? std::clamp(guitar.transient,0.0,1.0)
            : 0.0,
        29);

    publishParam(
        data,
        kDrumsBassTransientCompetition,
        pairStates_[0].transientCompetition,
        33);

    publishParam(
        data,
        kBassGuitarTransientCompetition,
        pairStates_[1].transientCompetition,
        34);

    publishParam(
        data,
        kDrumsGuitarTransientCompetition,
        pairStates_[2].transientCompetition,
        35);

    const double attackScores[3]{
        pairStates_[0].transientCompetition,
        pairStates_[1].transientCompetition,
        pairStates_[2].transientCompetition
    };

    const double attackObserved[3]{
        pairStates_[0].observedSeconds,
        pairStates_[1].observedSeconds,
        pairStates_[2].observedSeconds
    };

    const auto attackFinding=
        Analysis::chooseAttackFinding(
            attackScores,
            attackObserved);

    const double attackPairValue=
        attackFinding.pair<0
        ? 0.0
        : static_cast<double>(
            attackFinding.pair+1)/3.0;

    const double attackAdviceValue=
        attackFinding.pair<0
        ? 0.0
        : static_cast<double>(
            attackFinding.pair+1)/3.0;

    publishParam(
        data,
        kTopAttackPair,
        attackPairValue,
        36);

    publishParam(
        data,
        kTopAttackScore,
        attackFinding.score,
        37);

    publishParam(
        data,
        kTopAttackAdvice,
        attackAdviceValue,
        38);

    const int coachAttackPair=
        chooseCoachAttackPair();

    const double coachAttackPairValue=
        coachAttackPair<0
        ? 0.0
        : static_cast<double>(
            coachAttackPair+1)/
          static_cast<double>(
            Analysis::kRolePairCount);

    publishParam(
        data,
        kCoachAttackPair,
        coachAttackPairValue,
        46);

    publishParam(
        data,
        kCoachAttackAdvice,
        coachAttackPair<0
            ? 0.0
            : 1.0,
        47);

    return kResultOk;
}

tresult PLUGIN_API Processor::setState(
    IBStream* state){

    if(!state)
        return kInvalidArgument;

    IBStreamer s(state,kLittleEndian);
    int32 session=0;

    if(s.readInt32(session))
        session_=std::clamp(session,0,7);
    else
        session_=0;

    resetAnalysisState();
    return kResultOk;
}

tresult PLUGIN_API Processor::getState(
    IBStream* state){

    if(!state)
        return kInvalidArgument;

    IBStreamer s(state,kLittleEndian);

    return
        s.writeInt32(
            static_cast<int32>(session_))
        ? kResultOk
        : kResultFalse;
}

} // namespace MixDoctorator::Brain

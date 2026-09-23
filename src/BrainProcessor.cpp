#include "BrainProcessor.h"
#include "BrainIDs.h"
#include "MaskingModel.h"
#include "TimingModel.h"

#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/vstspeaker.h"

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace MixDoctorator::Brain {
using namespace Steinberg;
using namespace Steinberg::Vst;

Processor::Processor(){
    setControllerClass(kControllerUID);
}

void Processor::resetAnalysisState() noexcept{
    for(auto& p:pairStates_)
        p=PairState{};

    sessionFinding_=SessionFinding{};
    heldTopPair_=-1;

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

    ipc_.open();
    return kResultOk;
}

tresult PLUGIN_API Processor::setBusArrangements(
    SpeakerArrangement* in,int32 ni,
    SpeakerArrangement* out,int32 no){

    if(ni==1 &&
       no==1 &&
       in[0]==SpeakerArr::kStereo &&
       out[0]==SpeakerArr::kStereo)
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

void Processor::updatePair(
    const IPC::Snapshot& a,
    const IPC::Snapshot& b,
    PairState& state,
    int32 numSamples,
    std::int64_t currentSamplePosition) noexcept{

    const double dt=
        std::clamp(
            static_cast<double>(
                std::max<int32>(1,numSamples)) /
            std::max(8000.0,sampleRate_),
            0.0001,
            0.25);

    const bool timeCoherent=
        Analysis::samplePositionsCoherent(
            currentSamplePosition,
            a.samplePosition,
            b.samplePosition,
            numSamples);

    const bool active=
        a.connected &&
        b.connected &&
        timeCoherent &&
        a.rmsDb>-55.0 &&
        b.rmsDb>-55.0;

    double overlapTarget=0.0;
    double maskingTarget=0.0;
    double bandTargets[IPC::kBandCount]{};

    if(active){
        state.observedSeconds=
            std::min(
                45.0,
                state.observedSeconds+dt);

        const auto metrics=
            Analysis::evaluatePair(
                a.rmsDb,
                a.activity,
                a.bands,
                b.rmsDb,
                b.activity,
                b.bands);

        overlapTarget=metrics.overlap;
        maskingTarget=metrics.masking;

        for(int i=0;i<IPC::kBandCount;++i)
            bandTargets[i]=metrics.bandRisk[i];

        const double domAlpha=
            1.0-std::exp(-dt/1.8);

        state.dominance+=
            domAlpha*
            (metrics.dominance-state.dominance);
    }else{
        state.observedSeconds=
            std::max(
                0.0,
                state.observedSeconds-dt*0.20);
    }

    const double overlapAlpha=
        1.0-std::exp(
            -dt /
            ((overlapTarget>state.overlap)
                ? 0.60
                : 2.5));

    const double maskingAlpha=
        1.0-std::exp(
            -dt /
            ((maskingTarget>state.masking)
                ? 0.85
                : 3.5));

    state.overlap+=
        overlapAlpha*
        (overlapTarget-state.overlap);

    state.masking+=
        maskingAlpha*
        (maskingTarget-state.masking);

    const double bandAlpha=
        1.0-std::exp(-dt/1.5);

    for(int i=0;i<IPC::kBandCount;++i)
        state.bandRisk[i]+=
            bandAlpha*
            (bandTargets[i]-state.bandRisk[i]);

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
        timeConfidence *
        (
            0.60*riskConfidence +
            0.40*bandStability
        );

    const double confAlpha=
        1.0-std::exp(-dt/2.5);

    state.confidence+=
        confAlpha*
        (targetConfidence-state.confidence);
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

int Processor::adviceFor(
    int pairIndex,
    int bandIndex,
    double dominance) noexcept{

    const int side=
        dominance>0.20
        ? 1
        : dominance<-0.20
            ? -1
            : 0;

    if(pairIndex<0)
        return 0;

    if(pairIndex==0){
        if(bandIndex<=1){
            if(side>0) return 1;
            if(side<0) return 2;
            return 3;
        }

        if(bandIndex<=3)
            return 4;

        return 5;
    }

    if(pairIndex==1){
        if(bandIndex<=2){
            if(side>0) return 6;
            if(side<0) return 7;
            return 8;
        }

        if(bandIndex<=5)
            return 9;

        return 10;
    }

    if(pairIndex==2){
        if(bandIndex<=2)
            return 11;

        if(bandIndex<=6){
            if(side>0) return 12;
            if(side<0) return 13;
            return 14;
        }

        return 15;
    }

    return 0;
}

int Processor::chooseTopPair() noexcept{
    auto eligible=[this](int i){
        return
            pairStates_[i].observedSeconds>=2.5 &&
            pairStates_[i].masking>=0.14 &&
            pairStates_[i].confidence>=0.12;
    };

    if(heldTopPair_>=0 &&
       heldTopPair_<3 &&
       eligible(heldTopPair_)){

        int challenger=
            heldTopPair_;

        double challengerRank=
            pairStates_[heldTopPair_].masking *
            (0.60+
             0.40*
             pairStates_[heldTopPair_].confidence);

        for(int i=0;i<3;++i){
            if(!eligible(i))
                continue;

            const double rank=
                pairStates_[i].masking *
                (0.60+
                 0.40*
                 pairStates_[i].confidence);

            if(rank>challengerRank+0.04){
                challenger=i;
                challengerRank=rank;
            }
        }

        heldTopPair_=challenger;
        return heldTopPair_;
    }

    int best=-1;
    double bestRank=0.14;

    for(int i=0;i<3;++i){
        if(!eligible(i))
            continue;

        const double rank=
            pairStates_[i].masking *
            (0.60+
             0.40*
             pairStates_[i].confidence);

        if(rank>bestRank){
            best=i;
            bestRank=rank;
        }
    }

    heldTopPair_=best;
    return best;
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

        for(int32 i=0;i<n;++i)
            out[c][i]=
                in[c]
                ? in[c][i]
                : static_cast<T>(0);
    }
}

tresult PLUGIN_API Processor::process(
    ProcessData& data){

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

    const bool playing=
        data.processContext &&
        ((data.processContext->state &
          ProcessContext::kPlaying)!=0);

    const std::int64_t currentSamplePosition=
        data.processContext
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

    const bool restarted=
        playing &&
        !wasPlaying_;

    const bool jumpedBackward=
        currentSamplePosition>=0 &&
        lastProjectSample_>=0 &&
        currentSamplePosition+
            rewindTolerance<
        lastProjectSample_;

    if(restarted ||
       jumpedBackward)
        resetAnalysisState();

    wasPlaying_=playing;

    if(currentSamplePosition>=0)
        lastProjectSample_=
            currentSamplePosition;

    IPC::Snapshot drums,bass,guitar;

    const bool drumsOk=
        ipc_.read(
            IPC::Role::Drums,
            drums) &&
        drums.connected;

    const bool bassOk=
        ipc_.read(
            IPC::Role::Bass,
            bass) &&
        bass.connected;

    const bool guitarOk=
        ipc_.read(
            IPC::Role::ElectricGuitar,
            guitar) &&
        guitar.connected;

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
        drumsOk?1.0:0.0,
        0);

    publishParam(
        data,
        kDrumsLevel,
        level(drums,drumsOk),
        1);

    publishParam(
        data,
        kBassConnected,
        bassOk?1.0:0.0,
        2);

    publishParam(
        data,
        kBassLevel,
        level(bass,bassOk),
        3);

    publishParam(
        data,
        kGuitarConnected,
        guitarOk?1.0:0.0,
        4);

    publishParam(
        data,
        kGuitarLevel,
        level(guitar,guitarOk),
        5);

    updatePair(
        drums,bass,
        pairStates_[0],
        data.numSamples,
        currentSamplePosition);

    updatePair(
        bass,guitar,
        pairStates_[1],
        data.numSamples,
        currentSamplePosition);

    updatePair(
        drums,guitar,
        pairStates_[2],
        data.numSamples,
        currentSamplePosition);

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

    const double pairValue=
        (best<0)
        ? 0.0
        : static_cast<double>(best+1)/3.0;

    const double scoreValue=
        (best<0)
        ? 0.0
        : pairStates_[best].masking;

    const double bandValue=
        (best<0)
        ? 0.0
        : static_cast<double>(
            pairStates_[best].dominantBand)/8.0;

    const double dominanceValue=
        (best<0)
        ? 0.5
        : dominanceParam(
            pairStates_[best].dominance);

    const double confidenceValue=
        (best<0)
        ? 0.0
        : pairStates_[best].confidence;

    const int advice=
        (best<0)
        ? 0
        : adviceFor(
            best,
            pairStates_[best].dominantBand,
            pairStates_[best].dominance);

    const double adviceValue=
        static_cast<double>(advice)/15.0;

    publishParam(data,kTopPair,pairValue,18);
    publishParam(data,kTopScore,scoreValue,19);
    publishParam(data,kTopBand,bandValue,20);
    publishParam(data,kTopAdvice,adviceValue,21);
    publishParam(data,kTopDominance,dominanceValue,22);
    publishParam(data,kTopConfidence,confidenceValue,23);

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

    return kResultOk;
}

} // namespace MixDoctorator::Brain

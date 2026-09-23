#include "BrainProcessor.h"
#include "BrainIDs.h"

#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstspeaker.h"

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace MixDoctorator::Brain {
using namespace Steinberg;
using namespace Steinberg::Vst;

Processor::Processor() {
    setControllerClass(kControllerUID);
}

tresult PLUGIN_API Processor::initialize(FUnknown* c) {
    const auto r=AudioEffect::initialize(c);
    if(r!=kResultOk) return r;

    addAudioInput(STR16("Stereo In"),SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"),SpeakerArr::kStereo);
    ipc_.open();
    return kResultOk;
}

tresult PLUGIN_API Processor::setBusArrangements(
    SpeakerArrangement* in,int32 ni,
    SpeakerArrangement* out,int32 no) {

    if(ni==1 && no==1 &&
       in[0]==SpeakerArr::kStereo &&
       out[0]==SpeakerArr::kStereo) {
        return AudioEffect::setBusArrangements(in,ni,out,no);
    }

    return kResultFalse;
}

tresult PLUGIN_API Processor::canProcessSampleSize(int32 s) {
    return (s==kSample32 || s==kSample64) ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API Processor::setupProcessing(ProcessSetup& setup) {
    sampleRate_=(std::isfinite(setup.sampleRate) && setup.sampleRate>8000.0)
        ? setup.sampleRate : 44100.0;

    return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API Processor::setProcessing(TBool state) {
    if(state) {
        for(auto& p:pairStates_) {
            p.score=0.0;
            p.observedSeconds=0.0;
            p.dominance=0.0;
            p.dominantBand=0;
            for(double& b:p.bands) b=0.0;
        }
        heldTopPair_=-1;
    }

    AudioEffect::setProcessing(state);
    return kResultTrue;
}

void Processor::publishParam(
    ProcessData& data,
    Steinberg::Vst::ParamID id,
    double v,
    int slot) {

    v=std::clamp(v,0.0,1.0);

    if(std::abs(v-last_[slot])<0.0005 || !data.outputParameterChanges)
        return;

    int32 index=0;
    auto* q=data.outputParameterChanges->addParameterData(id,index);

    if(!q) return;

    int32 point=0;
    if(q->addPoint(0,v,point)==kResultTrue)
        last_[slot]=v;
}

void Processor::updatePair(
    const IPC::Snapshot& a,
    const IPC::Snapshot& b,
    PairState& state,
    int32 numSamples) noexcept {

    const double dt=std::clamp(
        static_cast<double>(std::max<int32>(1,numSamples)) /
            std::max(8000.0,sampleRate_),
        0.0001,
        0.25);

    const bool active=
        a.connected && b.connected &&
        a.rmsDb>-55.0 && b.rmsDb>-55.0;

    double target=0.0;
    double commonBands[IPC::kBandCount]{0.0,0.0,0.0,0.0,0.0};

    if(active) {
        state.observedSeconds=std::min(30.0,state.observedSeconds+dt);

        double rawOverlap=0.0;

        for(int i=0;i<IPC::kBandCount;++i) {
            commonBands[i]=std::min(
                std::max(0.0,a.bands[i]),
                std::max(0.0,b.bands[i]));

            rawOverlap+=commonBands[i];
        }

        rawOverlap=std::clamp(rawOverlap,0.0,1.0);

        const double jointActivity=std::sqrt(
            std::clamp(a.activity,0.0,1.0) *
            std::clamp(b.activity,0.0,1.0));

        const double levelGap=std::abs(a.rmsDb-b.rmsDb);
        const double proximity=0.35+0.65*std::exp(-levelGap/12.0);

        target=rawOverlap*(0.35+0.65*jointActivity)*proximity;
    } else {
        state.observedSeconds=std::max(0.0,state.observedSeconds-dt*0.25);
    }

    const double tau=(target>state.score) ? 0.65 : 3.0;
    const double alpha=1.0-std::exp(-dt/tau);
    state.score+=alpha*(target-state.score);

    const double bandAlpha=1.0-std::exp(-dt/1.25);

    if(active) {
        const double jointActivity=std::sqrt(
            std::clamp(a.activity,0.0,1.0) *
            std::clamp(b.activity,0.0,1.0));

        for(int i=0;i<IPC::kBandCount;++i) {
            const double weighted=commonBands[i]*(0.4+0.6*jointActivity);
            state.bands[i]+=bandAlpha*(weighted-state.bands[i]);
        }
    } else {
        const double releaseAlpha=1.0-std::exp(-dt/4.0);
        for(double& value:state.bands)
            value+=releaseAlpha*(0.0-value);
    }

    int best=0;
    for(int i=1;i<IPC::kBandCount;++i) {
        if(state.bands[i]>state.bands[best])
            best=i;
    }
    state.dominantBand=best;

    if(active) {
        const double aAmp=
            std::pow(10.0,a.rmsDb/20.0) *
            std::sqrt(std::max(0.0,a.bands[best]));

        const double bAmp=
            std::pow(10.0,b.rmsDb/20.0) *
            std::sqrt(std::max(0.0,b.bands[best]));

        const double aDb=20.0*std::log10(std::max(aAmp,1.0e-9));
        const double bDb=20.0*std::log10(std::max(bAmp,1.0e-9));

        const double targetDominance=
            std::clamp((aDb-bDb)/12.0,-1.0,1.0);

        const double domAlpha=1.0-std::exp(-dt/1.5);
        state.dominance+=domAlpha*(targetDominance-state.dominance);
    }
}

double Processor::severityFromState(const PairState& state) noexcept {
    if(state.observedSeconds<2.0) return 0.0;
    if(state.score<0.18) return 0.25;
    if(state.score<0.32) return 0.50;
    if(state.score<0.48) return 0.75;
    return 1.0;
}

double Processor::dominanceParam(double dominance) noexcept {
    if(dominance>0.20) return 1.0;
    if(dominance<-0.20) return 0.0;
    return 0.5;
}

int Processor::adviceFor(
    int pairIndex,
    int bandIndex,
    double dominance) noexcept {

    const int side=
        dominance>0.20 ? 1 :
        dominance<-0.20 ? -1 : 0;

    if(pairIndex<0) return 0;

    // 1..5: drums/bass
    if(pairIndex==0) {
        if(bandIndex==0) {
            if(side>0) return 1;
            if(side<0) return 2;
            return 3;
        }
        return 4;
    }

    // 5..9: bass/guitar
    if(pairIndex==1) {
        if(bandIndex<=1) {
            if(side>0) return 5;
            if(side<0) return 6;
            return 7;
        }
        return 8;
    }

    // 9..13: drums/guitar
    if(pairIndex==2) {
        if(bandIndex>=2) {
            if(side>0) return 9;
            if(side<0) return 10;
            return 11;
        }
        return 12;
    }

    return 0;
}

int Processor::chooseTopPair() noexcept {
    auto eligible=[this](int i) {
        return pairStates_[i].observedSeconds>=2.0 &&
               pairStates_[i].score>=0.18;
    };

    if(heldTopPair_>=0 && heldTopPair_<3 && eligible(heldTopPair_)) {
        int challenger=heldTopPair_;
        double challengerScore=pairStates_[heldTopPair_].score;

        for(int i=0;i<3;++i) {
            if(!eligible(i)) continue;

            if(pairStates_[i].score>challengerScore+0.05) {
                challenger=i;
                challengerScore=pairStates_[i].score;
            }
        }

        heldTopPair_=challenger;
        return heldTopPair_;
    }

    int best=-1;
    double bestScore=0.18;

    for(int i=0;i<3;++i) {
        if(eligible(i) && pairStates_[i].score>bestScore) {
            best=i;
            bestScore=pairStates_[i].score;
        }
    }

    heldTopPair_=best;
    return best;
}

template<typename T>
static void pass(
    AudioBusBuffers& input,
    AudioBusBuffers& output,
    int32 n) {

    const int32 ch=std::min(input.numChannels,output.numChannels);
    T** in=nullptr;
    T** out=nullptr;

    if constexpr(std::is_same_v<T,float>) {
        in=input.channelBuffers32;
        out=output.channelBuffers32;
    } else {
        in=input.channelBuffers64;
        out=output.channelBuffers64;
    }

    for(int32 c=0;c<ch;++c) {
        if(!out[c]) continue;

        for(int32 i=0;i<n;++i)
            out[c][i]=in[c] ? in[c][i] : static_cast<T>(0);
    }
}

tresult PLUGIN_API Processor::process(ProcessData& data) {
    if(data.numInputs>0 && data.numOutputs>0 && data.numSamples>0) {
        if(data.symbolicSampleSize==kSample64)
            pass<double>(data.inputs[0],data.outputs[0],data.numSamples);
        else
            pass<float>(data.inputs[0],data.outputs[0],data.numSamples);
    }

    IPC::Snapshot drums;
    IPC::Snapshot bass;
    IPC::Snapshot guitar;

    const bool drumsOk=ipc_.read(IPC::Role::Drums,drums) && drums.connected;
    const bool bassOk=ipc_.read(IPC::Role::Bass,bass) && bass.connected;
    const bool guitarOk=ipc_.read(IPC::Role::ElectricGuitar,guitar) && guitar.connected;

    auto level=[](const IPC::Snapshot& s,bool ok) {
        return ok ? std::clamp((s.rmsDb+60.0)/60.0,0.0,1.0) : 0.0;
    };

    publishParam(data,kDrumsConnected,drumsOk?1.0:0.0,0);
    publishParam(data,kDrumsLevel,level(drums,drumsOk),1);
    publishParam(data,kBassConnected,bassOk?1.0:0.0,2);
    publishParam(data,kBassLevel,level(bass,bassOk),3);
    publishParam(data,kGuitarConnected,guitarOk?1.0:0.0,4);
    publishParam(data,kGuitarLevel,level(guitar,guitarOk),5);

    updatePair(drums,bass,pairStates_[0],data.numSamples);
    updatePair(bass,guitar,pairStates_[1],data.numSamples);
    updatePair(drums,guitar,pairStates_[2],data.numSamples);

    publishParam(data,kDrumsBassOverlap,pairStates_[0].score,6);
    publishParam(data,kDrumsBassBand,static_cast<double>(pairStates_[0].dominantBand)/4.0,7);
    publishParam(data,kDrumsBassStatus,severityFromState(pairStates_[0]),8);

    publishParam(data,kBassGuitarOverlap,pairStates_[1].score,9);
    publishParam(data,kBassGuitarBand,static_cast<double>(pairStates_[1].dominantBand)/4.0,10);
    publishParam(data,kBassGuitarStatus,severityFromState(pairStates_[1]),11);

    publishParam(data,kDrumsGuitarOverlap,pairStates_[2].score,12);
    publishParam(data,kDrumsGuitarBand,static_cast<double>(pairStates_[2].dominantBand)/4.0,13);
    publishParam(data,kDrumsGuitarStatus,severityFromState(pairStates_[2]),14);

    const int best=chooseTopPair();

    const double pairValue=(best<0) ? 0.0 :
        static_cast<double>(best+1)/3.0;

    const double scoreValue=(best<0) ? 0.0 :
        pairStates_[best].score;

    const double bandValue=(best<0) ? 0.0 :
        static_cast<double>(pairStates_[best].dominantBand)/4.0;

    const double dominanceValue=(best<0) ? 0.5 :
        dominanceParam(pairStates_[best].dominance);

    const int advice=(best<0) ? 0 :
        adviceFor(
            best,
            pairStates_[best].dominantBand,
            pairStates_[best].dominance);

    const double adviceValue=static_cast<double>(advice)/12.0;

    publishParam(data,kTopPair,pairValue,15);
    publishParam(data,kTopScore,scoreValue,16);
    publishParam(data,kTopBand,bandValue,17);
    publishParam(data,kTopAdvice,adviceValue,18);
    publishParam(data,kTopDominance,dominanceValue,19);

    return kResultOk;
}

} // namespace MixDoctorator::Brain

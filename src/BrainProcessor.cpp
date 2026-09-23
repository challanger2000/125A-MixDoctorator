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

tresult PLUGIN_API Processor::setProcessing(TBool state) {
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

void Processor::overlap(
    const IPC::Snapshot& a,
    const IPC::Snapshot& b,
    double& score,
    int& dominantBand) noexcept {

    score=0.0;
    dominantBand=0;

    if(!a.connected || !b.connected ||
       a.rmsDb < -55.0 || b.rmsDb < -55.0) {
        return;
    }

    double strongest=-1.0;

    for(int i=0;i<IPC::kBandCount;++i) {
        const double common=std::min(
            std::max(0.0,a.bands[i]),
            std::max(0.0,b.bands[i]));

        score+=common;

        if(common>strongest) {
            strongest=common;
            dominantBand=i;
        }
    }

    score=std::clamp(score,0.0,1.0);
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

    double score=0.0;
    int band=0;

    overlap(drums,bass,score,band);
    publishParam(data,kDrumsBassOverlap,score,6);
    publishParam(data,kDrumsBassBand,static_cast<double>(band)/4.0,7);

    overlap(bass,guitar,score,band);
    publishParam(data,kBassGuitarOverlap,score,8);
    publishParam(data,kBassGuitarBand,static_cast<double>(band)/4.0,9);

    overlap(drums,guitar,score,band);
    publishParam(data,kDrumsGuitarOverlap,score,10);
    publishParam(data,kDrumsGuitarBand,static_cast<double>(band)/4.0,11);

    return kResultOk;
}

} // namespace MixDoctorator::Brain

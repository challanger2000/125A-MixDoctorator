#include "BrainProcessor.h"
#include "BrainIDs.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstspeaker.h"
#include <algorithm>
#include <cmath>
#include <type_traits>
namespace MixDoctorator::Brain {
using namespace Steinberg; using namespace Steinberg::Vst;
Processor::Processor(){ setControllerClass(kControllerUID); }
tresult PLUGIN_API Processor::initialize(FUnknown* c){
    const auto r=AudioEffect::initialize(c); if(r!=kResultOk) return r;
    addAudioInput(STR16("Stereo In"),SpeakerArr::kStereo); addAudioOutput(STR16("Stereo Out"),SpeakerArr::kStereo);
    ipc_.open(); return kResultOk;
}
tresult PLUGIN_API Processor::setBusArrangements(SpeakerArrangement* in,int32 ni,SpeakerArrangement* out,int32 no){
    if(ni==1&&no==1&&in[0]==SpeakerArr::kStereo&&out[0]==SpeakerArr::kStereo)
        return AudioEffect::setBusArrangements(in,ni,out,no);
    return kResultFalse;
}
tresult PLUGIN_API Processor::canProcessSampleSize(int32 s){ return (s==kSample32||s==kSample64)?kResultTrue:kResultFalse; }
tresult PLUGIN_API Processor::setProcessing(TBool state){ AudioEffect::setProcessing(state); return kResultTrue; }
void Processor::publishParam(ProcessData& data,Steinberg::Vst::ParamID id,double v,int slot){
    v=std::clamp(v,0.0,1.0); if(std::abs(v-last_[slot])<0.0005||!data.outputParameterChanges) return;
    int32 index=0; auto* q=data.outputParameterChanges->addParameterData(id,index); if(!q) return;
    int32 point=0; if(q->addPoint(0,v,point)==kResultTrue) last_[slot]=v;
}
template<typename T> static void pass(AudioBusBuffers& input,AudioBusBuffers& output,int32 n){
    const int32 ch=std::min(input.numChannels,output.numChannels); T** in=nullptr; T** out=nullptr;
    if constexpr(std::is_same_v<T,float>){ in=input.channelBuffers32; out=output.channelBuffers32; }
    else { in=input.channelBuffers64; out=output.channelBuffers64; }
    for(int32 c=0;c<ch;++c){ if(!out[c]) continue; for(int32 i=0;i<n;++i) out[c][i]=in[c]?in[c][i]:static_cast<T>(0); }
}
tresult PLUGIN_API Processor::process(ProcessData& data){
    if(data.numInputs>0&&data.numOutputs>0&&data.numSamples>0){
        if(data.symbolicSampleSize==kSample64) pass<double>(data.inputs[0],data.outputs[0],data.numSamples);
        else pass<float>(data.inputs[0],data.outputs[0],data.numSamples);
    }
    struct Row{IPC::Role role; Steinberg::Vst::ParamID connected,level; int base;};
    const Row rows[]={{IPC::Role::Drums,kDrumsConnected,kDrumsLevel,0},
                      {IPC::Role::Bass,kBassConnected,kBassLevel,2},
                      {IPC::Role::ElectricGuitar,kGuitarConnected,kGuitarLevel,4}};
    for(const auto& row:rows){
        IPC::Snapshot s; const bool ok=ipc_.read(row.role,s)&&s.connected;
        const double level=ok?std::clamp((s.rmsDb+60.0)/60.0,0.0,1.0):0.0;
        publishParam(data,row.connected,ok?1.0:0.0,row.base); publishParam(data,row.level,level,row.base+1);
    }
    return kResultOk;
}
}

#include "SensorProcessor.h"
#include "SensorIDs.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstspeaker.h"
#include <algorithm>
#include <cmath>
#include <type_traits>
namespace MixDoctorator::Sensor {
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
void Processor::readParameters(IParameterChanges* changes){
    if(!changes) return;
    for(int32 i=0;i<changes->getParameterCount();++i){
        auto* q=changes->getParameterData(i); if(!q||q->getPointCount()<=0||q->getParameterId()!=kRole) continue;
        int32 off=0; ParamValue v=0.0;
        if(q->getPoint(q->getPointCount()-1,off,v)!=kResultTrue) continue;
        const int index=std::clamp(static_cast<int>(std::lround(v*2.0)),0,2);
        role_=static_cast<IPC::Role>(index+1);
    }
}
template<typename T> static void copyMeasure(AudioBusBuffers& input,AudioBusBuffers& output,int32 n,double& sumSq,double& peak){
    const int32 ch=std::min(input.numChannels,output.numChannels);
    T** in=nullptr; T** out=nullptr;
    if constexpr(std::is_same_v<T,float>){ in=input.channelBuffers32; out=output.channelBuffers32; }
    else { in=input.channelBuffers64; out=output.channelBuffers64; }
    for(int32 c=0;c<ch;++c){ if(!out[c]) continue; for(int32 i=0;i<n;++i){
        const double x=in[c]?static_cast<double>(in[c][i]):0.0; out[c][i]=static_cast<T>(x);
        peak=std::max(peak,std::abs(x)); sumSq+=x*x;
    }}
}
tresult PLUGIN_API Processor::process(ProcessData& data){
    readParameters(data.inputParameterChanges);
    if(data.numInputs<1||data.numOutputs<1||data.numSamples<=0) return kResultOk;
    double sumSq=0.0,peak=0.0;
    const int32 ch=std::max<int32>(1,std::min(data.inputs[0].numChannels,data.outputs[0].numChannels));
    if(data.symbolicSampleSize==kSample64) copyMeasure<double>(data.inputs[0],data.outputs[0],data.numSamples,sumSq,peak);
    else copyMeasure<float>(data.inputs[0],data.outputs[0],data.numSamples,sumSq,peak);
    const double rms=std::sqrt(sumSq/std::max<double>(1.0,static_cast<double>(data.numSamples*ch)));
    const double rmsDb=IPC::dbFromAmplitude(rms), peakDb=IPC::dbFromAmplitude(peak);
    const double activity=std::clamp((rmsDb+60.0)/60.0,0.0,1.0);
    ipc_.publish(role_,rmsDb,peakDb,activity,0.0,0.0,0.0);
    return kResultOk;
}
tresult PLUGIN_API Processor::setState(IBStream* state){
    if(!state) return kInvalidArgument; IBStreamer s(state,kLittleEndian); int32 r=1;
    if(!s.readInt32(r)) return kResultFalse; role_=static_cast<IPC::Role>(std::clamp(r,1,3)); return kResultOk;
}
tresult PLUGIN_API Processor::getState(IBStream* state){
    if(!state) return kInvalidArgument; IBStreamer s(state,kLittleEndian);
    return s.writeInt32(static_cast<int32>(role_))?kResultOk:kResultFalse;
}
}

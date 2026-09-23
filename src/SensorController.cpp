#include "SensorController.h"
#include "SensorIDs.h"
#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include <algorithm>
#include <cstring>
namespace MixDoctorator::Sensor {
using namespace Steinberg; using namespace Steinberg::Vst;
tresult PLUGIN_API Controller::initialize(FUnknown* c){
    const auto r=EditController::initialize(c); if(r!=kResultOk) return r;
    auto* p=new StringListParameter(STR16("Source Role"),kRole,nullptr,ParameterInfo::kCanAutomate);
    p->appendString(STR16("Drums")); p->appendString(STR16("Bass")); p->appendString(STR16("Electric Guitar"));
    parameters.addParameter(p); return kResultOk;
}
tresult PLUGIN_API Controller::setComponentState(IBStream* state){
    if(!state) return kInvalidArgument; IBStreamer s(state,kLittleEndian); int32 r=1;
    if(!s.readInt32(r)) return kResultFalse; setParamNormalized(kRole,static_cast<double>(std::clamp(r,1,3)-1)/2.0); return kResultOk;
}
IPlugView* PLUGIN_API Controller::createView(FIDString name){
    if(name&&std::strcmp(name,ViewType::kEditor)==0) return new VSTGUI::VST3Editor(this,"view","Sensor.uidesc");
    return nullptr;
}
}

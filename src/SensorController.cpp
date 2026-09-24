#include "SensorController.h"
#include "SensorIDs.h"
#include "MixDoctoratorIPC.h"

#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "vstgui/plugin-bindings/vst3editor.h"

#include <algorithm>
#include <cstring>

namespace MixDoctorator::Sensor {
using namespace Steinberg;
using namespace Steinberg::Vst;

tresult PLUGIN_API Controller::initialize(FUnknown* c){
    const auto r=EditController::initialize(c);
    if(r!=kResultOk) return r;

    auto* role=new StringListParameter(
        STR16("Source Role"),kRole,nullptr,ParameterInfo::kCanAutomate);
    role->appendString(STR16("Schlagzeug"));
    role->appendString(STR16("Bass"));
    role->appendString(STR16("E-Gitarre"));
    role->appendString(STR16("Kick"));
    role->appendString(STR16("Snare"));
    role->appendString(STR16("Toms"));
    role->appendString(STR16("Becken / Hi-Hat"));
    role->appendString(STR16("Percussion"));
    role->appendString(STR16("Akustikgitarre"));
    role->appendString(STR16("Lead-Gesang"));
    role->appendString(STR16("Backing-Gesang"));
    role->appendString(STR16("Piano / Keys"));
    role->appendString(STR16("Synth"));
    role->appendString(STR16("Pad"));
    parameters.addParameter(role);

    auto* session=new StringListParameter(
        STR16("Session"),kSession,nullptr,0);
    session->appendString(STR16("A"));
    session->appendString(STR16("B"));
    session->appendString(STR16("C"));
    session->appendString(STR16("D"));
    session->appendString(STR16("E"));
    session->appendString(STR16("F"));
    session->appendString(STR16("G"));
    session->appendString(STR16("H"));
    parameters.addParameter(session);

    return kResultOk;
}

tresult PLUGIN_API Controller::setComponentState(IBStream* state){
    if(!state) return kInvalidArgument;

    IBStreamer s(state,kLittleEndian);

    int32 role=1;
    if(!s.readInt32(role))
        return kResultFalse;

    setParamNormalized(
        kRole,
        static_cast<double>(std::clamp(role,1,IPC::kRoleCount)-1)/
            static_cast<double>(IPC::kRoleCount-1));

    int32 session=0;
    if(s.readInt32(session)){
        setParamNormalized(
            kSession,
            static_cast<double>(std::clamp(session,0,7))/7.0);
    }else{
        setParamNormalized(kSession,0.0);
    }

    return kResultOk;
}

IPlugView* PLUGIN_API Controller::createView(FIDString name){
    if(name && std::strcmp(name,ViewType::kEditor)==0)
        return new VSTGUI::VST3Editor(this,"view","Sensor.uidesc");

    return nullptr;
}

} // namespace MixDoctorator::Sensor

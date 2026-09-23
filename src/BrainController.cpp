#include "BrainController.h"
#include "BrainIDs.h"

#include "pluginterfaces/base/ustring.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "vstgui/plugin-bindings/vst3editor.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace MixDoctorator::Brain {
using namespace Steinberg;
using namespace Steinberg::Vst;

tresult PLUGIN_API Controller::initialize(FUnknown* c) {
    const auto r=EditController::initialize(c);
    if(r!=kResultOk) return r;

    constexpr int32 ro=ParameterInfo::kIsReadOnly;

    parameters.addParameter(STR16("Drums Connected"),nullptr,1,0.0,ro,kDrumsConnected);
    parameters.addParameter(STR16("Drums RMS"),STR16("dB"),0,0.0,ro,kDrumsLevel);
    parameters.addParameter(STR16("Bass Connected"),nullptr,1,0.0,ro,kBassConnected);
    parameters.addParameter(STR16("Bass RMS"),STR16("dB"),0,0.0,ro,kBassLevel);
    parameters.addParameter(STR16("Guitar Connected"),nullptr,1,0.0,ro,kGuitarConnected);
    parameters.addParameter(STR16("Guitar RMS"),STR16("dB"),0,0.0,ro,kGuitarLevel);

    parameters.addParameter(STR16("Drums Bass Overlap"),STR16("%"),0,0.0,ro,kDrumsBassOverlap);
    parameters.addParameter(STR16("Drums Bass Band"),nullptr,4,0.0,ro,kDrumsBassBand);
    parameters.addParameter(STR16("Bass Guitar Overlap"),STR16("%"),0,0.0,ro,kBassGuitarOverlap);
    parameters.addParameter(STR16("Bass Guitar Band"),nullptr,4,0.0,ro,kBassGuitarBand);
    parameters.addParameter(STR16("Drums Guitar Overlap"),STR16("%"),0,0.0,ro,kDrumsGuitarOverlap);
    parameters.addParameter(STR16("Drums Guitar Band"),nullptr,4,0.0,ro,kDrumsGuitarBand);

    return kResultOk;
}

IPlugView* PLUGIN_API Controller::createView(FIDString name) {
    if(name && std::strcmp(name,ViewType::kEditor)==0)
        return new VSTGUI::VST3Editor(this,"view","Brain.uidesc");

    return nullptr;
}

tresult PLUGIN_API Controller::getParamStringByValue(
    Steinberg::Vst::ParamID id,
    ParamValue v,
    String128 out) {

    if(id==kDrumsConnected || id==kBassConnected || id==kGuitarConnected) {
        UString128 s;
        s.fromAscii(v>=0.5 ? "CONNECTED" : "OFFLINE");
        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kDrumsLevel || id==kBassLevel || id==kGuitarLevel) {
        const double db=std::clamp(v,0.0,1.0)*60.0-60.0;
        char b[32]{};
        std::snprintf(b,sizeof(b),"%.1f dB",db);

        UString128 s;
        s.fromAscii(b);
        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kDrumsBassOverlap ||
       id==kBassGuitarOverlap ||
       id==kDrumsGuitarOverlap) {

        char b[32]{};
        std::snprintf(
            b,sizeof(b),"%.0f %%",
            std::clamp(v,0.0,1.0)*100.0);

        UString128 s;
        s.fromAscii(b);
        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kDrumsBassBand ||
       id==kBassGuitarBand ||
       id==kDrumsGuitarBand) {

        static const char* names[5] = {
            "LOW 20-120",
            "LOW-MID 120-500",
            "MID 500-2k",
            "PRESENCE 2-6k",
            "HIGH 6k+"
        };

        const int index=std::clamp(
            static_cast<int>(std::lround(std::clamp(v,0.0,1.0)*4.0)),
            0,4);

        UString128 s;
        s.fromAscii(names[index]);
        s.copyTo(out,128);
        return kResultTrue;
    }

    return EditController::getParamStringByValue(id,v,out);
}

} // namespace MixDoctorator::Brain

#include "BrainController.h"
#include "BrainIDs.h"

#include "pluginterfaces/base/ustring.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "vstgui/plugin-bindings/vst3editor.h"

#include <algorithm>
#include <cmath>
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

    parameters.addParameter(STR16("Drums Bass Observed"),STR16("%"),0,0.0,ro,kDrumsBassOverlap);
    parameters.addParameter(STR16("Drums Bass Band"),nullptr,4,0.0,ro,kDrumsBassBand);
    parameters.addParameter(STR16("Drums Bass Attention"),nullptr,4,0.0,ro,kDrumsBassStatus);

    parameters.addParameter(STR16("Bass Guitar Observed"),STR16("%"),0,0.0,ro,kBassGuitarOverlap);
    parameters.addParameter(STR16("Bass Guitar Band"),nullptr,4,0.0,ro,kBassGuitarBand);
    parameters.addParameter(STR16("Bass Guitar Attention"),nullptr,4,0.0,ro,kBassGuitarStatus);

    parameters.addParameter(STR16("Drums Guitar Observed"),STR16("%"),0,0.0,ro,kDrumsGuitarOverlap);
    parameters.addParameter(STR16("Drums Guitar Band"),nullptr,4,0.0,ro,kDrumsGuitarBand);
    parameters.addParameter(STR16("Drums Guitar Attention"),nullptr,4,0.0,ro,kDrumsGuitarStatus);

    parameters.addParameter(STR16("Top Finding Pair"),nullptr,3,0.0,ro,kTopPair);
    parameters.addParameter(STR16("Top Finding Score"),STR16("%"),0,0.0,ro,kTopScore);
    parameters.addParameter(STR16("Top Finding Band"),nullptr,4,0.0,ro,kTopBand);
    parameters.addParameter(STR16("Suggested Check"),nullptr,11,0.0,ro,kTopAdvice);

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
       id==kDrumsGuitarOverlap ||
       id==kTopScore) {

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
       id==kDrumsGuitarBand ||
       id==kTopBand) {

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

    if(id==kDrumsBassStatus ||
       id==kBassGuitarStatus ||
       id==kDrumsGuitarStatus) {

        static const char* states[5] = {
            "OBSERVING",
            "CLEAR",
            "LOW",
            "MEDIUM",
            "HIGH"
        };

        const int index=std::clamp(
            static_cast<int>(std::lround(std::clamp(v,0.0,1.0)*4.0)),
            0,4);

        UString128 s;
        s.fromAscii(states[index]);
        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kTopPair) {
        static const char* pairs[4] = {
            "NONE",
            "DRUMS - BASS",
            "BASS - E-GUITAR",
            "DRUMS - E-GUITAR"
        };

        const int index=std::clamp(
            static_cast<int>(std::lround(std::clamp(v,0.0,1.0)*3.0)),
            0,3);

        UString128 s;
        s.fromAscii(pairs[index]);
        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kTopAdvice) {
        static const char* advice[12] = {
            "Keep listening - no strong finding yet",
            "Check kick/bass separation below 120 Hz",
            "Check drum/bass buildup around 120-500 Hz",
            "Check drum attack against bass harmonics",
            "Try reducing guitar low end before raising bass",
            "Try carving guitar low-mids or bass harmonics",
            "Check bass definition against guitar body",
            "Check bass attack/harmonics against guitar presence",
            "Check guitar low end against kick/toms",
            "Check guitar body against drum low-mids",
            "Check guitar presence against snare/cymbal attack",
            "Check guitar top end against cymbals"
        };

        const int index=std::clamp(
            static_cast<int>(std::lround(std::clamp(v,0.0,1.0)*11.0)),
            0,11);

        UString128 s;
        s.fromAscii(advice[index]);
        s.copyTo(out,128);
        return kResultTrue;
    }

    return EditController::getParamStringByValue(id,v,out);
}

} // namespace MixDoctorator::Brain

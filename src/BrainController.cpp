#include "BrainController.h"
#include "BrainIDs.h"

#include "base/source/fstreamer.h"
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

tresult PLUGIN_API Controller::initialize(FUnknown* c){
    const auto r=EditController::initialize(c);

    if(r!=kResultOk)
        return r;

    constexpr int32 ro=
        ParameterInfo::kIsReadOnly;

    auto* session=new StringListParameter(
        STR16("Session"),
        kSession,
        nullptr,
        0);

    session->appendString(STR16("A"));
    session->appendString(STR16("B"));
    session->appendString(STR16("C"));
    session->appendString(STR16("D"));
    session->appendString(STR16("E"));
    session->appendString(STR16("F"));
    session->appendString(STR16("G"));
    session->appendString(STR16("H"));
    parameters.addParameter(session);

    parameters.addParameter(STR16("Drums Connected"),nullptr,1,0.0,ro,kDrumsConnected);
    parameters.addParameter(STR16("Drums RMS"),STR16("dB"),0,0.0,ro,kDrumsLevel);
    parameters.addParameter(STR16("Bass Connected"),nullptr,1,0.0,ro,kBassConnected);
    parameters.addParameter(STR16("Bass RMS"),STR16("dB"),0,0.0,ro,kBassLevel);
    parameters.addParameter(STR16("Guitar Connected"),nullptr,1,0.0,ro,kGuitarConnected);
    parameters.addParameter(STR16("Guitar RMS"),STR16("dB"),0,0.0,ro,kGuitarLevel);

    parameters.addParameter(STR16("Drums Bass Overlap"),STR16("%"),0,0.0,ro,kDrumsBassOverlap);
    parameters.addParameter(STR16("Drums Bass Masking"),STR16("%"),0,0.0,ro,kDrumsBassMasking);
    parameters.addParameter(STR16("Drums Bass Band"),nullptr,8,0.0,ro,kDrumsBassBand);
    parameters.addParameter(STR16("Drums Bass Attention"),nullptr,4,0.0,ro,kDrumsBassStatus);

    parameters.addParameter(STR16("Bass Guitar Overlap"),STR16("%"),0,0.0,ro,kBassGuitarOverlap);
    parameters.addParameter(STR16("Bass Guitar Masking"),STR16("%"),0,0.0,ro,kBassGuitarMasking);
    parameters.addParameter(STR16("Bass Guitar Band"),nullptr,8,0.0,ro,kBassGuitarBand);
    parameters.addParameter(STR16("Bass Guitar Attention"),nullptr,4,0.0,ro,kBassGuitarStatus);

    parameters.addParameter(STR16("Drums Guitar Overlap"),STR16("%"),0,0.0,ro,kDrumsGuitarOverlap);
    parameters.addParameter(STR16("Drums Guitar Masking"),STR16("%"),0,0.0,ro,kDrumsGuitarMasking);
    parameters.addParameter(STR16("Drums Guitar Band"),nullptr,8,0.0,ro,kDrumsGuitarBand);
    parameters.addParameter(STR16("Drums Guitar Attention"),nullptr,4,0.0,ro,kDrumsGuitarStatus);

    parameters.addParameter(STR16("Top Finding Pair"),nullptr,3,0.0,ro,kTopPair);
    parameters.addParameter(STR16("Top Masking Risk"),STR16("%"),0,0.0,ro,kTopScore);
    parameters.addParameter(STR16("Top Finding Band"),nullptr,8,0.0,ro,kTopBand);
    parameters.addParameter(STR16("Suggested Check"),nullptr,15,0.0,ro,kTopAdvice);
    parameters.addParameter(STR16("Dominant Source"),nullptr,2,0.5,ro,kTopDominance);
    parameters.addParameter(STR16("Confidence"),STR16("%"),0,0.0,ro,kTopConfidence);

    parameters.addParameter(STR16("Session Pair"),nullptr,3,0.0,ro,kSessionPair);
    parameters.addParameter(STR16("Session Masking Risk"),STR16("%"),0,0.0,ro,kSessionScore);
    parameters.addParameter(STR16("Session Band"),nullptr,8,0.0,ro,kSessionBand);

    return kResultOk;
}

tresult PLUGIN_API Controller::setComponentState(
    IBStream* state){

    if(!state)
        return kInvalidArgument;

    IBStreamer s(state,kLittleEndian);
    int32 session=0;

    if(s.readInt32(session)){
        setParamNormalized(
            kSession,
            static_cast<double>(
                std::clamp(session,0,7))/7.0);
    }else{
        setParamNormalized(kSession,0.0);
    }

    return kResultOk;
}

IPlugView* PLUGIN_API Controller::createView(
    FIDString name){

    if(name &&
       std::strcmp(
           name,
           ViewType::kEditor)==0)
        return new VSTGUI::VST3Editor(
            this,
            "view",
            "Brain.uidesc");

    return nullptr;
}

tresult PLUGIN_API Controller::getParamStringByValue(
    Steinberg::Vst::ParamID id,
    ParamValue v,
    String128 out){

    if(id==kDrumsConnected ||
       id==kBassConnected ||
       id==kGuitarConnected){

        UString128 s;
        s.fromAscii(
            v>=0.5
            ? "CONNECTED"
            : "OFFLINE");
        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kDrumsLevel ||
       id==kBassLevel ||
       id==kGuitarLevel){

        const double db=
            std::clamp(v,0.0,1.0)*
            60.0-60.0;

        char b[32]{};
        std::snprintf(
            b,sizeof(b),
            "%.1f dB",
            db);

        UString128 s;
        s.fromAscii(b);
        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kDrumsBassOverlap ||
       id==kDrumsBassMasking ||
       id==kBassGuitarOverlap ||
       id==kBassGuitarMasking ||
       id==kDrumsGuitarOverlap ||
       id==kDrumsGuitarMasking ||
       id==kTopScore ||
       id==kTopConfidence ||
       id==kSessionScore){

        char b[32]{};
        std::snprintf(
            b,sizeof(b),
            "%.0f %%",
            std::clamp(v,0.0,1.0)*
            100.0);

        UString128 s;
        s.fromAscii(b);
        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kDrumsBassBand ||
       id==kBassGuitarBand ||
       id==kDrumsGuitarBand ||
       id==kTopBand ||
       id==kSessionBand){

        static const char* names[9]={
            "SUB 20-80",
            "BASS 80-160",
            "LOW-MID 160-300",
            "BODY 300-600",
            "MID 600-1.2k",
            "UPPER MID 1.2-2.5k",
            "PRESENCE 2.5-5k",
            "TREBLE 5-10k",
            "AIR 10k+"
        };

        const int index=
            std::clamp(
                static_cast<int>(
                    std::lround(
                        std::clamp(
                            v,0.0,1.0)*
                        8.0)),
                0,
                8);

        UString128 s;
        s.fromAscii(names[index]);
        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kDrumsBassStatus ||
       id==kBassGuitarStatus ||
       id==kDrumsGuitarStatus){

        static const char* states[5]={
            "OBSERVING",
            "CLEAR",
            "LOW",
            "MEDIUM",
            "HIGH"
        };

        const int index=
            std::clamp(
                static_cast<int>(
                    std::lround(
                        std::clamp(
                            v,0.0,1.0)*
                        4.0)),
                0,
                4);

        UString128 s;
        s.fromAscii(states[index]);
        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kTopPair ||
       id==kSessionPair){

        static const char* pairs[4]={
            "NONE",
            "DRUMS - BASS",
            "BASS - E-GUITAR",
            "DRUMS - E-GUITAR"
        };

        const int index=
            std::clamp(
                static_cast<int>(
                    std::lround(
                        std::clamp(
                            v,0.0,1.0)*
                        3.0)),
                0,
                3);

        UString128 s;
        s.fromAscii(pairs[index]);
        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kTopDominance){

        static const char* labels[3]={
            "SECOND SOURCE",
            "BALANCED",
            "FIRST SOURCE"
        };

        const int index=
            std::clamp(
                static_cast<int>(
                    std::lround(
                        std::clamp(
                            v,0.0,1.0)*
                        2.0)),
                0,
                2);

        UString128 s;
        s.fromAscii(labels[index]);
        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kTopAdvice){

        static const char* advice[16]={
            "Keep listening - no reliable masking finding yet",

            "Drums dominate sub/bass: check kick/toms before raising bass",
            "Bass dominates sub/bass: check bass weight before raising kick",
            "Drums and bass compete in sub/bass: decide which should lead",
            "Check drum/bass buildup in low-mids and body",
            "Check drum attack against bass definition",

            "Bass dominates guitar lows: check bass body/harmonics",
            "Guitar dominates bass lows: reduce guitar low-end/body first",
            "Bass and guitar compete in lows: separate their body ranges",
            "Check bass harmonics against guitar mids/presence",
            "Check guitar top end only if bass definition is actually lost",

            "Check guitar low end against kick/toms",
            "Drums dominate mids/presence: inspect snare/cymbal emphasis",
            "Guitar dominates mids/presence: inspect guitar bite/presence",
            "Drums and guitar compete in attack/presence: create space",
            "Check cymbal/guitar treble overlap before adding more top end"
        };

        const int index=
            std::clamp(
                static_cast<int>(
                    std::lround(
                        std::clamp(
                            v,0.0,1.0)*
                        15.0)),
                0,
                15);

        UString128 s;
        s.fromAscii(advice[index]);
        s.copyTo(out,128);
        return kResultTrue;
    }

    return EditController::
        getParamStringByValue(
            id,v,out);
}

} // namespace MixDoctorator::Brain

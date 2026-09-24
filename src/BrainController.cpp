#include "BrainController.h"
#include "BrainIDs.h"
#include "RoleModel.h"

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
    parameters.addParameter(STR16("Top Masking Index"),STR16("%"),0,0.0,ro,kTopScore);
    parameters.addParameter(STR16("Top Finding Band"),nullptr,8,0.0,ro,kTopBand);
    parameters.addParameter(STR16("Suggested Check"),nullptr,15,0.0,ro,kTopAdvice);
    parameters.addParameter(STR16("Dominant Source"),nullptr,2,0.5,ro,kTopDominance);
    parameters.addParameter(STR16("Confidence"),STR16("%"),0,0.0,ro,kTopConfidence);

    parameters.addParameter(STR16("Session Pair"),nullptr,3,0.0,ro,kSessionPair);
    parameters.addParameter(STR16("Session Masking Index"),STR16("%"),0,0.0,ro,kSessionScore);
    parameters.addParameter(STR16("Session Band"),nullptr,8,0.0,ro,kSessionBand);

    parameters.addParameter(STR16("Drums Transient"),STR16("%"),0,0.0,ro,kDrumsTransient);
    parameters.addParameter(STR16("Bass Transient"),STR16("%"),0,0.0,ro,kBassTransient);
    parameters.addParameter(STR16("Guitar Transient"),STR16("%"),0,0.0,ro,kGuitarTransient);

    parameters.addParameter(STR16("Drums Sensor Count"),nullptr,0,0.0,ro,kDrumsCount);
    parameters.addParameter(STR16("Bass Sensor Count"),nullptr,0,0.0,ro,kBassCount);
    parameters.addParameter(STR16("Guitar Sensor Count"),nullptr,0,0.0,ro,kGuitarCount);

    parameters.addParameter(STR16("Drums Bass Transient Competition"),STR16("%"),0,0.0,ro,kDrumsBassTransientCompetition);
    parameters.addParameter(STR16("Bass Guitar Transient Competition"),STR16("%"),0,0.0,ro,kBassGuitarTransientCompetition);
    parameters.addParameter(STR16("Drums Guitar Transient Competition"),STR16("%"),0,0.0,ro,kDrumsGuitarTransientCompetition);

    parameters.addParameter(STR16("Top Attack Pair"),nullptr,3,0.0,ro,kTopAttackPair);
    parameters.addParameter(STR16("Top Attack Index"),STR16("%"),0,0.0,ro,kTopAttackScore);
    parameters.addParameter(STR16("Top Attack Advice"),nullptr,3,0.0,ro,kTopAttackAdvice);

    parameters.addParameter(STR16("Coach Headline"),nullptr,6,0.0,ro,kCoachHeadline);
    parameters.addParameter(STR16("Coach Action"),nullptr,6,0.0,ro,kCoachAction);
    parameters.addParameter(STR16("Coach Listen"),nullptr,6,0.0,ro,kCoachListen);
    parameters.addParameter(STR16("Coach Reason"),nullptr,6,0.0,ro,kCoachReason);
    parameters.addParameter(STR16("Coach Evidence"),nullptr,3,0.0,ro,kCoachEvidence);
    parameters.addParameter(STR16("Coach Pair"),nullptr,Analysis::kRolePairCount,0.0,ro,kCoachPair);
    parameters.addParameter(STR16("Coach Band"),nullptr,8,0.0,ro,kCoachBand);
    parameters.addParameter(STR16("Coach Attack Pair"),nullptr,Analysis::kRolePairCount,0.0,ro,kCoachAttackPair);
    parameters.addParameter(STR16("Coach Attack Advice"),nullptr,1,0.0,ro,kCoachAttackAdvice);

    // UIViewSwitchContainer is driven by a real controller parameter. Leaving
    // the tag unbound creates a null-parameter listener path in VST3Editor
    // when the button ends its edit, which is unsafe in some hosts.
    parameters.addParameter(
        STR16("View Mode"),
        nullptr,
        1,
        0.0,
        ParameterInfo::kIsHidden,
        kViewMode);

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

    if(id==kDrumsCount ||
       id==kBassCount ||
       id==kGuitarCount){

        const int count=
            std::clamp(
                static_cast<int>(
                    std::lround(
                        std::clamp(v,0.0,1.0)*
                        24.0)),
                0,
                24);

        char b[32]{};
        std::snprintf(
            b,sizeof(b),
            "%d",
            count);

        UString128 s;
        s.fromAscii(b);
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
       id==kSessionScore ||
       id==kDrumsTransient ||
       id==kBassTransient ||
       id==kGuitarTransient ||
       id==kDrumsBassTransientCompetition ||
       id==kBassGuitarTransientCompetition ||
       id==kDrumsGuitarTransientCompetition ||
       id==kTopAttackScore){

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
       id==kSessionBand ||
       id==kCoachBand){

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
       id==kSessionPair ||
       id==kTopAttackPair){

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

    if(id==kTopAttackAdvice){

        static const char* advice[4]={
            "No sustained attack competition detected",
            "Drums and bass attacks coincide: check envelopes, timing or ducking",
            "Bass and guitar attacks coincide: check articulation and transient emphasis",
            "Drums and guitar attacks coincide: check pick/snare/cymbal attack space"
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
        s.fromAscii(advice[index]);
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

    if(id==kCoachHeadline ||
       id==kCoachAction ||
       id==kCoachListen ||
       id==kCoachReason){

        const int index=
            std::clamp(
                static_cast<int>(
                    std::lround(
                        std::clamp(
                            v,0.0,1.0)*
                        6.0)),
                0,
                6);

        static const char* headline[7]={
            "No reliable mix problem yet",
            "Two sources may be competing for low-end ownership",
            "Low-mid buildup may be reducing separation",
            "Midrange overlap may be blurring separation",
            "Presence overlap may be masking definition",
            "Upper-frequency overlap may be crowding the mix",
            "Two sources may be competing in their attacks"
        };

        static const char* action[7]={
            "Keep the mix playing. Do not change anything until the finding becomes stable.",
            "Decide which shown source should own this low range. On the other source, try a gentle cut or high-pass only if the mix improves.",
            "Compare the two shown sources in this range. Try a small 1-2 dB cut on the muddier one first.",
            "Choose which shown source needs to stay clearer here. Try a small 1-2 dB cut on the less important source.",
            "Before boosting presence, try a small cut on the shown source that can give up a little definition in this range.",
            "Compare the two shown sources and reduce the harsher or less important one by about 1-2 dB first.",
            "Check envelope, transient emphasis, timing or gentle ducking between the two shown sources before reaching for large EQ moves."
        };

        static const char* listen[7]={
            "Wait for a stable finding. A moving value alone is not a reason to change the mix.",
            "Listen for clearer low-end roles without losing weight or punch. Undo the change if the bottom becomes thin.",
            "Listen for less mud and clearer notes without making either source hollow.",
            "Listen for two distinct parts instead of one blurred midrange block. Undo the change if either source loses character.",
            "Listen for clearer definition without making the mix dull or pushing the other source too far forward.",
            "Listen for less harshness or crowding while keeping useful brightness and air.",
            "Listen for clearer attacks and groove. If the mix only gets weaker or unnatural, undo the change."
        };

        static const char* reason[7]={
            "No sufficiently stable conflict has been measured yet.",
            "The two shown sources overlap most strongly in the sub or bass range.",
            "The two shown sources overlap most strongly in the low-mid or body range.",
            "The two shown sources overlap most strongly in the midrange.",
            "The two shown sources overlap most strongly in the presence range.",
            "The two shown sources overlap most strongly in the treble or air range.",
            "The measured overlap also contains sustained transient competition."
        };

        const char* text=
            id==kCoachHeadline
            ? headline[index]
            : id==kCoachAction
                ? action[index]
                : id==kCoachListen
                    ? listen[index]
                    : reason[index];

        UString128 s;
        s.fromAscii(text);
        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kCoachEvidence){
        static const char* evidence[4]={
            "WAITING",
            "EARLY HINT",
            "STABLE HINT",
            "STRONG HINT"
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
        s.fromAscii(evidence[index]);
        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kCoachPair ||
       id==kCoachAttackPair){
        const int encoded=
            std::clamp(
                static_cast<int>(
                    std::lround(
                        std::clamp(v,0.0,1.0)*
                        static_cast<double>(
                            Analysis::kRolePairCount))),
                0,
                Analysis::kRolePairCount);

        if(encoded==0){
            UString128 s;
            s.fromAscii("NONE");
            s.copyTo(out,128);
            return kResultTrue;
        }

        IPC::Role first=IPC::Role::Unknown;
        IPC::Role second=IPC::Role::Unknown;

        char b[96]{};

        if(Analysis::decodeRolePair(
               encoded-1,
               first,
               second)){
            Analysis::orderRolePairForDisplay(
                first,
                second);
            std::snprintf(
                b,sizeof(b),
                "%s - %s",
                Analysis::roleName(first),
                Analysis::roleName(second));
        }else{
            std::snprintf(
                b,sizeof(b),
                "NONE");
        }

        UString128 s;
        s.fromAscii(b);
        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kCoachAttackAdvice){
        UString128 s;
        s.fromAscii(
            v>=0.5
            ? "Check timing, envelopes, transient emphasis or gentle ducking between these sources"
            : "No sustained attack competition detected");
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

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

    parameters.addParameter(STR16("Coach Headline"),nullptr,15,0.0,ro,kCoachHeadline);
    parameters.addParameter(STR16("Coach Action"),nullptr,15,0.0,ro,kCoachAction);
    parameters.addParameter(STR16("Coach Listen"),nullptr,15,0.0,ro,kCoachListen);
    parameters.addParameter(STR16("Coach Reason"),nullptr,15,0.0,ro,kCoachReason);
    parameters.addParameter(STR16("Coach Evidence"),nullptr,3,0.0,ro,kCoachEvidence);

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
                        15.0)),
                0,
                15);

        static const char* headline[16]={
            "No reliable masking problem yet",
            "Drums and bass may be fighting for the deepest lows",
            "Bass may be covering the drum low end",
            "Drums and bass share too much of the low end",
            "Drums and bass may be building up in the low-mids",
            "Drum attack may be hiding bass definition",
            "Bass may be too strong below the guitars",
            "Guitars may be too heavy below the bass",
            "Bass and guitars share too much low-end space",
            "Bass harmonics may be masking guitar definition",
            "Guitar top end may be masking bass articulation",
            "Guitar low end may be crowding kick or toms",
            "Drums may be covering guitar mids or presence",
            "Guitars may be covering drum mids or presence",
            "Drums and guitars may be competing for presence",
            "Cymbals and guitars may be sharing too much top end"
        };

        static const char* action[16]={
            "Keep the mix playing. Do not change EQ just to make a meter move.",
            "Open the drum EQ. In the shown range, try a small 1-2 dB cut only if the bass becomes clearer.",
            "Open the bass EQ. In the shown range, try a small 1-2 dB cut only if the kick becomes clearer.",
            "Choose which source should own the shown range, then try a gentle 1-2 dB cut on the other source.",
            "Compare both sources in the shown range. Start with a 1-2 dB cut on the muddier one.",
            "Check the shown range on drums first. Reduce only enough to reveal bass definition.",
            "Check the bass in the shown range. Try a gentle 1-2 dB cut before boosting the guitars.",
            "Check the guitars in the shown range. Try a gentle low-cut or 1-2 dB reduction first.",
            "Choose whether bass or guitars should lead in the shown range, then trim the other source gently.",
            "Check bass harmonics in the shown range. Try a 1-2 dB cut before adding more guitar presence.",
            "Do not boost the bass first. Trim the guitars slightly in the shown range and compare in the full mix.",
            "Check the guitars first. Remove only unnecessary low end in the shown range.",
            "Check drum or cymbal emphasis in the shown range. Try a small cut before boosting the guitars.",
            "Check guitar bite in the shown range. Try a small cut before making the drums louder.",
            "Choose the more important attack source, then make a small cut in the shown range on the other source.",
            "Compare cymbals and guitars in the shown range. Reduce the harsher source by about 1-2 dB first."
        };

        static const char* listen[16]={
            "Wait for a stable finding. A moving value alone is not a reason to change the mix.",
            "Keep it only if bass notes become clearer without making the drums weak.",
            "Keep it only if the kick is easier to hear without making the bass thin.",
            "Listen for separation and punch. If the low end loses weight, undo the change.",
            "Listen for less mud and clearer notes without making either source hollow.",
            "Listen for clearer bass notes while the drums still keep their attack.",
            "Listen for clearer guitars without losing the weight the bass should provide.",
            "Listen for clearer bass while the guitars still sound full enough.",
            "Listen for two distinct roles instead of one thick low-end block.",
            "Listen for clearer guitar notes without making the bass disappear.",
            "Listen for clearer bass articulation without making the guitars dull.",
            "Listen for cleaner kick and tom impact while the guitars stay powerful.",
            "Listen for clearer guitars while the drums still sound natural.",
            "Listen for clearer drums without making the guitars lose their character.",
            "Listen for clearer attacks. If both sources just get thinner, undo the change.",
            "Listen for less harshness and better separation without losing useful brightness."
        };

        static const char* reason[16]={
            "No stable conflict has been measured yet.",
            "Drums and bass overlap mainly in the deepest low range.",
            "Bass energy is stronger than the drums in the measured low range.",
            "Drums and bass are similarly strong in the measured low range.",
            "Drums and bass overlap mainly in the low-mid or body range.",
            "Drum and bass interaction is strongest around definition and attack.",
            "Bass energy is stronger than guitar energy in the measured low range.",
            "Guitar energy is stronger than bass energy in the measured low range.",
            "Bass and guitars are similarly strong in the measured low range.",
            "Bass and guitars overlap mainly through body, mids or harmonics.",
            "Bass articulation and guitar top end overlap in the measured range.",
            "Guitar low end overlaps with kick or tom energy.",
            "Drum energy is stronger in the measured mid or presence range.",
            "Guitar energy is stronger in the measured mid or presence range.",
            "Drums and guitars are similarly strong in the measured presence range.",
            "Cymbal and guitar energy overlap mainly in the upper range."
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

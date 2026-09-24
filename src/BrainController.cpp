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
    parameters.addParameter(STR16("Coach Target"),nullptr,IPC::kRoleCount,0.0,ro,kCoachTarget);
    parameters.addParameter(STR16("Coach Attack Pair"),nullptr,Analysis::kRolePairCount,0.0,ro,kCoachAttackPair);
    parameters.addParameter(STR16("Coach Attack Advice"),nullptr,1,0.0,ro,kCoachAttackAdvice);
    parameters.addParameter(STR16("All Sensor Count"),nullptr,0,0.0,ro,kAllSensorCount);
    parameters.addParameter(STR16("All Role Count"),nullptr,0,0.0,ro,kAllRoleCount);

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
            ? "VERBUNDEN"
            : "OFFLINE");
        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kAllSensorCount ||
       id==kAllRoleCount){

        const int maximum=
            id==kAllSensorCount
            ? IPC::kSensorSlotCount
            : IPC::kRoleCount;

        const int count=
            std::clamp(
                static_cast<int>(
                    std::lround(
                        std::clamp(v,0.0,1.0)*
                        static_cast<double>(
                            maximum))),
                0,
                maximum);

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
            "TIEFE MITTEN 160-300",
            "KOERPER 300-600",
            "MITTEN 600-1.2k",
            "OBERE MITTEN 1.2-2.5k",
            "PRAESENZ 2.5-5k",
            "HOEHEN 5-10k",
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
            "BEOBACHTEN",
            "KLAR",
            "NIEDRIG",
            "MITTEL",
            "HOCH"
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
            "KEINE",
            "SCHLAGZEUG - BASS",
            "BASS - E-GITARRE",
            "SCHLAGZEUG - E-GITARRE"
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
            "Keine anhaltende Attack-Konkurrenz erkannt",
            "Schlagzeug und Bass treffen gleichzeitig: Huellkurven, Timing oder Ducking pruefen",
            "Bass und Gitarre treffen gleichzeitig: Artikulation und Transienten pruefen",
            "Schlagzeug und Gitarre konkurrieren bei Attack: Anschlag, Snare und Becken pruefen"
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
            "ZWEITE QUELLE",
            "AUSGEGLICHEN",
            "ERSTE QUELLE"
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
            "Noch kein verlaessliches Mix-Problem erkannt",
            "Zwei Quellen konkurrieren moeglicherweise um den Tiefbass",
            "Tiefe Mitten koennen die Trennung verschlechtern",
            "Mitten-Ueberlappung kann die Trennung verwischen",
            "Praesenz-Ueberlappung kann Definition verdecken",
            "Hoehen-Ueberlappung kann den Mix verdichten",
            "Zwei Quellen konkurrieren moeglicherweise in den Attacks"
        };

        static const char* listen[7]={
            "Warte auf einen stabilen Hinweis. Ein bewegter Messwert allein ist kein Grund, den Mix zu aendern.",
            "Achte auf klarere Bassrollen ohne Gewicht oder Punch zu verlieren. Wird es duenn, Aenderung rueckgaengig.",
            "Achte auf weniger Matsch und klarere Noten, ohne dass eine Quelle hohl klingt.",
            "Achte auf getrennte Parts statt verschwommener Mitten. Verliert eine Quelle Charakter, rueckgaengig machen.",
            "Achte auf mehr Definition, ohne den Mix dumpf zu machen oder die andere Quelle zu weit nach vorn zu holen.",
            "Achte auf weniger Haerte oder Gedraenge, ohne nuetzliche Helligkeit und Air zu verlieren.",
            "Achte auf klarere Attacks und Groove. Wird der Mix schwaecher oder unnatuerlich, rueckgaengig machen."
        };

        static const char* reason[7]={
            "Noch kein ausreichend stabiler Konflikt gemessen.",
            "Die beiden Quellen ueberlappen sich am staerksten im Sub- oder Bassbereich.",
            "Die beiden Quellen ueberlappen sich am staerksten in tiefen Mitten oder Koerperbereich.",
            "Die beiden Quellen ueberlappen sich am staerksten in den Mitten.",
            "Die beiden Quellen ueberlappen sich am staerksten im Praesenzbereich.",
            "Die beiden Quellen ueberlappen sich am staerksten in Hoehen oder Air.",
            "Die gemessene Ueberlappung enthaelt zusaetzlich anhaltende Transienten-Konkurrenz."
        };

        const char* text=
            id==kCoachHeadline
            ? headline[index]
            : id==kCoachListen
                ? listen[index]
                : reason[index];

        UString128 s;
        s.fromAscii(text);
        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kCoachAction){
        const int index=
            std::clamp(
                static_cast<int>(
                    std::lround(
                        std::clamp(
                            v,0.0,1.0)*
                        6.0)),
                0,
                6);

        static const char* action[7]={
            "Mix weiterlaufen lassen. Erst etwas aendern, wenn der Hinweis stabil wird.",
            "Beide Quellen im Bereich vergleichen. Bei der stoerenderen Quelle vorsichtig Platz schaffen und im Mix gegenhoeren.",
            "Kick und Bass getrennt beurteilen. Festlegen, wer die Tiefe traegt; bei der anderen Quelle vorsichtig Platz schaffen.",
            "Gesang als Bezugspunkt. Bei Synth, Keys, Pad oder Gitarre pruefen, ob eine kleine Absenkung die Stimme klaert.",
            "Becken und andere Quelle vergleichen. Nicht automatisch Hoehen wegnehmen; zuerst die dominantere Quelle pruefen.",
            "Attack- und harmonische Quelle getrennt pruefen: Timing, Huellkurve oder kleine Entzerrung vor grossen EQ-Eingriffen.",
            "Bei Bass gegen eine harmonische Quelle zuerst unnoetige Tiefen der anderen Quelle pruefen."
        };

        UString128 s;
        s.fromAscii(action[index]);
        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kCoachEvidence){
        static const char* evidence[4]={
            "WARTEN",
            "ERSTER HINWEIS",
            "STABILER HINWEIS",
            "STARKER HINWEIS"
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
            s.fromAscii("KEINE");
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
                "KEINE");
        }

        UString128 s;
        s.fromAscii(b);
        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kCoachTarget){
        const int encoded=
            std::clamp(
                static_cast<int>(
                    std::lround(
                        std::clamp(v,0.0,1.0)*
                        static_cast<double>(
                            IPC::kRoleCount))),
                0,
                IPC::kRoleCount);

        UString128 s;

        if(encoded<=0){
            s.fromAscii("BEIDE VERGLEICHEN");
        }else{
            const auto role=
                static_cast<IPC::Role>(
                    encoded);

            char b[96]{};
            std::snprintf(
                b,sizeof(b),
                "ZUERST %s PRUEFEN",
                Analysis::roleName(role));

            s.fromAscii(b);
        }

        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kCoachAttackAdvice){
        UString128 s;
        s.fromAscii(
            v>=0.5
            ? "Timing, Huellkurven, Transienten oder sanftes Ducking zwischen diesen Quellen pruefen"
            : "Keine anhaltende Attack-Konkurrenz erkannt");
        s.copyTo(out,128);
        return kResultTrue;
    }

    if(id==kTopAdvice){

        static const char* advice[16]={
            "Weiterhoeren - noch kein verlaesslicher Masking-Hinweis",

            "Schlagzeug dominiert Sub/Bass: Kick/Toms pruefen, bevor Bass angehoben wird",
            "Bass dominiert Sub/Bass: Bassgewicht pruefen, bevor Kick angehoben wird",
            "Schlagzeug und Bass konkurrieren unten: entscheiden, wer fuehren soll",
            "Schlagzeug/Bass-Aufbau in tiefen Mitten und Koerper pruefen",
            "Schlagzeug-Attack gegen Bassdefinition pruefen",

            "Bass dominiert Gitarrentiefe: Bass-Koerper und Obertone pruefen",
            "Gitarre dominiert Basstiefe: zuerst Gitarren-Tiefen/Koerper pruefen",
            "Bass und Gitarre konkurrieren unten: Koerperbereiche trennen",
            "Bass-Obertone gegen Gitarren-Mitten/Praesenz pruefen",
            "Gitarren-Hoehen nur pruefen, wenn Bassdefinition wirklich verloren geht",

            "Gitarren-Tiefen gegen Kick/Toms pruefen",
            "Schlagzeug dominiert Mitten/Praesenz: Snare/Becken pruefen",
            "Gitarre dominiert Mitten/Praesenz: Biss/Praesenz pruefen",
            "Schlagzeug und Gitarre konkurrieren bei Attack/Praesenz: Platz schaffen",
            "Becken/Gitarren-Hoehen pruefen, bevor mehr Hoehen angehoben werden"
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

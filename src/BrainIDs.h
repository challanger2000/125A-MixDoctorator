#pragma once
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace MixDoctorator::Brain {

static const Steinberg::FUID kProcessorUID(
    0xA125D011,0x10114A01,0xB0010011,0x125A0011);

static const Steinberg::FUID kControllerUID(
    0xA125D012,0x10114A01,0xB0010011,0x125A0012);

enum ParamID : Steinberg::Vst::ParamID {
    kSession=199,

    kDrumsConnected=200,
    kDrumsLevel,
    kBassConnected,
    kBassLevel,
    kGuitarConnected,
    kGuitarLevel,

    kDrumsBassOverlap,
    kDrumsBassMasking,
    kDrumsBassBand,
    kDrumsBassStatus,

    kBassGuitarOverlap,
    kBassGuitarMasking,
    kBassGuitarBand,
    kBassGuitarStatus,

    kDrumsGuitarOverlap,
    kDrumsGuitarMasking,
    kDrumsGuitarBand,
    kDrumsGuitarStatus,

    kTopPair,
    kTopScore,
    kTopBand,
    kTopAdvice,
    kTopDominance,
    kTopConfidence,

    kSessionPair,
    kSessionScore,
    kSessionBand,

    kDrumsTransient,
    kBassTransient,
    kGuitarTransient,

    kDrumsCount,
    kBassCount,
    kGuitarCount,

    kDrumsBassTransientCompetition,
    kBassGuitarTransientCompetition,
    kDrumsGuitarTransientCompetition,

    kTopAttackPair,
    kTopAttackScore,
    kTopAttackAdvice,

    kCoachHeadline,
    kCoachAction,
    kCoachListen,
    kCoachReason,
    kCoachEvidence,
    kCoachPair,
    kCoachBand,
    kCoachTarget,
    kCoachAttackPair,
    kCoachAttackAdvice,
    kAllSensorCount,
    kAllRoleCount,
    kCoachMasking,
    kCoachDominance,
    kCoachConfidence,

    // Controller/UI-only parameter used by the details toggle.
    kViewMode=9000
};

} // namespace MixDoctorator::Brain

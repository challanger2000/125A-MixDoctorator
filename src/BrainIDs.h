#pragma once
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace MixDoctorator::Brain {

static const Steinberg::FUID kProcessorUID(
    0xA125D011,0x10114A01,0xB0010011,0x125A0011);

static const Steinberg::FUID kControllerUID(
    0xA125D012,0x10114A01,0xB0010011,0x125A0012);

enum ParamID : Steinberg::Vst::ParamID {
    kDrumsConnected=200,
    kDrumsLevel,
    kBassConnected,
    kBassLevel,
    kGuitarConnected,
    kGuitarLevel,

    kDrumsBassOverlap,
    kDrumsBassBand,
    kDrumsBassStatus,

    kBassGuitarOverlap,
    kBassGuitarBand,
    kBassGuitarStatus,

    kDrumsGuitarOverlap,
    kDrumsGuitarBand,
    kDrumsGuitarStatus,

    kTopPair,
    kTopScore,
    kTopBand,
    kTopAdvice,
    kTopDominance,
    kTopConfidence
};

} // namespace MixDoctorator::Brain

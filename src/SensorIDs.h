#pragma once
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"
namespace MixDoctorator::Sensor {
static const Steinberg::FUID kProcessorUID(0xA125D001,0x10114A01,0xB0010001,0x125A0001);
static const Steinberg::FUID kControllerUID(0xA125D002,0x10114A01,0xB0010001,0x125A0002);
enum ParamID : Steinberg::Vst::ParamID { kRole=100 };
}

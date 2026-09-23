#include "SensorProcessor.h"
#include "SensorController.h"
#include "SensorIDs.h"
#include "public.sdk/source/main/pluginfactory.h"
using namespace Steinberg; using namespace Steinberg::Vst;
BEGIN_FACTORY_DEF("125A","https://github.com/challanger2000/125A-MixDoctorator","")
DEF_CLASS2(INLINE_UID_FROM_FUID(MixDoctorator::Sensor::kProcessorUID),PClassInfo::kManyInstances,kVstAudioEffectClass,
 "125A MixDoctorator Sensor",Vst::kDistributable,PlugType::kFxAnalyzer,"0.0.1",kVstVersionString,
 MixDoctorator::Sensor::Processor::createInstance)
DEF_CLASS2(INLINE_UID_FROM_FUID(MixDoctorator::Sensor::kControllerUID),PClassInfo::kManyInstances,kVstComponentControllerClass,
 "125A MixDoctorator Sensor Controller",0,"","0.0.1",kVstVersionString,
 MixDoctorator::Sensor::Controller::createInstance)
END_FACTORY

#include "BrainProcessor.h"
#include "BrainController.h"
#include "BrainIDs.h"
#include "public.sdk/source/main/pluginfactory.h"
using namespace Steinberg; using namespace Steinberg::Vst;
BEGIN_FACTORY_DEF("125A","https://github.com/challanger2000/125A-MixDoctorator","")
DEF_CLASS2(INLINE_UID_FROM_FUID(MixDoctorator::Brain::kProcessorUID),PClassInfo::kManyInstances,kVstAudioEffectClass,
 "125A MixDoctorator Brain",Vst::kDistributable,PlugType::kFxAnalyzer,"0.0.1",kVstVersionString,
 MixDoctorator::Brain::Processor::createInstance)
DEF_CLASS2(INLINE_UID_FROM_FUID(MixDoctorator::Brain::kControllerUID),PClassInfo::kManyInstances,kVstComponentControllerClass,
 "125A MixDoctorator Brain Controller",0,"","0.0.1",kVstVersionString,
 MixDoctorator::Brain::Controller::createInstance)
END_FACTORY

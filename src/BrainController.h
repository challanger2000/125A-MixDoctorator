#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"
namespace MixDoctorator::Brain {
class Controller final : public Steinberg::Vst::EditController {
public:
    static Steinberg::FUnknown* createInstance(void*){ return static_cast<Steinberg::Vst::IEditController*>(new Controller()); }
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown*) override;
    Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString) override;
    Steinberg::tresult PLUGIN_API getParamStringByValue(Steinberg::Vst::ParamID,Steinberg::Vst::ParamValue,Steinberg::Vst::String128) override;
};
}

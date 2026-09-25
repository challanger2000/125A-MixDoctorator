#include "SensorProcessor.h"
#include "SensorIDs.h"
#include "AudioSafety.h"
#include "SensorGenerationPolicy.h"
#include "RuntimeInstanceId.h"
#include "StateValueModel.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/vstspeaker.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <chrono>
#include <thread>
#include <type_traits>

namespace MixDoctorator::Sensor {
static_assert(
    Analysis::SpectralAnalyzer::kBandCount==IPC::kBandCount,
    "Sensor spectral band count must match IPC");

using namespace Steinberg;
using namespace Steinberg::Vst;


Processor::Processor(){
    setControllerClass(
        kControllerUID);
}

Processor::~Processor(){
    stopIpcWorker();
    releaseIpcSlots();
}

tresult PLUGIN_API Processor::initialize(
    FUnknown* c){

    const auto r=
        AudioEffect::initialize(c);

    if(r!=kResultOk)
        return r;

    addAudioInput(
        STR16("Stereo In"),
        SpeakerArr::kStereo);

    addAudioOutput(
        STR16("Stereo Out"),
        SpeakerArr::kStereo);

    ipcReady_=ipc_.open();

    if(instanceId_==0)
        instanceId_=
            Analysis::makeRuntimeInstanceId();

    if(ipcReady_)
        startIpcWorker();

    return kResultOk;
}

tresult PLUGIN_API Processor::terminate(){
    stopIpcWorker();
    releaseIpcSlots();
    ipc_.close();
    ipcReady_=false;
    return AudioEffect::terminate();
}

void Processor::resetAnalysisMeters() noexcept{
    analyzerLeft_.reset();
    analyzerRight_.reset();
    transientDetector_.reset();
}

void Processor::releaseIpcSlots() noexcept{
    if(!ipcReady_ || instanceId_==0)
        return;

    for(int session=0;
        session<IPC::kSessionCount;
        ++session){

        for(int attempt=0;
            attempt<4 &&
            cachedSlot_[session]>=0;
            ++attempt){

            if(ipc_.release(
                   session,
                   instanceId_,
                   cachedSlot_[session]))
                break;

            std::this_thread::yield();
        }
    }

    lastPublishedSession_=-1;
    lastPublishedGeneration_=0;
}

void Processor::startIpcWorker(){
    if(ipcWorkerRunning_.exchange(
           true,
           std::memory_order_acq_rel))
        return;

    try{
        ipcWorker_=
            std::thread(
                [this]{
                    ipcWorkerLoop();
                });
    }catch(...){
        ipcWorkerRunning_.store(
            false,
            std::memory_order_release);
        ipcReady_=false;
    }
}

void Processor::stopIpcWorker() noexcept{
    ipcWorkerRunning_.store(
        false,
        std::memory_order_release);

    if(ipcWorker_.joinable())
        ipcWorker_.join();
}

void Processor::ipcWorkerLoop() noexcept{
    while(ipcWorkerRunning_.load(
              std::memory_order_acquire)){

        const auto currentGeneration=
            configGeneration_.load(
                std::memory_order_acquire);

        if(Analysis::sensorPublicationNeedsRelease(
               lastPublishedSession_,
               lastPublishedGeneration_,
               currentGeneration)){

            if(!ipc_.release(
                   lastPublishedSession_,
                   instanceId_,
                   cachedSlot_[
                       lastPublishedSession_])){
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(1));
                continue;
            }

            lastPublishedSession_=-1;
            lastPublishedGeneration_=0;
        }

        AnalysisPacket newest;

        if(!ipcQueue_.drainNewest(newest)){
            std::this_thread::sleep_for(
                std::chrono::milliseconds(1));
            continue;
        }

        const auto generation=
            configGeneration_.load(
                std::memory_order_acquire);

        if(!Analysis::sensorPacketGenerationCurrent(
               newest.generation,
               generation))
            continue;

        const int publishSession=
            IPC::clampSession(
                newest.session);

        if(lastPublishedSession_>=0 &&
           lastPublishedSession_!=publishSession){

            if(!ipc_.release(
                   lastPublishedSession_,
                   instanceId_,
                   cachedSlot_[
                       lastPublishedSession_]))
                continue;

            lastPublishedSession_=-1;
            lastPublishedGeneration_=0;
        }

        const bool published=
            ipc_.publish(
                publishSession,
                instanceId_,
                cachedSlot_[
                    publishSession],
                newest.role,
                newest.samplePosition,
                newest.rmsDb,
                newest.peakDb,
                newest.activity,
                newest.transient,
                newest.bands);

        if(published){
            lastPublishedSession_=
                publishSession;
            lastPublishedGeneration_=
                newest.generation;
        }
    }
}

tresult PLUGIN_API Processor::setBusArrangements(
    SpeakerArrangement* in,
    int32 ni,
    SpeakerArrangement* out,
    int32 no){

    if(ni==1 &&
       no==1 &&
       ((in[0]==SpeakerArr::kStereo &&
         out[0]==SpeakerArr::kStereo) ||
        (in[0]==SpeakerArr::kMono &&
         out[0]==SpeakerArr::kMono)))
        return AudioEffect::
            setBusArrangements(
                in,ni,out,no);

    return kResultFalse;
}

tresult PLUGIN_API Processor::canProcessSampleSize(
    int32 s){

    return
        (s==kSample32 ||
         s==kSample64)
        ? kResultTrue
        : kResultFalse;
}

tresult PLUGIN_API Processor::setupProcessing(
    ProcessSetup& setup){

    sampleRate_=
        (std::isfinite(setup.sampleRate) &&
         setup.sampleRate>8000.0)
        ? setup.sampleRate
        : 44100.0;

    analyzerLeft_.prepare(
        sampleRate_);

    analyzerRight_.prepare(
        sampleRate_);

    transientDetector_.prepare(
        sampleRate_);

    return AudioEffect::
        setupProcessing(setup);
}

tresult PLUGIN_API Processor::setProcessing(
    TBool state){

    if(state){
        analyzerLeft_.prepare(
            sampleRate_);

        analyzerRight_.prepare(
            sampleRate_);

        transientDetector_.prepare(
            sampleRate_);
    }else{
        // Invalidate the last publication even if the host keeps the plugin
        // instance alive. The worker notices the generation change and
        // releases the Sensor slot without waiting for heartbeat expiry.
        configGeneration_.fetch_add(
            1,
            std::memory_order_acq_rel);
    }

    AudioEffect::
        setProcessing(state);

    return kResultTrue;
}

void Processor::readParameters(
    IParameterChanges* changes){

    if(!changes)
        return;

    for(int32 i=0;
        i<changes->getParameterCount();
        ++i){

        auto* q=
            changes->
            getParameterData(i);

        if(!q || q->getPointCount()<=0)
            continue;

        int32 off=0;
        ParamValue v=0.0;

        if(q->getPoint(
               q->getPointCount()-1,
               off,
               v)!=kResultTrue)
            continue;

        if(q->getParameterId()==kRole){
            const auto previousRole=
                role_;

            role_=
                Analysis::decodeRoleNormalized(
                    v);

            if(role_!=previousRole){
                resetAnalysisMeters();
                configGeneration_.fetch_add(
                    1,
                    std::memory_order_acq_rel);
            }
        }else if(q->getParameterId()==kSession){
            const int previousSession=
                session_;

            session_=
                Analysis::decodeSessionNormalized(
                    v);

            if(session_!=previousSession)
                configGeneration_.fetch_add(
                    1,
                    std::memory_order_acq_rel);
        }
    }
}

template<typename T>
static void copyMeasure(
    AudioBusBuffers& input,
    AudioBusBuffers& output,
    int32 n,
    double& sumSq,
    double& peak,
    Processor* self){

    T** in=nullptr;
    T** out=nullptr;

    if constexpr(
        std::is_same_v<T,float>){
        in=input.channelBuffers32;
        out=output.channelBuffers32;
    }else{
        in=input.channelBuffers64;
        out=output.channelBuffers64;
    }

    for(int32 i=0;
        i<n;
        ++i){

        const T* leftBuffer=
            (in && input.numChannels>0)
            ? in[0]
            : nullptr;

        const T* rightBuffer=
            (in && input.numChannels>1)
            ? in[1]
            : nullptr;

        const double left=
            Analysis::readAudioSample(
                leftBuffer,
                i);

        const double right=
            input.numChannels>1
            ? Analysis::readAudioSample(
                rightBuffer,
                i)
            : left;

        if(out &&
           output.numChannels>0 &&
           out[0])
            out[0][i]=
                static_cast<T>(left);

        if(out &&
           output.numChannels>1 &&
           out[1])
            out[1][i]=
                static_cast<T>(right);

        peak=
            std::max(
                peak,
                std::max(
                    std::abs(left),
                    std::abs(right)));

        sumSq+=
            left*left+
            right*right;

        self->analyzeStereoSample(
            left,
            right);
    }
}

tresult PLUGIN_API Processor::process(
    ProcessData& data){

    readParameters(
        data.inputParameterChanges);

    if(data.numInputs<1 ||
       data.numOutputs<1 ||
       data.numSamples<=0)
        return kResultOk;

    double sumSq=0.0;
    double peak=0.0;

    if(data.symbolicSampleSize==kSample64)
        copyMeasure<double>(
            data.inputs[0],
            data.outputs[0],
            data.numSamples,
            sumSq,
            peak,
            this);
    else
        copyMeasure<float>(
            data.inputs[0],
            data.outputs[0],
            data.numSamples,
            sumSq,
            peak,
            this);

    const double rms=
        std::sqrt(
            sumSq/
            std::max<double>(
                1.0,
                static_cast<double>(
                    data.numSamples*2)));

    const double rmsDb=
        IPC::dbFromAmplitude(
            rms);

    const double peakDb=
        IPC::dbFromAmplitude(
            peak);

    const double activity=
        std::clamp(
            (rmsDb+60.0)/60.0,
            0.0,
            1.0);

    const auto bands=
        Analysis::
        combineStereoFractions(
            analyzerLeft_,
            analyzerRight_);

    const double transient=
        transientDetector_.value();

    const std::int64_t samplePosition=
        data.processContext
        ? static_cast<std::int64_t>(
            data.processContext->
            projectTimeSamples)
        : -1;

    AnalysisPacket packet;
    packet.generation=
        configGeneration_.load(
            std::memory_order_acquire);
    packet.session=session_;
    packet.role=role_;
    packet.samplePosition=samplePosition;
    packet.rmsDb=rmsDb;
    packet.peakDb=peakDb;
    packet.activity=activity;
    packet.transient=transient;

    for(int i=0;i<IPC::kBandCount;++i)
        packet.bands[i]=bands[
            static_cast<std::size_t>(i)];

    // Bounded, non-blocking handoff. If the worker is briefly behind,
    // dropping one analysis frame is preferable to blocking the audio thread.
    if(ipcReady_)
        ipcQueue_.push(packet);

    return kResultOk;
}

tresult PLUGIN_API Processor::setState(
    IBStream* state){

    if(!state)
        return kInvalidArgument;

    IBStreamer s(
        state,
        kLittleEndian);

    int32 r=1;

    if(!s.readInt32(r))
        return kResultFalse;

    role_=
        Analysis::sanitizeRoleState(
            r);

    int32 session=0;
    const bool hasStoredSession=
        s.readInt32(session);

    session_=
        Analysis::legacySensorSessionFallback(
            hasStoredSession,
            session);

    resetAnalysisMeters();
    configGeneration_.fetch_add(
        1,
        std::memory_order_acq_rel);

    return kResultOk;
}

tresult PLUGIN_API Processor::getState(
    IBStream* state){

    if(!state)
        return kInvalidArgument;

    IBStreamer s(
        state,
        kLittleEndian);

    if(!s.writeInt32(
           static_cast<int32>(
               role_)))
        return kResultFalse;

    return
        s.writeInt32(
            static_cast<int32>(
                session_))
        ? kResultOk
        : kResultFalse;
}

} // namespace MixDoctorator::Sensor

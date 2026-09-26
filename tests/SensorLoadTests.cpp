#include "../src/SpectralAnalyzer.h"
#include "../src/TransientModel.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

using namespace MixDoctorator::Analysis;

struct SensorState {
    SpectralAnalyzer left;
    SpectralAnalyzer right;
    TransientDetector transient;
};

struct LoadStats {
    double meanMs{0.0};
    double p95Ms{0.0};
    double p99Ms{0.0};
    double maxMs{0.0};
    int overruns{0};
};

static double percentile(
    std::vector<double> values,
    double p){

    assert(!values.empty());

    std::sort(
        values.begin(),
        values.end());

    const auto index=
        static_cast<std::size_t>(
            std::clamp(
                p,
                0.0,
                1.0)*
            static_cast<double>(
                values.size()-1));

    return values[index];
}

static LoadStats runScenario(
    int sensorCount,
    double sampleRate,
    int blockSize){

    constexpr int maxSensors=24;
    constexpr double seconds=1.0;
    const int totalSamples=
        static_cast<int>(
            sampleRate*seconds);

    assert(std::isfinite(sampleRate));
    assert(sampleRate>=44100.0);
    assert(blockSize>0);

    assert(sensorCount>=1);
    assert(sensorCount<=maxSensors);

    auto sensors=
        std::make_unique<
            std::array<
                SensorState,
                maxSensors>>();

    for(int i=0;i<sensorCount;++i){
        auto& sensor=
            (*sensors)[
                static_cast<std::size_t>(i)];

        sensor.left.prepare(sampleRate);
        sensor.right.prepare(sampleRate);
        sensor.transient.prepare(sampleRate);
    }

    constexpr double pi=
        3.14159265358979323846;

    std::vector<double> blockTimes;
    blockTimes.reserve(
        static_cast<std::size_t>(
            (totalSamples+blockSize-1)/
            blockSize));

    const auto callbackPeriod=
        std::chrono::duration<double>(
            static_cast<double>(blockSize)/
            sampleRate);

    auto nextCallback=
        std::chrono::steady_clock::now();

    for(int base=0;
        base<totalSamples;
        base+=blockSize){

        // Pace callbacks like a realtime host. Waiting is deliberately outside
        // the measured DSP interval: blockTimes records processing cost only,
        // while overruns are still judged against the callback deadline.
        std::this_thread::sleep_until(
            nextCallback);

        const auto blockStart=
            std::chrono::steady_clock::now();

        const int end=
            std::min(
                totalSamples,
                base+blockSize);

        for(int sensorIndex=0;
            sensorIndex<sensorCount;
            ++sensorIndex){

            auto& sensor=
                (*sensors)[
                    static_cast<std::size_t>(
                        sensorIndex)];

            const double f1=
                70.0+
                23.0*
                static_cast<double>(
                    sensorIndex);

            const double f2=
                700.0+
                97.0*
                static_cast<double>(
                    sensorIndex);

            for(int i=base;i<end;++i){
                const double t=
                    static_cast<double>(i)/
                    sampleRate;

                const double left=
                    0.12*
                    std::sin(
                        2.0*pi*f1*t)+
                    0.05*
                    std::sin(
                        2.0*pi*f2*t);

                const double right=
                    0.11*
                    std::sin(
                        2.0*pi*
                        (f1+11.0)*t)+
                    0.04*
                    std::sin(
                        2.0*pi*
                        (f2+37.0)*t);

                sensor.left.push(left);
                sensor.right.push(right);

                const double power=
                    0.5*
                    (left*left+
                     right*right);

                sensor.transient.pushPower(
                    power);
            }
        }

        const auto blockEnd=
            std::chrono::steady_clock::now();

        blockTimes.push_back(
            std::chrono::duration<
                double,
                std::milli>(
                    blockEnd-
                    blockStart).count());

        nextCallback+=
            std::chrono::duration_cast<
                std::chrono::steady_clock::duration>(
                    callbackPeriod);
    }

    double energy=0.0;

    for(int i=0;i<sensorCount;++i){
        const auto& sensor=
            (*sensors)[
                static_cast<std::size_t>(i)];

        for(double value:sensor.left.energy())
            energy+=value;

        for(double value:sensor.right.energy())
            energy+=value;

        assert(
            std::isfinite(
                sensor.transient.value()));
    }

    assert(std::isfinite(energy));
    assert(energy>0.0);

    double totalMs=0.0;
    double maxMs=0.0;
    int overruns=0;

    const double blockDeadlineMs=
        1000.0*
        static_cast<double>(blockSize)/
        sampleRate;

    for(double value:blockTimes){
        assert(std::isfinite(value));
        assert(value>=0.0);

        totalMs+=value;
        maxMs=
            std::max(
                maxMs,
                value);

        if(value>blockDeadlineMs)
            ++overruns;
    }

    LoadStats out;
    out.meanMs=
        totalMs/
        static_cast<double>(
            blockTimes.size());

    out.p95Ms=
        percentile(
            blockTimes,
            0.95);

    out.p99Ms=
        percentile(
            blockTimes,
            0.99);

    out.maxMs=maxMs;
    out.overruns=overruns;

    // The test does not claim host CPU percentage because a DAW may schedule
    // plugin instances across cores. This guards catastrophic single-thread
    // regressions while still recording useful distribution statistics.
    assert(out.meanMs<5.0);
    assert(out.p99Ms<20.0);
    assert(out.maxMs<40.0);

    return out;
}

int main(){
    const int counts[]{1,4,24};

    for(int sensorCount:counts){
        const auto stats=
            runScenario(
                sensorCount,
                48000.0,
                256);

        std::cout
            << "Sensor load "
            << sensorCount
            << ": mean="
            << stats.meanMs
            << " ms, p95="
            << stats.p95Ms
            << " ms, p99="
            << stats.p99Ms
            << " ms, max="
            << stats.maxMs
            << " ms, overruns="
            << stats.overruns
            << "\n";
    }

    struct SensorConfig {
        double sampleRate;
        int blockSize;
    };

    const SensorConfig configs[]{
        {44100.0,128},
        {48000.0,256},
        {96000.0,512}
    };

    for(const auto& config:configs){
        const auto stats=
            runScenario(
                4,
                config.sampleRate,
                config.blockSize);

        std::cout
            << "Sensor config 4: rate="
            << config.sampleRate
            << " Hz, block="
            << config.blockSize
            << ", mean="
            << stats.meanMs
            << " ms, p99="
            << stats.p99Ms
            << " ms, max="
            << stats.maxMs
            << " ms, overruns="
            << stats.overruns
            << "\n";
    }

    return 0;
}

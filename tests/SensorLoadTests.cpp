#include "../src/SpectralAnalyzer.h"
#include "../src/TransientModel.h"

#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>

int main(){
    using namespace MixDoctorator::Analysis;

    constexpr int sensorCount=24;
    constexpr double sampleRate=48000.0;
    constexpr int blockSize=256;
    constexpr double seconds=1.0;
    constexpr int totalSamples=
        static_cast<int>(sampleRate*seconds);

    struct SensorState {
        SpectralAnalyzer left;
        SpectralAnalyzer right;
        TransientDetector transient;
    };

    auto sensors=
        std::make_unique<
            std::array<
                SensorState,
                sensorCount>>();

    for(auto& sensor:*sensors){
        sensor.left.prepare(sampleRate);
        sensor.right.prepare(sampleRate);
        sensor.transient.prepare(sampleRate);
    }

    constexpr double pi=
        3.14159265358979323846;

    const auto start=
        std::chrono::steady_clock::now();

    for(int base=0;
        base<totalSamples;
        base+=blockSize){

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
    }

    const auto end=
        std::chrono::steady_clock::now();

    const double elapsedMs=
        std::chrono::duration<double,std::milli>(
            end-start).count();

    double energy=0.0;

    for(const auto& sensor:*sensors){
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

    // Catastrophic-regression guard. This intentionally does not claim a
    // host-wide CPU percentage because DAWs may schedule instances across
    // cores. It verifies that 24 full Sensor analyzers remain comfortably
    // bounded on one CI worker for one second of 48 kHz stereo material.
    assert(elapsedMs<5000.0);

    std::cout
        << "Sensor load smoke: "
        << sensorCount
        << " sensors, "
        << sampleRate
        << " Hz, "
        << blockSize
        << " samples, "
        << elapsedMs
        << " ms for "
        << seconds
        << " s material\n";

    return 0;
}

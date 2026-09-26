#include "../src/AudioSafety.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main(){
    using MixDoctorator::Analysis::
        sanitizeAudioSample;
    using MixDoctorator::Analysis::
        readAudioSample;

    const double finiteValues[]={
        0.0,
        -0.0,
        1.0,
        -1.0,
        0.125,
        -0.875,
        1.0e-300,
        std::numeric_limits<double>::denorm_min(),
        std::numeric_limits<double>::max(),
        -std::numeric_limits<double>::max()
    };

    for(double value:finiteValues){
        const double out=
            sanitizeAudioSample(value);

        assert(std::isfinite(out));
        assert(out==value);
    }

    assert(
        sanitizeAudioSample(
            std::numeric_limits<double>::
                quiet_NaN())==0.0);

    assert(
        sanitizeAudioSample(
            std::numeric_limits<double>::
                infinity())==0.0);

    assert(
        sanitizeAudioSample(
            -std::numeric_limits<double>::
                infinity())==0.0);

    const float floatBuffer[]{
        0.25f,
        -0.5f,
        std::numeric_limits<float>::
            quiet_NaN(),
        std::numeric_limits<float>::
            denorm_min(),
        std::numeric_limits<float>::
            max()
    };

    assert(std::abs(
        readAudioSample(
            floatBuffer,0)-0.25)<1.0e-7);

    assert(std::abs(
        readAudioSample(
            floatBuffer,1)+0.5)<1.0e-7);

    assert(
        readAudioSample(
            floatBuffer,2)==0.0);

    assert(
        readAudioSample(
            floatBuffer,3)==
        static_cast<double>(
            std::numeric_limits<float>::
                denorm_min()));

    assert(
        readAudioSample(
            floatBuffer,4)==
        static_cast<double>(
            std::numeric_limits<float>::
                max()));

    assert(
        readAudioSample<float>(
            nullptr,0)==0.0);

    {
        const double huge=
            std::numeric_limits<double>::
                max();

        const double safe=
            MixDoctorator::Analysis::
            sanitizeAnalysisSample(huge);

        assert(std::isfinite(safe));
        assert(safe>0.0);
        assert(safe<huge);

        const double power=
            MixDoctorator::Analysis::
            analysisSamplePower(huge);

        const double stereoPower=
            MixDoctorator::Analysis::
            analysisStereoPower(
                huge,
                -huge);

        assert(std::isfinite(power));
        assert(power>0.0);
        assert(std::isfinite(stereoPower));
        assert(stereoPower>0.0);
    }

    assert(
        readAudioSample(
            floatBuffer,-1)==0.0);

    {
        const float monoIn[]{
            0.25f,
            -0.5f,
            0.0f,
            1.0f,
            -1.0f
        };

        float monoOut[5]{};

        MixDoctorator::Analysis::
        copySanitizedAudioBlock(
            monoIn,
            monoOut,
            5);

        for(int i=0;i<5;++i)
            assert(monoOut[i]==monoIn[i]);
    }

    {
        const double stereoLeftIn[]{
            0.125,
            -0.875,
            0.0,
            1.0e-300
        };

        const double stereoRightIn[]{
            -0.125,
            0.875,
            0.5,
            -0.5
        };

        double stereoLeftOut[4]{};
        double stereoRightOut[4]{};

        MixDoctorator::Analysis::
        copySanitizedAudioBlock(
            stereoLeftIn,
            stereoLeftOut,
            4);

        MixDoctorator::Analysis::
        copySanitizedAudioBlock(
            stereoRightIn,
            stereoRightOut,
            4);

        for(int i=0;i<4;++i){
            assert(stereoLeftOut[i]==stereoLeftIn[i]);
            assert(stereoRightOut[i]==stereoRightIn[i]);
        }
    }

    {
        const double badBlock[]{
            0.25,
            std::numeric_limits<double>::
                quiet_NaN(),
            std::numeric_limits<double>::
                infinity(),
            -std::numeric_limits<double>::
                infinity(),
            -0.75
        };

        double sanitized[5]{};

        MixDoctorator::Analysis::
        copySanitizedAudioBlock(
            badBlock,
            sanitized,
            5);

        assert(sanitized[0]==0.25);
        assert(sanitized[1]==0.0);
        assert(sanitized[2]==0.0);
        assert(sanitized[3]==0.0);
        assert(sanitized[4]==-0.75);
    }

    std::cout
        << "AudioSafety tests passed\n";

    return 0;
}

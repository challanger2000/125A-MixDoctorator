#include "../src/PairMeasurement.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main(){
    using namespace MixDoctorator;
    using namespace MixDoctorator::Analysis;

    auto snapshot=[](
        IPC::Role role,
        std::int64_t position,
        double rmsDb,
        int band){

        IPC::Snapshot s;
        s.connected=true;
        s.role=role;
        s.samplePosition=position;
        s.rmsDb=rmsDb;
        s.peakDb=rmsDb+6.0;
        s.activity=0.9;
        s.transient=0.4;

        if(band>=0 &&
           band<IPC::kBandCount)
            s.bands[band]=1.0;

        return s;
    };

    constexpr std::int64_t pos=100000;
    constexpr int block=256;

    {
        auto a=snapshot(
            IPC::Role::Kick,
            pos,
            -12.0,
            1);

        auto b=snapshot(
            IPC::Role::Bass,
            pos,
            -13.0,
            1);

        const auto m=measurePair(
            a,b,pos,block,false);

        assert(m.active);
        assert(m.masking>0.5);
        assert(m.overlap>0.9);
        assert(std::isfinite(
            m.transientCompetition));
    }

    // Purely adjacent bands are invisible to the direct model but visible to
    // the conservative Coach model.
    {
        auto a=snapshot(
            IPC::Role::LeadVocal,
            pos,
            -18.0,
            5);

        auto b=snapshot(
            IPC::Role::Synth,
            pos,
            -18.0,
            6);

        const auto direct=measurePair(
            a,b,pos,block,false);

        const auto coach=measurePair(
            a,b,pos,block,true);

        assert(direct.active);
        assert(coach.active);
        assert(direct.masking<1.0e-12);
        assert(coach.masking>0.14);
    }

    // Disconnected or incoherent sources never produce active measurement.
    {
        auto a=snapshot(
            IPC::Role::Kick,
            pos,
            -12.0,
            1);

        auto b=snapshot(
            IPC::Role::Bass,
            pos+50000,
            -12.0,
            1);

        auto m=measurePair(
            a,b,pos,block,true);

        assert(!m.active);
        assert(m.masking==0.0);

        b.samplePosition=pos;
        b.connected=false;

        m=measurePair(
            a,b,pos,block,true);

        assert(!m.active);
        assert(m.masking==0.0);
    }

    // Non-finite RMS input is treated as inactive rather than entering
    // expensive/non-deterministic analysis.
    {
        auto a=snapshot(
            IPC::Role::Kick,
            pos,
            -12.0,
            1);

        auto b=snapshot(
            IPC::Role::Bass,
            pos,
            -12.0,
            1);

        b.rmsDb=
            std::numeric_limits<double>::
            quiet_NaN();

        const auto m=measurePair(
            a,b,pos,block,true);

        assert(!m.active);
        assert(m.masking==0.0);
        assert(m.overlap==0.0);
    }

    std::cout
        << "Pair measurement tests passed\n";

    return 0;
}

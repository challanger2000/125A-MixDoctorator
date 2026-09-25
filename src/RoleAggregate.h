#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace MixDoctorator::Analysis {

constexpr int kAggregateBandCount=9;

struct AggregateMetrics {
    bool valid{false};
    int count{0};
    double rmsDb{-120.0};
    double peakDb{-120.0};
    double activity{0.0};
    double transient{0.0};
    std::array<double,kAggregateBandCount> bands{};
};

class RoleAggregate {
public:
    void reset() noexcept {
        totalPower_=0.0;
        weightedActivity_=0.0;
        peakDb_=-120.0;
        transient_=0.0;
        count_=0;
        bandPower_.fill(0.0);
    }

    bool add(
        double rmsDb,
        double peakDb,
        double activity,
        double transient,
        const double* bands) noexcept {

        if(!bands ||
           !std::isfinite(rmsDb))
            return false;

        peakDb=
            std::isfinite(peakDb)
            ? peakDb
            : -120.0;

        activity=
            std::isfinite(activity)
            ? activity
            : 0.0;

        transient=
            std::isfinite(transient)
            ? transient
            : 0.0;

        const double power=
            std::pow(
                10.0,
                rmsDb/10.0);

        if(!std::isfinite(power) ||
           power<=0.0)
            return false;

        totalPower_+=power;

        weightedActivity_+=
            power*
            std::clamp(
                activity,
                0.0,
                1.0);

        peakDb_=
            std::max(
                peakDb_,
                peakDb);

        transient_=
            std::max(
                transient_,
                std::clamp(
                    transient,
                    0.0,
                    1.0));

        for(int i=0;
            i<kAggregateBandCount;
            ++i){

            const double band=
                std::isfinite(bands[i])
                ? bands[i]
                : 0.0;

            bandPower_[i]+=
                power*
                std::clamp(
                    band,
                    0.0,
                    1.0);
        }

        ++count_;
        return true;
    }

    AggregateMetrics result() const noexcept {
        AggregateMetrics out;

        if(count_<=0 ||
           totalPower_<=1.0e-20)
            return out;

        out.valid=true;
        out.count=count_;
        out.rmsDb=
            10.0*
            std::log10(
                totalPower_);
        out.peakDb=
            peakDb_;
        out.activity=
            std::clamp(
                weightedActivity_/
                totalPower_,
                0.0,
                1.0);
        out.transient=
            transient_;

        double bandTotal=0.0;

        for(double value:bandPower_)
            bandTotal+=value;

        if(bandTotal>1.0e-20){
            for(int i=0;
                i<kAggregateBandCount;
                ++i){

                out.bands[i]=
                    bandPower_[i]/
                    bandTotal;
            }
        }

        return out;
    }

private:
    double totalPower_{0.0};
    double weightedActivity_{0.0};
    double peakDb_{-120.0};
    double transient_{0.0};
    int count_{0};
    std::array<double,kAggregateBandCount> bandPower_{};
};

} // namespace MixDoctorator::Analysis

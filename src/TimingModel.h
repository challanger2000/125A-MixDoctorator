#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>

namespace MixDoctorator::Analysis {

inline bool samplePositionsCoherent(
    std::int64_t current,
    std::int64_t a,
    std::int64_t b,
    std::int64_t blockSize) noexcept {

    const std::int64_t tolerance=
        std::max<std::int64_t>(
            4096,
            std::max<std::int64_t>(
                1,
                blockSize)*
            4);

    // If the host position is unavailable, retain heartbeat-based fallback.
    // When both sensor positions are known we can still reject an obviously
    // incoherent pair.
    if(current<0){
        if(a>=0 && b>=0)
            return
                std::llabs(a-b)<=
                tolerance;

        return true;
    }

    // With a valid host position, mixed timing certainty is unsafe: one
    // source can be proven current while the other cannot be aligned.
    if((a<0)!=(b<0))
        return false;

    // If neither sensor exposes a sample position, fall back to heartbeat
    // freshness instead of disabling analysis entirely.
    if(a<0 && b<0)
        return true;

    return
        std::llabs(
            a-current)<=
            tolerance &&
        std::llabs(
            b-current)<=
            tolerance &&
        std::llabs(
            a-b)<=
            tolerance;
}

inline bool sampleRangesCoherent(
    std::int64_t current,
    std::int64_t aPosition,
    std::int64_t aMin,
    std::int64_t aMax,
    std::int64_t bPosition,
    std::int64_t bMin,
    std::int64_t bMax,
    std::int64_t blockSize) noexcept {

    const std::int64_t tolerance=
        std::max<std::int64_t>(
            4096,
            std::max<std::int64_t>(
                1,
                blockSize)*
            4);

    auto resolve=[](
        std::int64_t position,
        std::int64_t minPosition,
        std::int64_t maxPosition,
        std::int64_t& outMin,
        std::int64_t& outMax) noexcept {

        if(minPosition>=0 &&
           maxPosition>=minPosition){
            outMin=minPosition;
            outMax=maxPosition;
            return true;
        }

        if(position>=0){
            outMin=position;
            outMax=position;
            return true;
        }

        outMin=-1;
        outMax=-1;
        return false;
    };

    std::int64_t aLo=-1,aHi=-1;
    std::int64_t bLo=-1,bHi=-1;

    const bool aKnown=
        resolve(
            aPosition,
            aMin,
            aMax,
            aLo,
            aHi);

    const bool bKnown=
        resolve(
            bPosition,
            bMin,
            bMax,
            bLo,
            bHi);

    if(current>=0){
        if(aKnown!=bKnown)
            return false;

        if(!aKnown)
            return true;

        if(std::llabs(aLo-current)>tolerance ||
           std::llabs(aHi-current)>tolerance ||
           std::llabs(bLo-current)>tolerance ||
           std::llabs(bHi-current)>tolerance)
            return false;
    }else{
        if(aKnown!=bKnown)
            return true;

        if(!aKnown)
            return true;
    }

    const auto earliest=
        std::min(aLo,bLo);

    const auto latest=
        std::max(aHi,bHi);

    return
        latest-earliest<=
        tolerance;
}

} // namespace MixDoctorator::Analysis

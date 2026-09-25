#pragma once
#include "MixDoctoratorIPC.h"
#include <algorithm>
#include <cmath>

namespace MixDoctorator::Analysis {

inline int sanitizeSessionState(
    int session) noexcept {

    return
        std::clamp(
            session,
            0,
            IPC::kSessionCount-1);
}

inline IPC::Role sanitizeRoleState(
    int role) noexcept {

    return
        static_cast<IPC::Role>(
            std::clamp(
                role,
                1,
                IPC::kRoleCount));
}

inline double encodeSessionNormalized(
    int session) noexcept {

    constexpr int maximum=
        IPC::kSessionCount-1;

    if constexpr(maximum<=0)
        return 0.0;

    return
        static_cast<double>(
            sanitizeSessionState(
                session))/
        static_cast<double>(
            maximum);
}

inline int decodeSessionNormalized(
    double normalized) noexcept {

    if(!std::isfinite(normalized))
        return 0;

    constexpr int maximum=
        IPC::kSessionCount-1;

    return
        sanitizeSessionState(
            static_cast<int>(
                std::lround(
                    std::clamp(
                        normalized,
                        0.0,
                        1.0)*
                    static_cast<double>(
                        maximum))));
}

inline double encodeRoleNormalized(
    IPC::Role role) noexcept {

    constexpr int maximum=
        IPC::kRoleCount-1;

    const int safeRole=
        static_cast<int>(
            sanitizeRoleState(
                static_cast<int>(role)));

    return
        maximum>0
        ? static_cast<double>(
            safeRole-1)/
          static_cast<double>(
            maximum)
        : 0.0;
}

inline IPC::Role decodeRoleNormalized(
    double normalized) noexcept {

    if(!std::isfinite(normalized))
        return IPC::Role::Drums;

    constexpr int maximum=
        IPC::kRoleCount-1;

    const int index=
        std::clamp(
            static_cast<int>(
                std::lround(
                    std::clamp(
                        normalized,
                        0.0,
                        1.0)*
                    static_cast<double>(
                        maximum))),
            0,
            maximum);

    return
        static_cast<IPC::Role>(
            index+1);
}

} // namespace MixDoctorator::Analysis

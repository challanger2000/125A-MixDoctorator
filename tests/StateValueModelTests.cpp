#include "../src/StateValueModel.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

using namespace MixDoctorator;
using namespace MixDoctorator::Analysis;

int main(){
    assert(sanitizeSessionState(-10)==0);
    assert(sanitizeSessionState(0)==0);
    assert(sanitizeSessionState(IPC::kSessionCount-1)==IPC::kSessionCount-1);
    assert(sanitizeSessionState(99)==IPC::kSessionCount-1);

    for(int session=0;
        session<IPC::kSessionCount;
        ++session){

        const double normalized=
            encodeSessionNormalized(session);

        assert(normalized>=0.0);
        assert(normalized<=1.0);
        assert(
            decodeSessionNormalized(
                normalized)==session);
    }

    assert(
        decodeSessionNormalized(
            std::numeric_limits<double>::
                quiet_NaN())==0);

    assert(
        sanitizeRoleState(-99)==
        IPC::Role::Drums);

    assert(
        sanitizeRoleState(999)==
        static_cast<IPC::Role>(
            IPC::kRoleCount));

    for(int role=1;
        role<=IPC::kRoleCount;
        ++role){

        const auto r=
            static_cast<IPC::Role>(
                role);

        const double normalized=
            encodeRoleNormalized(r);

        assert(normalized>=0.0);
        assert(normalized<=1.0);
        assert(
            decodeRoleNormalized(
                normalized)==r);
    }

    assert(
        decodeRoleNormalized(
            std::numeric_limits<double>::
                quiet_NaN())==
        IPC::Role::Drums);

    std::cout
        << "State value mapping tests passed\n";

    return 0;
}

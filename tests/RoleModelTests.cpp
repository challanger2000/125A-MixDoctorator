#include "../src/RoleModel.h"
#include <cassert>
#include <iostream>

int main(){
    using namespace MixDoctorator;
    using namespace MixDoctorator::Analysis;

    assert(roleFamily(IPC::Role::Kick)==RoleFamily::RhythmLow);
    assert(roleFamily(IPC::Role::Snare)==RoleFamily::RhythmMid);
    assert(roleFamily(IPC::Role::Cymbals)==RoleFamily::RhythmHigh);
    assert(roleFamily(IPC::Role::Bass)==RoleFamily::Bass);
    assert(roleFamily(IPC::Role::ElectricGuitar)==RoleFamily::Guitar);
    assert(roleFamily(IPC::Role::LeadVocal)==RoleFamily::Vocal);
    assert(roleFamily(IPC::Role::Synth)==RoleFamily::KeysSynth);
    assert(roleFamily(IPC::Role::Pad)==RoleFamily::Pad);

    assert(isLowEndRole(IPC::Role::Kick));
    assert(isLowEndRole(IPC::Role::Bass));
    assert(!isLowEndRole(IPC::Role::LeadVocal));

    assert(isTransientRole(IPC::Role::Snare));
    assert(!isTransientRole(IPC::Role::Pad));

    assert(isHarmonicRole(IPC::Role::LeadVocal));
    assert(isHarmonicRole(IPC::Role::ElectricGuitar));
    assert(!isHarmonicRole(IPC::Role::Kick));

    assert(!rolesComparableForCoach(
        IPC::Role::Drums,
        IPC::Role::Kick));
    assert(!rolesComparableForCoach(
        IPC::Role::Snare,
        IPC::Role::Drums));
    assert(rolesComparableForCoach(
        IPC::Role::Kick,
        IPC::Role::Bass));
    assert(rolesComparableForCoach(
        IPC::Role::Snare,
        IPC::Role::ElectricGuitar));

    assert(kRolePairCount==91);
    assert(encodeRolePair(IPC::Role::Drums,IPC::Role::Bass)==0);
    assert(encodeRolePair(IPC::Role::Bass,IPC::Role::Drums)==0);

    IPC::Role a=IPC::Role::Unknown;
    IPC::Role b=IPC::Role::Unknown;
    assert(decodeRolePair(
        encodeRolePair(IPC::Role::LeadVocal,IPC::Role::Synth),
        a,b));
    assert(a==IPC::Role::LeadVocal);
    assert(b==IPC::Role::Synth);
    assert(!decodeRolePair(-1,a,b));

    int counts[IPC::kRoleCount]{};
    counts[roleToIndex(IPC::Role::Kick)]=1;
    counts[roleToIndex(IPC::Role::Bass)]=2;
    counts[roleToIndex(IPC::Role::LeadVocal)]=1;

    const auto summary=
        summarizeRoleCounts(counts);

    assert(summary.sensors==4);
    assert(summary.roles==3);

    const auto emptySummary=
        summarizeRoleCounts(nullptr);

    assert(emptySummary.sensors==0);
    assert(emptySummary.roles==0);

    std::cout << "Role model tests passed\n";
    return 0;
}

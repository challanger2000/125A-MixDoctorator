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

    std::cout << "Role model tests passed\n";
    return 0;
}

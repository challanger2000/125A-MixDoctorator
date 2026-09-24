#pragma once

namespace MixDoctorator::Analysis {

struct PrimaryFindingSelection {
    int pair{-1};
    bool current{false};
};

// Select an eligible current finding before a previously observed session
// finding. This is presentation logic, not psychoacoustic validation.
inline PrimaryFindingSelection selectPrimaryFinding(
    int currentPair,
    int sessionPair) noexcept {
    if(currentPair>=0 && currentPair<3)
        return {currentPair,true};
    if(sessionPair>=0 && sessionPair<3)
        return {sessionPair,false};
    return {};
}

} // namespace MixDoctorator::Analysis

#include "../src/IpcResponsePolicy.h"
#include <cassert>
#include <iostream>

using namespace MixDoctorator::Analysis;

int main(){
    constexpr int session=3;
    constexpr std::uint64_t generation=42;
    constexpr std::int64_t now=100000;
    constexpr int block=256;

    assert(
        ipcResponseMatchesContext(
            session,generation,
            session,generation));

    assert(
        !ipcResponseMatchesContext(
            session-1,generation,
            session,generation));

    assert(
        !ipcResponseMatchesContext(
            session,generation-1,
            session,generation));

    assert(
        ipcResponseIsFresh(
            session,generation,now,
            session,generation,now,block));

    // A response from the previous Session must never leak through even when
    // timing is otherwise perfect.
    assert(
        !ipcResponseIsFresh(
            session-1,generation,now,
            session,generation,now,block));

    // A response queued before reset/session change is rejected by generation.
    assert(
        !ipcResponseIsFresh(
            session,generation-1,now,
            session,generation,now,block));

    // Correct identity but stale worker timing is not a fresh diagnosis.
    assert(
        !ipcResponseIsFresh(
            session,generation,now-50000,
            session,generation,now,block));

    // A known host timeline must reject an untimed worker response.
    assert(
        !ipcResponseIsFresh(
            session,generation,-1,
            session,generation,now,block));

    // If the host itself has no sample timeline, heartbeat/generation
    // freshness remains the intentional fallback.
    assert(
        ipcResponseIsFresh(
            session,generation,-1,
            session,generation,-1,block));

    // A one-block worker delay remains inside the intentional grace zone.
    assert(
        ipcResponseIsFresh(
            session,generation,now-block,
            session,generation,now,block));

    std::cout
        << "IPC response policy tests passed\n";

    return 0;
}

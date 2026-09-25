#include "../src/IpcResponsePolicy.h"
#include "../src/MixDoctoratorIPC.h"
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

    // Exhaustive Session isolation: only an exact Session match may ever
    // drive the Brain. This protects all A-H combinations, not just one
    // representative mismatch.
    for(int currentSession=0;
        currentSession<MixDoctorator::IPC::kSessionCount;
        ++currentSession){

        for(int responseSession=0;
            responseSession<MixDoctorator::IPC::kSessionCount;
            ++responseSession){

            const bool expected=
                responseSession==currentSession;

            assert(
                ipcResponseMatchesContext(
                    responseSession,
                    generation,
                    currentSession,
                    generation)==expected);

            assert(
                ipcResponseIsFresh(
                    responseSession,
                    generation,
                    now,
                    currentSession,
                    generation,
                    now,
                    block)==expected);
        }
    }

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

    // Repeated Brain resets must reject every earlier worker response,
    // not only the immediately previous generation.
    {
        std::uint64_t currentGeneration=1000;

        for(int reset=0;reset<64;++reset){
            const auto staleGeneration=
                currentGeneration;

            ++currentGeneration;

            assert(
                !ipcResponseMatchesContext(
                    session,
                    staleGeneration,
                    session,
                    currentGeneration));

            assert(
                !ipcResponseIsFresh(
                    session,
                    staleGeneration,
                    now,
                    session,
                    currentGeneration,
                    now,
                    block));

            assert(
                ipcResponseIsFresh(
                    session,
                    currentGeneration,
                    now,
                    session,
                    currentGeneration,
                    now,
                    block));
        }
    }

    // A one-block worker delay remains inside the intentional grace zone.
    assert(
        ipcResponseIsFresh(
            session,generation,now-block,
            session,generation,now,block));

    std::cout
        << "IPC response policy tests passed\n";

    return 0;
}

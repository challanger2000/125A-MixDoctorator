#include "../src/MixDoctoratorIPC.h"

#include <array>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <thread>
#include <string>

int main(int argc,char** argv){
#ifdef _WIN32
    using namespace MixDoctorator::IPC;

    constexpr int processSession=1;
    constexpr std::uint64_t processId=
        0x125A00ABCDEF1234ull;

    if(argc>1 &&
       std::string(argv[1])=="--writer-lock-owner-child"){

        Sleep(750);
        return 0;
    }

    if(argc>1 &&
       std::string(argv[1])=="--ipc-open-race-child"){

        HANDLE gate=
            OpenEventW(
                SYNCHRONIZE,
                FALSE,
                L"Local\\125A_MixDoctorator_OpenRaceGate");

        assert(gate!=nullptr);

        const DWORD waitResult=
            WaitForSingleObject(
                gate,
                5000u);

        assert(waitResult==WAIT_OBJECT_0);

        CloseHandle(gate);

        SharedMemory childMemory;
        assert(childMemory.open());

        // Keep mappings alive briefly so all contenders overlap.
        Sleep(100);

        return 0;
    }

    if(argc>1 &&
       std::string(argv[1])=="--ipc-child"){

        SharedMemory childMemory;
        assert(childMemory.open());

        int childSlot=-1;
        double childBands[kBandCount]{};
        childBands[5]=1.0;

        const auto deadline=
            GetTickCount64()+750u;

        do{
            assert(
                childMemory.publish(
                    processSession,
                    processId,
                    childSlot,
                    Role::Synth,
                    424242,
                    -15.0,
                    -3.0,
                    0.75,
                    0.25,
                    childBands));

            Sleep(10);
        }while(GetTickCount64()<deadline);

        assert(
            childMemory.release(
                processSession,
                processId,
                childSlot));

        return 0;
    }

    // Simultaneous first-open stress: several independent processes wait on
    // one gate and then open all eight named mappings at nearly the same time.
    // This exercises the initState/Interlocked initialization contract itself,
    // not merely later cross-process reads/writes.
    {
        HANDLE gate=
            CreateEventW(
                nullptr,
                TRUE,
                FALSE,
                L"Local\\125A_MixDoctorator_OpenRaceGate");

        assert(gate!=nullptr);

        wchar_t exePath[MAX_PATH]{};

        const DWORD pathLength=
            GetModuleFileNameW(
                nullptr,
                exePath,
                MAX_PATH);

        assert(pathLength>0);
        assert(pathLength<MAX_PATH);

        constexpr int childCount=8;
        std::array<
            PROCESS_INFORMATION,
            childCount> children{};

        for(int i=0;i<childCount;++i){
            std::wstring command=
                L"\""+
                std::wstring(exePath)+
                L"\" --ipc-open-race-child";

            STARTUPINFOW startup{};
            startup.cb=sizeof(startup);

            assert(
                CreateProcessW(
                    nullptr,
                    command.data(),
                    nullptr,
                    nullptr,
                    FALSE,
                    0,
                    nullptr,
                    nullptr,
                    &startup,
                    &children[
                        static_cast<std::size_t>(i)])!=FALSE);
        }

        assert(SetEvent(gate)!=FALSE);

        for(auto& child:children){
            const DWORD waitResult=
                WaitForSingleObject(
                    child.hProcess,
                    10000u);

            assert(waitResult==WAIT_OBJECT_0);

            DWORD exitCode=1;
            assert(
                GetExitCodeProcess(
                    child.hProcess,
                    &exitCode)!=FALSE);

            assert(exitCode==0);

            CloseHandle(child.hThread);
            CloseHandle(child.hProcess);
        }

        CloseHandle(gate);
    }

    // Abandoned writer-lock recovery: a live owner must never be stolen.
    // Once that exact process is confirmed terminated and the heartbeat is
    // stale, the worker may safely claim the orphaned lock.
    {
        wchar_t exePath[MAX_PATH]{};

        const DWORD pathLength=
            GetModuleFileNameW(
                nullptr,
                exePath,
                MAX_PATH);

        assert(pathLength>0);
        assert(pathLength<MAX_PATH);

        std::wstring command=
            L"\""+
            std::wstring(exePath)+
            L"\" --writer-lock-owner-child";

        STARTUPINFOW startup{};
        startup.cb=sizeof(startup);

        PROCESS_INFORMATION process{};

        assert(
            CreateProcessW(
                nullptr,
                command.data(),
                nullptr,
                nullptr,
                FALSE,
                0,
                nullptr,
                nullptr,
                &startup,
                &process)!=FALSE);

        volatile LONG simulatedLock=
            static_cast<LONG>(
                process.dwProcessId);

        constexpr std::uint64_t staleNow=10000u;
        constexpr std::uint64_t staleHeartbeat=0u;

        assert(
            !acquireWriterLock(
                simulatedLock,
                staleNow,
                staleHeartbeat));

        const DWORD waitResult=
            WaitForSingleObject(
                process.hProcess,
                5000u);

        assert(waitResult==WAIT_OBJECT_0);

        DWORD exitCode=1;
        assert(
            GetExitCodeProcess(
                process.hProcess,
                &exitCode)!=FALSE);
        assert(exitCode==0);

        assert(
            acquireWriterLock(
                simulatedLock,
                staleNow,
                staleHeartbeat));

        assert(
            simulatedLock==
            currentWriterProcessId());

        releaseWriterLock(
            simulatedLock);

        assert(simulatedLock==0);

        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
    }

    SharedMemory writerMemory;
    SharedMemory readerMemory;
    assert(writerMemory.open());
    assert(readerMemory.open());

    // Genuine cross-process IPC: launch this test executable as a child
    // process. The child owns and publishes its own SharedMemory instance;
    // the parent must observe the coherent Sensor snapshot through the named
    // mapping, exactly as two independent DAW host processes would.
    {
        wchar_t exePath[MAX_PATH]{};

        const DWORD pathLength=
            GetModuleFileNameW(
                nullptr,
                exePath,
                MAX_PATH);

        assert(pathLength>0);
        assert(pathLength<MAX_PATH);

        std::wstring command=
            L"\""+
            std::wstring(exePath)+
            L"\" --ipc-child";

        STARTUPINFOW startup{};
        startup.cb=sizeof(startup);

        PROCESS_INFORMATION process{};

        assert(
            CreateProcessW(
                nullptr,
                command.data(),
                nullptr,
                nullptr,
                FALSE,
                0,
                nullptr,
                nullptr,
                &startup,
                &process)!=FALSE);

        bool observedChild=false;
        const auto deadline=
            GetTickCount64()+2000u;

        while(GetTickCount64()<deadline &&
              !observedChild){

            const auto now=
                static_cast<std::uint64_t>(
                    GetTickCount64());

            for(int i=0;i<kSensorSlotCount;++i){
                Snapshot s;

                if(!readerMemory.readSlot(
                       processSession,
                       i,
                       s,
                       now))
                    continue;

                if(!s.connected ||
                   s.instanceId!=processId)
                    continue;

                assert(s.role==Role::Synth);
                assert(s.samplePosition==424242);
                assert(std::abs(s.rmsDb+15.0)<1.0e-12);
                assert(std::abs(s.peakDb+3.0)<1.0e-12);
                assert(std::abs(s.activity-0.75)<1.0e-12);
                assert(std::abs(s.transient-0.25)<1.0e-12);
                assert(std::abs(s.bands[5]-1.0)<1.0e-12);

                observedChild=true;
                break;
            }

            if(!observedChild)
                Sleep(1);
        }

        assert(observedChild);

        const DWORD waitResult=
            WaitForSingleObject(
                process.hProcess,
                5000u);

        assert(waitResult==WAIT_OBJECT_0);

        DWORD exitCode=1;
        assert(
            GetExitCodeProcess(
                process.hProcess,
                &exitCode)!=FALSE);
        assert(exitCode==0);

        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
    }

    constexpr int session=6;
    constexpr std::uint64_t id=
        0x125A0000CC771234ull;

    int cachedSlot=-1;
    double initialBands[kBandCount]{};
    initialBands[0]=1.0;

    assert(
        writerMemory.publish(
            session,
            id,
            cachedSlot,
            Role::Drums,
            1,
            -10.001,
            -1.001,
            0.01,
            0.01,
            initialBands));

    assert(cachedSlot>=0);
    const int claimedSlot=cachedSlot;

    std::atomic<bool> finished{false};
    std::atomic<int> coherentReads{0};

    std::thread writer([&]{
        for(std::int64_t n=2;n<=20000;++n){
            double bands[kBandCount]{};
            bands[0]=static_cast<double>(n);
            bands[1]=static_cast<double>(n)+0.25;

            const bool ok=
                writerMemory.publish(
                    session,
                    id,
                    cachedSlot,
                    Role::Drums,
                    n,
                    -10.0-
                        static_cast<double>(n)*0.001,
                    -1.0-
                        static_cast<double>(n)*0.001,
                    static_cast<double>(n%100)/100.0,
                    static_cast<double>(n%50)/50.0,
                    bands);

            assert(ok);
        }

        finished.store(
            true,
            std::memory_order_release);
    });

    while(!finished.load(
              std::memory_order_acquire) ||
          coherentReads.load(
              std::memory_order_relaxed)==0){

        Snapshot s;
        const auto now=
            static_cast<std::uint64_t>(
                GetTickCount64());

        if(!readerMemory.readSlot(
               session,
               claimedSlot,
               s,
               now))
            continue;

        if(s.instanceId!=id)
            continue;

        const auto n=
            s.samplePosition;

        if(n<1)
            continue;

        const double expectedRms=
            -10.0-
            static_cast<double>(n)*0.001;

        const double expectedPeak=
            -1.0-
            static_cast<double>(n)*0.001;

        assert(s.role==Role::Drums);
        assert(std::abs(
            s.rmsDb-expectedRms)<1.0e-12);
        assert(std::abs(
            s.peakDb-expectedPeak)<1.0e-12);
        assert(std::abs(
            s.bands[0]-
            static_cast<double>(n))<1.0e-12);

        if(n>=2)
            assert(std::abs(
                s.bands[1]-
                (static_cast<double>(n)+0.25))<
                1.0e-12);

        ++coherentReads;
    }

    writer.join();

    assert(coherentReads.load()>0);

    // Same instance ID published in another session must not leak here.
    int otherSlot=-1;
    double otherBands[kBandCount]{};
    otherBands[2]=1.0;

    assert(
        writerMemory.publish(
            5,
            id+1,
            otherSlot,
            Role::Bass,
            100,
            -20.0,
            -3.0,
            0.5,
            0.2,
            otherBands));

    bool leaked=false;
    const auto now=
        static_cast<std::uint64_t>(
            GetTickCount64());

    for(int i=0;i<kSensorSlotCount;++i){
        Snapshot s;
        if(readerMemory.readSlot(
               session,
               i,
               s,
               now) &&
           s.instanceId==id+1)
            leaked=true;
    }

    assert(!leaked);

    assert(
        dbFromAmplitude(
            std::numeric_limits<double>::
                quiet_NaN())==-180.0);

    std::cout
        << "IPC concurrency tests passed\n";
#else
    std::cout
        << "IPC concurrency tests skipped on non-Windows\n";
#endif

    return 0;
}

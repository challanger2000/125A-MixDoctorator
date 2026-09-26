#include "../src/MixDoctoratorIPC.h"

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

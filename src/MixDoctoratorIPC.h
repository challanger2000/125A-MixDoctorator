#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cwchar>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace MixDoctorator::IPC {

enum class Role : std::uint32_t {
    Unknown=0,
    Drums=1,
    Bass=2,
    ElectricGuitar=3
};

constexpr int kRoleCount=3;
constexpr int kBandCount=9;
constexpr int kSessionCount=8;
constexpr int kSensorSlotCount=24;
constexpr std::uint32_t kMagic=0x4D445038u;
constexpr std::uint32_t kVersion=8u;

inline int clampSession(int session) noexcept {
    return std::clamp(session,0,kSessionCount-1);
}

struct alignas(64) Slot {
#ifdef _WIN32
    volatile LONG sequence{0};
    volatile LONG role{0};
    volatile LONG active{0};
    volatile LONG writerLock{0};
    volatile LONG64 instanceId{0};
    volatile LONG64 heartbeatMs{0};
    volatile LONG64 samplePosition{-1};
#else
    std::int32_t sequence{0},role{0},active{0},reserved{0};
    std::int64_t instanceId{0};
    std::int64_t heartbeatMs{0};
    std::int64_t samplePosition{-1};
#endif
    double rmsDb{-120.0};
    double peakDb{-120.0};
    double activity{0.0};
    double transient{0.0};
    double bands[kBandCount]{};
};

struct SharedBlock {
#ifdef _WIN32
    volatile LONG initState{0}; // 0=new, 1=initializing, 2=ready
#else
    std::int32_t initState{0};
#endif
    std::uint32_t magic{kMagic};
    std::uint32_t version{kVersion};
    Slot slots[kSensorSlotCount]{};
};

struct Snapshot {
    std::uint64_t instanceId{0};
    Role role{Role::Unknown};
    bool connected{false};
    std::uint64_t heartbeatMs{0};
    std::int64_t samplePosition{-1};
    double rmsDb{-120.0};
    double peakDb{-120.0};
    double activity{0.0};
    double transient{0.0};
    int aggregateCount{0};
    double bands[kBandCount]{};
};

inline double dbFromAmplitude(double v) noexcept {
    if(!std::isfinite(v) || v<=1.0e-9)
        return -180.0;

    return 20.0*std::log10(v);
}

class SharedMemory {
public:
    ~SharedMemory(){ close(); }
    SharedMemory()=default;
    SharedMemory(const SharedMemory&)=delete;
    SharedMemory& operator=(const SharedMemory&)=delete;

    bool open() noexcept {
#ifdef _WIN32
        if(opened_) return true;

        for(int session=0;session<kSessionCount;++session){
            wchar_t name[96]{};

            std::swprintf(
                name,
                sizeof(name)/sizeof(name[0]),
                L"Local\\125A_MixDoctorator_POC_v8_S%d",
                session+1);

            mapping_[session]=CreateFileMappingW(
                INVALID_HANDLE_VALUE,
                nullptr,
                PAGE_READWRITE,
                0,
                static_cast<DWORD>(sizeof(SharedBlock)),
                name);

            if(!mapping_[session]){
                close();
                return false;
            }

            const bool existed=
                GetLastError()==ERROR_ALREADY_EXISTS;

            block_[session]=static_cast<SharedBlock*>(
                MapViewOfFile(
                    mapping_[session],
                    FILE_MAP_ALL_ACCESS,
                    0,
                    0,
                    sizeof(SharedBlock)));

            if(!block_[session]){
                close();
                return false;
            }

            auto* shared=
                block_[session];

            if(!existed){
                if(InterlockedCompareExchange(
                       &shared->initState,
                       1,
                       0)!=0){
                    close();
                    return false;
                }

                ZeroMemory(
                    shared->slots,
                    sizeof(shared->slots));

                shared->magic=kMagic;
                shared->version=kVersion;

                for(auto& slot:shared->slots)
                    slot.samplePosition=-1;

                MemoryBarrier();
                InterlockedExchange(
                    &shared->initState,
                    2);
            }else{
                // open() is not called from process(); a short bounded wait is
                // acceptable here and avoids two processes initializing the
                // same mapping concurrently.
                int waits=0;

                while(shared->initState==1 &&
                      waits<100){
                    Sleep(1);
                    ++waits;
                }

                if(shared->initState==0){
                    if(InterlockedCompareExchange(
                           &shared->initState,
                           1,
                           0)==0){

                        ZeroMemory(
                            shared->slots,
                            sizeof(shared->slots));

                        shared->magic=kMagic;
                        shared->version=kVersion;

                        for(auto& slot:shared->slots)
                            slot.samplePosition=-1;

                        MemoryBarrier();
                        InterlockedExchange(
                            &shared->initState,
                            2);
                    }else{
                        int settleWaits=0;
                        while(shared->initState==1 &&
                              settleWaits<100){
                            Sleep(1);
                            ++settleWaits;
                        }
                    }
                }

                if(shared->initState!=2 ||
                   shared->magic!=kMagic ||
                   shared->version!=kVersion){
                    close();
                    return false;
                }
            }
        }

        opened_=true;
        return true;
#else
        return false;
#endif
    }

    void close() noexcept {
#ifdef _WIN32
        for(int session=0;session<kSessionCount;++session){
            if(block_[session]){
                UnmapViewOfFile(block_[session]);
                block_[session]=nullptr;
            }

            if(mapping_[session]){
                CloseHandle(mapping_[session]);
                mapping_[session]=nullptr;
            }
        }

        opened_=false;
#endif
    }

    bool publish(
        int session,
        std::uint64_t instanceId,
        int& cachedSlot,
        Role role,
        std::int64_t samplePosition,
        double rmsDb,
        double peakDb,
        double activity,
        double transient,
        const double* bands) noexcept {

#ifdef _WIN32
        if(!opened_ ||
           instanceId==0 ||
           !bands)
            return false;

        const int sessionIndex=
            clampSession(session);

        auto* block=
            block_[sessionIndex];

        if(!block)
            return false;

        const auto now=
            static_cast<std::uint64_t>(
                GetTickCount64());

        int slotIndex=-1;
        Slot* lockedSlot=nullptr;

        auto tryOwned=[&](
            int index)->bool {

            if(index<0 ||
               index>=kSensorSlotCount)
                return false;

            auto& candidate=
                block->slots[index];

            if(InterlockedCompareExchange(
                   &candidate.writerLock,
                   1,
                   0)!=0)
                return false;

            if(static_cast<std::uint64_t>(
                   candidate.instanceId)!=
               instanceId){
                InterlockedExchange(
                    &candidate.writerLock,
                    0);
                return false;
            }

            slotIndex=index;
            lockedSlot=&candidate;
            return true;
        };

        if(!tryOwned(cachedSlot)){
            for(int i=0;
                i<kSensorSlotCount &&
                !lockedSlot;
                ++i){

                if(tryOwned(i))
                    break;
            }
        }

        if(!lockedSlot){
            for(int i=0;
                i<kSensorSlotCount;
                ++i){

                auto& candidate=
                    block->slots[i];

                if(InterlockedCompareExchange(
                       &candidate.writerLock,
                       1,
                       0)!=0)
                    continue;

                const LONG64 currentId=
                    candidate.instanceId;

                const auto heartbeat=
                    static_cast<std::uint64_t>(
                        candidate.heartbeatMs);

                const bool stale=
                    currentId==0 ||
                    (now>=heartbeat &&
                     (now-heartbeat)>2000u);

                if(!stale){
                    InterlockedExchange(
                        &candidate.writerLock,
                        0);
                    continue;
                }

                slotIndex=i;
                lockedSlot=&candidate;
                break;
            }
        }

        if(!lockedSlot)
            return false;

        cachedSlot=slotIndex;

        auto& s=*lockedSlot;

        InterlockedIncrement(
            &s.sequence);

        MemoryBarrier();

        // instanceId is part of the same seqlock-protected snapshot as the
        // payload. A reclaimed slot can therefore never expose a new owner
        // together with stale data from the previous owner.
        s.active=0;
        s.instanceId=
            static_cast<LONG64>(
                instanceId);
        s.role=
            static_cast<LONG>(
                role);
        s.samplePosition=
            static_cast<LONG64>(
                samplePosition);

        s.rmsDb=rmsDb;
        s.peakDb=peakDb;
        s.activity=activity;
        s.transient=transient;

        for(int i=0;i<kBandCount;++i)
            s.bands[i]=bands[i];

        s.heartbeatMs=
            static_cast<LONG64>(
                now);
        s.active=1;

        MemoryBarrier();

        InterlockedIncrement(
            &s.sequence);

        InterlockedExchange(
            &s.writerLock,
            0);

        return true;
#else
        (void)session;
        (void)instanceId;
        (void)cachedSlot;
        (void)role;
        (void)samplePosition;
        (void)rmsDb;
        (void)peakDb;
        (void)activity;
        (void)transient;
        (void)bands;

        return false;
#endif
    }

    bool readSlot(
        int session,
        int slotIndex,
        Snapshot& out,
        std::uint64_t nowMs) noexcept {

#ifdef _WIN32
        if(!opened_ ||
           slotIndex<0 ||
           slotIndex>=kSensorSlotCount)
            return false;

        const int sessionIndex=
            clampSession(session);

        auto* block=
            block_[sessionIndex];

        if(!block)
            return false;

        const auto& s=
            block->slots[slotIndex];

        for(int attempt=0;attempt<5;++attempt){
            const LONG before=
                s.sequence;

            MemoryBarrier();

            if(before&1)
                continue;

            Snapshot t;
            LONG active=0;

            t.instanceId=
                static_cast<std::uint64_t>(
                    s.instanceId);

            t.role=
                static_cast<Role>(
                    s.role);

            t.heartbeatMs=
                static_cast<std::uint64_t>(
                    s.heartbeatMs);

            t.samplePosition=
                static_cast<std::int64_t>(
                    s.samplePosition);

            t.rmsDb=s.rmsDb;
            t.peakDb=s.peakDb;
            t.activity=s.activity;
            t.transient=s.transient;
            active=s.active;

            for(int i=0;i<kBandCount;++i)
                t.bands[i]=s.bands[i];

            MemoryBarrier();

            const LONG after=
                s.sequence;

            if(before==after &&
               !(after&1)){

                t.connected=
                    t.instanceId!=0 &&
                    active!=0 &&
                    nowMs>=t.heartbeatMs &&
                    (nowMs-t.heartbeatMs)<1500u;

                out=t;
                return true;
            }
        }
#endif
        return false;
    }

private:
#ifdef _WIN32
    HANDLE mapping_[kSessionCount]{};
    SharedBlock* block_[kSessionCount]{};
    bool opened_{false};
#endif
};

} // namespace MixDoctorator::IPC

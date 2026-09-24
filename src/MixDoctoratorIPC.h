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
constexpr std::uint32_t kMagic=0x4D445036u;
constexpr std::uint32_t kVersion=6u;

inline int clampSession(int session) noexcept {
    return std::clamp(session,0,kSessionCount-1);
}

struct alignas(64) Slot {
#ifdef _WIN32
    volatile LONG sequence{0};
    volatile LONG role{0};
    volatile LONG active{0};
    volatile LONG reserved{0};
    volatile LONG64 heartbeatMs{0};
    volatile LONG64 samplePosition{-1};
#else
    std::int32_t sequence{0},role{0},active{0},reserved{0};
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
    std::uint32_t magic{kMagic};
    std::uint32_t version{kVersion};
    Slot slots[kRoleCount]{};
};

struct Snapshot {
    Role role{Role::Unknown};
    bool connected{false};
    std::uint64_t heartbeatMs{0};
    std::int64_t samplePosition{-1};
    double rmsDb{-120.0};
    double peakDb{-120.0};
    double activity{0.0};
    double transient{0.0};
    double bands[kBandCount]{};
};

inline int slotForRole(Role r) noexcept {
    const int v=static_cast<int>(r);
    return (v>=1 && v<=kRoleCount) ? v-1 : -1;
}

inline double dbFromAmplitude(double v) noexcept {
    return 20.0*std::log10(std::max(v,1.0e-9));
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
                L"Local\\125A_MixDoctorator_POC_v6_S%d",
                session+1);

            mapping_[session]=CreateFileMappingW(
                INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,
                static_cast<DWORD>(sizeof(SharedBlock)),name);

            if(!mapping_[session]){
                close();
                return false;
            }

            const bool existed=
                GetLastError()==ERROR_ALREADY_EXISTS;

            block_[session]=static_cast<SharedBlock*>(
                MapViewOfFile(
                    mapping_[session],
                    FILE_MAP_ALL_ACCESS,0,0,sizeof(SharedBlock)));

            if(!block_[session]){
                close();
                return false;
            }

            if(!existed ||
               block_[session]->magic!=kMagic ||
               block_[session]->version!=kVersion){

                ZeroMemory(
                    block_[session],
                    sizeof(SharedBlock));

                block_[session]->magic=kMagic;
                block_[session]->version=kVersion;

                for(auto& slot:block_[session]->slots)
                    slot.samplePosition=-1;
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
        Role role,
        std::int64_t samplePosition,
        double rmsDb,
        double peakDb,
        double activity,
        double transient,
        const double* bands) noexcept {

#ifdef _WIN32
        if(!opened_ || !bands)
            return false;

        const int sessionIndex=clampSession(session);
        const int idx=slotForRole(role);

        if(idx<0 || !block_[sessionIndex])
            return false;

        auto& s=block_[sessionIndex]->slots[idx];

        InterlockedIncrement(&s.sequence);
        MemoryBarrier();

        s.role=static_cast<LONG>(role);
        s.samplePosition=static_cast<LONG64>(samplePosition);
        s.rmsDb=rmsDb;
        s.peakDb=peakDb;
        s.activity=activity;
        s.transient=transient;

        for(int i=0;i<kBandCount;++i)
            s.bands[i]=bands[i];

        s.heartbeatMs=static_cast<LONG64>(GetTickCount64());
        s.active=1;

        MemoryBarrier();
        InterlockedIncrement(&s.sequence);
        return true;
#else
        (void)session;(void)role;(void)samplePosition;(void)rmsDb;
        (void)peakDb;(void)activity;(void)transient;(void)bands;
        return false;
#endif
    }

    bool read(
        int session,
        Role role,
        Snapshot& out) noexcept {

#ifdef _WIN32
        if(!opened_)
            return false;

        const int sessionIndex=clampSession(session);
        const int idx=slotForRole(role);

        if(idx<0 || !block_[sessionIndex])
            return false;

        const auto& s=block_[sessionIndex]->slots[idx];

        for(int attempt=0;attempt<5;++attempt){
            const LONG before=s.sequence;
            MemoryBarrier();

            if(before&1)
                continue;

            Snapshot t;
            t.role=static_cast<Role>(s.role);
            t.heartbeatMs=static_cast<std::uint64_t>(s.heartbeatMs);
            t.samplePosition=static_cast<std::int64_t>(s.samplePosition);
            t.rmsDb=s.rmsDb;
            t.peakDb=s.peakDb;
            t.activity=s.activity;
            t.transient=s.transient;

            for(int i=0;i<kBandCount;++i)
                t.bands[i]=s.bands[i];

            MemoryBarrier();
            const LONG after=s.sequence;

            if(before==after && !(after&1)){
                const auto now=static_cast<std::uint64_t>(GetTickCount64());

                t.connected=
                    s.active!=0 &&
                    now>=t.heartbeatMs &&
                    (now-t.heartbeatMs)<1500u;

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

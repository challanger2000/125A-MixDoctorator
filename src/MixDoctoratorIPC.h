#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

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
constexpr std::uint32_t kMagic=0x4D445034u;
constexpr std::uint32_t kVersion=4u;

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
    double bands[kBandCount]{};
};

inline int slotForRole(Role r) noexcept {
    const int v=
        static_cast<int>(r);

    return
        (v>=1 && v<=kRoleCount)
        ? v-1
        : -1;
}

inline double dbFromAmplitude(
    double v) noexcept {

    return
        20.0*
        std::log10(
            std::max(
                v,
                1.0e-9));
}

class SharedMemory {
public:
    ~SharedMemory(){
        close();
    }

    SharedMemory()=default;
    SharedMemory(
        const SharedMemory&)=delete;

    SharedMemory& operator=(
        const SharedMemory&)=delete;

    bool open() noexcept {
#ifdef _WIN32
        if(block_)
            return true;

        mapping_=
            CreateFileMappingW(
                INVALID_HANDLE_VALUE,
                nullptr,
                PAGE_READWRITE,
                0,
                static_cast<DWORD>(
                    sizeof(
                        SharedBlock)),
                L"Local\\125A_MixDoctorator_POC_v4");

        if(!mapping_)
            return false;

        const bool existed=
            GetLastError()==
            ERROR_ALREADY_EXISTS;

        block_=
            static_cast<SharedBlock*>(
                MapViewOfFile(
                    mapping_,
                    FILE_MAP_ALL_ACCESS,
                    0,
                    0,
                    sizeof(
                        SharedBlock)));

        if(!block_){
            CloseHandle(
                mapping_);

            mapping_=nullptr;

            return false;
        }

        if(!existed ||
           block_->magic!=kMagic ||
           block_->version!=kVersion){

            ZeroMemory(
                block_,
                sizeof(
                    SharedBlock));

            block_->magic=
                kMagic;

            block_->version=
                kVersion;

            for(auto& slot:
                block_->slots)
                slot.samplePosition=-1;
        }

        return true;
#else
        return false;
#endif
    }

    void close() noexcept {
#ifdef _WIN32
        if(block_){
            UnmapViewOfFile(
                block_);

            block_=nullptr;
        }

        if(mapping_){
            CloseHandle(
                mapping_);

            mapping_=nullptr;
        }
#endif
    }

    bool publish(
        Role role,
        std::int64_t samplePosition,
        double rmsDb,
        double peakDb,
        double activity,
        const double* bands) noexcept {

#ifdef _WIN32
        if(!block_ &&
           !open())
            return false;

        const int idx=
            slotForRole(
                role);

        if(idx<0 ||
           !bands)
            return false;

        auto& s=
            block_->
            slots[idx];

        InterlockedIncrement(
            &s.sequence);

        MemoryBarrier();

        s.role=
            static_cast<LONG>(
                role);

        s.samplePosition=
            static_cast<LONG64>(
                samplePosition);

        s.rmsDb=rmsDb;
        s.peakDb=peakDb;
        s.activity=activity;

        for(int i=0;
            i<kBandCount;
            ++i)
            s.bands[i]=
                bands[i];

        s.heartbeatMs=
            static_cast<LONG64>(
                GetTickCount64());

        s.active=1;

        MemoryBarrier();

        InterlockedIncrement(
            &s.sequence);

        return true;
#else
        (void)role;
        (void)samplePosition;
        (void)rmsDb;
        (void)peakDb;
        (void)activity;
        (void)bands;

        return false;
#endif
    }

    bool read(
        Role role,
        Snapshot& out) noexcept {

#ifdef _WIN32
        if(!block_ &&
           !open())
            return false;

        const int idx=
            slotForRole(
                role);

        if(idx<0)
            return false;

        const auto& s=
            block_->
            slots[idx];

        for(int attempt=0;
            attempt<5;
            ++attempt){

            const LONG before=
                s.sequence;

            MemoryBarrier();

            if(before&1)
                continue;

            Snapshot t;

            t.role=
                static_cast<Role>(
                    s.role);

            t.heartbeatMs=
                static_cast<
                    std::uint64_t>(
                        s.heartbeatMs);

            t.samplePosition=
                static_cast<
                    std::int64_t>(
                        s.samplePosition);

            t.rmsDb=
                s.rmsDb;

            t.peakDb=
                s.peakDb;

            t.activity=
                s.activity;

            for(int i=0;
                i<kBandCount;
                ++i)
                t.bands[i]=
                    s.bands[i];

            MemoryBarrier();

            const LONG after=
                s.sequence;

            if(before==after &&
               !(after&1)){

                const auto now=
                    static_cast<
                        std::uint64_t>(
                            GetTickCount64());

                t.connected=
                    s.active!=0 &&
                    now>=
                        t.heartbeatMs &&
                    (now-
                     t.heartbeatMs)<
                        1500u;

                out=t;

                return true;
            }
        }
#endif

        return false;
    }

private:
#ifdef _WIN32
    HANDLE mapping_{
        nullptr};

    SharedBlock* block_{
        nullptr};
#endif
};

} // namespace MixDoctorator::IPC

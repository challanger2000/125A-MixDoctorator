#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <type_traits>

namespace MixDoctorator::Realtime {

template<typename T,std::size_t Capacity>
class SpscQueue {
    static_assert(Capacity>=2,"SPSC capacity must be at least two");
    static_assert(std::is_trivially_copyable_v<T>,
                  "SPSC payload must be trivially copyable");

public:
    bool push(const T& value) noexcept {
        const auto head=
            head_.load(std::memory_order_relaxed);
        const auto next=
            increment(head);

        if(next==
           tail_.load(std::memory_order_acquire))
            return false;

        storage_[head]=value;
        head_.store(next,std::memory_order_release);
        return true;
    }

    bool pop(T& value) noexcept {
        const auto tail=
            tail_.load(std::memory_order_relaxed);

        if(tail==
           head_.load(std::memory_order_acquire))
            return false;

        value=storage_[tail];
        tail_.store(
            increment(tail),
            std::memory_order_release);
        return true;
    }

private:
    static constexpr std::size_t kStorage=
        Capacity+1;

    static constexpr std::size_t increment(
        std::size_t value) noexcept {
        return (value+1)%kStorage;
    }

    std::array<T,kStorage> storage_{};
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
};

static_assert(
    std::atomic<std::size_t>::is_always_lock_free,
    "MixDoctorator realtime queues require lock-free atomic indices");

} // namespace MixDoctorator::Realtime

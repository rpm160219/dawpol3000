// Lock-free kolejka jeden-producent / jeden-konsument (UI -> audio).
#pragma once
#include <atomic>
#include <stdint.h>

namespace dp {

template <typename T, int N>
class Spsc {
    static_assert((N & (N - 1)) == 0, "N musi byc potega 2");
    T buf_[N];
    std::atomic<uint32_t> w_{0}, r_{0};

public:
    bool push(const T& v) {
        uint32_t w = w_.load(std::memory_order_relaxed);
        if (w - r_.load(std::memory_order_acquire) >= (uint32_t)N) return false;
        buf_[w & (N - 1)] = v;
        w_.store(w + 1, std::memory_order_release);
        return true;
    }
    bool pop(T& v) {
        uint32_t r = r_.load(std::memory_order_relaxed);
        if (r == w_.load(std::memory_order_acquire)) return false;
        v = buf_[r & (N - 1)];
        r_.store(r + 1, std::memory_order_release);
        return true;
    }
};

}  // namespace dp

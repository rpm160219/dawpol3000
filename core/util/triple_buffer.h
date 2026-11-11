// Potrojny bufor bez blokad: UI publikuje cale migawki danych (np. pattern),
// watek audio bierze najnowsza. Brak alokacji, brak lockow.
#pragma once
#include <atomic>

namespace dp {

template <typename T>
class TripleBuffer {
    T buf_[3];
    std::atomic<int> mid_{1};  // indeks srodkowego bufora | 4 = swiezy
    int back_ = 0, front_ = 2;

public:
    // Watek UI: zapisz cala migawke do back(), potem publish().
    T& back() { return buf_[back_]; }
    void publish() { back_ = mid_.exchange(back_ | 4, std::memory_order_acq_rel) & 3; }
    // Watek audio: consume() zwraca true, gdy przyszla nowa migawka; czytaj front().
    bool consume() {
        if (!(mid_.load(std::memory_order_acquire) & 4)) return false;
        front_ = mid_.exchange(front_, std::memory_order_acq_rel) & 3;
        return true;
    }
    const T& front() const { return buf_[front_]; }
};

}  // namespace dp

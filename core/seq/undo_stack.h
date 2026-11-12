// Historia zmian projektu (undo): pierscien migawek calego projektu.
// Jedna pozycja = jeden "gest" edycji (wcisniecie przycisku albo jedno pokrecenie galki) —
// App zatwierdza krok dopiero, gdy przez klatke nic sie nie zmienia.
// Pamiec: DEPTH x ~210 KB (32 kroki ~ 7 MB), przydzielona raz przy starcie.
#pragma once
#include "seq/project.h"

namespace dp {

class UndoStack {
public:
    static const int DEPTH = 32;
    void clear() { n_ = 0; }
    int size() const { return n_; }
    void push(const Project& p) {
        head_ = (head_ + 1) % DEPTH;
        buf_[head_] = p;
        if (n_ < DEPTH) n_++;
    }
    bool pop(Project& out) {
        if (n_ == 0) return false;
        out = buf_[head_];
        head_ = (head_ + DEPTH - 1) % DEPTH;
        n_--;
        return true;
    }

private:
    Project buf_[DEPTH];
    int head_ = 0, n_ = 0;
};

}  // namespace dp

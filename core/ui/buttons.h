// Stan przyciskow dla ekranow: wcisniecie, puszczenie, auto-powtarzanie (D-pad).
#pragma once
#include "ui/input.h"

namespace dp {

class Buttons {
public:
    void update(const InputState& in, float dt) {
        for (int i = 0; i < BTN_COUNT; i++) {
            prev_[i] = cur_[i];
            cur_[i] = in.btn[i];
            rep_[i] = false;
            if (cur_[i] && !prev_[i]) { held_[i] = 0; next_[i] = kDelay; rep_[i] = true; }
            else if (cur_[i]) {
                held_[i] += dt;
                if (held_[i] >= next_[i]) { rep_[i] = true; next_[i] += kRate; }
            }
        }
    }
    bool down(int b) const { return cur_[b]; }
    bool pressed(int b) const { return cur_[b] && !prev_[b]; }
    bool released(int b) const { return !cur_[b] && prev_[b]; }
    bool repeat(int b) const { return rep_[b]; }  // wcisniecie + powtorzenia przy trzymaniu
    // Kopia z wylaczonymi przyciskami (np. gdy pasek akcji przejmuje A/B/X/Y).
    Buttons masked(const int* list, int n) const {
        Buttons c = *this;
        for (int i = 0; i < n; i++) c.cur_[list[i]] = c.prev_[list[i]] = c.rep_[list[i]] = false;
        return c;
    }

private:
    static constexpr float kDelay = 0.32f, kRate = 0.06f;
    bool cur_[BTN_COUNT] = {}, prev_[BTN_COUNT] = {}, rep_[BTN_COUNT] = {};
    float held_[BTN_COUNT] = {}, next_[BTN_COUNT] = {};
};

}  // namespace dp

// Analog jako enkoder wzgledny: wychylenie = predkosc zmiany (martwa strefa + krzywa).
#pragma once
#include <math.h>

namespace dp {

class RelEncoder {
public:
    // Ciagla zmiana: zwraca przyrost (jednostki/s przy pelnym wychyleniu = rate).
    static float delta(float axis, float dt, float rate) { return shape(axis) * rate * dt; }
    // Zmiana skokowa (zabki): zwraca liczbe krokow do wykonania w tej klatce.
    int steps(float axis, float dt, float stepsPerSec) {
        float s = shape(axis);
        if (s == 0.f) { acc_ = 0.f; first_ = true; return 0; }
        if (first_) { first_ = false; acc_ = 0.f; return s > 0 ? 1 : -1; }  // pierwszy krok od razu
        acc_ += s * stepsPerSec * dt;
        int n = (int)acc_;
        acc_ -= n;
        return n;
    }

private:
    float acc_ = 0.f;
    bool first_ = true;
    static float shape(float a) {
        const float dz = 0.2f;
        float m = fabsf(a);
        if (m < dz) return 0.f;
        float v = (m - dz) / (1.f - dz);
        v = v * v;
        return a < 0 ? -v : v;
    }
};

}  // namespace dp

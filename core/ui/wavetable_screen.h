// Ekran silnika WAVETABLE: port 1:1 ekranu z op1r (Hundekraft, "oscilloscope stack", wavetable_screen.c + gen_svg.py,
// GPL-3.0-or-later). Ta sama geometria (wavetable_geom.h), kolory i wzory; ekran OP-1 320x160 skalowany 2x.
// Czesc statyczna (stos 10 klatek tabeli, podloga, szyny, os, krzywa-duch) rysowana raz do cache.
// Czesc ruchoma jak w oryginale: zielona klatka przy WAVE (+ slabsza kopia przesunieta o DETUNE i dryf faz),
// niebieski nawias/znacznik/glowica, biala krzywa filtra (CUTOFF/RESONANCE), czerwone luki rezonansu,
// lawendowe linie i harmoniczne. Poziom = wejscie ze sciezki (puchnie do 15 %).
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

namespace dp {

class WavetableScreen {
public:
    static constexpr float S = 2.f;     // skala ekranu OP-1 -> nasz
    static constexpr float OX = 0.f;    // przesuniecie (nasze px)
    static constexpr float OY = 44.f;
    // k = galki 0..32767; level = poziom sciezki 0..1; drift = ph2-ph1 najglosniejszego glosu; voices = ile gra
    void draw(Canvas& c, const int16_t k[4], float level, uint32_t drift, int voices, float dt);

private:
    Canvas* cache_ = nullptr;
    int32_t act_ = 0, lvl_ = 0;  // aktywnosc Q10, poziom Q15
    uint32_t ph_ = 0, drift_ = 0;
    int32_t k0_ = -1, k3_ = -1, nwave_ = 0, peak_ = 0;
    int32_t wx_[65], wy_[65];
    int32_t cy_[241];
    void buildCache();
};

}  // namespace dp

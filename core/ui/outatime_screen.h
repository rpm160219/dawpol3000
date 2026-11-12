// Ekran silnika OUTATIME: port 1:1 ekranu z paczki Hundekraft (outatime_screen.c + outatime.svg) — DeLorean na drodze,
// wyswietlacze "time circuits" (rok docelowy, rok glosu, rok wyjazdu), predkosciomierz MPH, odczyt GW, plomienie zaplonu
// (TRAIL) za samochodem. Ekran OP-1 320x160 skalowany 2x.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

extern "C" struct OutatimeCore;

namespace dp {

class OutatimeScreen {
public:
    void draw(Canvas& c, const int16_t words[8], const int16_t live[4], const ::OutatimeCore* core, bool page2);
};

void drawOutatimeIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

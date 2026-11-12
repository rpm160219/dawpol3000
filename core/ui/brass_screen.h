// Ekran silnika BRASSOPHONE: port 1:1 ekranu z op1r (horn.c + brass-handmade.svg, nvmsocool) — kod C oryginalu
// rysuje przez funkcje zwrotne. Rog w stylu Dr. Seussa: niebieskie tlumiki na czarach obracaja sie z TIMBRE, zielony
// suwak puzonu wysuwa sie z SWEEP, bialy balon pompuje sie (i drga) z VIBRATO, czerwony spinner kreci sie z VIB RATE.
// Ekran OP-1 320x160 skalowany 2x.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

namespace dp {

class BrassScreen {
public:
    void draw(Canvas& c, const int16_t live[4]);
};

void drawBrassIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

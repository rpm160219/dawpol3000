// Ekran silnika SID: port ekranu z op1r (sid_screen.c + sid.svg, Hundekraft). Statyczny rysunek (sid.svg) i odczyty po
// prawej jak w oryginale: WAVE + nazwa fali (niebieski), VOICES / WIDTH / INTERVAL (zielony), CUTOFF (bialy),
// RES (czerwony) — galki jak slysza je glosy. Odczyty nasza czcionka (wielkie litery; oryginal: czcionka OP-1).
// Ekran OP-1 320x160 skalowany 2x.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

namespace dp {

class SidScreen {
public:
    void draw(Canvas& c, const int16_t k[4]);

private:
    Canvas* cache_ = nullptr;
};

void drawSidIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

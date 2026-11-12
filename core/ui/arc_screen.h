// Ekran silnika ARC: port 1:1 ekranu z op1r (czesc "screen" w arc.c + duddell.svg, ultralazr) — kod C oryginalu rysuje
// linie. Portret fazowy ostatniego glosu jako niebieski warkocz za dwiema elektrodami weglowymi (okrag dla sinusa,
// kanciasta petla relaksacji przy wysokim CURRENT, rozmyty od HISS), zielony portret 2. lampy przy sprzezeniu, luk
// miedzy koncowkami rosnie i migocze z poziomem, koncowki zarza sie z CARBON, czerwone iskry przy zaplonie. Tarcze z
// dolu oryginalu i napis proporcji pominiete (dublowalyby galki DAW-POL). Ekran OP-1 320x160 skalowany 2x.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

extern "C" struct ArcCore;

namespace dp {

class ArcScreen {
public:
    void draw(Canvas& c, const int16_t words[8], const int16_t live[4], const ::ArcCore* core, bool page2);
};

void drawArcIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

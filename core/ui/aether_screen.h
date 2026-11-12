// Ekran silnika AETHER: port 1:1 ekranu z op1r (czesc "screen" w aether.c + aether.svg, pcppcp) — kod C oryginalu
// ustawia wezly i rysuje linie. Antena satelitarna na wiezy sledzi przechylenie (u nas wirtualne ze strony TILT):
// przod / tyl unosi czasze, bok obraca ja; FORM pokazuje 1..4 fale radiowe, drgajace przy graniu; kropkowane sygnaly
// jasnieja z obwiednia; gwiazdy w kolorach ORBIT / SPACE / DEPTH. Tarcze, suwaki i etykiety oryginalu pominiete
// (dublowalyby galki DAW-POL). Ekran OP-1 320x160 skalowany 2x.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

extern "C" struct AetherCore;

namespace dp {

class AetherScreen {
public:
    void draw(Canvas& c, const int16_t words[8], const int16_t live[4], const ::AetherCore* core, bool page2);
};

void drawAetherIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

// Ekran silnika VAPORA: port 1:1 ekranu z op1r (vapora_screen.c + vapora.svg, AiAAAA) — oryginalny kod C rysuje przez
// core/ui/vapora_screen_c.c. Proceduralny ocean z siatki trojkatow (66 wierzcholkow, 165 krawedzi), ksztalt fali =
// CHARACTER (14 wzorow), wysokosc = MORPH; slonce wschodzi z BRIGHTNESS (tarcza "zacmienia" sie przy niskim), pulsuje
// na czerwono z MOTION w tempie SPEED; gwiazdy i znak w niebie = INTERVAL, nocne niebo przy ciemnym brzmieniu; kazda
// nuta = fala uderzeniowa na wodzie (dluzsza przy trzymaniu). SUN = wzor slonca (7, tylko obraz).
// Ekran OP-1 320x160 skalowany 2x.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

extern "C" struct VaporaCore;

namespace dp {

class VaporaScreen {
public:
    // knobs = 8 slow (strona 2 w 5..7), live = galki 0..3 po modulacji, track = numer sciezki (inna = nowy widok)
    void draw(Canvas& c, const int16_t knobs[8], const int16_t live[4], const ::VaporaCore* core, int sun, int track);
};

// ilustracja na karcie ENGINE: ten sam ekran na zywo (bez nut), wpisany w prostokat
void drawVaporaIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

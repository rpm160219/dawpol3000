// Ekran silnika HOLLOW SAINT: port 1:1 ekranu z op1r (czesc "screen" w hollow_saint.c + hollow_saint.svg) — kod C
// oryginalu rysuje przez funkcje zwrotne. Lira korbowa z gory w gotyckim luku: kolo i korba kreca sie przy granych
// nutach (szarpniecia na uderzeniach kola), struny drgaja (burdony z DRONE, melodia z STRINGS, trompeta z "psem"),
// pies grzechocze z brzeczykiem, zapalone klawisze granych nut, platki rozety zapalane CATHEDRAL, aureola swietego i
// dzwon kolyszacy sie przy uderzeniu; RELIC: kaseta z obracajacymi sie szpulami i kurz. Wskazniki galek z dolu
// oryginalu pominiete (dublowalyby galki DAW-POL). Ekran OP-1 320x160 skalowany 2x.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

extern "C" struct HollowCore;

namespace dp {

class HollowScreen {
public:
    void draw(Canvas& c, const int16_t words[8], const int16_t live[4], const ::HollowCore* core, bool relic);
};

void drawHollowIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

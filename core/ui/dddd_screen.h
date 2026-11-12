// Ekran silnika DDDD: port 1:1 ekranu z op1r (czesc "screen" w dddd.c, pcppcp) — kod C oryginalu rysuje linie.
// Minimalistyczna osa (z gory), znieksztalcana galkami: FEEDBACK wydluza odwlok, DESTROY poszarpuje i zgrubia linie,
// SOURCE zmienia skrzydla (gladkie .. kanciaste), HANDS przyspiesza trzepot i rzuca osa przy skokach "rak" (czerwone
// rozdarcia); dzwiek nia trzesie. Dolny rzad z napisami pominiety (dublowalby galki). Ekran OP-1 320x160 skalowany 2x.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

extern "C" struct DdddCore;

namespace dp {

class DdddScreen {
public:
    void draw(Canvas& c, const int16_t words[8], const int16_t live[4], const ::DdddCore* core, bool page2);
};

void drawDdddIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

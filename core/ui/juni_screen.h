// Ekran silnika JUNI: port 1:1 ekranu z op1r (czesc "screen" w juni.c + juni.svg, Hundekraft) — kod C oryginalu rysuje
// linie na nieruchomym rysunku. Biala noc nad fiordem: slonce nad horyzontem (TONE, aureola z RESONANCE), grzbiety gor
// rysuja ksztalt fali (SHAPE, z PWM), ognisko na brzegu buchnie z SWEEP i obwiednia, odbicia na wodzie faluja z
// CHORUS, linie wody z HPF. Ekran OP-1 320x160 skalowany 2x.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

extern "C" struct JuniCore;

namespace dp {

class JuniScreen {
public:
    void draw(Canvas& c, const int16_t words[8], const int16_t live[4], const ::JuniCore* core);
};

void drawJuniIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

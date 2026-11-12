// Ekran startowy: nazwa programu (DAW-POL 3000) przy uruchomieniu aplikacji.
// Animacja ~2,5 s: fala rysowana od lewej w 4 kolorach enkoderow, potem napis i podpis; na koncu wygaszenie.
// Dowolny przycisk pomija (App::update).
#pragma once
#include "gfx/canvas.h"

namespace dp {

static const float SPLASH_LEN = 2.6f;  // sekundy
void drawSplash(Canvas& c, float t);   // t = czas od startu (s)

}  // namespace dp

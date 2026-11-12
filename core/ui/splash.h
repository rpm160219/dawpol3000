// Ekran startowy: nazwa programu (DAW-POL 3000) przy uruchomieniu aplikacji.
// ~4,5 s: animacja ~1,7 s (fala rysowana od lewej w 4 kolorach enkoderow, potem napis i podpis), logo stoi, na koncu wygaszenie.
// Dowolny przycisk pomija (App::update).
#pragma once
#include "gfx/canvas.h"

namespace dp {

static const float SPLASH_LEN = 4.5f;  // sekundy (animacja ~1,7 s, potem logo stoi, wygaszenie 0,4 s)
void drawSplash(Canvas& c, float t);   // t = czas od startu (s)

}  // namespace dp

// Wlasny cienki font wektorowy (kreskowy). Tylko wielkie litery, cyfry, interpunkcja.
#pragma once
#include "gfx/canvas.h"

namespace dp {

enum TextAlign { ALIGN_LEFT = 0, ALIGN_CENTER = 1, ALIGN_RIGHT = 2 };

// size = wysokosc wielkiej litery w pikselach, y = gora liter. Zwraca szerokosc.
float drawText(Canvas& c, float x, float y, const char* s, float size, float width, Color col,
               int align = ALIGN_LEFT);
float textWidth(const char* s, float size);
// Kreski napisu jako odcinki (x od 0, y = 0 gora liter) — do rysowania z wlasnym przeksztalceniem (np. ekran GIN).
void textStrokes(const char* s, float size, void (*seg)(void* u, float x0, float y0, float x1, float y1), void* u);

}  // namespace dp

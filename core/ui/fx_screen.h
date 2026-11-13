// Ekran efektu op1r (JUNI CHORUS: brzozy nad woda, SISTER: nawa katedry z rozeta i swiecami) — oryginalny kod C ekranu
// (core/ui/fx_screen_*.c) na nieruchomym rysunku z SVG efektu, napisy czcionka DAW-POL. Ekran OP-1 320x160 w skali s.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

namespace dp {

// tel = telemetria egzemplarza z miksera (AudioEngine::fxTelemetry); reset = pierwsza klatka po otwarciu
void drawFxScreen(Canvas& c, int fx, const int16_t w[8], bool page2, const uint8_t* tel, bool reset, float x, float y, float s);

}  // namespace dp

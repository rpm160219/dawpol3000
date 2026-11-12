// Ekran silnika GIN (perkusja SquareWave Studio z paczki Hundekraft 3.1.4): port 1:1 ekranu gin_screen.c — tryb gry:
// butelka GIN (A, niebieska) i TONIC (B, czerwona) przelewaja sie do kieliszka przy MORPH, babelki = udzial szumu,
// puls przy uderzeniu; strony edytora: wykresy fali, obwiedni, modulacji, filtra, miksu, EQ i dynamiki. Napisy wlasna
// czcionka DAW-POL (oryginal uzywa czcionki z firmware OP-1). Ekran OP-1 320x160 skalowany 2x.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

extern "C" {
struct GinUi;
}

namespace dp {

class GinScreen {
public:
    void draw(Canvas& c, const struct ::GinUi* ui, uint32_t hits);
};

uint32_t ginNowMs();
void drawGinIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

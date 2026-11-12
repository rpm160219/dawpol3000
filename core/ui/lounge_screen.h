// Ekran silnika LOUNGE (wlasny rysunek, uklad wg zrzutu KEYS z "OP-1 Mods"): przekroj pianina elektrycznego pod
// pokrywa (LID) z koktajlem — TINE: blok, listwa tonowa, zab, mlotek, sprezynka strojenia, przetwornik z kablem;
// REED: jezyczek z ciezarkiem i przetwornik plytkowy; CLAV: struna z tlumikiem z przedzy, stycznik i dwa przetworniki.
// BARK = wysokosc zamachu mlotka, TONE = polozenie przetwornika, DECAY = krzywa w rogu, TYPE = nazwa (czerwona).
// Drgania i mlotek animowane z silnika; na dole kratki 6 glosow.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

extern "C" struct LoungeCore;

namespace dp {

class LoungeScreen {
public:
    void draw(Canvas& c, const int16_t knobs[4], const ::LoungeCore* core, int voices, float dt);
private:
    float t_ = 0, swing_ = 0, motion_ = 0;
    uint32_t strikes_ = 0;
};

void drawLoungeIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

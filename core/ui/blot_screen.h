// Ekran silnika BLOT: port 1:1 ekranu z op1r (czesc "screen" w blot.c, pcppcp) — kod C oryginalu rysuje linie przez
// funkcje zwrotna. Kwasowy smiley: oczy otwieraja sie z CUTOFF, usmiech poglebia sie z RESONANCE (dolki od 60 %), usta
// otwieraja sie z kazda nuta (ENV MOD x obwiednia filtra), krople ponizej dluzsze z DECAY, kontur faluje z DRIVE,
// mrugniecie przy akcencie. Dolny rzad z napisami pominiety (dublowalby galki DAW-POL). Ekran OP-1 320x160 skalowany 2x.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

extern "C" struct BlotCore;

namespace dp {

class BlotScreen {
public:
    void draw(Canvas& c, const int16_t words[8], const int16_t live[4], const ::BlotCore* core);
};

void drawBlotIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

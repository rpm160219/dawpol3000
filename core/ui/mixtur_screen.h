// Ekran silnika MIXTUR: port 1:1 ekranu z op1r (czesc "screen" w mixtur.c + mixtur.svg, ultralazr) — kod C oryginalu
// rysuje przez funkcje zwrotne. Drabina undertonow: 24 struny z mostka, struna n ma dlugosc 1/n (koncowki = hiperbola
// szeregu); cztery wybrane struny w kolorach galek, przy trzymanych klawiszach zwolniony zegar dzielnikow — paciorek
// schodzi o stopien na takt, struna blyska przy zawinieciu. Pod spodem dwie krzywe formantow na osi log (150 Hz ..
// 6 kHz): szare, na stronie 2 (L3) niebieska i zielona. Napisy z dolu oryginalu pominiete (dublowalyby galki).
// Ekran OP-1 320x160 skalowany 2x.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

extern "C" struct MixturCore;

namespace dp {

class MixturScreen {
public:
    void draw(Canvas& c, const int16_t words[8], const int16_t live[4], const ::MixturCore* core, bool page2);
};

void drawMixturIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

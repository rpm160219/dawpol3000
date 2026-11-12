// Ekran silnika SUPERSAW: port 1:1 ekranu z op1r (czesc "screen" w supersaw.c + supersaw.svg, pcppcp) — kod C
// oryginalu rysuje przez funkcje zwrotne. Minimalistyczna pila lancuchowa: gorny bieg lancucha = ksztalt fali jako zeby
// (pila -> kwadrat z MORPH) z przyciemniona niebieska kopia dryfujaca z DETUNE (dudnienie), dolny bieg = sub oktawe
// nizej (wysokosc = SUB), dym z wydechu = SUB, spust czerwony i drganie pily = DRIVE, korek paliwa niebieski = DETUNE,
// uchwyt zielony = MORPH. Lancuch rusza przy trzymanych klawiszach. Bez rzedu tarcz OP-1 (galki rysuje DAW-POL). Ekran OP-1 320x160 skalowany 2x.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

extern "C" struct SupersawCore;

namespace dp {

class SupersawScreen {
public:
    void draw(Canvas& c, const int16_t live[4], const ::SupersawCore* core);
};

void drawSupersawIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

// Ekran silnika EVA: port 1:1 ekranu z op1r (Hundekraft: czesc "screen" w eva.c + eva.svg, GPL-3.0-or-later).
// Astronautka w helmie. Na zywo jak w oryginale: cztery wskazniki (PRESSURE, FREQ, OXYGEN, COM — 0..16 zapalonych
// kresek, maski przesuwaja sie w prawo), celownik na wizjerze idzie za FREQ od lewego do prawego oka, wizjer paruje
// z oddechem (poziom silnika, gdy gra; inaczej spokojny oddech z OXYGEN), czerwony pasek radia: wylaczony przy COM 0,
// miga przy sygnale Quindar, znika przy zaniku nosnej. Strona 2 COM (L3): napis OCT i oktawa sygnalu na wskazniku.
// Ekran OP-1 320x160 skalowany 2x. Rysowanie w kolejnosci dokumentu (statyczny poczatek w cache).
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"
#include "synth/eva_dsp.h"

namespace dp {

class EvaScreen {
public:
    // k = galki (po modulacji, gdy gra); k7 = slowo 7 (oktawa sygnalu); page2 = strona 2 COM; view = stan dzwieku
    void draw(Canvas& c, const int16_t k[4], int16_t k7, bool page2, const EvaView& view);

private:
    Canvas* cache_ = nullptr;
    int firstLive_ = 0;
    uint32_t lastBlocks_ = 0, frame_ = 0, seed_ = 0x1234567u;
    int32_t act_ = 0, fog_ = 0;
    uint32_t rnd() { return (seed_ = seed_ * 1664525u + 1013904223u) >> 16; }
};

// Ilustracja silnika (karta ENGINE): cala scena przy domyslnych galkach
void drawEvaIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

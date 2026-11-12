// Ekran silnika OXBOW: port 1:1 ekranu z kennel (Hundekraft: czesc "screen" w oxbow.c + oxbow.svg + oxbow_geom.h).
// Kudlaty byk z niebieskimi rogami, czerwone ptaki na rogach, zielony dzwonek, gwiazdy. Na zywo jak w oryginale:
// rogi rozszerzaja sie z CUTOFF (ponizej domyslnej opadaja), dzwonek kolysze sie z RESONANCE (mocniej, gdy gra),
// oddech rosnie z ENV AMOUNT i "puchnie" z obwiednia filtra, ptaki wedruja po rogach z DETUNE, gwiazda na kazdy
// grajacy glos; na dole odczyty CUTOFF / RES / ENV / DETUNE (strona 2: WAVE / FILTER / DRIVE).
// Ekran OP-1 320x160 skalowany 1,82x z marginesami (pasek odczytow nad napisami strony). Tlo w cache. Rysowanie w kolejnosci dokumentu (statyczny poczatek w cache).
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"
#include "synth/oxbow_dsp.h"

namespace dp {

// OXBOW+: odczyty pod bykiem pokazuja typ filtra (bialy) i ENV>PITCH (czerwony) zamiast ENV i DETUNE; oddech reaguje
// na typ filtra, ptaki na ENV>PITCH (decyzja Patryka: ENV AMOUNT jest na stronie FILTER ENV, DETUNE na OSC 2).
struct OxbowPlusShow { int ftype; int envPitch; };  // envPitch -64..63

class OxbowScreen {
public:
    // k = galki (po modulacji, gdy gra); words = strona 2 (WAVE, FILTER, DRIVE); page2 = pokazuj strone 2
    void draw(Canvas& c, const int16_t k[4], const int16_t words[3], bool page2, const OxbowView& view,
              const OxbowPlusShow* plus = nullptr);

private:
    Canvas* cache_ = nullptr;
    int32_t bgKey_[4] = {};  // poza rogow i ptakow, z ktora narysowano tlo w cache
    uint32_t blocks_ = 0, frame_ = 0, swing_ = 0;
    int32_t act_ = 0, puff_ = 1024;
};

// Ilustracja silnika (karta ENGINE): cala scena przy domyslnych galkach
void drawOxbowIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

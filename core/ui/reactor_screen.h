// Ekran silnika REACTOR: port 1:1 ekranu z op1r (Hundekraft: czesc "screen" w reactor.c + reactor.svg +
// reactor_geom.h, GPL-3.0-or-later). Elektrownia jadrowa z pulpitem. Na zywo jak w oryginale: prety wysuwane z RODS,
// wskazowki czterech pokretel (galki), wskazniki (chlodziwo, promieniowanie z trzaskami, prety, temperatura rdzenia),
// para z wiezy (wyzsza i szybsza, gdy goraco), czerwone pasy komina i przycisk migaja w stanie krytycznym, odczyty
// "COOLING 50" itd. Drzenie: gdy rdzen zbliza sie do stanu krytycznego, elektrownia brzeczy, w stanie krytycznym i przy
// ucieczce trzesie sie (kazda czesc osobno), ~1 s po ostatnim glosie jest dokladnie w spoczynku.
// Ekran OP-1 320x160 skalowany 2x.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"
#include "synth/reactor_dsp.h"

namespace dp {

class ReactorScreen {
public:
    // k = galki (po modulacji, gdy gra); view = stan glosow; dt = czas klatki (s)
    void draw(Canvas& c, const int16_t k[4], const ReactorView& view, float dt);
};

// Ilustracja silnika (karta ENGINE): cala scena w spoczynku przy domyslnych galkach
void drawReactorIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

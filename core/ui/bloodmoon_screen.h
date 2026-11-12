// Ekran silnika BLOODMOON: port 1:1 ekranu z op1r (moon.c + bloodmoon.svg, Nola Drift / kennel) — kod C oryginalu
// ustawia wezly. Krwawy ksiezyc za katedra: ksiezyc wschodzi z MOONRISE, niebieska rozeta obraca sie z BLOOD, zielone
// dzwony na wiezach kolysza sie z ECLIPSE, biale gwiazdy migocza z HAUNTING. Napisy z nazwami i wartosciami galek
// pominiete (dublowalyby galki DAW-POL). Ekran OP-1 320x160 skalowany 2x.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

namespace dp {

class BloodScreen {
public:
    void draw(Canvas& c, const int16_t live[4]);
};

void drawBloodIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

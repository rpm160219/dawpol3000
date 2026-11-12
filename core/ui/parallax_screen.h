// Ekran silnika PARALLAX (wlasny, clean room wg zrzutu ekranu "dimension" z Opus): u gory nazwy galek w kolorach
// enkoderow (WAVEFORM, UNISON z suwakiem, FILTER FREQ z wartoscia w Hz, RES), po lewej kolo fal przewijane galka,
// w srodku oscyloskop XY (sygnal po filtrze: x = s(t), y = s(t + T/4)) z cieniem rozstrojonego glosu unison, osie
// -1 / 0 / 1, po prawej czerwony szczyt rezonansu.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

namespace dp {

class ParallaxScreen {
public:
    void draw(Canvas& c, const int16_t knobs[4], float dt);
private:
    int16_t last_[4] = {-1, -1, -1, -1};
    float main_[128], ghost_[128];
    float anim_ = 0;
};

void drawParallaxIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

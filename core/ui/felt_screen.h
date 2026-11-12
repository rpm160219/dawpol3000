// Ekran silnika FELT (wlasny rysunek): przekroj fortepianu — podniesiona pokrywa na podporce, swieca, struny od
// kolkow do mostka (1..3 wg CORDA, napis UNA / DUE / TRE CORDE), mlotek z warstwami filcu (FELT) i pasek filcu przed
// strunami, tlumik (unosi sie przy trzymanym klawiszu), klawisz na dzwigni, plyta rezonansowa z zebrami, krzywa DECAY,
// czerwony pyl mechaniki (NOISE) przy uderzeniu. Mlotek wylatuje do strun przy kazdym uderzeniu, struny drgaja.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

extern "C" struct FeltCore;

namespace dp {

class FeltScreen {
public:
    void draw(Canvas& c, const int16_t knobs[4], const ::FeltCore* core, float dt);
private:
    float t_ = 0, swing_ = 1, motion_ = 0, damper_ = 0, dust_ = 0;
    uint32_t strikes_ = 0;
};

void drawFeltIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

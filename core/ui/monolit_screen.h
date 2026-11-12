// Ekran silnika MONOLIT: port 1:1 ekranu z op1r (Hundekraft: czesc "screen" w monolit.c + monolit.svg). Monolit w nocnym
// krajobrazie. Na zywo jak w oryginale: zielony grzbiet nad monolitem = filtr (waski i stromy przy niskim CUTOFF,
// szeroki przy otwartym), HEAT podnosi szczyt i czerwieni jego czubek, niebieskie kregi na ziemi = SHAPE (2..7,
// rozchodza sie, gdy CONTOUR jest otwarty), biala przerywana orbita = CONTOUR (krotka i niska dla pluck, szeroka
// dla swell) z kropka wspinajaca sie z obwiednia; strona 2: BODY / VEL / AGE w prawym gornym rogu.
// Ekran OP-1 320x160 skalowany 2x.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

namespace dp {

class MonolitScreen {
public:
    // k = galki (po modulacji, gdy gra); words = strona 2 (BODY, VEL, AGE); env = CONTOUR glosu (Q15)
    void draw(Canvas& c, const int16_t k[4], const int16_t words[3], bool page2, int32_t env, bool live);

private:
    Canvas* cache_ = nullptr;
    int32_t phase_ = 0;
};

void drawMonolitIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

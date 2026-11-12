// Ekran silnika TAFFY: port 1:1 ekranu z op1r (Hundekraft, "bubblegum": screen.c + taffy.svg, GPL-3.0-or-later).
// Dziewczyna z profilu dmucha balon z gumy. Statyczna grafika (dziewczyna, chmury, gwiazdy, usta) z taffy.svg
// (taffy_art.h) w cache; na zywo jak w oryginale: obrys balonu = fala (WAVE, niebieski), glebokosc platkow = DCW
// (zielony, tez napiecie nici), iskry = ENV (bialy), odblaski = RESONANCE (czerwony), rozmiar = poziom glosu.
// Ekran OP-1 320x160 skalowany 2x (jak WAVETABLE).
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

namespace dp {

class TaffyScreen {
public:
    // k = galki 0..32767; playing = czy glos gra; env = obwiednia barwy (Q15); lev = poziom glosu x velocity (Q15)
    void draw(Canvas& c, const int16_t k[4], bool playing, int16_t env, int16_t lev);

private:
    Canvas* cache_ = nullptr;
    static const int NW = 32, N = 132;
    int16_t wave_[NW];
    int32_t wn_[2][NW];
    int waveSa_ = -1;
    int32_t waveDv_ = -1;
    int32_t rad_[N];
    int32_t hlR_ = 0, amp_ = 0;
    int32_t eLev_ = 0, k_[4] = {}, kw_ = -1, kd_ = -1, dcw_ = 0, flash_ = 0, size_ = 0, bx_ = 0, rot_ = 0, ripple_ = 0;
    bool playing_ = false;
    void buildCache();
    void animate(const int16_t k[4], bool playing, int32_t env, int32_t lev);
    void drawLive(Canvas& c);
    int32_t radiusAt(uint32_t a) const;
    void polar(uint32_t a, int32_t r, int32_t* x, int32_t* y) const;
    void strand(Canvas& c, int32_t x0, int32_t y0, int32_t x2, int32_t y2, int32_t bow, int32_t tq, int32_t* tx, int32_t* ty);
    int32_t bow(int32_t b) const;
    void dash(Canvas& c, uint32_t a, int32_t r0, int32_t r1);
};

}  // namespace dp

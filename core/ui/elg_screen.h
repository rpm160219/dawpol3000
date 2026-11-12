// Ekran silnika ELG: port 1:1 ekranu z op1r (Hundekraft: screen.c + elg.svg + elg_geom.h, GPL-3.0-or-later).
// Los w ludowej koronie pod zorza. Statyczna scena z elg.svg (elg_art.h) w cache; na zywo jak w oryginale:
// poroze rozchyla sie z interwalem (INTERVAL, niebieski), oblaczki oddechu rosna z rozstrojeniem (DETUNE, zielony)
// i poziomem, poswiata ksiezyca (TIMBRE, bialy), zorza faluje z ruchem i LFO silnika (MOVEMENT, czerwony),
// gwiazdy migocza, gdy gra. Siersc, twarz i gwiazdy liniami z elg_geom.h. Ekran OP-1 320x160 skalowany 2x.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"
#include "synth/elg_dsp.h"

namespace dp {

class ElgScreen {
public:
    // set = galki jak ustawione; view = stan dzwieku (bloki, poziom, LFO, galki jak slysza je glosy)
    void draw(Canvas& c, const int16_t set[4], const ElgView& view);

private:
    Canvas* cache_ = nullptr;
    uint32_t lastBlocks_ = 0, aph_ = 0, tph_ = 0;
    int32_t act_ = 0, asx_ = 0, asy_ = 0, aurAmp_ = 0, aurStr_ = 0, glow_ = 0, breath_ = 1024;
    bool playing_ = false;
    void animate(const int16_t set[4], const ElgView& v);
    void drawLive(Canvas& c);  // elementy na zywo (oddech, poswiata, zorza, poroze, siersc, twarz, gwiazdy)
    friend void drawElgIllustration(Canvas& c, float x, float y, float w, float h);
};

// Ilustracja silnika (karta ENGINE): cala scena przy domyslnych galkach
void drawElgIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

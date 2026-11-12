// Ekran silnika PEW: port 1:1 ekranu z op1r (Hundekraft, "jazz club": screen.c + pew.svg, GPL-3.0-or-later).
// Klub jazzowy z organami i glosnikiem rotacyjnym. Statyczna scena z pew.svg (pew_art.h) w cache; na zywo jak
// w oryginale: suwaki na organach = rejestracja (REGISTER, niebieski + strona 2), nuty wylatujace przy kazdym
// uderzeniu (PERCUSSION, zielony), dym papierosa i swiatlo lampy (DRIVE / CHORUS, bialy), czerwone linie
// wokol glosnika krecace sie z wirnikami dzwieku (LESLIE), podswietlony klawisz na kazdy trzymany glos.
// Ekran OP-1 320x160 skalowany 2x (jak WAVETABLE i TAFFY).
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"
#include "synth/pew_dsp.h"

namespace dp {

class PewScreen {
public:
    // k = galki (po modulacji, gdy gra); push = strona 2 (wysuniecie suwakow); view = stan dzwieku (wirniki itd.)
    void draw(Canvas& c, const int16_t k[4], const int16_t push[3], const PewView& view);

private:
    Canvas* cache_ = nullptr;
    uint32_t lastBlocks_ = 0, lastStrikes_ = 0, t_ = 0;
    int32_t act_ = 0;
    int32_t barShown_[9] = {};
    int32_t noteP_[3] = {1024, 1024, 1024}, notePop_[3] = {}, noteM_[3][3] = {};
    int nextNote_ = 0;
    bool first_ = true;
    void buildCache();
    void animate(const int16_t k[4], const int16_t push[3], const PewView& v);
    void notes(Canvas& c);
    void bars(Canvas& c);
    void keys(Canvas& c, const PewView& v);
    void lamp(Canvas& c, int32_t white);
    void smoke(Canvas& c, int32_t white);
    void rotor(Canvas& c, const PewView& v, int32_t red);
};

}  // namespace dp

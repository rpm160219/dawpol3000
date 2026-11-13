// Ekran silnika VELOUR: scena orkiestry (wlasna grafika DAW-POL). Szesc instrumentow na statywach = szesc rejestrow
// (kontrabas, wiolonczela, altowka, skrzypce, trabka, rog): jasnosc i kolor = poziom rejestru, struny drgaja
// z obwiednia klawiszy (kontrabas i wiolonczela z obwiednia basu), z trabki i rogu plyna fale. Trzy reflektory nad
// scena = trzy linie ensemble (przesuniete o 120 st.): migocza z tremolo, kolysza sie z chorusem. Kula dyskotekowa =
// phaser (obroty = RATE, plamki = COLOR). Kurtyna u gory podnosi sie z CRESCENDO i opada z SUSTAIN.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

struct VelourCore;

namespace dp {

class VelourScreen {
public:
    void draw(Canvas& c, const int16_t* p, const ::VelourCore* core, float dt);
private:
    float env_ = 0, bass_ = 0, tremPh_ = 0, chorPh_ = 0, phasPh_ = 0, vib_ = 0, wave_ = 0;
};

void drawVelourIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

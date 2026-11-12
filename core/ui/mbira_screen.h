// Ekran silnika MBIRA: port 1:1 ekranu z op1r (Hundekraft: czesc "screen" w mbira.c + mbira.svg, GPL-3.0-or-later).
// Kalimba w trzech czwartych z kciukiem na jezyczku i cztery wskazniki po prawej. Na zywo jak w oryginale: zapalone
// kreski wskaznikow (STRIKE, TINE, BOX, BUZZ — galki jak slysza je glosy; strona 2: napisy RING / DOUBLE / WAH i ich
// wartosci), gdy dzwieczy: biale luki drgan przy czubku jezyczka, kregi z otworu, czerwone kreski przy kapslach (BUZZ).
// Ekran OP-1 320x160 skalowany 1,85x z marginesami (scena siega obu krawedzi). Rysowanie w kolejnosci dokumentu (statyczny poczatek w cache).
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"
#include "synth/mbira_dsp.h"

namespace dp {

class MbiraScreen {
public:
    // k = galki (po modulacji, gdy gra); words = strona 2 (RING, DOUBLE, WAH); page2 = pokazuj strone 2
    void draw(Canvas& c, const int16_t k[4], const int16_t words[3], bool page2, const MbiraView& view);

private:
    Canvas* cache_ = nullptr;
    int firstLive_ = 0;
    uint32_t frame_ = 0, rng_ = 7;
};

// Ilustracja silnika (karta ENGINE): cala scena przy domyslnych galkach
void drawMbiraIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

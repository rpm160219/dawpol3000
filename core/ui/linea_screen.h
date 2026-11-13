// Ekran silnika LINEA (wlasna grafika DAW-POL).
//  PLAY: generatywna "konstelacja" patcha — cztery partiale (Upper 1/2, Lower 1/2) jako formy w rogach kwadratu
//        wektora: partial syntetyczny = gladka rozeta (fala SQU / SAW, rezonans = ostrosc platkow), partial PCM =
//        chmura iskier (wybucha przy uderzeniu klawisza); para z modulatorem pierscieniowym polaczona iskrzaca linia.
//        Ksztalty i kolory z nazwy i parametrow patcha (kazdy patch wyglada inaczej). Punkt wektora (balans partiali x
//        balans tonow) powieksza i rozjasnia formy; CUTOFF = jasnosc poswiaty, RESONANCE = ostrosc. Na dole nazwa w
//        stylu wyswietlacza LCD.
//  Strony edycji: bursztynowy wyswietlacz 2 x 40 w stylu syntezatorow LA z konca lat 80 + wykresy (obwiednie,
//        struktura, EQ).
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"
#include "seq/project.h"

struct LineaCore;

namespace dp {

class LineaScreen {
public:
    void drawPlay(Canvas& c, const uint8_t* patch, const ::LineaCore* core, int cut, int res, const char* bankLabel, float dt);
private:
    float t_ = 0, glow_ = 0;
    uint32_t seenNotes_ = 0;
    float burst_[4] = {0, 0, 0, 0};
};

// wyswietlacz LCD (bursztynowy, 2 wiersze): prostokat i tekst w stylu segmentowym
void lineaLcd(Canvas& c, float x, float y, float w, float h, const char* l1, const char* l2);
void drawLineaIllustration(Canvas& c, float x, float y, float w, float h);
// LINEA: pierwszy patch pierwszego banku, gdy sciezka nie ma jeszcze patcha (okno TRACK, SYNTH)
void lineaEnsure(Track& t);
bool lineaLoadPatch(Track& t, int bank, int slot);

}  // namespace dp

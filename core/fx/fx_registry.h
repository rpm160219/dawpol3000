// Efekty miksera (inserty i AUX): nazwy, galki, rodzaje galek, wartosci domyslne (z plugin.json oryginalow op1r).
// Slowa galek: 0..3 = strona 1 (niebieska, zielona, biala, czerwona), 5..7 = strona 2 niebieska / zielona / biala,
// 4 = strona 2 czerwona (np. FREEZE w STUTTER — w OP-1 przelacznik pozycji, w DAW-POL slowo slotu).
#pragma once
#include <stdint.h>
#include "seq/project.h"

namespace dp {

enum FxClass : uint8_t { FXC_NONE = 0, FXC_INSERT = 1, FXC_AUX = 2 };
// rodzaj galki: ciagla (0..32767), kroki "srodek przedzialu" ((2i+1)*32768/2n, FX_STEPS / PAGE_STEPS / sx_range),
// lista (i/(n-1) zakresu, OP1_PRESS_LIST), liczba calkowita (0..n-1, OP1_PRESS_EXTEND), krok co 512 (sx_select)
enum FxKnobKind : uint8_t { FXK_CONT = 0, FXK_MID, FXK_LIST, FXK_INT, FXK_SEL512 };

struct FxInfo {
    const char* name;       // na liscie
    const char* shortName;  // w pigulce kanalu
    uint8_t cls;            // FXC_INSERT / FXC_AUX
    const char* desc[4];    // opis (okno wyboru)
    const char* knob[4];    // galki strony 1
    const char* page2[4];   // galki strony 2 (slowa 5, 6, 7, 4), "" = brak
    int16_t def[8];
    uint8_t kind[8];        // FxKnobKind per slowo
    uint8_t steps[8];       // liczba krokow (galki nieciagle)
    const char* labels[8];  // nazwy krokow "A|B|C" (NULL = numer)
};
const FxInfo& fxInfo(int fx);
void fxDefaults(FxSlot& s, int fx);  // ustawia typ i domyslne slowa galek
int fxKnobWord(int page, int k);     // slowo galki k (0..3) na stronie page (0 / 1); -1 = brak
int fxStepOf(const FxInfo& fi, int word, int16_t v);       // krok galki nieciaglej
int16_t fxWordOfStep(const FxInfo& fi, int word, int step);  // slowo dla kroku
void fxValueText(const FxInfo& fi, int word, int16_t v, char* out, int n);  // wartosc do wyswietlenia
int fxListOf(int cls, int* out, int max);  // efekty danej klasy (do listy wyboru)

}  // namespace dp

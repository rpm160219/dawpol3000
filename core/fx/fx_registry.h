// Efekty miksera (inserty i AUX): nazwy, galki, wartosci domyslne (z plugin.json oryginalow op1r).
#pragma once
#include <stdint.h>
#include "seq/project.h"

namespace dp {

struct FxInfo {
    const char* name;       // na liscie
    const char* shortName;  // w pigulce kanalu
    const char* desc[4];    // opis (okno wyboru)
    const char* knob[4];    // galki strony 1
    const char* page2[3];   // galki strony 2 (slowa 5..7), "" = brak
    int16_t def[8];
};
const FxInfo& fxInfo(int fx);
void fxDefaults(FxSlot& s, int fx);  // ustawia typ i domyslne slowa galek

}  // namespace dp

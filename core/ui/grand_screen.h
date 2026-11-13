// Ekran silnika GRAND (wlasny rysunek): przekroj fortepianu koncertowego. Pokrywa podnoszona LID (z podporka), pulpit z
// nutami, struna od kolkow do mostka (1..3 wg rejestru, wachlarz wg UNISON), mlotek pod struna w miejscu STRIKE (filc:
// miekki = duzy i ciemny, twardy = maly i jasny; splaszcza sie przy uderzeniu), z miejsca uderzenia rozbiegaja sie fale
// i odbijaja od koncow (falowod), zageszczenia drgan podluznych przy GROWL, kregi na plycie przy BOARD, tlumik,
// krzywa DECAY w rogu.
#pragma once
#include <stdint.h>
#include "gfx/canvas.h"

extern "C" struct GrandCore;

namespace dp {

class GrandScreen {
public:
    void draw(Canvas& c, const int16_t knobs[8], const ::GrandCore* core, float dt);  // slowa 0..3 galki, 5..7 strona 2
private:
    float t_ = 0, since_ = 9, swing_ = 1, motion_ = 0, damper_ = 0;
    uint32_t strikes_ = 0;
};

void drawGrandIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

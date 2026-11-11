// Ikony i ilustracje rysowane wektorowo (cienka kreska, styl "line art").
// (x, y) = lewy gorny rog, s = rozmiar kwadratu ikony w pikselach.
#pragma once
#include "gfx/canvas.h"

namespace dp {

enum IconId {
    ICON_KEYS,      // sciezka MIDI (klawiatura)
    ICON_WAVE,      // sciezka audio (fala)
    ICON_PATTERN,   // pattern / piano roll
    ICON_ARRANGE,   // aranzacja (klipy)
    ICON_PLAY, ICON_STOP, ICON_REC,
    ICON_PLUS,
    ICON_TAPE,      // szpule
    ICON_METRONOME,
    ICON_LOOP,
    ICON_NOTE,      // nuta
    ICON_PROJECT,   // dyskietka (projekt)
    ICON_COUNT
};

void drawIcon(Canvas& c, IconId id, float x, float y, float s, Color col, float lw = 1.5f);

// Wieksze ilustracje do okien dialogowych.
void drawSynthIllustration(Canvas& c, float x, float y, float w, float h);
void drawTapeIllustration(Canvas& c, float x, float y, float w, float h);
void drawFloppyIllustration(Canvas& c, float x, float y, float w, float h);

}  // namespace dp

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
// Ilustracja silnika (karta ENGINE): 0 = WAVETABLE, 1 = WAVETABLE+, 2 = TAFFY, 3 = TEST SAW.
// Styl jak ekran WAVETABLE: stos klatek fal w perspektywie, podloga z kropek, nawias.
// engine: 0 WAVETABLE, 1 WAVETABLE+, 2 TAFFY, 3 PEW, 11 TEST SAW (kolejnosc listy na karcie ENGINE;
// 4 ELG, 5 EVA, 6 MBIRA, 7 REACTOR, 8 OXBOW, 9 OXBOW+, 10 SID rysuja ui/*_screen: draw*Illustration)
void drawEngineIllustration(Canvas& c, int engine, float x, float y, float w, float h);
// Ikony kategorii presetow (cat = PresetCat, -1 = ALL), siatka 16x16 jak drawIcon
void drawCategoryIcon(Canvas& c, int cat, float x, float y, float s, Color col, float lw = 1.5f);

}  // namespace dp

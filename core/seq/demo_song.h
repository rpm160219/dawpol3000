// Utwory demo budowane w kodzie (nuty, patterny, klipy, brzmienia z presetow fabrycznych).
#pragma once
#include "seq/project.h"

namespace dp {

// "ALL ENGINES": A-moll, 96 BPM, 16 taktow (Am - F - C - G), kazdy silnik ma swoja partie:
// ELG dron, EVA chor, MBIRA arpeggio, WAVETABLE bas, WAVETABLE+ stopa i pluck, PEW organy, TAFFY melodia, REACTOR akcenty, OXBOW akordy dete, SID arpeggio, MONOLIT linia acid.
// Uklad: 1-4 intro (dron, chor, kalimba), 5-8 groove (+ bas, stopa, organy), 9-12 calosc (+ melodia, pluck, dete),
// 13-16 wyciszenie (dron, chor, kalimba, melodia, akcenty).
void buildDemoAllEngines(Project& p);
void buildDemoSynthwave(Project& p);  // SYNTHWAVE: 16 taktow (GIN, PARALLAX, JUNI, OXBOW)

bool factoryInstrument(const char* engineDir, const char* name, Track& t);  // io/factory_presets.cpp

}  // namespace dp

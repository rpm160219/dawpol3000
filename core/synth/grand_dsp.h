// Silnik GRAND — wlasny, oszczedny model fizyczny fortepianu koncertowego (DAW-POL), wzorowany na architekturze modelu
// Claytona Oteya (FigBug/Piano: falowody strun, nieliniowe zderzenie mlotka ze struna, wspolny mostek, plyta jako
// siec poglosowa), napisany od zera i odchudzony pod Cortex-A53. Na glos 1..3 struny (bas 1, srodek 2, gora 3):
// falowod rozciety w miejscu uderzenia (odcinek kolki <-> mlotek, odcinek mlotek <-> mostek), sztywnosc (kaskada
// filtrow wszechprzepustowych), strata zalezna od czestotliwosci, strojenie poprawione o opoznienia fazowe. Mlotek:
// masa zalezna od klawisza, filc F = K c^p (twardnieje w gore klawiatury), calkowany w 4 podkrokach tylko podczas
// kontaktu; sila dzielona na struny. Mostek: struny sprzezone macierza (1-b) I + b H (H = Householder, stabilnie).
// Pomruk basu: nieliniowosc napiecia (kwadrat predkosci na mostku). Plyta: siec FDN 4 linii. Tlumik po puszczeniu
// (od E6 w gore brak tlumikow, jak w fortepianie). Galki: HAMMER, STRIKE, DECAY, LID; strona 2 (slowa 5..7): UNISON,
// GROWL, BOARD. Glos liczony przy 48 kHz i interpolowany do 96 kHz gospodarza.
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct GrandCore;
struct GrandCore* grandCreate(void);
void grandNoteOn(struct GrandCore* p, int voice);
void grandNoteOff(struct GrandCore* p, int voice);
void grandRender(struct GrandCore* p, int voice, int16_t* out, const int16_t* knobs, uint32_t inc, float rate, int vel);
float grandDecaySeconds(int knob);       // wybrzmienie przy C4
float grandStrikeRatio(int knob);        // miejsce uderzenia (ulamek dlugosci struny od kolkow)
// ekran
float grandMotion(const struct GrandCore* p);      // drgania 0..1
uint32_t grandStrikes(const struct GrandCore* p);  // licznik uderzen
float grandSquash(const struct GrandCore* p);      // scisniecie filcu ostatniego uderzenia 0..1
int grandHeld(const struct GrandCore* p);          // trzymane klawisze
int grandLastStrings(const struct GrandCore* p);   // liczba strun ostatniej nuty

#ifdef __cplusplus
}
#endif

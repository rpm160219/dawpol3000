// Oryginalny DSP silnika MONOLIT z op1r (Hundekraft, GPL-3.0-or-later; third_party/op1r_monolit/monolit.c bez zmian):
// bas / lead — 3 piły polyBLEP + sub (kwadrat oktawe nizej), czterobiegunowy filtr drabinkowy (zero-delay, z MOAI).
// Galki-makra (0..32767): 0 SHAPE (jedna pila -> pila + sub -> 2 rozstrojone -> 3 pily -> szeroki stos), 1 CUTOFF
// (80 Hz .. 13 kHz + jeszcze oktawa, sledzenie klawiatury 60 %), 2 CONTOUR (obwiednia filtra: pluck .. swell),
// 3 HEAT (pelniej i przester -> rezonans -> samooscylacja). Strona 2: knobs[5] BODY (interwal OSC 3: SUB, UNI, 5TH,
// OCT, SUB2), [6] VEL (wplyw velocity na filtr i glosnosc), [7] AGE (dryf oscylatorow i rozrzut filtra).
// monolit.c jest w C; stan glosu podmieniany przed wywolaniem (instancja = sciezka).
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct MonolitCore;
struct MonolitCore* monolitCreate(void);
void monolitNoteOn(struct MonolitCore* p, int voice, const int16_t* knobs);
void monolitNoteOff(struct MonolitCore* p, int voice);
void monolitRender(struct MonolitCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel);
int32_t monolitEnv(const struct MonolitCore* p);  // obwiednia (CONTOUR) ostatnio renderowanego glosu, Q15 — dla ekranu

#ifdef __cplusplus
}
#endif

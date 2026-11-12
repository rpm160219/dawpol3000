// Oryginalny silnik MIXTUR z op1r (ultralazr, Hundekraft; GPL-3.0-or-later; third_party/op1r_mixtur/mixtur.c bez zmian):
// organy subharmoniczne wg Mixtur-Trautonium Oskara Sali — grany klawisz to najwyzszy ton, 4 dzielniki pily f/n
// (n = 1..24, wspolna faza, bez dryfu), 2 rownolegle formanty (filtry pasmowe SVF), miekkie kolano na wyjsciu.
// Galki (0..32767): 0..3 = undertony A..D (OFF, 1..24 — 25 stopni). Strona 2: knobs[5] F1 (180 Hz .. 4,07 kHz),
// [6] F2 (500 Hz .. 5,66 kHz), [7] RES (Q 0,7 .. 22); 0 = 8192 (392 Hz / 917 Hz / Q 3,2). Bez wlasnej obwiedni.
// Pamiec glosu jak w firmware: 6 slotow po 0x24 bajty w instancji; sygnatury own[] (wspolna tablica oryginalu)
// podmieniane per instancja. Tu tez ekran oryginalu (drabina undertonow) przez funkcje zwrotne.
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct MixturCore;
struct MixturCore* mixturCreate(void);
void mixturNoteOn(struct MixturCore* p, int voice, const int16_t* knobs);
void mixturNoteOff(struct MixturCore* p, int voice);
void mixturRender(struct MixturCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel);

int mixturKnobN(int16_t k);         // undertone galki: 0 = OFF, 1..24
int mixturF1Hz(int16_t w);          // strona 2 (0 = 8192 jak w oryginale)
int mixturF2Hz(int16_t w);
int mixturResQ10(int16_t w);        // Q x 10

typedef void (*MxwLineFn)(void* user, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* rgba);
typedef void (*MxwArtFn)(void* user);
// knobs = 8 slow sciezki, live = galki 0..3 po modulacji, page2 = strona 2 widoczna (krzywe formantow w kolorze)
void mixturScreenUpdate(void* user, MxwLineFn line_fn, MxwArtFn art_fn, const struct MixturCore* core,
                        const int16_t knobs[8], const int16_t live[4], int page2, int dt_ms);

#ifdef __cplusplus
}
#endif

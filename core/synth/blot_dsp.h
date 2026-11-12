// Oryginalny silnik BLOT z op1r (pcppcp, GPL-3.0-or-later; filtr wg TeeBeeFilter z Open303, Robin Schmidt, MIT;
// third_party/op1r_blot/blot.c bez zmian): w stylu TB-303 — pila / kwadrat w rezonansowy filtr 4-biegunowy (stopnie
// obciazaja sie nawzajem), gornoprzepustowy w sprzezeniu, opadajaca obwiednia filtra, akcent przy velocity >= 100.
// Galki: 0 CUTOFF (30 Hz .. 15 kHz), 1 RESONANCE, 2 ENV MOD (0..5 oktaw), 3 DECAY (40..640 ms). Strona 2 (Shift w OP-1):
// knobs[5] WAVE (pila .. kwadrat), [6] ACCENT (0 = 20000), [7] DRIVE. Slide = portamento gospodarza (MONO + GLIDE).
// Stan glosu = slot 0x24 B w instancji (self). Tu tez ekran oryginalu (kwasowy smiley) przez funkcje zwrotne.
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct BlotCore;
struct BlotCore* blotCreate(void);
void blotNoteOn(struct BlotCore* p, int voice, const int16_t* knobs);
void blotNoteOff(struct BlotCore* p, int voice);
void blotRender(struct BlotCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel);
int blotDecayMs(int16_t k);

typedef void (*BlwLineFn)(void* user, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* rgba);
void blotScreenUpdate(void* user, BlwLineFn line_fn, const struct BlotCore* core, const int16_t knobs[8],
                      const int16_t live[4], int dt_ms);

#ifdef __cplusplus
}
#endif

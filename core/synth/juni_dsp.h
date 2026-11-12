// Oryginalny silnik JUNI z op1r (Hundekraft, GPL-3.0-or-later; third_party/op1r_juni/juni.c bez zmian): 6-glosowy poli
// w stylu wczesnych lat 80. (jeden DCO na glos): pila / puls / sub z polyBLEP, filtr drabinkowy 4-biegunowy z miekkim
// wejsciem, gornoprzepustowy, chorus I / II / I+II w glosie. Galki-makra: 0 TONE (cutoff, 60 % sledzenia), 1 SHAPE
// (5 mieszanek), 2 SWEEP (obwiednia na filtr + rezonans), 3 CHORUS; strona 2: knobs[5] RESONANCE, [6] HPF, [7] PWM
// RATE. Jedyna obwiednia to obwiednia glosnosci (u nas AMP ENV) — prowadzi tez filtr. Stan pliku (glosy, wspolne LFO,
// linie chorusa) per instancja, podmieniany leniwie.
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct JuniCore;
struct JuniCore* juniCreate(void);
void juniNoteOn(struct JuniCore* p, int voice, const int16_t* knobs);
void juniRender(struct JuniCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel, int env);

typedef void (*JnwLineFn)(void* user, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* rgba);
typedef void (*JnwArtFn)(void* user);
void juniScreenUpdate(void* user, JnwLineFn line_fn, JnwArtFn art_fn, const struct JuniCore* core, const int16_t knobs[8],
                      const int16_t live[4], int dt_ms);

#ifdef __cplusplus
}
#endif

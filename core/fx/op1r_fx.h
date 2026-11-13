// Efekty op1r (Hundekraft) jako inserty / efekty AUX miksera DAW-POL: oryginalny kod bez zmian (tryb OP1_HOST),
// wiele niezaleznych egzemplarzy (oryginal ma 3 stale pozycje: synth, drum, master). Blok 64 probek stereo int16 przy
// 44,1 kHz jak w OP-1 (konwersja czestotliwosci: core/audio/mixer.cpp). Slowa galek w[8]: 0..3 strona 1, 5..7 strona 2.
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void* fxJuniChorusCreate(void);
void fxJuniChorusProcess(void* inst, int16_t* l, int16_t* r, const int16_t* w);
void* fxSisterCreate(void);
void fxSisterProcess(void* inst, int16_t* l, int16_t* r, const int16_t* w);
#ifdef __cplusplus
}
#endif

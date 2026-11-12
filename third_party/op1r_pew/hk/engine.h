/* DAW-POL 3000: zaslepka hk/engine.h dla pew.c (zamiast interfejsu firmware OP-1).
 * pew.c ma wspolny stan wszystkich glosow (tablice organow, wirniki Leslie, perkusja), wiec wrapper
 * (core/synth/pew_dsp.cpp) wkleja go do klasy: jedna instancja = jedna sciezka. "self" = numer glosu 0..5.
 * Wysokosc (ENGINE_PITCH) i velocity sa polami tej klasy, ustawianymi przed kazdym render. */
#ifndef ENGINE_H
#define ENGINE_H
#include <stdint.h>
#define ENGINE_VOICES 6
#define ENGINE_BLOCK 128
#define ENGINE_PITCH pitch_
#define ENGINE_VELOCITY(v) vel_
static inline int ENGINE_VOICE(const void* self) { return (int)(uintptr_t)self; }
static inline int16_t engine_sat16(int32_t x) { return (int16_t)(x > 32767 ? 32767 : x < -32768 ? -32768 : x); }
#endif

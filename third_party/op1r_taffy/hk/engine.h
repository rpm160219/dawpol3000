/* DAW-POL 3000: zaslepka hk/engine.h dla taffy.c (zamiast interfejsu firmware OP-1).
 * Glosy wszystkich instancji w jednej tablicy: indeks = instancja * 6 + glos, przekazywany jako "self".
 * Wysokosc (ENGINE_PITCH) i velocity ustawia wrapper (core/synth/wavetable_synth.cpp) przed kazdym render. */
#ifndef ENGINE_H
#define ENGINE_H
#include <stdint.h>
#define ENGINE_VOICES (6 * 32)
#define ENGINE_BLOCK 128
extern const uint32_t* tf_pitch_ptr;
extern int tf_vel_now;
#define ENGINE_PITCH tf_pitch_ptr
static inline int ENGINE_VOICE(const void* self) { return (int)(uintptr_t)self; }
static inline int ENGINE_VELOCITY(int voice) { (void)voice; return tf_vel_now; }
static inline int16_t engine_sat16(int32_t x) { return (int16_t)(x > 32767 ? 32767 : x < -32768 ? -32768 : x); }
void engine_note_on(void* self, const int16_t* knobs);
void engine_render(void* self, int16_t* out, const int16_t* knobs);
void engine_note_off(void* self, const int16_t* knobs);
#endif

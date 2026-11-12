// Oryginalny DSP silnika SID z op1r (adaptacja Hundekraft prymitywow cRSID 1.58 Hermita; third_party/op1r_sid/sid.c
// i sid_dsp.c bez zmian): kazdy glos to trzy oscylatory (pryma, oktawa, kwinta albo interwal) i rezonansowy filtr
// dolnoprzepustowy 8580. Galki (0..32767): 0 WAVE (triangle, saw, pulse, noise, sync, ring — powtarzane co 512),
// 1 VOICES / WIDTH / INTERVAL (wg fali), 2 CUTOFF, 3 RESONANCE; knobs[7] = trym presetu.
// sid.c jest w C; stan glosu podmieniany przed wywolaniem (instancja = sciezka).
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct SidCore;
struct SidCore* sidCreate(void);
void sidNoteOn(struct SidCore* p, int voice, const int16_t* knobs);
void sidRender(struct SidCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel);
unsigned sidWaveMode(int16_t knob0);        // 0 TRIANGLE .. 5 RING
unsigned sidIntervalSemi(int knob1);        // interwal SYNC / RING w poltonach

#ifdef __cplusplus
}
#endif

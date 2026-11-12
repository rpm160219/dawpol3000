// OXBOW+ (DAW-POL): rozszerzona wersja silnika OXBOW — dwa niezalezne oscylatory (fala, interwal, strojenie, poziom,
// szerokosc pulsu), szum, SYNC, X-MOD, obwiednia filtra na wysokosc OSC 2, regulowana obwiednia filtra, keytrack,
// velocity, VINTAGE. DSP: third_party/kennel_oxbow/plugins/oxbow/oxbow_plus.c (zmodyfikowana kopia oxbow.c).
// Galki jak w OXBOW: 0 CUTOFF, 1 RESONANCE, 2 ENV AMOUNT, 3 DETUNE (OSC 2); knobs[6] FILTER, [7] DRIVE.
#pragma once
#include <stdint.h>
#include "../../third_party/kennel_oxbow/plugins/oxbow/oxbow_plus.h"

#ifdef __cplusplus
extern "C" {
#endif

struct OxbowPlusCore;
struct OxbowPlusCore* oxbowPlusCreate(void);
void oxbowPlusNoteOn(struct OxbowPlusCore* p, int voice, const int16_t* knobs, int vel, const obx_params* prm);
void oxbowPlusNoteOff(struct OxbowPlusCore* p, int voice);
void oxbowPlusRender(struct OxbowPlusCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel,
                     const obx_params* prm);
struct OxbowView;
void oxbowPlusView(const struct OxbowPlusCore* p, struct OxbowView* v);
// czasy (s) -> indeksy tablic obwiedni filtra: atak do pelnego poziomu, zanik/wybrzmienie do ~-43 dB (jak nasze ADSR)
uint32_t oxbowPlusAttackIndex(float seconds);
uint32_t oxbowPlusDecayIndex(float seconds);
float oxbowPlusAttackSeconds(uint32_t idx);
float oxbowPlusDecaySeconds(uint32_t idx);

#ifdef __cplusplus
}
#endif

// Oryginalny DSP silnika TAFFY z op1r (Hundekraft, GPL-3.0-or-later; third_party/op1r_taffy/taffy.c bez zmian):
// synteza z zakrzywianiem fazy w stylu Casio CZ. Glosy wszystkich instancji w jednej tablicy (indeks globalny).
// Galki (0..32767): 0 WAVE (saw -> square -> pulse -> saw-pulse), 1 DCW (znieksztalcenie = jasnosc),
// 2 ENV (glebokosc wewnetrznej obwiedni barwy ~0,2 s), 3 RESONANCE (fala rezonansowa CZ).
#pragma once
#include <stdint.h>

namespace dp {

static const int TAFFY_MAX_VOICES = 6 * 32;
int taffyAllocVoices(int n);  // zwraca indeks pierwszego glosu instancji
void taffyNoteOn(int voice);
// Blok 128 probek glosu: knobs[8] jak w oryginale (knobs[7] = trym presetu, 0 = domyslny), pitch[128] = przyrost fazy
void taffyRender(int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel);
int16_t taffyVoiceEnv(int voice);  // wewnetrzna obwiednia barwy glosu (Q15), dla ekranu
void taffyWaveCycle(int16_t* y, int n, int32_t k0, int32_t dcw);  // jeden okres fali bez rezonansu (ekran)

}  // namespace dp

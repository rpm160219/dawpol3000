// Oryginalny DSP silnika ELG z op1r (Hundekraft, GPL-3.0-or-later; third_party/op1r_elg/elg.c bez zmian):
// syntezator dronow ("los"): trzy piły na glos (pryma, rozstrojona pryma, trzeci glos o interwal wyzej),
// przester "growl" i rezonansowy filtr dolnoprzepustowy, wszystko powoli sie rusza.
// Galki (0..32767): 0 INTERVAL (unison, kwinta, oktawa, oktawa + kwinta, dwie oktawy), 1 DETUNE (do ~+-20 centow),
// 2 TIMBRE (filtr 40 Hz .. 6 kHz + sila przesteru), 3 MOVEMENT (LFO 0,03..2 Hz na filtr i glosnosc + dryf stroju).
// Zmienne pliku (glosy, wartosci dla ekranu) — stad jedna instancja na sciezke (jak PEW).
#pragma once
#include <stdint.h>

namespace dp {

struct ElgCore;
ElgCore* elgCreate();  // poza watkiem audio
void elgNoteOn(ElgCore* p, int voice);
// syncPh >= 0: faza ruchu (LFO MOVEMENT) przypieta do tempa, 0..2^32 jako int64; < 0 = swobodny bieg jak w oryginale
void elgRender(ElgCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel, int64_t syncPh = -1);

struct ElgView { uint32_t blocks; int32_t level, lfo; int16_t knobs[4]; };  // dla ekranu (jak volatile w oryginale)
void elgView(const ElgCore* p, ElgView* v);
int elgInterval(int32_t value);  // stopien interwalu z galki (0..4), jak elg_interval

}  // namespace dp

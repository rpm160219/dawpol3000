// Oryginalny DSP silnika EVA z op1r (Hundekraft, GPL-3.0-or-later; third_party/op1r_eva/eva.c bez zmian):
// "kosmiczny chor" — cieply spiewany pad z lat 80: pila + miekki puls (rozstrojone, polyBLEP), filtr otwierajacy
// sie z oddechem, barwa samoglosek (oo-ah-ee), wibrato z dryfem, chorus ensemble i radio w helmie astronautki.
// Galki (0..32767): 0 PRESSURE (jasnosc i atak), 1 FREQ (samogloska), 2 OXYGEN (oddech, chorus, falowanie),
// 3 COM (radio: cieple pasmo -> waski kanal, szum, zaniki, sygnaly Quindar, powyzej 75% kruszenie).
// knobs[7] = oktawa sygnalu Quindar (strona 2 COM w oryginale): <10923 nizej, do 21845 oryginal, wyzej wyzej.
// eva.c jest w C (kompilowany jako C); stan kazdej sciezki podmieniany przed wywolaniem (jedna instancja = sciezka).
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct EvaCore;
struct EvaCore* evaCreate(void);
void evaNoteOn(struct EvaCore* p, int voice, const int16_t* knobs, int vel);
void evaNoteOff(struct EvaCore* p, int voice);   // tylko gdy klawisz glosu jest trzymany (bezpieczne wielokrotnie)
int evaHeld(const struct EvaCore* p, int voice);
void evaRender(struct EvaCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel);
// dla ekranu: bloki, oddech (OXYGEN), nosna (zaniki), czy gra sygnal Quindar
struct EvaView { uint32_t blocks; int32_t breath, gate, beep; };
void evaView(const struct EvaCore* p, struct EvaView* v);

#ifdef __cplusplus
}
#endif

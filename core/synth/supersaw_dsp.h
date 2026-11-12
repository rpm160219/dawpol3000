// Oryginalny silnik SUPERSAW z op1r (pcppcp, GPL-3.0-or-later; third_party/op1r_supersaw/supersaw.c bez zmian):
// 2 rozstrojone piły (druga przechodzi w odwrócona -> kwadrat), sub sinus oktawe nizej, przester (miekkie obcinanie),
// filtr jednobiegunowy 4,9 kHz. Galki (0..32767): 0 DETUNE, 1 MORPH (SAW -> SQUARE), 2 SUB, 3 DRIVE. Bez wlasnej
// obwiedni (glosnosc z ADSR gospodarza). Stan glosu = voice_t w instancji (self = wskaznik, jak pamiec glosu OP-1).
// Tu tez ekran oryginalu (pila lancuchowa) — kod C rysuje przez funkcje zwrotne.
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct SupersawCore;
struct SupersawCore* supersawCreate(void);
void supersawNoteOn(struct SupersawCore* p, int voice, const int16_t* knobs);
void supersawNoteOff(struct SupersawCore* p, int voice);
void supersawRender(struct SupersawCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel);

// ekran (watek UI). Wspolrzedne Q10 w ukladzie OP-1 320x160; grupa rysunku: -1 statyczne, 0 saw, 1 sprk, 2..5 bar0..3
typedef void (*SswLineFn)(void* user, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* rgba);
typedef void (*SswArtFn)(void* user, const int32_t* const* m);  // m[grupa] = macierz 3x3 Q10 (0 = brak przeksztalcenia)
void supersawScreenUpdate(void* user, SswLineFn line_fn, SswArtFn art_fn, const struct SupersawCore* core,
                          const int16_t live[4], int dt_ms);

#ifdef __cplusplus
}
#endif

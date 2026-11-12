// Oryginalny silnik BRASSOPHONE z op1r (nvmsocool, GPL-3.0-or-later; third_party/op1r_brass/brass.c bez zmian):
// sinus z faza "wyginana" przez tablice (od czystego sinusa do jasnej blachy); barwa (przesuniecie w tablicy) i lekki
// podjazd wysokosci (~50 centow) prowadzone przez obwiednie glosnosci glosu, wibrato wlacza sie dopiero na
// podtrzymaniu. Galki (0..32767): 0 TIMBRE (start barwy; gora = czysty sinus), 1 SWEEP (ile obwiednia rozjasnia),
// 2 VIBRATO (glebokosc do ~90 centow), 3 VIB RATE (0..32 Hz). Obwiednia = AMP ENV gospodarza (OP1_VOICE_ENV w OP-1).
// Stan glosu = slot 0x24 bajtow w instancji (self). Tu tez ekran oryginalu (rog) przez funkcje zwrotne.
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct BrassCore;
struct BrassCore* brassCreate(void);
void brassNoteOn(struct BrassCore* p, int voice, const int16_t* knobs);
void brassNoteOff(struct BrassCore* p, int voice);
// env = obwiednia glosnosci glosu w tym bloku (Q15, 0..32767)
void brassRender(struct BrassCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel, int env);

typedef void (*BrwLineFn)(void* user, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* rgba);
typedef void (*BrwArtFn)(void* user, const int32_t* const* m);  // m[grupa]: 0 white, 1 blue-2, 2 blue-1, 3 green-move, 4 red-spinner
void brassScreenUpdate(void* user, BrwLineFn line_fn, BrwArtFn art_fn, const int16_t live[4], int dt_ms);

#ifdef __cplusplus
}
#endif

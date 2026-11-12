// Oryginalny silnik ARC z op1r (ultralazr, GPL-3.0-or-later; third_party/op1r_arc/arc.c bez zmian): "spiewajacy luk"
// Duddella (1899) — kazdy glos to oscylator Van der Pola (lampa lukowa): czysty sinus -> trzcina -> wrzaskliwa
// relaksacja, szum plazmy, zaplon i trzaski, druga sprzezona lampa. Galki: 0 CURRENT (mu), 1 CARBON (parzyste
// harmoniczne), 2 HISS (szum plazmy), 3 STRIKE (zaplon + trzaski). Strona 2: knobs[5] RATIO 2. lampy (1/2..2, zapadki;
// 0 = 1:1), [6] COUPLING (0 = 2. lampa wyl.), [7] BRIGHTNESS (0 = neutralnie). Stan glosu = slot 0x24 B w instancji;
// dane ekranu (portret fazowy, poziomy, glos sledzony) w instancji.
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ArcCore;
struct ArcCore* arcCreate(void);
void arcNoteOn(struct ArcCore* p, int voice, const int16_t* knobs);
void arcNoteOff(struct ArcCore* p, int voice);
void arcRender(struct ArcCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel);
void arcSetSeed(struct ArcCore* p, int voice, uint32_t seed);  // test zgodnosci: ziarno szumu jak w harnessie
int arcRatioQ14(int16_t w);                                     // proporcja 2. lampy (Q14, 16384 = 1:1)

typedef void (*ArwLineFn)(void* user, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* rgba);
typedef void (*ArwArtFn)(void* user);
void arcScreenUpdate(void* user, ArwLineFn line_fn, ArwArtFn art_fn, const struct ArcCore* core, const int16_t knobs[8],
                     const int16_t live[4], int page2, int dt_ms);

#ifdef __cplusplus
}
#endif

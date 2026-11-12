// Oryginalny silnik DDDD z op1r (pcppcp, GPL-3.0-or-later; third_party/op1r_dddd/plugins/dddd/dddd.c + dddd/dd_core.c
// bez zmian): ostry szum, parafonicznie — jedna wspolna "sciana" dla wszystkich klawiszy: zrodla (szum, trzaski, chaos,
// metal) -> petla sprzezenia (jeden odczep na trzymany klawisz, 4 najnowsze) -> filtr -> niszczenie (fold, clip, crush)
// -> limiter; losowe "rece" nia ruszaja. Gra najnizszy brzmiacy glos, pozostale milcza. Galki: 0 SOURCE, 1 FEEDBACK,
// 2 DESTROY, 3 HANDS; strona 2: knobs[5] FILTER (LP -> BP -> HP), [6] SPREAD (rozrzut petli), [7] RATE (crush 1..1/32).
// Obwiednia glosnosci glosu (u nas AMP ENV) otwiera filtr. Caly stan (sciana 16 KB, glosy, kolejnosc nut) per
// instancja, podmieniany leniwie.
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct DdddCore;
struct DdddCore* ddddCreate(void);
void ddddNoteOn(struct DdddCore* p, int voice, const int16_t* knobs);
void ddddNoteOff(struct DdddCore* p, int voice);
void ddddRender(struct DdddCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel, int env);

typedef void (*DdwLineFn)(void* user, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* rgba);
void ddddScreenUpdate(void* user, DdwLineFn line_fn, const struct DdddCore* core, const int16_t knobs[8],
                      const int16_t live[4], int page2, int dt_ms);

#ifdef __cplusplus
}
#endif

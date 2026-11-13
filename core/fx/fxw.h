/* fxw.h — wiele egzemplarzy efektu op1r w mikserze DAW-POL, bez zmian w kodzie efektu.
 * Efekty op1r trzymaja stan w tablicach statycznych indeksowanych pozycja FX (0..2) i w pamieci roboczej pozycji
 * (op1_fx_scratch). Wrapper przed dolaczeniem zrodla przekierowuje kazda taka tablice makrem na wskaznik:
 *     #define st (*fxw_st)        ->  "static tape_t st[4];" staje sie "static tape_t (*fxw_st)[4];"
 * i przed kazdym wywolaniem ustawia wskazniki na pamiec egzemplarza (FXW_FIELD / FXW_BIND), a op1_fx_scratch
 * zwraca pamiec robocza egzemplarza. Efekt zawsze dziala jako pozycja 0. Zegar tempa (op1_beat_clock) i zrodlo
 * sidechain (op1_sc_source) ustawia mikser (fx_common.c). Dolaczac na poczatku pliku wrappera. */
#ifndef DP_FXW_H
#define DP_FXW_H
#ifndef OP1_HOST
#define OP1_HOST
#endif
#define op1_fx_scratch fxw_unused_scratch
#define op1_fx_param_q15 fxw_unused_q15
#include "op1_plugin.h"
#undef op1_fx_scratch
#undef op1_fx_param_q15
#include <stdlib.h>
#include <string.h>
#include "core/fx/op1r_fx.h"
static void* fxw_scratch;
static inline void* op1_fx_scratch(int p) { (void)p; return fxw_scratch; }
static inline int32_t op1_fx_param_q15(const int16_t* p, int i) { return p[i] < 0 ? 0 : p[i]; }
extern op1_beat_t fxw_beat;                  /* zegar tempa bloku (fx_common.c, ustawia mikser) */
#define FXW_BEAT() (*host_beat() = fxw_beat)
/* pola egzemplarza: pelna tablica (wszystkie pozycje, uzywana jest 0) */
#define FXW_FIELD(n) __typeof__(*fxw_##n) n##_s;
#define FXW_BIND(x, n) fxw_##n = &(x)->n##_s;
/* telemetria dla ekranu: element [0] wybranych tablic */
#define FXW_TFIELD(n) __typeof__((*fxw_##n)[0]) n##_t;
#define FXW_TCOPY(o, n) memcpy(&(o)->n##_t, (const void*)&(*fxw_##n)[0], sizeof (o)->n##_t);
/* zmienne skalarne (#define x (*fxw_x)) */
#define FXW_FIELD1(n) __typeof__(*fxw_##n) n##_s;
#define FXW_BIND1(x, n) fxw_##n = &(x)->n##_s;
#define FXW_TFIELD1(n) __typeof__(*fxw_##n) n##_t;
#define FXW_TCOPY1(o, n) memcpy(&(o)->n##_t, (const void*)fxw_##n, sizeof (o)->n##_t);
/* wersje do X-makr (zmienne lokalne: x = egzemplarz, out = telemetria typu Tel) */
#define FXW_BINDX(n) fxw_##n = &x->s.n##_s;
#define FXW_BIND1X(n) fxw_##n = &x->s.n##_s;
#define FXW_TCOPYX(n) memcpy(&((Tel*)out)->n##_t, (const void*)&(*fxw_##n)[0], sizeof ((Tel*)out)->n##_t);
#define FXW_TCOPY1X(n) memcpy(&((Tel*)out)->n##_t, (const void*)fxw_##n, sizeof ((Tel*)out)->n##_t);
#define FXW_SCRATCH_BYTES (OP1_FX_SCRATCH_BYTES + 64)
#endif

/* Wspolne dla efektow op1r w mikserze: tabela implementacji (numery = FxId z seq/project.h), zegar tempa bloku
 * (op1_beat_clock w kazdym wrapperze czyta fxw_beat) i zrodlo sidechain (op1_sc_source, ENTE). */
#define OP1_HOST
#include "op1_plugin.h"
#include <stdlib.h>
#include <string.h>
#include "core/fx/op1r_fx.h"

op1_beat_t fxw_beat = {22050 << 14, 22050 << 14, 0x100000, 0, 0};
static const int16_t* sc_cur;

void fxSetBeat(int32_t len, int32_t next, int running) {
    fxw_beat.len = len;
    fxw_beat.next = next;
    fxw_beat.step = running ? 0x100000 : 0x100000;  /* zegar biegnie zawsze (jak metronom OP-1 bez tasmy) */
    fxw_beat.follows_tape = 0;
}
void fxSetSidechain(const int16_t* sc) { sc_cur = sc; }
int op1_sc_source(int src, int16_t* out) {
    (void)src;
    if (sc_cur) { memcpy(out, sc_cur, 64 * sizeof(int16_t)); return 1; }
    memset(out, 0, 64 * sizeof(int16_t));
    return 0;
}

/* JUNI CHORUS i SISTER (starsze wrappery) w tej samej tabeli */
static void jc_st(void* i, int16_t* l, int16_t* r, const int16_t* w) { fxJuniChorusProcess(i, l, r, w); }
static void jc_mo(void* i, int16_t* io, const int16_t* w) { int16_t r[64]; memcpy(r, io, sizeof r); fxJuniChorusProcess(i, io, r, w); }
static void jc_tel(void* i, void* out) { (void)i; fxJuniChorusTelemetry(out); }
static void si_st(void* i, int16_t* l, int16_t* r, const int16_t* w) { fxSisterProcess(i, l, r, w); }
static void si_mo(void* i, int16_t* io, const int16_t* w) { int16_t r[64]; memcpy(r, io, sizeof r); fxSisterProcess(i, io, r, w); }
static void plain_free(void* i) { free(i); }
static const FxImpl FXI_juni_chorus = {1, fxJuniChorusCreate, jc_mo, jc_st, jc_tel, 0, plain_free};
static const FxImpl FXI_sister = {1, fxSisterCreate, si_mo, si_st, fxSisterTelemetry, 0, plain_free};
extern const FxImpl FXI_snowglobe, FXI_rackverb, FXI_tape, FXI_father, FXI_stutter, FXI_crush, FXI_ente;

const FxImpl* fxImpl(int fx) {
    switch (fx) {  /* kolejnosc = enum FxId (seq/project.h) */
    case 1: return &FXI_juni_chorus;
    case 2: return &FXI_sister;
    case 3: return &FXI_snowglobe;
    case 4: return &FXI_rackverb;
    case 5: return &FXI_tape;
    case 6: return &FXI_father;
    case 7: return &FXI_stutter;
    case 8: return &FXI_crush;
    case 9: return &FXI_ente;
    default: return 0;
    }
}

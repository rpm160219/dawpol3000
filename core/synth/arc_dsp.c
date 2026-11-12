/* Opakowanie arc.c (C, bez zmian; petle C jak w buildach hosta — jadra Blackfin arc_dsp.S pominiete). Stan glosu pod
 * "self" (slot jak pamiec glosu OP-1) w instancji sciezki. Globalne dane ekranu oryginalu (arc_trace, arc_tpos,
 * arc_level) przekierowane makrami (op1_plugin.h) do buforow instancji; glos sledzony (arc_tvoice) podmieniany.
 * Tablica strojenia (arc_gp) zalezy tylko od stalych — wspolna. Ekran: druga kompilacja w core/ui/arc_screen_c.c. */
#include "synth/arc_dsp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

const int32_t* arw_pitch_ptr = 0;
int arw_vel = 100;
int16_t arc_sine_tab[8192];
const void* arw_base = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "../../third_party/op1r_arc/arc.c"
#pragma GCC diagnostic pop

/* ekran tej kopii nieuzywany — zaslepki */
volatile uint8_t arw_eng_page[4];
const int16_t* arw_knobs;
const int16_t* arw_live;
void* arw_find_node(void* scr, const char* id) { (void)scr; (void)id; return 0; }
void arw_screen_draw(void* scr, int dt) { (void)scr; (void)dt; }
void* arw_raster(void) { return 0; }
void arw_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c) { (void)r; (void)a; (void)b; (void)w; (void)c; }

struct ArcCore {
    uint32_t mem[OP1_VOICES][OP1_VOICE_SIZE / 4];
    int16_t trace[ARC_TN][4] __attribute__((aligned(8)));
    volatile uint16_t tpos;
    volatile int16_t level[OP1_VOICES];
    uint8_t tvoice;
};

static void sineInit(void) {
    if (arc_sine_tab[2048]) return;
    for (int i = 0; i < 8192; i++) arc_sine_tab[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));  /* jak OP-1 */
}
static void enter(struct ArcCore* p) {
    arw_base = p->mem;
    arw_trace_ptr = &p->trace;
    arw_tpos_ptr = &p->tpos;
    arw_level_ptr = &p->level;
    arw_tvoice = p->tvoice;
}
static void leave(struct ArcCore* p) { p->tvoice = arw_tvoice; }

struct ArcCore* arcCreate(void) {
    sineInit();
    gp_decode(ARC_KN);  /* tablica strojenia (w oryginale: init ekranu) */
    struct ArcCore* p = (struct ArcCore*)calloc(1, sizeof(struct ArcCore));
    if (p) p->tvoice = 0xff;
    return p;
}
void arcNoteOn(struct ArcCore* p, int voice, const int16_t* knobs) {
    enter(p);
    arw_engine.note_on(p->mem[voice], knobs);
    leave(p);
}
void arcNoteOff(struct ArcCore* p, int voice) {
    if (((const arc_voice_t*)p->mem[voice])->gate) arw_engine.note_off(p->mem[voice]);
}
void arcRender(struct ArcCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel) {
    enter(p);
    arw_pitch_ptr = (const int32_t*)pitch;
    arw_vel = vel;
    arw_engine.render(p->mem[voice], out, knobs);
    leave(p);
}
void arcSetSeed(struct ArcCore* p, int voice, uint32_t seed) { ((arc_voice_t*)p->mem[voice])->rng = seed; }
int arcRatioQ14(int16_t w) { return ratio_of(clampk(w)); }

/* dla ekranu */
void arcCopyForScreen(const struct ArcCore* p, void* mem, void* trace, uint16_t* tpos, int16_t* level, uint8_t* tvoice);
void arcCopyForScreen(const struct ArcCore* p, void* mem, void* trace, uint16_t* tpos, int16_t* level, uint8_t* tvoice) {
    memcpy(mem, p->mem, sizeof p->mem);
    memcpy(trace, p->trace, sizeof p->trace);
    *tpos = p->tpos;
    for (int i = 0; i < OP1_VOICES; i++) { level[i] = p->level[i]; }
    *tvoice = p->tvoice;
}

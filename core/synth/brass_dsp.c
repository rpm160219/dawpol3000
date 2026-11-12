/* Opakowanie brass.c i horn.c (C, bez zmian; petla C jak w buildach hosta — jadro Blackfin brass_dsp.S pominiete).
 * Stan glosu pod "self" (slot jak pamiec glosu OP-1) w instancji sciezki — bez podmiany stanu. Obwiednia glosnosci
 * glosu przekazywana w brw_env (oryginal czyta ostatnia probke bloku: OP1_VOICE_ENV[127]). */
#include "synth/brass_dsp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

const int32_t* brw_pitch_ptr = 0;
int brw_vel = 100;
int16_t brw_env[128];
int16_t brw_sine[8192];
const int16_t* brw_live = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "../../third_party/op1r_brass/brass.c"
#include "../../third_party/op1r_brass/horn.c"
#pragma GCC diagnostic pop

struct BrassCore { uint32_t mem[OP1_VOICES][OP1_VOICE_SIZE / 4]; };

static void sineInit(void) {
    if (brw_sine[2048]) return;
    for (int i = 0; i < 8192; i++) brw_sine[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));  /* jak host_sine / OP-1 */
}

struct BrassCore* brassCreate(void) {
    sineInit();
    return (struct BrassCore*)calloc(1, sizeof(struct BrassCore));
}
void brassNoteOn(struct BrassCore* p, int voice, const int16_t* knobs) { ENGINE_brass.note_on(p->mem[voice], knobs); }
void brassNoteOff(struct BrassCore* p, int voice) {
    if (((const brass_voice_t*)p->mem[voice])->held) ENGINE_brass.note_off(p->mem[voice]);
}
void brassRender(struct BrassCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel, int env) {
    brw_pitch_ptr = (const int32_t*)pitch;
    brw_vel = vel;
    brw_env[OP1_BLOCK - 1] = (int16_t)(env < 0 ? 0 : (env > 32767 ? 32767 : env));
    ENGINE_brass.render(p->mem[voice], out, knobs);
}

/* ---- ekran ---- */
static const char* const node_ids[5] = {"white", "blue-2", "blue-1", "green-move", "red-spinner"};
static int32_t node_m[5][9];
static int node_set[5];
static void* cb_user;
static BrwLineFn cb_line;
static BrwArtFn cb_art;
static int screen_ready;

void* brw_find_node(void* scr, const char* id) {
    (void)scr;
    for (int i = 0; i < 5; i++) if (!strcmp(id, node_ids[i])) return node_m[i];
    return 0;
}
void brw_node_set_transform(void* node, const int32_t* m) {
    memcpy(node, m, sizeof(int32_t) * 9);
    node_set[((int32_t(*)[9])node) - node_m] = 1;
}
void brw_screen_draw(void* scr, int dt) {
    (void)scr; (void)dt;
    const int32_t* m[5];
    for (int i = 0; i < 5; i++) m[i] = node_set[i] ? node_m[i] : 0;
    cb_art(cb_user, m);
}
void* brw_raster(void) { return cb_user; }
void brw_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c) { cb_line(r, a->x, a->y, b->x, b->y, w, c); }

void brassScreenUpdate(void* user, BrwLineFn line_fn, BrwArtFn art_fn, const int16_t live[4], int dt_ms) {
    sineInit();
    cb_user = user; cb_line = line_fn; cb_art = art_fn;
    brw_live = live;
    if (!screen_ready) { SCREEN_horn.init(0); screen_ready = 1; }
    SCREEN_horn.update(0, dt_ms);
}

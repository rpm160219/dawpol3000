/* Opakowanie bloodmoon.c i moon.c (C, bez zmian; petla C — jadro Blackfin bloodmoon_dsp.S pominiete). Stan glosu pod
 * "self" (slot jak pamiec glosu OP-1) w instancji sciezki; jedyny wspolny stan audio — ziarno bm_seed — podmieniany
 * per instancja. Ekran: wezly (przeksztalcenie, widocznosc) dla funkcji zwrotnej rysunku. */
#include "synth/bloodmoon_dsp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

const int32_t* bmw_pitch_ptr = 0;
int bmw_vel = 100;
int16_t bmw_sine[8192];
const int16_t* bmw_live = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "../../third_party/op1r_bloodmoon/bloodmoon.c"
#include "../../third_party/op1r_bloodmoon/moon.c"
#pragma GCC diagnostic pop

struct BloodCore { uint32_t mem[OP1_VOICES][OP1_VOICE_SIZE / 4]; uint32_t seed; };

static void sineInit(void) {
    if (bmw_sine[2048]) return;
    for (int i = 0; i < 8192; i++) bmw_sine[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));  /* jak OP-1 */
}

struct BloodCore* bloodCreate(void) {
    sineInit();
    struct BloodCore* p = (struct BloodCore*)calloc(1, sizeof(struct BloodCore));
    if (p) p->seed = 0x2545F491u;  /* jak bm_seed oryginalu */
    return p;
}
void bloodNoteOn(struct BloodCore* p, int voice, const int16_t* knobs) {
    bm_seed = p->seed;
    ENGINE_bloodmoon.note_on(p->mem[voice], knobs);
    p->seed = bm_seed;
}
void bloodNoteOff(struct BloodCore* p, int voice) {
    if (((const bm_voice_t*)p->mem[voice])->held) ENGINE_bloodmoon.note_off(p->mem[voice]);
}
void bloodRender(struct BloodCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel) {
    bmw_pitch_ptr = (const int32_t*)pitch;
    bmw_vel = vel;
    ENGINE_bloodmoon.render(p->mem[voice], out, knobs);
}

/* ---- ekran ---- */
static const char* const node_ids[9] = {"stars0", "stars1", "stars2", "moon", "cathedral", "windows", "rose", "bellL", "bellR"};
static int32_t node_m[9][9];
static uint8_t node_set[9], node_vis[9];
static void* cb_user;
static BmwArtFn cb_art;
static int screen_ready;

void* bmw_find_node(void* scr, const char* id) {
    (void)scr;
    for (int i = 0; i < 9; i++) if (!strcmp(id, node_ids[i])) return node_m[i];
    return 0;
}
void bmw_node_set_transform(void* node, const int32_t* m) {
    memcpy(node, m, sizeof(int32_t) * 9);
    node_set[((int32_t(*)[9])node) - node_m] = 1;
}
void bmw_node_set_visible(void* node, int on) { node_vis[((int32_t(*)[9])node) - node_m] = (uint8_t)(on != 0); }
void bmw_screen_draw(void* scr, int dt) {
    (void)scr; (void)dt;
    const int32_t* m[9];
    for (int i = 0; i < 9; i++) m[i] = node_set[i] ? node_m[i] : 0;
    cb_art(cb_user, m, node_vis);
}
void* bmw_raster(void) { return cb_user; }

void bloodScreenUpdate(void* user, BmwArtFn art_fn, const int16_t live[4], int dt_ms) {
    sineInit();
    cb_user = user; cb_art = art_fn;
    bmw_live = live;
    if (!screen_ready) {
        for (int i = 0; i < 9; i++) node_vis[i] = 1;
        SCREEN_bloodmoon.init(0);
        screen_ready = 1;
    }
    SCREEN_bloodmoon.update(0, dt_ms);
}

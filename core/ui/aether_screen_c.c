/* Ekran AETHER: aether.c skompilowany drugi raz (osobne zmienne ekranu, osobny czujnik aes_accel), z funkcjami
 * zwrotnymi rysunku. Stan glosow i czujnik wirtualny kopiowane z instancji sciezki przed kazda klatka. Napisy
 * (etykiety tarcz) pominiete; tarcze, suwaki i luki wartosci (dublowalyby galki) pomija rysowanie (aether_screen.cpp). */
#define OP1_HOST 1
#include <string.h>
#include "synth/aether_dsp.h"

int16_t aes_accel[3] = {0, 0, 16384};
#define host_accel aes_accel
#define ENGINE_aether aes_engine_unused
#define SCREEN_aether aes_screen
#define aew_pitch_ptr aes_pitch_unused
#define aew_vel aes_vel_unused
#define aew_env aes_env
#define aew_base aes_base_unused
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "../../third_party/op1r_aether/aether.c"
#pragma GCC diagnostic pop
const int32_t* aes_pitch_unused;
int aes_vel_unused;
int16_t aes_env[128];
const void* aes_base_unused;
volatile uint8_t aes_eng_page[4];
const int16_t* aes_knobs;
const int16_t* aes_live;

void aetherCopyForScreen(const struct AetherCore* p, void* stDst, int* cur, int16_t accel[3]);

static const char* const node_ids[] = {"waves", "wave0", "wave1", "wave2", "wave3", "dish", "d0", "d1", "d2", "d3",
    "s0", "s1", "s2", "dd0", "dd1", "dd2", "dd3", "sd0", "sd1", "sd2"};
enum { NNODE = sizeof node_ids / sizeof node_ids[0] };
static int32_t node_m[NNODE][9];
static uint8_t node_set[NNODE], node_vis[NNODE];
static void* cb_user;
static AewLineFn cb_line;
static AewArtFn cb_art;
static int screen_ready;

void* aes_find_node(void* scr, const char* id) {
    (void)scr;
    for (int i = 0; i < NNODE; i++) if (!strcmp(id, node_ids[i])) return node_m[i];
    return 0;  /* igly, uchwyty (nd*, nl*, hb*, hd*): w ukrytych tarczach */
}
void aes_node_set_transform(void* node, const int32_t* m) {
    memcpy(node, m, sizeof(int32_t) * 9);
    node_set[((int32_t(*)[9])node) - node_m] = 1;
}
void aes_node_set_visible(void* node, int on) { node_vis[((int32_t(*)[9])node) - node_m] = (uint8_t)(on != 0); }
void aes_screen_draw(void* scr, int dt) {
    (void)scr; (void)dt;
    const int32_t* m[NNODE];
    for (int i = 0; i < NNODE; i++) m[i] = node_set[i] ? node_m[i] : 0;
    cb_art(cb_user, m, node_vis);
}
void* aes_raster(void) { return cb_user; }
void aes_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c) {
    if (w == 0x600) return;  /* luki wartosci tarcz (dublowalyby galki DAW-POL) */
    cb_line(r, a->x, a->y, b->x, b->y, w, c);
}

void aetherScreenUpdate(void* user, AewLineFn line_fn, AewArtFn art_fn, const struct AetherCore* core,
                        const int16_t knobs[8], const int16_t live[4], int page2, int dt_ms) {
    cb_user = user; cb_line = line_fn; cb_art = art_fn;
    if (core) aetherCopyForScreen(core, st, (int*)&ae_cur, aes_accel);
    else { aes_accel[0] = 0; aes_accel[1] = 0; aes_accel[2] = 16384; }
    aes_knobs = knobs;
    aes_live = live;
    for (int i = 0; i < 3; i++) aes_eng_page[i] = (uint8_t)(page2 != 0);
    if (!screen_ready) {
        for (int i = 0; i < NNODE; i++) node_vis[i] = 1;
        for (int i = 0; i < 128; i++) aes_env[i] = 0;
        aes_screen.init(0);
        screen_ready = 1;
    }
    aes_screen.update(0, dt_ms);
}

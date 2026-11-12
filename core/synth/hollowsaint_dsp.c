/* Opakowanie hollow_saint.c (C, bez zmian; petle C jak w buildach hosta — jadra Blackfin hs_*.S pominiete), z flagami
 * wersji kennel z plugin.json (wspolny pogłos i tasma, pudlo przy 22 kHz, lzejsza tasma, tanszy szum smyczka, struna
 * przy polowie czestotliwosci od C5). Caly stan oryginalu siedzi w zmiennych pliku (glosy V[], wspolny etap BUS/BV,
 * linie pogłosu, burdon, licznik blokow, tasma): kazda instancja (sciezka) ma wlasna kopie, podmieniana leniwie, gdy
 * przychodzi wywolanie innej instancji (lista HS_STATE). Pamieci podreczne w funkcjach (tempo -> faza taktu) zaleza
 * tylko od zegara — wspolne. Wywolania z jednego watku audio; ekran (watek UI) czyta stan biezacej instancji. */
#include "synth/hollowsaint_dsp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

const int32_t* hsw_pitch_ptr = 0;
int hsw_vel = 100;
int16_t hsw_env[128];
int16_t hsw_sine[8192];
const void* hsw_base = 0;
volatile uint8_t hsw_eng_page[4];
const int16_t* hsw_knobs = 0;
const int16_t* hsw_live = 0;

#define HS_SHARED_ROOM
#define HS_SHARED_RELIC
#define HS_CAT11K
#define HS_RELIC_LITE
#define HS_BOW_CHEAP
#define HS_HIGH_HALF
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#include "../../third_party/op1r_hollowsaint/hollow_saint.c"
#pragma GCC diagnostic pop
op1_beat_t hsw_beat;

/* stan oryginalu na instancje */
#define HS_STATE \
    X(V) X(blk) X(last_vi) X(v_blk) X(v_inc) X(owner) X(wheel_free) X(wheel_new_block) X(relic_new_block) \
    X(beat_n) X(beat_last) X(relic_wow) X(relic_wow2) X(relic_flut) X(relic_rng) X(relic_sag) X(relic_sag_t) \
    X(hs_relic_wob) X(hs_blocks) X(hs_stroke) X(hs_dog) X(hs_bell_n) X(hs_bell_lvl) X(BUS) X(BV) X(room_a1) \
    X(room_a2) X(room_r) X(bus_new_block) X(bus_rb)

struct HollowCore {
    uint32_t mem[OP1_VOICES][OP1_VOICE_SIZE / 4];  /* "self" glosow (tylko adres -> indeks) */
#define X(n) unsigned char s_##n[sizeof(n)];
    HS_STATE
#undef X
};

static struct HollowCore* cur;   /* instancja, ktorej stan jest teraz w zmiennych pliku */
static struct HollowCore fresh;  /* stan poczatkowy (jak po zaladowaniu oryginalu) */
static int fresh_ready;

static void save(struct HollowCore* p) {
#define X(n) memcpy(p->s_##n, (const void*)&n, sizeof(n));
    HS_STATE
#undef X
}
static void load(const struct HollowCore* p) {
#define X(n) memcpy((void*)&n, p->s_##n, sizeof(n));
    HS_STATE
#undef X
}
static void enter(struct HollowCore* p) {
    if (cur != p) {
        if (cur) save(cur);
        load(p);
        cur = p;
    }
    hsw_base = p->mem;
}

static void sineInit(void) {
    if (hsw_sine[2048]) return;
    for (int i = 0; i < 8192; i++) hsw_sine[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));  /* jak OP-1 */
}

struct HollowCore* hollowCreate(void) {
    sineInit();
    if (!fresh_ready) { save(&fresh); fresh_ready = 1; }  /* pierwsze wywolanie: zmienne pliku = stan startowy */
    struct HollowCore* p = (struct HollowCore*)malloc(sizeof *p);
    if (p) *p = fresh;
    return p;
}

void hollowNoteOn(struct HollowCore* p, int voice, const int16_t* knobs) {
    enter(p);
    ENGINE_hollow_saint.note_on(p->mem[voice], knobs);
}
void hollowNoteOff(struct HollowCore* p, int voice) {
    enter(p);
    if (V[voice].held) ENGINE_hollow_saint.note_off(p->mem[voice]);
}
void hollowRender(struct HollowCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel,
                  int env, int relic, int32_t beatLen, int32_t beatNext, int32_t beatStep) {
    enter(p);
    hsw_pitch_ptr = (const int32_t*)pitch;
    hsw_vel = vel;
    const int16_t e = (int16_t)(env < 0 ? 0 : (env > 32767 ? 32767 : env));
    for (int i = 0; i < 128; i++) hsw_env[i] = e;  /* obwiednia OP-1: stala w bloku */
    hsw_eng_page[1] = (uint8_t)(relic != 0);
    hsw_beat.len = beatLen; hsw_beat.next = beatNext; hsw_beat.step = beatStep;
    ENGINE_hollow_saint.render(p->mem[voice], out, knobs);
}

int hollowDroneKey(int16_t w) { int16_t k[8] = {0}; k[5] = w; return drone_key(k); }
int hollowStrokeZone(int16_t w) { int16_t k[8] = {0}; k[7] = w; return stroke_zone(k); }

/* ---- ekran ---- */
static const char* const node_ids[] = {"bell", "halo", "kp0", "kp1", "kp2", "kp3", "kp4", "kp5", "p0", "p1", "p2",
    "p3", "p4", "p5", "p6", "p7", "wheel", "s0", "s1", "s2", "s3", "s4", "dog", "crank", "m0", "m1", "m2", "m3", "kbd",
    "kdot", "relic", "rl0", "rl1", "rage0", "rage", "dust", "r0", "r1", "r2", "r3", "r4", "r5", "r6"};
enum { NNODE = sizeof node_ids / sizeof node_ids[0] };
static int32_t node_m[NNODE][9];
static uint8_t node_set[NNODE], node_vis[NNODE];
static void* cb_user;
static HswArtFn cb_art;
static int screen_ready;

void* hsw_find_node(void* scr, const char* id) {
    (void)scr;
    for (int i = 0; i < NNODE; i++) if (!strcmp(id, node_ids[i])) return node_m[i];
    return 0;
}
void hsw_node_set_transform(void* node, const int32_t* m) {
    memcpy(node, m, sizeof(int32_t) * 9);
    node_set[((int32_t(*)[9])node) - node_m] = 1;
}
void hsw_node_set_visible(void* node, int on) { node_vis[((int32_t(*)[9])node) - node_m] = (uint8_t)(on != 0); }
void hsw_screen_draw(void* scr, int dt) {
    (void)scr; (void)dt;
    const int32_t* m[NNODE];
    for (int i = 0; i < NNODE; i++) m[i] = node_set[i] ? node_m[i] : 0;
    cb_art(cb_user, m, node_vis);
}

void hollowScreenUpdate(void* user, HswArtFn art_fn, const struct HollowCore* core, const int16_t knobs[8],
                        const int16_t live[4], int relic, int dt_ms) {
    (void)core;  /* ekran czyta stan biezacej instancji (zmienne pliku) */
    sineInit();
    cb_user = user; cb_art = art_fn;
    hsw_knobs = knobs;
    hsw_live = live;
    if (!screen_ready) {
        for (int i = 0; i < NNODE; i++) node_vis[i] = 1;
        SCREEN_hollow_saint.init(0);
        screen_ready = 1;
    }
    /* strony 2 gałek: tylko RELIC (kaseta) — wskazniki klucza i tempa uderzen dublowalyby galki DAW-POL */
    const uint8_t keep1 = hsw_eng_page[1];
    hsw_eng_page[0] = 0; hsw_eng_page[2] = 0; hsw_eng_page[1] = (uint8_t)(relic != 0);
    SCREEN_hollow_saint.update(0, dt_ms);
    hsw_eng_page[1] = keep1;
}

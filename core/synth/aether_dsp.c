/* Opakowanie aether.c (C, bez zmian, z OP1_HOST: czujnik = host_accel). Audio: stan glosow st[6] i ae_cur podmieniany
 * per instancja (kopiowany do / z instancji przy kazdym wywolaniu, ~400 B). Czujnik wirtualny: przy nucie "poziomo"
 * (dom), w bloku przechylenie ze strony TILT (+ modulacja per glos). Ekran: core/ui/aether_screen_c.c kompiluje aether.c
 * drugi raz (osobne zmienne ekranu i osobny czujnik), stan glosow i czujnik kopiuje z instancji. */
#define OP1_HOST 1
#define AE_SCR(n) aewx_##n  /* ekran tej kopii nieuzywany: zaslepki nizej */
#include "synth/aether_dsp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

const int32_t* aew_pitch_ptr = 0;
int aew_vel = 100;
int16_t aew_env[128];
int16_t aew_sine[8192];
const void* aew_base = 0;
int16_t host_accel[3] = {0, 0, 16384};

#define ENGINE_aether aew_engine
#define SCREEN_aether aew_screen_unused
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "../../third_party/op1r_aether/aether.c"
#pragma GCC diagnostic pop

/* ekran tej kopii (nieuzywany; ekran ma osobna kompilacje) — zaslepki */
volatile uint8_t aewx_eng_page[4];
const int16_t* aewx_knobs;
const int16_t* aewx_live;
void* aewx_find_node(void* scr, const char* id) { (void)scr; (void)id; return 0; }
void aewx_node_set_transform(void* node, const int32_t* m) { (void)node; (void)m; }
void aewx_node_set_visible(void* node, int on) { (void)node; (void)on; }
void aewx_screen_draw(void* scr, int dt) { (void)scr; (void)dt; }
void* aewx_raster(void) { return 0; }
void aewx_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c) { (void)r; (void)a; (void)b; (void)w; (void)c; }

struct AetherCore {
    uint32_t mem[OP1_VOICES][OP1_VOICE_SIZE / 4];
    ae_t st[OP1_VOICES];
    int cur;
    uint32_t shph[OP1_VOICES];   /* faza drgania (SHAKE) per glos */
    int16_t accel[3];            /* ostatni czujnik (ekran) */
};

static void sineInit(void) {
    if (aew_sine[2048]) return;
    for (int i = 0; i < 8192; i++) aew_sine[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));  /* jak OP-1 */
}
static void enter(struct AetherCore* p) { memcpy(st, p->st, sizeof st); ae_cur = p->cur; aew_base = p->mem; }
static void leave(struct AetherCore* p) { memcpy(p->st, st, sizeof st); p->cur = ae_cur; }

struct AetherCore* aetherCreate(void) {
    sineInit();
    struct AetherCore* p = (struct AetherCore*)calloc(1, sizeof(struct AetherCore));
    if (p) p->accel[2] = 16384;
    return p;
}
void aetherNoteOn(struct AetherCore* p, int voice, const int16_t* knobs) {
    enter(p);
    host_accel[0] = 0; host_accel[1] = 0; host_accel[2] = 16384;  /* dom = poziomo */
    aew_engine.note_on(p->mem[voice], knobs);
    leave(p);
}
void aetherRender(struct AetherCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel,
                  int env, float tiltPitch, float tiltVol, float shake) {
    enter(p);
    const float tp = tiltPitch < -1.f ? -1.f : (tiltPitch > 1.f ? 1.f : tiltPitch);
    const float tv = tiltVol < 0.f ? 0.f : (tiltVol > 1.f ? 1.f : tiltVol);
    const float sk = shake < 0.f ? 0.f : (shake > 1.f ? 1.f : shake);
    const double ap = tp * (60.0 * M_PI / 180.0), av = tv * (30.0 * M_PI / 180.0);   /* przod / tyl, prawy bok */
    p->shph[voice] += 40000000u;                       /* ~7 Hz przy 750 blokach / s */
    /* drganie osi Z: prog detektora oryginalu ~650 jednostek (1 g = 16384), pelne potrzasanie ~2050 — SHAKE liniowo */
    const double amp = sk > 0.f ? 650.0 + sk * 1400.0 : 0.0;
    const double sz = amp * sin(p->shph[voice] * (2.0 * M_PI / 4294967296.0));
    host_accel[0] = (int16_t)lrint(-16384.0 * sin(av));
    host_accel[1] = (int16_t)lrint(-16384.0 * sin(ap));
    host_accel[2] = (int16_t)lrint(16384.0 * cos(ap) * cos(av) + sz);
    memcpy(p->accel, host_accel, sizeof p->accel);
    const int16_t e = (int16_t)(env < 0 ? 0 : (env > 32767 ? 32767 : env));
    for (int i = 0; i < 128; i++) aew_env[i] = e;
    aew_pitch_ptr = (const int32_t*)pitch;
    aew_vel = vel;
    aew_engine.render(p->mem[voice], out, knobs);
    leave(p);
}

/* dla ekranu (core/ui/aether_screen_c.c) */
void aetherCopyForScreen(const struct AetherCore* p, void* stDst, int* cur, int16_t accel[3]);
void aetherCopyForScreen(const struct AetherCore* p, void* stDst, int* cur, int16_t accel[3]) {
    memcpy(stDst, p->st, sizeof p->st);
    *cur = p->cur;
    memcpy(accel, p->accel, sizeof p->accel);
}

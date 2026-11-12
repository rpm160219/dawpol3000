/* Opakowanie juni.c (C, bez zmian; sciezki __bfin__ nieaktywne). Stan oryginalu w zmiennych pliku (glosy vc[6],
 * wspolne parametry bloku, LFO PWM i chorusa, linie chorusa, liczniki dla ekranu) — kazda instancja (sciezka) ma wlasna
 * kopie, podmieniana leniwie (lista JN_STATE). Ekran: druga kompilacja w core/ui/juni_screen_c.c. */
#include "synth/juni_dsp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

const int32_t* jnw_pitch_ptr = 0;
int jnw_vel = 100;
int16_t jnw_env[128];
int16_t jn_sine_tab[8192];
const void* jnw_base = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wparentheses"
#include "../../third_party/op1r_juni/juni.c"
#pragma GCC diagnostic pop

/* ekran tej kopii nieuzywany — zaslepki */
const int16_t* jnw_knobs;
const int16_t* jnw_live;
void jnw_screen_draw_cached(void* scr, int dt, int flags) { (void)scr; (void)dt; (void)flags; }
void* jnw_raster(void) { return 0; }
void jnw_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c) { (void)r; (void)a; (void)b; (void)w; (void)c; }

#define JN_STATE X(vc) X(last_voice) X(as) X(ap) X(ab) X(pw_blk) X(pwm_ph) X(pwm_depth) X(lfo_s) X(lfo_f) X(inc_s) \
    X(dep_s) X(dep_f) X(dry_g) X(wet_g) X(cut_base) X(env_amt) X(reso_q15) X(ghp) X(out_g) X(jn_blocks) X(jn_vblk) \
    X(dline32) X(wlast) X(whp)

struct JuniCore {
    uint32_t mem[OP1_VOICES][OP1_VOICE_SIZE / 4];
#define X(n) unsigned char s_##n[sizeof(n)];
    JN_STATE
#undef X
    volatile int32_t env0[OP1_VOICES];   /* dla ekranu */
    volatile uint32_t blocks, vblk[OP1_VOICES];
};
static struct JuniCore* cur;
static struct JuniCore fresh;
static int fresh_ready;
static void save(struct JuniCore* p) {
#define X(n) memcpy(p->s_##n, (const void*)&n, sizeof(n));
    JN_STATE
#undef X
}
static void load(const struct JuniCore* p) {
#define X(n) memcpy((void*)&n, p->s_##n, sizeof(n));
    JN_STATE
#undef X
}
static void enter(struct JuniCore* p) {
    if (cur != p) { if (cur) save(cur); load(p); cur = p; }
    jnw_base = p->mem;
}
static void sineInit(void) {
    if (jn_sine_tab[2048]) return;
    for (int i = 0; i < 8192; i++) jn_sine_tab[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));  /* jak OP-1 */
}

struct JuniCore* juniCreate(void) {
    sineInit();
    if (!fresh_ready) { save(&fresh); fresh_ready = 1; }
    struct JuniCore* p = (struct JuniCore*)malloc(sizeof *p);
    if (p) { *p = fresh; memset((void*)p->env0, 0, sizeof p->env0); p->blocks = 0; memset((void*)p->vblk, 0, sizeof p->vblk); }
    return p;
}
void juniNoteOn(struct JuniCore* p, int voice, const int16_t* knobs) {
    enter(p);
    jnw_engine.note_on(p->mem[voice], knobs);
}
void juniRender(struct JuniCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel, int env) {
    enter(p);
    jnw_pitch_ptr = (const int32_t*)pitch;
    jnw_vel = vel;
    const int16_t e = (int16_t)(env < 0 ? 0 : (env > 32767 ? 32767 : env));
    for (int i = 0; i < 128; i++) jnw_env[i] = e;
    jnw_engine.render(p->mem[voice], out, knobs);
    p->env0[voice] = vc[voice].env0;
    p->blocks = jn_blocks;
    p->vblk[voice] = jn_vblk[voice];
}

void juniCopyForScreen(const struct JuniCore* p, int32_t* env0, uint32_t* blocks, uint32_t* vblk);
void juniCopyForScreen(const struct JuniCore* p, int32_t* env0, uint32_t* blocks, uint32_t* vblk) {
    for (int i = 0; i < OP1_VOICES; i++) { env0[i] = p->env0[i]; vblk[i] = p->vblk[i]; }
    *blocks = p->blocks;
}

/* Opakowanie supersaw.c (C, bez zmian). Stan glosu oryginalu jest w pamieci wskazywanej przez "self" (voice_t,
 * w firmware to obiekt glosu) — u nas tablica voice_t[6] w instancji sciezki, wiec zadnej podmiany stanu.
 * Ekran oryginalu (statyczne zmienne ekranu = jeden widok naraz) czyta voice_t ogladanej sciezki (bramka klawisza). */
#include "synth/supersaw_dsp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

const int32_t* ssw_pitch_ptr = 0;
int ssw_vel = 100;
int16_t ssw_sine[8192];
const void* ssw_view = 0;
int ssw_voice_size = 0;
const int16_t* ssw_live = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "../../third_party/op1r_supersaw/supersaw.c"
#pragma GCC diagnostic pop

struct SupersawCore { voice_t v[OP1_VOICES]; };

static void sineInit(void) {
    if (ssw_sine[2048]) return;
    for (int i = 0; i < 8192; i++) ssw_sine[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));  /* jak host_sine / OP-1 */
}

struct SupersawCore* supersawCreate(void) {
    sineInit();
    struct SupersawCore* p = (struct SupersawCore*)calloc(1, sizeof *p);
    return p;
}
void supersawNoteOn(struct SupersawCore* p, int voice, const int16_t* knobs) { ENGINE_supersaw.note_on(&p->v[voice], knobs); }
void supersawNoteOff(struct SupersawCore* p, int voice) { if (p->v[voice].gate) ENGINE_supersaw.note_off(&p->v[voice]); }
void supersawRender(struct SupersawCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel) {
    ssw_pitch_ptr = (const int32_t*)pitch;
    ssw_vel = vel;
    ENGINE_supersaw.render(&p->v[voice], out, knobs);
}

/* ---- ekran ---- */
static const char* const node_ids[6] = {"saw", "sprk", "bar0", "bar1", "bar2", "bar3"};
static int32_t node_m[6][9];
static int node_set[6];
static void* cb_user;
static SswLineFn cb_line;
static SswArtFn cb_art;
static int screen_ready;
static const voice_t idle_voices[OP1_VOICES];

void* op1_find_node(void* scr, const char* id) {
    (void)scr;
    for (int i = 0; i < 6; i++) if (!strcmp(id, node_ids[i])) return node_m[i];
    return 0;
}
void op1_node_set_transform(void* node, const int32_t* m) {
    memcpy(node, m, sizeof(int32_t) * 9);
    node_set[((int32_t(*)[9])node) - node_m] = 1;
}
void op1_screen_draw(void* scr, int dt) {
    (void)scr; (void)dt;
    const int32_t* m[6];
    for (int i = 0; i < 6; i++) m[i] = node_set[i] ? node_m[i] : 0;
    cb_art(cb_user, m);
}
void* op1_raster(void) { return cb_user; }
void op1_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c) { cb_line(r, a->x, a->y, b->x, b->y, w, c); }

void supersawScreenUpdate(void* user, SswLineFn line_fn, SswArtFn art_fn, const struct SupersawCore* core,
                          const int16_t live[4], int dt_ms) {
    sineInit();
    cb_user = user; cb_line = line_fn; cb_art = art_fn;
    ssw_view = core ? (const void*)core->v : (const void*)idle_voices;
    ssw_voice_size = (int)sizeof(voice_t);
    ssw_live = live;
    if (!screen_ready) { SCREEN_supersaw.init(0); screen_ready = 1; }
    SCREEN_supersaw.update(0, dt_ms);
}

/* Opakowanie outatime.c + ota_dsp.c (C, bez zmian; sciezki Blackfin / L1 nieaktywne — petle C). Stan glosow oryginalu
 * (ota_home.vc[6]) i trzymane klawisze w zmiennych pliku: kopiowane z / do instancji przy kazdym wywolaniu (ok. 64 B na
 * glos). Pomocnicze bufory ota_dsp.c (scratch) sa za kazdym razem zapisywane od nowa — wspolne. Dane dla ekranu
 * (outatime_blocks, outatime_held, outatime_newest, lustra dryfu i zaplonu) przemianowane i przenoszone do instancji. */
#include "synth/outatime_dsp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

const int32_t* otw_pitch_ptr = 0;
int otw_vel = 100;
const void* otw_base = 0;
int16_t ot_sine_tab[8192];

#define outatime_blocks otw_blocks
#define outatime_held otw_held
#define outatime_newest otw_newest
#define outatime_voice otw_voice
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "../../third_party/kennel_outatime/plugins/outatime/outatime.c"
#include "../../third_party/kennel_outatime/plugins/outatime/ota_dsp.c"
#pragma GCC diagnostic pop

/* ekran tej kopii nieuzywany (osobna kompilacja) — zaslepki */
const int16_t* ots_knobs;
const int16_t* ots_live;
volatile uint8_t ots_eng_page[4];

struct OutatimeCore {
    uint32_t mem[OP1_VOICES][OP1_VOICE_SIZE / 4];
    kv_voice vc[OP1_VOICES];
    int32_t held[OP1_VOICES];
    int32_t nheld, newest;
    volatile uint32_t blocks;
    struct ota_voice mirror[OP1_VOICES];
};

static void enter(struct OutatimeCore* p, int vi) {
    otw_base = p->mem;
    ota_home.vc[vi] = p->vc[vi];
    for (int i = 0; i < OP1_VOICES; i++) held[i] = p->held[i];
    otw_held = p->nheld; otw_newest = p->newest; otw_blocks = p->blocks;
}
static void leave(struct OutatimeCore* p, int vi) {
    p->vc[vi] = ota_home.vc[vi];
    for (int i = 0; i < OP1_VOICES; i++) p->held[i] = held[i];
    p->nheld = otw_held; p->newest = otw_newest; p->blocks = otw_blocks;
    p->mirror[vi] = mirror[vi];
}
static void sineInit(void) {
    if (ot_sine_tab[2048]) return;
    for (int i = 0; i < 8192; i++) ot_sine_tab[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));  /* jak OP-1 */
}

struct OutatimeCore* outatimeCreate(void) {
    sineInit();
    struct OutatimeCore* p = (struct OutatimeCore*)calloc(1, sizeof(struct OutatimeCore));
    if (p) p->newest = -1;
    return p;
}
void outatimeNoteOn(struct OutatimeCore* p, int voice, const int16_t* knobs) {
    enter(p, voice);
    ENGINE_outatime.note_on(p->mem[voice], knobs);
    leave(p, voice);
}
void outatimeNoteOff(struct OutatimeCore* p, int voice) {
    if (!p->held[voice]) return;
    enter(p, voice);
    ENGINE_outatime.note_off(p->mem[voice]);
    leave(p, voice);
}
void outatimeRender(struct OutatimeCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel) {
    enter(p, voice);
    otw_pitch_ptr = (const int32_t*)pitch;
    otw_vel = vel;
    ENGINE_outatime.render(p->mem[voice], out, knobs);
    leave(p, voice);
}

void outatimeCopyForScreen(const struct OutatimeCore* p, uint32_t* blocks, int32_t* nheld, int32_t* newest,
                           struct ota_voice* mir);
void outatimeCopyForScreen(const struct OutatimeCore* p, uint32_t* blocks, int32_t* nheld, int32_t* newest,
                           struct ota_voice* mir) {
    *blocks = p->blocks; *nheld = p->nheld; *newest = p->newest;
    memcpy(mir, p->mirror, sizeof p->mirror);
}

/* Silnik GIN: oryginalne pliki z third_party/kennel_gin (kompilowane osobno, flagi jak w plugin.json) + przydzial
 * odbiornikow, zestawy i konwersja czestotliwosci DAW-POL. Sekwencja wywolan adaptera jak w gin_fw.c (kennel):
 * gin_sdk_bind x12 -> gin_sdk_load_kit -> gin_sdk_publish_prepared x24 -> gin_sdk_select_engine -> gin_sdk_activate;
 * uderzenie: gin_sdk_publish_hit + gin_sdk_trigger_bound; blok: gin_sdk_render_bound + gin_fw_output_sample. */
#include "synth/gin_dsp.h"
#include <stdlib.h>
#include <string.h>
#include "gin_adapter.h"
#include "gin_fw_level.h"
#include "gin_factory.h"
#include "gin_prepare.h"
#include "po32_lut.h"
#include "po32_param_maps.h"
#include "../factory_bank/gin_factory_bank.h"

#define VPTR 0x0190f8a4u
#define UNITY_GAIN 16384  /* natywne wzmocnienie OP-1 (Q14) = 1 */

/* Rejestr silnikow perkusji (op1_drum.h): w DAW-POL jest tylko GIN. */
const op1_drum_engine_t* op1_drum_find(const char* name) { return name && !strcmp(name, "gin") ? &DRUM_ENGINE_gin : 0; }
static uint32_t st_trigger(void* s, const int16_t* p) { (void)s; (void)p; return OP1_DRUM_UNBOUND; }
static uint32_t st_render(void* s, int16_t* o, const int16_t* p) { (void)s; (void)p; memset(o, 0, 256); return OP1_DRUM_UNBOUND; }
static uint32_t st_status(const void* s) { (void)s; return 0; }
static uint32_t st_release(void* s, const int16_t* p) { (void)s; (void)p; return OP1_DRUM_UNBOUND; }
static const op1_drum_engine_t stock = {st_trigger, st_render, st_status, st_release};

typedef struct { uint32_t vptr, pad[17]; } Receiver;  /* 72 bajty jak odbiornik OP-1 */

struct GinCore {
    gin_sdk_runtime_t rt;
    Receiver rec[12];
    int16_t base[24 * 8];          /* slowa kanoniczne klawiszy (OP-1: 8 slow na klawisz, slowo 7 = znacznik zestawu) */
    gin_preset_owned_t kit;
    gin_slot_ticket_t ticket;
    int kitIndex;
    GinSound cur[24];              /* zastosowane dzwieki */
    po32_patch_params_t patch[24];
    float duration[24];
    gin_prepared_t prep[24];       /* przygotowanie przy velocity 127 */
    uint32_t words[24][21];
    uint8_t pubVel[24];            /* velocity aktualnie opublikowanego przygotowania */
    uint8_t fresh[12];             /* odbiornik uderzony od ostatniego bloku */
    uint32_t hits;
    int lastKey;
    /* 88,2 kHz -> 48 kHz */
    float ring[132];               /* 3 probki historii + 128 nowych */
    int have;                      /* probek w ring[3..] */
    double pos;                    /* pozycja odczytu w ring (od indeksu 1) */
};

static void slotInit(struct GinCore* p) {
    static const int32_t adjust[4] = {0, 0, 0, 0};
    gin_sdk_init(&p->rt);
    for (int i = 0; i < 12; i++) {
        p->rec[i].vptr = VPTR;
        gin_sdk_bind(&p->rt, (uint32_t)i, &p->rec[i], 72, VPTR, &stock, adjust);
    }
}

static int sameAsFactory(int kit, int s, const GinSound* g) {
    GinSound f;
    ginFactorySound(kit, s, &f);
    return g->morph == 0 && !memcmp(f.ep, g->ep, sizeof f.ep);
}

/* Przygotowanie jednego dzwieku: nieedytowany = sciezka fabryczna (koniec A, zapisany czas trwania — tak powstaje
 * slownik factory.dictionary.bin), edytowany = publish() z gin_controls.c (MORPH, czas z parametrow). */
static void prepareSound(struct GinCore* p, int s, const GinSound* g) {
    float* out = (float*)&p->patch[s];
    if (sameAsFactory(p->kitIndex, s, g)) {
        memcpy(out, g->ep[0], sizeof(float) * 21);
        p->duration[s] = gin_factory_duration((unsigned)p->kitIndex, (unsigned)s);
    } else {
        const float m = g->morph * .001f;
        for (unsigned i = 0; i < 21; i++) out[i] = g->ep[0][i] * (1 - m) + g->ep[1][i] * m;
        float tone = .01f * po32_lut_powf(1000, p->patch[s].OscDcy) + po32_param_attack_seconds(p->patch[s].OscAtk);
        float noise = .01f * po32_lut_powf(1000, p->patch[s].NEnvDcy) + po32_param_attack_seconds(p->patch[s].NEnvAtk);
        float duration = tone > noise ? tone : noise;
        duration += .05f;
        if (duration > 10) duration = 10;
        p->duration[s] = duration;
    }
    gin_integer_prepare(&p->prep[s], &p->patch[s], 127, p->duration[s]);
    memcpy(p->words[s], &p->patch[s], sizeof p->words[s]);
    gin_sdk_activate(&p->rt);
    gin_sdk_publish_prepared(&p->rt, p->ticket, p->kit.sounds[s].id, p->words[s], &p->prep[s], 127);
    p->pubVel[s] = 127;
    p->cur[s] = *g;
}

static void loadKit(struct GinCore* p, int kit, const GinSound* sounds) {
    if (kit < 0 || kit >= (int)GIN_FACTORY_KIT_COUNT) kit = 0;
    gin_sdk_activate(&p->rt);
    gin_factory_load_index((unsigned)kit, &p->kit);
    memset(p->base, 0, sizeof p->base);
    for (int k = 0; k < 24; k++) p->base[8 * k + 7] = (int16_t)(4096 + kit);
    p->kitIndex = kit;
    gin_sdk_load_kit(&p->rt, 0, p->base, &p->kit, &p->ticket);
    for (int s = 0; s < 24; s++) {
        GinSound f;
        if (!sounds) ginFactorySound(kit, s, &f);
        prepareSound(p, s, sounds ? &sounds[s] : &f);
    }
    for (int i = 0; i < 12; i++) gin_sdk_select_engine(&p->rt, (uint32_t)i, "gin");
    gin_sdk_selection(&p->rt, 0, 0);
    gin_sdk_activate(&p->rt);
}

struct GinCore* ginCreate(void) {
    struct GinCore* p = (struct GinCore*)calloc(1, sizeof(struct GinCore));
    if (!p) return 0;
    p->kitIndex = -1;
    p->lastKey = -1;
    p->pos = 1.0;
    slotInit(p);
    loadKit(p, 0, 0);
    return p;
}

void ginSetKit(struct GinCore* p, int kit, const GinSound* sounds) {
    if (!p) return;
    if (kit != p->kitIndex) { loadKit(p, kit, sounds); return; }
    if (!sounds) return;
    for (int s = 0; s < 24; s++)
        if (memcmp(&p->cur[s], &sounds[s], sizeof(GinSound))) {
            if (sounds[s].morph == p->cur[s].morph && !memcmp(sounds[s].ep, p->cur[s].ep, sizeof sounds[s].ep)) p->cur[s] = sounds[s];
            else prepareSound(p, s, &sounds[s]);  /* tylko strona edycji inna: dzwiek bez zmian */
        }
}

int ginKeyOfNote(int note) { return ((note - 36) % 24 + 24) % 24; }

void ginNoteOn(struct GinCore* p, int note, int vel) {
    if (!p) return;
    const int key = ginKeyOfNote(note);
    if (vel < 1) vel = 1;
    if (vel > 127) vel = 127;
    gin_sdk_activate(&p->rt);
    int s = -1;
    for (unsigned i = 0; i < p->kit.count; i++) if (p->kit.sounds[i].id == p->kit.key_map[key]) { s = (int)i; break; }
    if (s < 0) return;
    if (p->pubVel[s] != vel) {  /* dynamika: przygotowanie libpo32 z prawdziwa velocity (OP-1 zawsze 127) */
        gin_prepared_t pv;
        if (vel == 127) pv = p->prep[s];
        else if (gin_integer_prepare(&pv, &p->patch[s], vel, p->duration[s])) return;
        gin_sdk_publish_prepared(&p->rt, p->ticket, p->kit.sounds[s].id, p->words[s], &pv, (uint32_t)vel);
        p->pubVel[s] = (uint8_t)vel;
    }
    /* odbiornik: wolny (nie gra, nie uderzony w tym bloku), inaczej najstarszy */
    int v = -1;
    for (int i = 0; i < 12 && v < 0; i++) {
        const gin_sdk_binding_t* b = &p->rt.binding[i];
        if (!p->fresh[i] && !b->owned.active && !b->tail_left && !b->resample_left) v = i;
    }
    if (v < 0) {
        uint32_t best = 0;
        for (int i = 0; i < 12; i++)
            if (!p->fresh[i] && (v < 0 || p->rt.binding[i].block_age > best)) { best = p->rt.binding[i].block_age; v = i; }
    }
    if (v < 0) v = 0;
    if (gin_sdk_publish_hit(&p->rt, (uint32_t)v, p->ticket, (uint32_t)key, (uint32_t)vel)) return;
    if (gin_sdk_trigger_bound(&p->rt.binding[v], &p->base[8 * key])) return;
    p->fresh[v] = 1;
    p->hits++;
    p->lastKey = key;
}

void ginAllOff(struct GinCore* p) {
    if (!p) return;
    gin_sdk_activate(&p->rt);
    for (int i = 0; i < 12; i++) gin_sdk_retire_voice(&p->rt.binding[i]);
}

void ginRenderNative(struct GinCore* p, int32_t mix[128]) {
    int16_t out[128];
    memset(mix, 0, sizeof(int32_t) * 128);
    gin_sdk_activate(&p->rt);
    for (int v = 0; v < 12; v++) {
        gin_sdk_binding_t* b = &p->rt.binding[v];
        p->fresh[v] = 0;
        if (!b->owned.active && !b->tail_left && !b->resample_left) continue;
        if (gin_sdk_render_bound(b, out, p->base)) continue;
        for (int i = 0; i < 128; i++) mix[i] += gin_fw_output_sample(out[i], UNITY_GAIN);
    }
}

int ginRender(struct GinCore* p, float* dst, int n) {
    if (!p) return 0;
    const double step = 88200.0 / 48000.0;
    int voices = 0;
    for (int v = 0; v < 12; v++) {
        const gin_sdk_binding_t* b = &p->rt.binding[v];
        if (b->owned.active || b->tail_left || b->resample_left) voices++;
    }
    for (int i = 0; i < n; i++) {
        /* potrzebne ring[k-1 .. k+2] dla k = floor(pos) */
        while ((int)p->pos + 2 >= 3 + p->have) {
            if (p->have >= 128) {  /* przesun: zostaw 3 ostatnie probki jako historie */
                const int keep = 3 + p->have - 3;
                p->ring[0] = p->ring[keep]; p->ring[1] = p->ring[keep + 1]; p->ring[2] = p->ring[keep + 2];
                p->pos -= keep;
                p->have = 0;
            }
            int32_t mix[128];
            ginRenderNative(p, mix);
            for (int k = 0; k < 128; k++) p->ring[3 + k] = mix[k] * (1.f / 32768.f);
            p->have = 128;
        }
        const int k = (int)p->pos;
        const float t = (float)(p->pos - k);
        const float y0 = p->ring[k - 1], y1 = p->ring[k], y2 = p->ring[k + 1], y3 = p->ring[k + 2];
        const float c1 = .5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.f * y2 - .5f * y3, c3 = .5f * (y3 - y0) + 1.5f * (y1 - y2);
        dst[i] += ((c3 * t + c2) * t + c1) * t + y1;
        p->pos += step;
    }
    return voices;
}

uint32_t ginHits(const struct GinCore* p) { return p ? p->hits : 0; }
int ginLastKey(const struct GinCore* p) { return p ? p->lastKey : -1; }

int ginFactoryCount(void) { return (int)GIN_FACTORY_BANK_COUNT; }
const char* ginFactoryName(int kit) { return kit >= 0 && kit < (int)GIN_FACTORY_BANK_COUNT ? gin_factory_bank[kit].name : ""; }
const char* ginSoundName(int kit, int s) {
    return kit >= 0 && kit < (int)GIN_FACTORY_BANK_COUNT && s >= 0 && s < 24 ? gin_factory_bank[kit].sound_name[s] : "";
}
int ginKeySound(int kit, int key) {
    if (kit < 0 || kit >= (int)GIN_FACTORY_BANK_COUNT || key < 0 || key >= 24) return 0;
    const uint32_t id = gin_factory_bank[kit].key_map[key];
    return id >= 1000 && id < 1024 ? (int)(id - 1000) : 0;  /* gin_factory.c: id dzwieku = 1000 + indeks */
}
void ginFactorySound(int kit, int s, GinSound* out) {
    memset(out, 0, sizeof *out);
    if (kit < 0 || kit >= (int)GIN_FACTORY_BANK_COUNT || s < 0 || s >= 24) return;
    for (int side = 0; side < 2; side++) memcpy(out->ep[side], &gin_factory_bank[kit].patch[s][side], sizeof(float) * 21);
}

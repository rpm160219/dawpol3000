/* Ekran VAPORA: oryginalny vapora_screen.c (op1r, AiAAAA) skompilowany bez zmian jako C. Zaslepki z
 * third_party/op1r_vapora/op1_plugin.h: linie (op1_raster_line) i tarcza slonca z SVG (op1_screen_draw) ida do
 * funkcji zwrotnych core/ui/vapora_screen.cpp. Skrzynki glosow (ps_mail) i trzymane nuty kopiowane z instancji
 * sciezki przed kazda klatka. Stopka z napisami (Shift) i "easter egg" oryginalu wylaczone — galki rysuje DAW-POL. */
#define OP1_HOST 1
#include <string.h>
#include "synth/vapora_dsp.h"

#define ps_mail vps_mail
#define ps_held_serial vps_held
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "../../third_party/op1r_vapora/vapora_screen.c"
#pragma GCC diagnostic pop
#include "ui/vapora_screen_c.h"

ps_mailbox vps_mail[PS_VOICES];
volatile uint32_t vps_held[PS_VOICES];
volatile uint8_t vps_eng_page[4];
const int16_t* vps_knobs;
const int16_t* vps_live;

/* wezly SVG: eclipse_curve (tarcza, przesuniecie (140, cy-20)) i cover_curve (czarna zaslona, przesuniecie w grupie) */
typedef struct { int visible; int32_t m[9]; } vps_node;
static vps_node n_group = {1, {1024, 0, 0, 0, 1024, 0, 0, 0, 1024}}, n_cover = {1, {1024, 0, 0, 0, 1024, 0, 0, 0, 1024}};
static uint8_t n_fill[4] = {255, 255, 255, 255};
static void* cb_user;
static VpsLineFn cb_line;
static VpsDiskFn cb_disk;

void* op1_find_node(void* scr, const char* id) {
    (void)scr;
    return !strcmp(id, "eclipse_curve") ? (void*)&n_group : !strcmp(id, "cover_curve") ? (void*)&n_cover : 0;
}
uint8_t* op1_node_fill_rgba(void* scr, const char* id) { (void)scr; return !strcmp(id, "sunfill") ? n_fill : 0; }
void op1_node_set_visible(void* node, int on) { ((vps_node*)node)->visible = on != 0; }
void op1_node_set_transform(void* node, const int32_t* m) { memcpy(((vps_node*)node)->m, m, sizeof(int32_t) * 9); }
void op1_screen_draw(void* scr, int dt) {
    (void)scr; (void)dt;
    if (!n_group.visible) return;
    static const uint8_t black[4] = {0, 0, 0, 255};
    const int32_t gx = n_group.m[6] + 20 * 1024, gy = n_group.m[7] + 20 * 1024;  /* srodek kola (20,20) w grupie */
    cb_disk(cb_user, gx, gy, 20 * 1024, n_fill);
    if (n_cover.visible) cb_disk(cb_user, gx + n_cover.m[6], gy + n_cover.m[7], 20 * 1024, black);
}
void* op1_raster(void) { return cb_user; }
void op1_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c) {
    cb_line(r, a->x, a->y, b->x, b->y, w, c);
}

static int last_track = -1;
void vaporaScreenUpdate(void* user, VpsLineFn line_fn, VpsDiskFn disk_fn, const struct VaporaCore* core,
                        const int16_t knobs[8], const int16_t live[4], int sun, int track, int dt_ms) {
    cb_user = user; cb_line = line_fn; cb_disk = disk_fn;
    if (core) {
        memcpy((void*)vps_mail, vaporaMail(core), sizeof vps_mail);
        const volatile uint32_t* h = vaporaHeld(core);
        for (int i = 0; i < PS_VOICES; i++) vps_held[i] = h[i];
    }
    vps_knobs = knobs;
    vps_live = live;
    if (track != last_track) { view_slot = -1; last_track = track; }  /* inna sciezka: nowy widok (reset_view) */
    sun_design = sun < 0 ? 0 : sun % PS_SUN_DESIGNS;
    z = u = 0;  /* "easter egg" oryginalu wylaczony */
    SCREEN_vapora.update(user, dt_ms);
}

const char* vaporaCharacterName(int c) { return c >= 0 && c < PS_CHARACTERS ? ps_character_names[c] : "?"; }
const char* vaporaIntervalName(int i) { return i >= 0 && i < PS_INTERVALS ? ps_interval_names[i] : "?"; }
const char* vaporaSunName(int s) { return s >= 0 && s < PS_SUN_DESIGNS ? ps_sun_names[s] : "?"; }
int vaporaSpeedMilliHz(int q) { return ps_speed_millihz(ps_secondary((int16_t)q)); }
int vaporaCharacterOf(int16_t w) { int16_t r[8] = {0}; r[5] = w; return ps_decode(r).character; }
int vaporaIntervalOf(int16_t w) { int16_t r[8] = {0}; r[6] = w; return ps_decode(r).interval; }

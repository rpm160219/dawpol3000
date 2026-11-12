/* Ekran GIN: gin_screen.c (SquareWave Studio, bez zmian) w trybie OP1_HOST — butelki A (GIN) i B (TONIC) przelewaja
 * sie do kieliszka przy MORPH, babelki z szumu (BALANCE), puls przy uderzeniu; edytor: wykresy stron parametrow.
 * Stan edycji = gin_ui.c (oryginal) na kopii zestawu sciezki. Napisy: wlasna czcionka DAW-POL (gin_screen.cpp). */
#include <string.h>
#include "synth/gin_dsp.h"
#include "gin_ui.h"
#include "gin_screen.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wformat-truncation"
#include "gin_screen.c"
#pragma GCC diagnostic pop

typedef void (*GinLineFn)(void* user, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* rgba);

static void* cb_user;
static GinLineFn cb_line;
static gin_ui_snapshot_t cur_snap;
static uint32_t cur_now, cur_hits;
int gin_dp_mode;  /* tryb ekranu dla filtra napisow (gin_screen.cpp) */

void* op1_raster(void) { return cb_user; }
void op1_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c) {
    cb_line(r, a->x, a->y, b->x, b->y, w, c);
}
void op1_screen_draw(void* scr, int dt) { (void)scr; (void)dt; }
uint32_t gin_native_snapshot(gin_ui_snapshot_t* out, uint32_t* now) { *out = cur_snap; *now = cur_now; return 0; }
uint32_t gin_native_hit_count(void) { return cur_hits; }

/* ---- stan edycji (gin_ui_t) ---- */
typedef struct GinUi { gin_ui_t u; int kit; } GinUi;
static GinUi s_ui[1];

GinUi* ginUiGet(void) { return s_ui; }

void ginUiLoad(GinUi* g, const GinSound* s, int kit, int selected, int mode, int page, int shift) {
    static gin_ui_sound_t snd[24];
    for (int i = 0; i < 24; i++) {
        snd[i].id = 1000u + (uint32_t)i;
        memset(snd[i].name, 0, sizeof snd[i].name);
        strncpy(snd[i].name, ginSoundName(kit, i), 23);
        if (!snd[i].name[0]) strcpy(snd[i].name, "SOUND");
        memcpy(&snd[i].endpoint[0], s[i].ep[0], sizeof(float) * 21);
        memcpy(&snd[i].endpoint[1], s[i].ep[1], sizeof(float) * 21);
        snd[i].morph = s[i].morph > 1000 ? 1000 : s[i].morph;
        snd[i].side = s[i].side ? GIN_UI_B : GIN_UI_A;
    }
    gin_ui_init(&g->u);
    gin_ui_load(&g->u, snd, 24, 44100);
    strncpy(g->u.kit_name, ginFactoryName(kit), 23);
    g->kit = kit;
    gin_ui_select(&g->u, (uint32_t)(selected < 0 ? 0 : selected > 23 ? 23 : selected));
    g->u.mode = (uint32_t)mode;
    g->u.page = (uint32_t)page;
    g->u.shift = shift;
}
int ginUiSelected(const GinUi* g) { return (int)g->u.selected; }
const char* ginUiName(const GinUi* g) { return g->u.sound[g->u.selected].name; }
void ginUiSelect(GinUi* g, int i) { gin_ui_select(&g->u, (uint32_t)i); }
/* obrot "enkodera" (0..3) o d zabkow; 1 = zmienil sie dzwiek */
int ginUiRotate(GinUi* g, int enc, int d, uint32_t now) {
    const uint32_t r = g->u.revision;
    if (d > 96) d = 96;
    if (d < -96) d = -96;
    gin_ui_rotate(&g->u, (unsigned)enc, d, now);
    return g->u.revision != r && g->u.dirty;
}
void ginUiToggleSide(GinUi* g) { gin_ui_press(&g->u, 0); }
void ginUiSound(const GinUi* g, int i, GinSound* out) {
    const gin_ui_sound_t* s = &g->u.sound[i];
    memcpy(out->ep[0], &s->endpoint[0], sizeof(float) * 21);
    memcpy(out->ep[1], &s->endpoint[1], sizeof(float) * 21);
    out->morph = s->morph;
    out->side = s->side == GIN_UI_B;
    out->pad = 0;
}
/* odczyt galki do paska DAW-POL: nazwa, wartosc, jednostka (z gin_ui_snapshot, jak na ekranie OP-1) */
void ginUiReadout(const GinUi* g, uint32_t now, int enc, char* label, char* value, char* unit, int* active) {
    static gin_ui_snapshot_t v;
    gin_ui_snapshot(&g->u, now, &v);
    strcpy(label, v.readout[enc].label);
    strcpy(value, v.readout[enc].value);
    strcpy(unit, v.readout[enc].unit);
    *active = v.readout[enc].active;
}

void ginScreenDraw(void* user, GinLineFn line_fn, const GinUi* g, uint32_t now, uint32_t hits, int dt) {
    static int inited;
    if (!inited) { SCREEN_gin.init(0); inited = 1; }
    cb_user = user;
    cb_line = line_fn;
    gin_ui_snapshot(&g->u, now, &cur_snap);
    for (int i = 0; i < 4; i++) strcpy(cur_snap.readout[i].label, "-");  /* odczyty galek: na pasku DAW-POL */
    cur_snap.decoration = g->kit == 2 ? GIN_UI_DECOR_LINN : g->kit == 5 ? GIN_UI_DECOR_FISH : GIN_UI_DECOR_NONE;
    cur_now = now;
    cur_hits = hits;
    gin_dp_mode = (int)g->u.mode;
    SCREEN_gin.update(0, dt);
}

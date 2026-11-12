/* DAW-POL 3000: zaslepka op1_plugin.h dla dddd.c (zamiast interfejsu firmware OP-1 / op1r). Kompilowany dwa razy: audio
 * (core/synth/dddd_dsp.c) i ekran (core/ui/dddd_screen_c.c); przedrostek DD_P rozdziela symbole. Glos = "self" = slot
 * 0x24 B w instancji (indeks z adresu); wysokosc, velocity, obwiednia glosnosci (OP1_VOICE_ENV; u nas AMP ENV) i
 * tablica sinusa z wrappera. Ekran: linie przez funkcje zwrotna, napisy dolnego rzedu pominiete. */
#ifndef OP1_PLUGIN_H
#define OP1_PLUGIN_H
#include <stdint.h>
#ifndef DD_P
#define DD_P(n) ddw_##n
#endif
#define OP1_BLOCK 128
#define OP1_VOICES 6
#define OP1_VOICE_RATE 88200
#define OP1_VOICE_SIZE 0x24u
#define OP1_ENG_PAGE_WORD(k) ((k) < 3 ? 5 + (k) : 7)
extern const int32_t* DD_P(pitch_ptr);
extern int DD_P(vel);
extern int16_t DD_P(env)[128];
extern int16_t dd_sine_tab[8192];
extern const void* DD_P(base);
#define OP1_PITCH_INC DD_P(pitch_ptr)
#define OP1_VOICE_ENV DD_P(env)
static inline int32_t op1_sin(int32_t k) { return dd_sine_tab[k & 8191]; }
static inline int op1_voice_index(const void* self) { return (int)(((uintptr_t)self - (uintptr_t)DD_P(base)) / OP1_VOICE_SIZE); }
static inline int32_t op1_voice_vel_gain(const void* self) {
    (void)self;
    int32_t v = DD_P(vel);
    return v > 0 && v <= 127 ? v * 258 : 32767;
}
static inline void op1_dbg(const char* text, int a, int b) { (void)text; (void)a; (void)b; }
typedef struct {
    void (*note_on)(void* self, const int16_t* params);
    void (*render)(void* self, int16_t* out, const int16_t* params);
    void (*note_off)(void* self);
    uint8_t press[4];
} op1_engine_t;
enum { OP1_PRESS_NONE = 0, OP1_PRESS_PAGE = 1, OP1_PRESS_LATCH = 2, OP1_PRESS_BROWSE = 3, OP1_PRESS_SHIFT = 4 };
typedef struct { int32_t x, y; } op1_pt;
typedef struct {
    void (*init)(void* scr);
    void (*update)(void* scr, int dt);
} op1_screen_t;
#define ENGINE_dddd DD_P(engine)
#define SCREEN_dddd DD_P(screen)

/* ---- ekran ---- */
#define OP1_Q10(v) ((int32_t)((v) * 1024))
#define OP1_SCREEN_SLOT 0
#define OP1_TEXT_SIZE 0xA4
#define op1_text_ctor(obj, x, y, size, rgba) ((void)(obj), (void)(x), (void)(y), (void)(size), (void)(rgba))
#define op1_text_set(t, s) ((void)(t), (void)(s))
#define op1_text_draw(t, r, m) ((void)(t), (void)(r), (void)(m))
extern volatile uint8_t DD_P(eng_page)[4];
#define op1_eng_page DD_P(eng_page)
extern const int16_t* DD_P(knobs);
extern const int16_t* DD_P(live);
static inline const int16_t* op1_synth_knobs(int slot) { (void)slot; return DD_P(knobs); }
static inline void op1_synth_knobs_live(int slot, int16_t* k) { (void)slot; for (int i = 0; i < 4; i++) k[i] = DD_P(live)[i]; }
static inline const int16_t* op1_knobs_shown(int fx, int pos, const int16_t* w, int16_t* buf) {
    (void)fx; (void)pos;
    for (int i = 0; i < 8; i++) buf[i] = w[i];
    return buf;
}
#define op1_screen_draw DD_P(screen_draw)
#define op1_raster DD_P(raster)
#define op1_raster_line DD_P(raster_line)
void op1_screen_draw(void* scr, int dt);
void* op1_raster(void);
void op1_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c);
#endif

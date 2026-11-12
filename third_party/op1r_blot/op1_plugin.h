/* DAW-POL 3000: zaslepka op1_plugin.h dla blot.c (zamiast interfejsu firmware OP-1 / op1r).
 * Glos = "self" = slot 0x24 bajtow w instancji sciezki (indeks z adresu); wysokosc, velocity i tablica sinusa z wrappera
 * core/synth/blot_dsp.c. Ekran: linie przez funkcje zwrotna (wlasne nazwy symboli); napisy z dolu ekranu oryginalu
 * pominiete — dublowalyby galki DAW-POL. */
#ifndef OP1_PLUGIN_H
#define OP1_PLUGIN_H
#include <stdint.h>
#define OP1_BLOCK 128
#define OP1_VOICES 6
#define OP1_VOICE_RATE 88200
#define OP1_VOICE_SIZE 0x24u
extern const int32_t* blw_pitch_ptr;
extern int blw_vel;
extern int16_t blw_sine[8192];
extern const void* blw_base;
#define OP1_PITCH_INC blw_pitch_ptr
static inline int32_t op1_sin(int32_t k) { return blw_sine[k & 8191]; }
static inline int op1_voice_index(const void* self) { return (int)(((uintptr_t)self - (uintptr_t)blw_base) / OP1_VOICE_SIZE); }
static inline int32_t op1_voice_vel_gain(const void* self) {
    (void)self;
    int32_t v = blw_vel;
    return v > 0 && v <= 127 ? v * 258 : 32767;
}
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

/* ---- ekran ---- */
#define OP1_Q10(v) ((int32_t)((v) * 1024))
#define OP1_SCREEN_SLOT 0
#define OP1_TEXT_SIZE 0xA4
#define op1_text_ctor(obj, x, y, size, rgba) ((void)(obj), (void)(x), (void)(y), (void)(size), (void)(rgba))
#define op1_text_set(t, s) ((void)(t), (void)(s))
#define op1_text_draw(t, r, m) ((void)(t), (void)(r), (void)(m))
extern const void* blw_view;
#define OP1_VOICE(i) ((const void*)((const char*)blw_view + (i) * OP1_VOICE_SIZE))
extern const int16_t* blw_knobs;
extern const int16_t* blw_live;
static inline const int16_t* op1_synth_knobs(int slot) { (void)slot; return blw_knobs; }
static inline void op1_synth_knobs_live(int slot, int16_t* k) { (void)slot; for (int i = 0; i < 4; i++) k[i] = blw_live[i]; }
static inline const int16_t* op1_knobs_shown(int fx, int pos, const int16_t* w, int16_t* buf) {
    (void)fx; (void)pos;
    for (int i = 0; i < 8; i++) buf[i] = w[i];
    return buf;
}
static inline int op1_shift_held(void) { return 0; }
#define op1_screen_draw blw_screen_draw
#define op1_raster blw_raster
#define op1_raster_line blw_raster_line
void op1_screen_draw(void* scr, int dt);
void* op1_raster(void);
void op1_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c);
#endif

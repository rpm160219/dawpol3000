/* DAW-POL 3000: zaslepka op1_plugin.h dla bloodmoon.c / moon.c (zamiast interfejsu firmware OP-1 / op1r).
 * Glos = "self" = slot 0x24 bajtow w instancji sciezki (jak pamiec glosu OP-1). Wysokosc, velocity i tablica sinusa z
 * wrappera core/synth/bloodmoon_dsp.c. Ekran: wezly SVG przez funkcje zwrotne (wlasne nazwy symboli); napisy oryginalu
 * (nazwy i wartosci galek) pominiete — dublowalyby galki DAW-POL. */
#ifndef OP1_PLUGIN_H
#define OP1_PLUGIN_H
#include <stdint.h>
#define OP1_BLOCK 128
#define OP1_VOICES 6
#define OP1_VOICE_RATE 88200
#define OP1_VOICE_SIZE 0x24u
extern const int32_t* bmw_pitch_ptr;
extern int bmw_vel;
extern int16_t bmw_sine[8192];
#define OP1_PITCH_INC bmw_pitch_ptr
#define OP1_SINE_TABLE ((const int16_t*)bmw_sine)
static inline const int16_t* op1_sin_tab(void) { return bmw_sine; }
static inline int32_t op1_mul16(int32_t a, int32_t b) { return (a * b) >> 15; }
static inline int16_t op1_sat16(int32_t x) { return x > 32767 ? 32767 : (x < -32768 ? -32768 : (int16_t)x); }
static inline int32_t op1_voice_vel_gain(const void* self) {
    (void)self;
    int32_t v = bmw_vel;
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
extern const int16_t* bmw_live;
static inline void op1_synth_knobs_live(int slot, int16_t* k) { (void)slot; for (int i = 0; i < 4; i++) k[i] = bmw_live[i]; }
#define op1_find_node bmw_find_node
#define op1_node_set_transform bmw_node_set_transform
#define op1_node_set_visible bmw_node_set_visible
#define op1_screen_draw bmw_screen_draw
#define op1_raster bmw_raster
void* op1_find_node(void* scr, const char* id);
void op1_node_set_transform(void* node, const int32_t* m);
void op1_node_set_visible(void* node, int on);
static inline void op1_node_rotate(void* node, int32_t cx, int32_t cy, int32_t s15, int32_t c15) {
    int32_t c = c15 >> 5, s = s15 >> 5;
    int32_t m[9] = {c, s, 0, -s, c, 0, cx - ((cx * c - cy * s) >> 10), cy - ((cx * s + cy * c) >> 10), 1024};
    op1_node_set_transform(node, m);
}
void op1_screen_draw(void* scr, int dt);
void* op1_raster(void);
#endif

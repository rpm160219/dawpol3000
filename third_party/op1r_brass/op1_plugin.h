/* DAW-POL 3000: zaslepka op1_plugin.h dla brass.c / horn.c (zamiast interfejsu firmware OP-1 / op1r).
 * Glos = "self" = slot 0x24 bajtow w instancji sciezki (jak pamiec glosu OP-1). Wysokosc (OP1_PITCH_INC), velocity,
 * obwiednia glosnosci glosu (OP1_VOICE_ENV — silnik czyta ja dla barwy, podjazdu i wibrata; u nas AMP ENV) i tablica
 * sinusa z wrappera core/synth/brass_dsp.c. Ekran: wezly SVG i linie przez funkcje zwrotne (wlasne nazwy symboli). */
#ifndef OP1_PLUGIN_H
#define OP1_PLUGIN_H
#include <stdint.h>
#define OP1_BLOCK 128
#define OP1_VOICES 6
#define OP1_VOICE_RATE 88200
#define OP1_VOICE_SIZE 0x24u
extern const int32_t* brw_pitch_ptr;
extern int brw_vel;
extern int16_t brw_env[OP1_BLOCK];
extern int16_t brw_sine[8192];
#define OP1_PITCH_INC brw_pitch_ptr
#define OP1_VOICE_ENV brw_env
#define OP1_SINE_Q brw_sine
static inline int32_t op1_sin(int32_t k) { return brw_sine[k & 8191]; }
static inline int32_t op1_mul16(int32_t a, int32_t b) { return (a * b) >> 15; }
static inline int16_t op1_sat16(int32_t x) { return x > 32767 ? 32767 : (x < -32768 ? -32768 : (int16_t)x); }
static inline int32_t op1_voice_vel_gain(const void* self) {
    (void)self;
    int32_t v = brw_vel;
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
extern const int16_t* brw_live;
static inline void op1_synth_knobs_live(int slot, int16_t* k) { (void)slot; for (int i = 0; i < 4; i++) k[i] = brw_live[i]; }
#define op1_find_node brw_find_node
#define op1_node_set_transform brw_node_set_transform
#define op1_screen_draw brw_screen_draw
#define op1_raster brw_raster
#define op1_raster_line brw_raster_line
void* op1_find_node(void* scr, const char* id);
void op1_node_set_transform(void* node, const int32_t* m);
static inline void op1_node_rotate(void* node, int32_t cx, int32_t cy, int32_t s15, int32_t c15) {
    int32_t c = c15 >> 5, s = s15 >> 5;
    int32_t m[9] = {c, s, 0, -s, c, 0, cx - ((cx * c - cy * s) >> 10), cy - ((cx * s + cy * c) >> 10), 1024};
    op1_node_set_transform(node, m);
}
void op1_screen_draw(void* scr, int dt);
void* op1_raster(void);
void op1_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c);
#endif

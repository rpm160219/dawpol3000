/* DAW-POL 3000: zaslepka op1_plugin.h dla supersaw.c (zamiast interfejsu firmware OP-1 / op1r).
 * Glos = "self" = wskaznik na voice_t z instancji sciezki (jak pamiec glosu w firmware); wysokosc (OP1_PITCH_INC),
 * velocity i tablica sinusa z wrappera core/synth/supersaw_dsp.c. Ekran: wezly SVG, linie i rysunek idą do funkcji
 * zwrotnych core/ui/supersaw_screen.cpp. */
#ifndef OP1_PLUGIN_H
#define OP1_PLUGIN_H
#include <stdint.h>
#define OP1_BLOCK 128
#define OP1_VOICES 6
#define OP1_VOICE_RATE 88200
extern const int32_t* ssw_pitch_ptr;
extern int ssw_vel;
extern int16_t ssw_sine[8192];
#define OP1_PITCH_INC ssw_pitch_ptr
static inline int32_t op1_sin(int32_t k) { return ssw_sine[k & 8191]; }
static inline int32_t op1_mul16(int32_t a, int32_t b) { return (a * b) >> 15; }
static inline int16_t op1_sat16(int32_t x) { return x > 32767 ? 32767 : (x < -32768 ? -32768 : (int16_t)x); }
static inline int32_t op1_voice_vel_gain(const void* self) {
    (void)self;
    int32_t v = ssw_vel;
    return v > 0 && v <= 127 ? v * 258 : 32767;
}
typedef struct {
    void (*note_on)(void* self, const int16_t* params);
    void (*render)(void* self, int16_t* out, const int16_t* params);
    void (*note_off)(void* self);
    uint8_t press[4];
} op1_engine_t;
typedef struct { int32_t x, y; } op1_pt;
typedef struct {
    void (*init)(void* scr);
    void (*update)(void* scr, int dt);
} op1_screen_t;

/* ---- ekran ---- (wlasne nazwy symboli: ekran VAPORA ma swoje op1_*) */
#define op1_find_node ssw_find_node
#define op1_node_set_transform ssw_node_set_transform
#define op1_screen_draw ssw_screen_draw
#define op1_raster ssw_raster
#define op1_raster_line ssw_raster_line
#define OP1_Q10(v) ((int32_t)((v) * 1024))
#define OP1_SCREEN_SLOT 0
extern const void* ssw_view;          /* glosy ogladanej sciezki (voice_t[6]) */
extern int ssw_voice_size;
#define OP1_VOICE(i) ((const void*)((const char*)ssw_view + (i) * ssw_voice_size))
extern const int16_t* ssw_live;       /* galki 0..3 po modulacji */
static inline void op1_synth_knobs_live(int slot, int16_t* k) { (void)slot; for (int i = 0; i < 4; i++) k[i] = ssw_live[i]; }
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

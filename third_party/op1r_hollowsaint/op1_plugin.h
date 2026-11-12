/* DAW-POL 3000: zaslepka op1_plugin.h dla hollow_saint.c (zamiast interfejsu firmware OP-1 / op1r).
 * Glos = "self" = slot 0x24 bajtow w instancji (indeks z adresu); stan oryginalu jest w zmiennych pliku, podmienianych
 * per instancja przez core/synth/hollowsaint_dsp.c. Wysokosc, velocity, obwiednia glosnosci (OP1_VOICE_ENV; u nas
 * AMP ENV), zegar taktu (op1_beat_clock; u nas tempo i pozycja transportu), strona 2 zielonej galki (op1_eng_page[1] =
 * RELIC wlaczony) i tablica sinusa z wrappera. Ekran: wezly SVG i linie przez funkcje zwrotne (wlasne nazwy). */
#ifndef OP1_PLUGIN_H
#define OP1_PLUGIN_H
#include <stdint.h>
#define OP1_BLOCK 128
#define OP1_VOICES 6
#define OP1_VOICE_RATE 88200
#define OP1_VOICE_SIZE 0x24u
extern const int32_t* hsw_pitch_ptr;
extern int hsw_vel;
extern int16_t hsw_env[128];
extern int16_t hsw_sine[8192];
extern const void* hsw_base;
extern volatile uint8_t hsw_eng_page[4];
#define OP1_PITCH_INC hsw_pitch_ptr
#define OP1_VOICE_ENV hsw_env
#define OP1_SINE_TABLE hsw_sine
#define op1_eng_page hsw_eng_page
static inline int op1_voice_index(const void* self) { return (int)(((uintptr_t)self - (uintptr_t)hsw_base) / OP1_VOICE_SIZE); }
static inline int32_t op1_voice_vel_gain(const void* self) {
    (void)self;
    int32_t v = hsw_vel;
    return v > 0 && v <= 127 ? v * 258 : 32767;
}
typedef struct {
    int32_t len, next, step, tape_pos;
    uint8_t follows_tape;
} op1_beat_t;
extern op1_beat_t hsw_beat;
static inline void op1_beat_clock(op1_beat_t* b) { *b = hsw_beat; }
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
extern const int16_t* hsw_knobs;
extern const int16_t* hsw_live;
static inline const int16_t* op1_synth_knobs(int slot) { (void)slot; return hsw_knobs; }
static inline void op1_synth_knobs_live(int slot, int16_t* k) { (void)slot; for (int i = 0; i < 4; i++) k[i] = hsw_live[i]; }
#define op1_find_node hsw_find_node
#define op1_node_set_transform hsw_node_set_transform
#define op1_node_set_visible hsw_node_set_visible
#define op1_screen_draw hsw_screen_draw
void* op1_find_node(void* scr, const char* id);
void op1_node_set_transform(void* node, const int32_t* m);
void op1_node_set_visible(void* node, int on);
static inline void op1_node_rotate(void* node, int32_t cx, int32_t cy, int32_t s15, int32_t c15) {
    int32_t c = c15 >> 5, s = s15 >> 5;
    int32_t m[9] = {c, s, 0, -s, c, 0, cx - ((cx * c - cy * s) >> 10), cy - ((cx * s + cy * c) >> 10), 1024};
    op1_node_set_transform(node, m);
}
void op1_screen_draw(void* scr, int dt);
#endif

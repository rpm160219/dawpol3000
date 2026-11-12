/* DAW-POL 3000: zaslepka op1_plugin.h dla aether.c (zamiast interfejsu firmware OP-1 / op1r).
 * aether.c jest kompilowany z OP1_HOST: czujnik przechylenia = tablica host_accel (wypelnia ja wrapper z wirtualnego
 * przechylenia strony TILT; RG35XX nie ma czujnika ruchu). Glos = "self" = slot 0x24 B w instancji (indeks z adresu);
 * wysokosc, velocity, obwiednia (tylko ekran) i tablica sinusa z wrappera. Ekran: linie i wezly SVG przez funkcje
 * zwrotne, napisy pominiete. Wlasne nazwy symboli ekranu (aew_* / aes_* przez wrapper). */
#ifndef OP1_PLUGIN_H
#define OP1_PLUGIN_H
#include <stdint.h>
#define OP1_BLOCK 128
#define OP1_VOICES 6
#define OP1_VOICE_RATE 88200
#define OP1_VOICE_SIZE 0x24u
extern const int32_t* aew_pitch_ptr;
extern int aew_vel;
extern int16_t aew_env[128];
extern int16_t aew_sine[8192];
extern const void* aew_base;
#define OP1_PITCH_INC aew_pitch_ptr
#define OP1_VOICE_ENV aew_env
static inline int32_t op1_sin(int32_t k) { return aew_sine[k & 8191]; }
static inline int16_t op1_sat16(int32_t x) { return x > 32767 ? 32767 : (x < -32768 ? -32768 : (int16_t)x); }
static inline int op1_voice_index(const void* self) { return (int)(((uintptr_t)self - (uintptr_t)aew_base) / OP1_VOICE_SIZE); }
static inline int32_t op1_voice_vel_gain(const void* self) {
    (void)self;
    int32_t v = aew_vel;
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

/* ---- ekran ---- (AE_SCR: przedrostek symboli ekranu; kopia audio dostaje zaslepki aewx_*) */
#ifndef AE_SCR
#define AE_SCR(n) aes_##n
#endif
#define OP1_Q10(v) ((int32_t)((v) * 1024))
#define OP1_SCREEN_SLOT 0
#define OP1_TEXT_SIZE 0xA4
#define op1_text_ctor(obj, x, y, size, rgba) ((void)(obj), (void)(x), (void)(y), (void)(size), (void)(rgba))
#define op1_text_set(t, s) ((void)(t), (void)(s))
#define op1_text_draw(t, r, m) ((void)(t), (void)(r), (void)(m))
extern volatile uint8_t AE_SCR(eng_page)[4];
#define op1_eng_page AE_SCR(eng_page)
extern const int16_t* AE_SCR(knobs);
extern const int16_t* AE_SCR(live);
static inline const int16_t* op1_synth_knobs(int slot) { (void)slot; return AE_SCR(knobs); }
static inline void op1_synth_knobs_live(int slot, int16_t* k) { (void)slot; for (int i = 0; i < 4; i++) k[i] = AE_SCR(live)[i]; }
#define op1_find_node AE_SCR(find_node)
#define op1_node_set_transform AE_SCR(node_set_transform)
#define op1_node_set_visible AE_SCR(node_set_visible)
#define op1_screen_draw AE_SCR(screen_draw)
#define op1_raster AE_SCR(raster)
#define op1_raster_line AE_SCR(raster_line)
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
void op1_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c);
#endif

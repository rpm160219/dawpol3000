/* DAW-POL 3000: zaslepka op1_plugin.h dla vapora.c / vapora_screen.c (zamiast interfejsu firmware OP-1 / op1r).
 * Glos = "self" (numer 0..5; OP1_VOICE_BASE 0, rozmiar 1, wiec valid_voice() dziala bez zmian); wysokosc
 * (OP1_PITCH_INC) i velocity daje wrapper core/synth/vapora_dsp.c. Ekran: rysowanie linii i tarczy slonca (SVG)
 * przekazywane do core/ui/vapora_screen.cpp przez wskazniki funkcji (core/ui/vapora_screen_c.c). */
#ifndef OP1_PLUGIN_H
#define OP1_PLUGIN_H
#include <stdint.h>
#define OP1_BLOCK 128
#define OP1_VOICES 6
#define OP1_VOICE_RATE 88200
#define OP1_VOICE_BASE 0u
#define OP1_VOICE_SIZE 1u
extern const int32_t* vpw_pitch_ptr;
extern int vpw_vel;
extern const int16_t vpw_env[128];
#define OP1_PITCH_INC vpw_pitch_ptr
#define OP1_VOICE_ENV vpw_env
static inline int op1_voice_index(const void* self) { return (int)(uintptr_t)self; }
static inline int32_t op1_voice_vel_gain(const void* self) {
    (void)self;
    int32_t v = vpw_vel;
    return v > 0 && v <= 127 ? v * 258 : 32767;
}
static inline int op1_current_slot(void) { return 0; }
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

/* ---- ekran (tylko vapora_screen.c) ---- */
extern volatile uint8_t vps_eng_page[4];
#define op1_eng_page vps_eng_page
#define OP1_SCREEN_SLOT 0
extern const int16_t* vps_knobs;     /* 8 slow sciezki (strona 2 = slowa 5..7) */
extern const int16_t* vps_live;      /* galki 0..3 po modulacji */
static inline const int16_t* op1_synth_knobs(int slot) { (void)slot; return vps_knobs; }
static inline void op1_synth_knobs_live(int slot, int16_t* k) { (void)slot; for (int i = 0; i < 4; i++) k[i] = vps_live[i]; }
static inline const int16_t* op1_knobs_shown(int fx, int pos, const int16_t* w, int16_t* buf) {
    (void)fx; (void)pos;
    for (int i = 0; i < 8; i++) buf[i] = w[i];
    return buf;
}
void* op1_find_node(void* scr, const char* id);
uint8_t* op1_node_fill_rgba(void* scr, const char* id);
void op1_node_set_visible(void* node, int on);
void op1_node_set_transform(void* node, const int32_t* m);
void op1_screen_draw(void* scr, int dt);
void* op1_raster(void);
void op1_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c);
/* napisy oryginalu (stopka po Shift i "easter egg") — u nas niewidoczne; galki rysuje DAW-POL */
#define OP1_TEXT_SIZE 0xA4
#define op1_text_ctor(obj, x, y, size, rgba) ((void)(obj), (void)(x), (void)(y), (void)(size), (void)(rgba))
#define op1_text_set(t, s) ((void)(t), (void)(s))
#define op1_text_draw(t, r, m) ((void)(t), (void)(r), (void)(m))
#endif

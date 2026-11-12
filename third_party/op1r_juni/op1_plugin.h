/* DAW-POL 3000: zaslepka op1_plugin.h dla juni.c (zamiast interfejsu firmware OP-1 / op1r). Kompilowany dwa razy: audio
 * (core/synth/juni_dsp.c) i ekran (core/ui/juni_screen_c.c); przedrostek JN_P rozdziela symbole. Glos = "self" = slot
 * 0x24 B w instancji (indeks z adresu); wysokosc, velocity, obwiednia glosnosci (OP1_VOICE_ENV; u nas AMP ENV, prowadzi
 * tez filtr) i tablica sinusa z wrappera. Ekran: rysunek SVG i linie przez funkcje zwrotne. */
#ifndef OP1_PLUGIN_H
#define OP1_PLUGIN_H
#include <stdint.h>
#ifndef JN_P
#define JN_P(n) jnw_##n
#endif
#define OP1_BLOCK 128
#define OP1_VOICES 6
#define OP1_VOICE_RATE 88200
#define OP1_VOICE_SIZE 0x24u
extern const int32_t* JN_P(pitch_ptr);
extern int JN_P(vel);
extern int16_t JN_P(env)[128];
extern int16_t jn_sine_tab[8192];
extern const void* JN_P(base);
#define OP1_PITCH_INC JN_P(pitch_ptr)
#define OP1_VOICE_ENV JN_P(env)
#define OP1_SINE_TABLE jn_sine_tab
static inline int16_t op1_sat16(int32_t x) { return x > 32767 ? 32767 : (x < -32768 ? -32768 : (int16_t)x); }
static inline int op1_voice_index(const void* self) { return (int)(((uintptr_t)self - (uintptr_t)JN_P(base)) / OP1_VOICE_SIZE); }
static inline int32_t op1_voice_vel_gain(const void* self) {
    (void)self;
    int32_t v = JN_P(vel);
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
#define ENGINE_juni JN_P(engine)
#define SCREEN_juni JN_P(screen)

/* ---- ekran ---- */
#define OP1_Q10(v) ((int32_t)((v) * 1024))
#define OP1_SCREEN_SLOT 0
extern const int16_t* JN_P(knobs);
extern const int16_t* JN_P(live);
static inline const int16_t* op1_synth_knobs(int slot) { (void)slot; return JN_P(knobs); }
static inline void op1_synth_knobs_live(int slot, int16_t* k) { (void)slot; for (int i = 0; i < 4; i++) k[i] = JN_P(live)[i]; }
#define op1_screen_draw_cached JN_P(screen_draw_cached)
#define op1_raster JN_P(raster)
#define op1_raster_line JN_P(raster_line)
void op1_screen_draw_cached(void* scr, int dt, int flags);
void* op1_raster(void);
void op1_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c);
#endif

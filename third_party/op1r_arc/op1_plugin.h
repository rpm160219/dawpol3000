/* DAW-POL 3000: zaslepka op1_plugin.h dla arc.c (zamiast interfejsu firmware OP-1 / op1r). Kompilowany dwa razy: audio
 * (core/synth/arc_dsp.c) i ekran (core/ui/arc_screen_c.c); przedrostek AR_P rozdziela symbole obu kopii. Glos = "self" =
 * slot 0x24 B w instancji (indeks z adresu); wysokosc, velocity i tablica sinusa z wrappera. Wspolne dane ekranu
 * oryginalu (portret fazowy, poziomy) przekierowane do instancji sciezki. */
#ifndef OP1_PLUGIN_H
#define OP1_PLUGIN_H
#include <stdint.h>
#ifndef AR_P
#define AR_P(n) arw_##n
#endif
#define OP1_BLOCK 128
#define OP1_VOICES 6
#define OP1_VOICE_RATE 88200
#define OP1_VOICE_SIZE 0x24u
extern const int32_t* AR_P(pitch_ptr);
extern int AR_P(vel);
extern int16_t arc_sine_tab[8192];
extern const void* AR_P(base);
#define OP1_PITCH_INC AR_P(pitch_ptr)
static inline int32_t op1_sin(int32_t k) { return arc_sine_tab[k & 8191]; }
static inline int op1_voice_index(const void* self) { return (int)(((uintptr_t)self - (uintptr_t)AR_P(base)) / OP1_VOICE_SIZE); }
#define OP1_VOICE(i) ((const void*)((const char*)AR_P(base) + (i) * OP1_VOICE_SIZE))
static inline int32_t op1_voice_vel_gain(const void* self) {
    (void)self;
    int32_t v = AR_P(vel);
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

/* dane ekranu oryginalu (globalne w arc.c) -> bufory instancji */
#define arc_trace (*AR_P(trace_ptr))
#define arc_tpos (*AR_P(tpos_ptr))
#define arc_level (*AR_P(level_ptr))
#define arc_tvoice AR_P(tvoice)
#define ENGINE_arc AR_P(engine)
#define SCREEN_arc AR_P(screen)

/* ---- ekran ---- */
#define OP1_Q10(v) ((int32_t)((v) * 1024))
#define OP1_SCREEN_SLOT 0
extern volatile uint8_t AR_P(eng_page)[4];
#define op1_eng_page AR_P(eng_page)
extern const int16_t* AR_P(knobs);
extern const int16_t* AR_P(live);
static inline const int16_t* op1_synth_knobs(int slot) { (void)slot; return AR_P(knobs); }
static inline void op1_synth_knobs_live(int slot, int16_t* k) { (void)slot; for (int i = 0; i < 4; i++) k[i] = AR_P(live)[i]; }
static inline const int16_t* op1_knobs_shown(int fx, int pos, const int16_t* w, int16_t* buf) {
    (void)fx; (void)pos;
    for (int i = 0; i < 8; i++) buf[i] = w[i];
    return buf;
}
#define op1_find_node AR_P(find_node)
#define op1_screen_draw AR_P(screen_draw)
#define op1_raster AR_P(raster)
#define op1_raster_line AR_P(raster_line)
void* op1_find_node(void* scr, const char* id);
void op1_screen_draw(void* scr, int dt);
void* op1_raster(void);
void op1_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c);
#endif

/* DAW-POL 3000: zaslepka op1_plugin.h dla outatime.c / outatime_screen.c (zamiast interfejsu firmware OP-1 / kennel).
 * Glos = "self" = slot 0x24 B w instancji (indeks z adresu); wysokosc, velocity i tablica sinusa z wrappera
 * core/synth/outatime_dsp.c. Ekran: rysunek SVG i linie przez funkcje zwrotne (core/ui/outatime_screen_c.c). */
#ifndef OP1_PLUGIN_H
#define OP1_PLUGIN_H
#include <stdint.h>
#define OP1_BLOCK 128
#define OP1_VOICES 6
#define OP1_VOICE_RATE 88200
#define OP1_VOICE_SIZE 0x24u
#define OP1_L1_ALIGN(n) (((uint32_t)(n) + 31u) & ~31u)
extern const int32_t* otw_pitch_ptr;
extern int otw_vel;
extern const void* otw_base;
extern int16_t ot_sine_tab[8192];
#define OP1_PITCH_INC otw_pitch_ptr
#define OP1_SINE_TABLE ot_sine_tab
static inline int op1_voice_index(const void* self) { return (int)(((uintptr_t)self - (uintptr_t)otw_base) / OP1_VOICE_SIZE); }
static inline int32_t op1_voice_vel_gain(const void* self) {
    (void)self;
    int32_t v = otw_vel;
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
extern volatile uint8_t ots_eng_page[4];
#define op1_eng_page ots_eng_page
extern const int16_t* ots_knobs;
extern const int16_t* ots_live;
static inline const int16_t* op1_synth_knobs(int slot) { (void)slot; return ots_knobs; }
static inline void op1_synth_knobs_live(int slot, int16_t* k) { (void)slot; for (int i = 0; i < 4; i++) k[i] = ots_live[i]; }
#define op1_screen_draw_cached ots_screen_draw_cached
#define op1_raster ots_raster
#define op1_raster_line ots_raster_line
void op1_screen_draw_cached(void* scr, int dt, int flags);
void* op1_raster(void);
void op1_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c);
#endif

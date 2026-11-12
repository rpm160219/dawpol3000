/* DAW-POL 3000: zaslepka op1_plugin.h dla mbira.c (zamiast interfejsu firmware OP-1 / op1r).
 * Glos = "self" (numer 0..5); wysokosc (OP1_PITCH_INC), velocity (OP1_VOICE_VEL) i tablice sinusa (jak na OP-1:
 * lrint(32767 sin(2 pi k / 8192))) daje wrapper core/synth/mbira_dsp.c. Funkcje ekranu sa puste: ekran MBIRA jest
 * przeniesiony osobno (core/ui/mbira_screen.cpp). */
#ifndef OP1_PLUGIN_H
#define OP1_PLUGIN_H
#include <stdint.h>
#define OP1_BLOCK 128
#define OP1_VOICES 6
#define OP1_VOICE_RATE 88200
extern const int32_t* mbw_pitch_ptr;
extern int16_t mbw_vel[OP1_VOICES];
extern int16_t mbw_sine[8192];
#define OP1_PITCH_INC mbw_pitch_ptr
#define OP1_VOICE_VEL mbw_vel
#define OP1_SINE_TABLE ((const int16_t*)mbw_sine)
static inline int op1_voice_index(const void* self) { return (int)(uintptr_t)self; }
static inline int32_t op1_voice_vel_gain(const void* self) {
    int32_t v = mbw_vel[op1_voice_index(self)] >> 8;
    return v > 0 && v <= 127 ? v * 258 : 32767;
}
static inline int16_t op1_sat16(int32_t x) { return x > 32767 ? 32767 : (x < -32768 ? -32768 : (int16_t)x); }
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
/* ekran oryginalu: nieuzywany (zaslepki, zeby mbira.c kompilowal sie bez zmian) */
extern uint8_t mbw_eng_page[4];
#define op1_eng_page mbw_eng_page
#define OP1_SCREEN_SLOT 0
static inline void* op1_find_node(void* scr, const char* id) { (void)scr; (void)id; return 0; }
static inline void op1_node_set_visible(void* n, int on) { (void)n; (void)on; }
static inline void op1_screen_draw_cached(void* scr, int dt, void* first) { (void)scr; (void)dt; (void)first; }
static inline void* op1_raster(void) { return 0; }
static inline void op1_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c) {
    (void)r; (void)a; (void)b; (void)w; (void)c;
}
static inline const int16_t* op1_synth_knobs(int slot) { static const int16_t z[8]; (void)slot; return z; }
static inline void op1_synth_knobs_live(int slot, int16_t* k) { (void)slot; for (int i = 0; i < 4; i++) k[i] = 0; }
#endif

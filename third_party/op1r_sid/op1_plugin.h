/* DAW-POL 3000: zaslepka op1_plugin.h dla sid.c (zamiast interfejsu firmware OP-1 / op1r).
 * Glos = "self" (numer 0..5); wysokosc (OP1_PITCH_INC), velocity i tablica sinusa (jak na OP-1) daje wrapper
 * core/synth/sid_dsp_wrap.c. Funkcje ekranu sa puste: ekran SID przeniesiony do core/ui/sid_screen.cpp. */
#ifndef OP1_PLUGIN_H
#define OP1_PLUGIN_H
#include <stdint.h>
#define OP1_BLOCK 128
#define OP1_VOICES 6
#define OP1_VOICE_RATE 88200
#define OP1_TRIM_ONE 8192
extern const int32_t* sidw_pitch_ptr;
extern int sidw_vel;
extern int16_t sidw_sine[8192];
#define OP1_PITCH_INC sidw_pitch_ptr
#define OP1_SINE_TABLE ((const int16_t*)sidw_sine)
static inline int op1_voice_index(const void* self) { return (int)(uintptr_t)self; }
static inline int32_t op1_voice_vel_gain(const void* self) {
    (void)self;
    int32_t v = sidw_vel;
    return v > 0 && v <= 127 ? v * 258 : 32767;
}
static inline int16_t op1_sat16(int32_t x) { return x > 32767 ? 32767 : (x < -32768 ? -32768 : (int16_t)x); }
static inline int32_t op1_preset_trim(const int16_t* params) { return params[7] > 0 ? params[7] : OP1_TRIM_ONE; }
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
/* ekran oryginalu: nieuzywany (zaslepki, zeby reactor.c kompilowal sie bez zmian; OP1_HOST = sciezka hosta) */
#define OP1_SCREEN_SLOT 0
static inline void* op1_find_node(void* scr, const char* id) { (void)scr; (void)id; return 0; }
static inline void op1_node_set_transform(void* n, const int32_t* m) { (void)n; (void)m; }
static inline void op1_node_set_visible(void* n, int on) { (void)n; (void)on; }
static inline void op1_screen_draw(void* scr, int dt) { (void)scr; (void)dt; }
static inline void* op1_raster(void) { return 0; }
static inline void op1_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c) {
    (void)r; (void)a; (void)b; (void)w; (void)c;
}
static inline void op1_synth_knobs_live(int slot, int16_t* k) { (void)slot; for (int i = 0; i < 4; i++) k[i] = 0; }
#endif

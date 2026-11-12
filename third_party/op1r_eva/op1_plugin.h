/* DAW-POL 3000: zaslepka op1_plugin.h dla eva.c (zamiast interfejsu firmware OP-1 / op1r).
 * Glos = "self" (numer 0..5); wysokosc (OP1_PITCH_INC) i velocity ustawia wrapper (core/synth/eva_dsp.c) przed
 * kazdym wywolaniem. Funkcje ekranu sa puste: ekran EVA jest przeniesiony osobno (core/ui/eva_screen.cpp). */
#ifndef OP1_PLUGIN_H
#define OP1_PLUGIN_H
#include <stdint.h>
#define OP1_BLOCK 128
#define OP1_VOICES 6
#define OP1_VOICE_RATE 88200
extern const int32_t* eva_pitch_ptr;
extern int eva_vel_now;
extern const int16_t eva_sine_table[8192];
#define OP1_PITCH_INC eva_pitch_ptr
#define OP1_SINE_TABLE eva_sine_table
static inline int op1_voice_index(const void* self) { return (int)(uintptr_t)self; }
static inline int32_t op1_voice_vel_gain(const void* self) {
    (void)self;
    int32_t v = eva_vel_now;
    return v > 0 && v <= 127 ? v * 258 : 32767;
}
typedef struct {
    void (*note_on)(void* self, const int16_t* params);
    void (*render)(void* self, int16_t* out, const int16_t* params);
    void (*note_off)(void* self);
    uint8_t press[4];
} op1_engine_t;
enum { OP1_PRESS_NONE = 0, OP1_PRESS_PAGE = 1, OP1_PRESS_LATCH = 2, OP1_PRESS_BROWSE = 3, OP1_PRESS_SHIFT = 4 };
#define OP1_PRESS_PAGE_STEPS(n) (0x40 + (n))
typedef struct {
    void (*init)(void* scr);
    void (*update)(void* scr, int dt);
} op1_screen_t;
#define OP1_Q10(v) ((int32_t)((v) * 1024))
/* ekran oryginalu: nieuzywany (zaslepki, zeby eva.c kompilowal sie bez zmian) */
extern uint8_t op1_eng_page[4];
#define OP1_SCREEN_SLOT 0
static inline void* op1_find_node(void* scr, const char* id) { (void)scr; (void)id; return 0; }
static inline void op1_node_set_transform(void* n, const int32_t* m) { (void)n; (void)m; }
static inline void op1_node_set_visible(void* n, int on) { (void)n; (void)on; }
static inline void op1_screen_draw(void* scr, int dt) { (void)scr; (void)dt; }
static inline const int16_t* op1_synth_knobs(int slot) { static const int16_t z[8]; (void)slot; return z; }
static inline void op1_synth_knobs_live(int slot, int16_t* k) { (void)slot; for (int i = 0; i < 4; i++) k[i] = 0; }
#endif

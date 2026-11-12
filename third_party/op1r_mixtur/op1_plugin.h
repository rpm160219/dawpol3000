/* DAW-POL 3000: zaslepka op1_plugin.h dla mixtur.c (zamiast interfejsu firmware OP-1 / op1r).
 * Pamiec glosu = 6 slotow po 0x24 bajty w instancji sciezki (jak w firmware: "self" wskazuje slot, indeks z adresu);
 * wysokosc (OP1_PITCH_INC) i velocity z wrappera core/synth/mixtur_dsp.c. Ekran: linie przez funkcje zwrotne. */
#ifndef OP1_PLUGIN_H
#define OP1_PLUGIN_H
#include <stdint.h>
#define OP1_BLOCK 128
#define OP1_VOICES 6
#define OP1_VOICE_RATE 88200
#define OP1_VOICE_SIZE 0x24u
extern const int32_t* mxw_pitch_ptr;
extern int mxw_vel;
extern const void* mxw_base;          /* pamiec glosow aktualnej instancji */
#define OP1_PITCH_INC mxw_pitch_ptr
#define OP1_Q10(v) ((int32_t)((v) * 1024))
static inline int op1_voice_index(const void* self) {
    return (int)(((uintptr_t)self - (uintptr_t)mxw_base) / OP1_VOICE_SIZE);
}
extern const void* mxw_view;          /* ekran: pamiec glosow ogladanej sciezki */
#define OP1_VOICE(i) ((const void*)((const char*)mxw_view + (i) * OP1_VOICE_SIZE))
static inline int32_t op1_voice_vel_gain(const void* self) {
    (void)self;
    int32_t v = mxw_vel;
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

/* ---- ekran (wlasne nazwy symboli) ---- */
#define OP1_SCREEN_SLOT 0
extern uint8_t mxw_eng_page[4];
#define op1_eng_page mxw_eng_page
extern const int16_t* mxw_knobs;      /* 8 slow sciezki */
extern const int16_t* mxw_live;       /* galki 0..3 po modulacji */
static inline const int16_t* op1_synth_knobs(int slot) { (void)slot; return mxw_knobs; }
static inline void op1_synth_knobs_live(int slot, int16_t* k) { (void)slot; for (int i = 0; i < 4; i++) k[i] = mxw_live[i]; }
static inline const int16_t* op1_knobs_shown(int fx, int pos, const int16_t* w, int16_t* buf) {
    (void)fx; (void)pos;
    for (int i = 0; i < 8; i++) buf[i] = w[i];
    return buf;
}
#define op1_screen_draw mxw_screen_draw
#define op1_raster mxw_raster
void op1_screen_draw(void* scr, int dt);
void* op1_raster(void);
#endif

/* Zaslepka op1_plugin.h dla ekranu GIN w DAW-POL: tylko typy i funkcje rastra, ktore gin_screen.c uzywa w trybie
 * OP1_HOST (bez wezlow SVG ekranu OP-1). Implementacja w core/ui/gin_screen_c.c. */
#ifndef DP_GIN_OP1_PLUGIN_H
#define DP_GIN_OP1_PLUGIN_H
#include <stdint.h>
/* wlasne nazwy (inne ekrany maja swoje op1_raster*) */
#define op1_raster gns_raster
#define op1_raster_line gns_raster_line
#define op1_screen_draw gns_screen_draw
typedef struct { int32_t x, y; } op1_pt;
typedef struct { void (*init)(void* scr); void (*update)(void* scr, int dt); } op1_screen_t;
void* op1_raster(void);
void op1_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* rgba);
void op1_screen_draw(void* scr, int dt);
/* z kennel synth_sdk/op1_plugin.h (Hundekraft, GPL-3.0-or-later) */
static inline int32_t op1_f01_to_q15(uint32_t f)          /* IEEE float in [0,1] -> 0..32767, no libgcc */
{
    if ((int32_t)f <= 0) return 0;                        /* <= +0, or negative */
    int e = (int)((f >> 23) & 0xff) - 127;
    if (e >= 0) return 32767;
    uint32_t m = (f & 0x7fffff) | 0x800000;               /* 1.m * 2^e */
    int sh = 23 - 15 - e;                                 /* value*32768 = m >> sh */
    if (sh >= 32) return 0;
    uint32_t v = m >> sh;
    return v > 32767 ? 32767 : (int32_t)v;
}
#endif

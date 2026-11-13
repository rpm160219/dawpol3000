/* Ekran JUNI CHORUS: juni_chorus_screen.c (op1r, Hundekraft) bez zmian; telemetria z kopii egzemplarza w mikserze. */
#define juni_chorus_blocks jcs_blocks
#define juni_chorus_ph jcs_ph
#define juni_chorus_inc jcs_inc
#define juni_chorus_depth jcs_depth
#define juni_chorus_level jcs_level
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "juni_chorus_screen.c"
#pragma GCC diagnostic pop
#include "ui/fx_screen_c.h"
volatile uint32_t jcs_blocks[3], jcs_ph[3], jcs_inc[3];
volatile int32_t jcs_depth[3];
volatile int16_t jcs_level[3];
void fxScreenJuniChorus(const void* tel, int reset, int dt) {
    const uint32_t* t = (const uint32_t*)tel;
    jcs_blocks[0] = t[0]; jcs_ph[0] = t[1]; jcs_inc[0] = t[2]; jcs_depth[0] = (int32_t)t[3]; jcs_level[0] = (int16_t)t[4];
    if (reset) SCREEN_juni_chorus.init(0);
    SCREEN_juni_chorus.update(0, dt);
}

/* Ekran SISTER: sister_screen.c (op1r, Hundekraft) bez zmian; telemetria (si_pos_t) z kopii egzemplarza w mikserze. */
#define sister_pos sis_pos
#include <string.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "sister_screen.c"
#pragma GCC diagnostic pop
#include "ui/fx_screen_c.h"
si_pos_t sis_pos[3];
void fxScreenSister(const void* tel, int reset, int dt) {
    memcpy(&sis_pos[0], tel, sizeof(si_pos_t));
    if (reset) SCREEN_sister.init(0);
    SCREEN_sister.update(0, dt);
}

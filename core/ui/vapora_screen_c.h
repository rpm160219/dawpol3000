/* Interfejs C ekranu VAPORA (core/ui/vapora_screen_c.c). Wspolrzedne Q10 w ukladzie ekranu OP-1 320x160. */
#ifndef VAPORA_SCREEN_C_H
#define VAPORA_SCREEN_C_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct VaporaCore;
typedef void (*VpsLineFn)(void* user, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* rgba);
typedef void (*VpsDiskFn)(void* user, int32_t cx, int32_t cy, int32_t r, const uint8_t* rgba);
/* knobs = 8 slow sciezki (strona 2 w 5..7), live = galki 0..3 po modulacji, sun = wzor slonca 0..6 */
void vaporaScreenUpdate(void* user, VpsLineFn line_fn, VpsDiskFn disk_fn, const struct VaporaCore* core,
                        const int16_t knobs[8], const int16_t live[4], int sun, int track, int dt_ms);
const char* vaporaCharacterName(int c);  /* male litery jak w oryginale */
const char* vaporaIntervalName(int i);
const char* vaporaSunName(int s);
int vaporaSpeedMilliHz(int q);
int vaporaCharacterOf(int16_t w);
int vaporaIntervalOf(int16_t w);
#ifdef __cplusplus
}
#endif
#endif

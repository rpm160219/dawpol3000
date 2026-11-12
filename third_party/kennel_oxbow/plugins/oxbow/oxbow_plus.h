/* OXBOW+ (DAW-POL 3000): parametry rozszerzonej wersji oxbow.c (oxbow_plus.c). Wartosci domyslne = neutralny glos
 * oryginalu (OSC 1 i 2 = SAW, poziomy 110 / 100, puls 50 %, bez szumu, keytrack 64, velocity 40, vintage 30,
 * obwiednia filtra A 20 / D 70 / S 40 / R 60). */
#ifndef OXBOW_PLUS_H
#define OXBOW_PLUS_H
#include <stdint.h>
typedef struct {
    uint32_t w1, w2;           /* fala: W_OFF, W_SAW, W_PLS, W_BOTH (obv_params.h) */
    int32_t semi1, semi2;      /* interwaly w poltonach */
    int32_t cent8_1, cent8_2;  /* strojenie OSC 1 i dodatkowe OSC 2 (modulacja), Q8 poltonu */
    int32_t lvl1, lvl2, lvln;  /* poziomy 0..126 (oryginal: (lvl * lvl) >> 7) */
    uint32_t pw1, pw2;         /* szerokosc pulsu jako faza (2^32 = caly okres) */
    uint32_t sync, xmod;       /* hard sync OSC 2, glebokosc X-MOD 0..127 */
    int32_t envo2;             /* obwiednia filtra -> wysokosc OSC 2, -64..63 */
    int32_t keytrk, velflt, vint;
    uint32_t fa, fd, fr;       /* indeksy ENV_K_ATT / ENV_K_DEC 0..127 */
    int32_t sus;               /* podtrzymanie obwiedni filtra, Q24 */
} obx_params;
#define OBX_DEFAULTS { 1, 1, 0, 0, 0, 0, (110 * 110) >> 7, (100 * 100) >> 7, 0, (64u << 25) + (1u << 24), \
                       (64u << 25) + (1u << 24), 0, 0, 0, 64, 40, 30, 20, 70, 60, 40 * ((1 << 24) / 127) }
extern obx_params obx;
#endif

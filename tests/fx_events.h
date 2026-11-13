/* Scenariusz testu efektow: szum z obwiednia (impulsy i cisza = ogony), galki zmieniane co 400 blokow. */
#pragma once
#define FXE_BLOCKS 3000
static void fxe_block(int b, int seed, int16_t* l, int16_t* r, int16_t* w) {
    static uint32_t rs[2] = {1u, 777u};
    for (int i = 0; i < 64; i++) {
        rs[seed] = rs[seed] * 1664525u + 1013904223u;
        const int on = (b % 300) < 120;
        const int32_t x = on ? (int32_t)(rs[seed] >> 16) - 32768 : 0;
        l[i] = (int16_t)(x / 2);
        r[i] = (int16_t)((x * (seed ? -1 : 1)) / 3);
    }
    const int ph = b / 400;
    w[0] = (int16_t)((ph * 9001 + 4000) & 32767); w[1] = (int16_t)((ph * 5003 + 20000) & 32767);
    w[2] = (int16_t)((ph * 7001 + 1000) & 32767); w[3] = (int16_t)(16000 + ph * 2000);
    w[4] = 0; w[5] = 0; w[6] = (int16_t)(ph & 1 ? 0 : 1200); w[7] = 0;
}

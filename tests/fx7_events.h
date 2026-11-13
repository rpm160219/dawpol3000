/* Scenariusz testu efektow (TAPE, STUTTER, SNOWGLOBE, FATHER, RACKVERB, CRUSH, ENTE): fx_events.h + slowa strony 2
 * zmieniane co 400 blokow, zegar tempa (zmiana tempa co 1000 blokow) i sygnal sidechain (impulsy co 50 blokow). */
#pragma once
#include "fx_events.h"
static void fx7_block(int b, int seed, int tapeSync, int16_t* l, int16_t* r, int16_t* w) {
    fxe_block(b, seed, l, r, w);
    const int ph = b / 400;
    w[5] = (int16_t)((ph * 3001 + 2000) & 32767);
    w[6] = (int16_t)((ph * 12345 + 300) & 32767);
    w[7] = tapeSync ? (int16_t)(ph % 10) : (int16_t)((ph * 7777 + 100) & 32767);
}
static void fx7_beat(int b, int32_t* len, int32_t* next) {
    static const int32_t L[3] = {22050 << 14, 18375 << 14, 27562 << 14};  /* 120, 144, 96 BPM */
    const int32_t ln = L[(b / 1000) % 3];
    const int64_t t = (int64_t)b * (64 << 14);
    *len = ln;
    *next = (int32_t)(ln - t % ln);
}
static void fx7_sc(int b, int16_t* sc) {
    for (int i = 0; i < 64; i++) sc[i] = (int16_t)((b % 50) < 6 ? ((i * 977 + b * 31) & 0x7fff) - 16384 : 0);
}

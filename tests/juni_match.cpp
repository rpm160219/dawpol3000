// Test zgodnosci portu JUNI z oryginalem op1r: ta sama fraza co harness/audio_harness.c (C3 E3 G3 C4 G3 E3,
// 276 blokow na nute, velocity 127 / 64, note_off w bloku 220, zastepcza obwiednia harnessa: atak 20 blokow, 24575,
// wybrzmienie od note_off), glos 88,2 kHz, decymacja 2:1 tym samym filtrem co harness. Silnik czyta obwiednie, wiec
// test wola wrapper bezposrednio (w DAW-POL obwiednie daje AMP ENV silnika glosow).
// Uzycie: juni_match ref.raw
#include "synth/juni_dsp.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char** argv) {
    if (argc < 2) return 2;
    FILE* f = fopen(argv[1], "rb");
    if (!f) return 2;
    static int16_t ref[6 * 276 * 64];
    size_t nref = fread(ref, 2, sizeof ref / 2, f);
    fclose(f);
    JuniCore* p = juniCreate();
    const int16_t k[8] = {18000, 16384, 11000, 9800, 0x5301, 0, 0, 0};
    double h[63], z[63] = {0};
    for (int n = 0; n < 63; n++) {
        double x = n - 31.0, w = 0.42 - 0.5 * cos(2 * M_PI * n / 62) + 0.08 * cos(4 * M_PI * n / 62);
        h[n] = (x == 0 ? 0.5 : sin(M_PI * x / 2) / (M_PI * x)) * w;
    }
    static const int notes[] = {48, 52, 55, 60, 55, 52};
    long n = 0, diff = 0, maxd = 0;
    uint32_t pitch[128];
    int16_t out[128];
    for (int i = 0; i < 6; i++) {
        const uint32_t inc = (uint32_t)(int32_t)lrint(4294967296.0 * 440.0 * pow(2, (notes[i] - 69) / 12.0) / 88200.0);
        for (int j = 0; j < 128; j++) pitch[j] = inc;
        juniNoteOn(p, 0, k);
        for (int b = 0; b < 276; b++) {

            const int e = b < 20 ? 32767 * b / 20 : b < 220 ? 24575 : 24575 - 24575 * (b - 220) / 56;
            juniRender(p, 0, out, k, pitch, i & 1 ? 64 : 127, e);
            for (int j = 0; j < 128; j++) {
                for (int t = 62; t > 0; t--) z[t] = z[t - 1];
                z[0] = out[j];
                if (j & 1) {
                    double a = 0;
                    for (int t = 0; t < 63; t++) a += h[t] * z[t];
                    long v = lrint(a);
                    v = v > 32767 ? 32767 : (v < -32768 ? -32768 : v);
                    long e2 = labs(v - ref[n]);
                    if (e2) diff++;
                    if (e2 > maxd) maxd = e2;
                    n++;
                }
            }
        }
    }
    printf("probek: %ld (ref %zu), roznych: %ld, max roznica: %ld\n", n, nref, diff, maxd);
    return diff ? 1 : 0;
}

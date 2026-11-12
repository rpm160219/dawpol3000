// Test zgodnosci portu REACTOR z oryginalem op1r: ta sama fraza co harness/audio_harness.c
// (C3 E3 G3 C4 G3 E3, 276 blokow na nute, velocity 127 / 64 na zmiane, galki domyslne), glos 88,2 kHz, decymacja 2:1.
// Uzycie: reactor_match ref.raw  (ref.raw = wyjscie oryginalnego harnessa, int16 mono 44,1 kHz)
#include "synth/wavetable_synth.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
using namespace dp;
int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: reactor_match ref.raw\n"); return 2; }
    FILE* f = fopen(argv[1], "rb");
    if (!f) return 2;
    static int16_t ref[6 * 276 * 64];
    size_t nref = fread(ref, 2, sizeof ref / 2, f);
    fclose(f);
    static WavetableSynth s;
    s.init(44100.f);
    s.setKind(6);  // REACTOR
    s.setEnvelope(0, 0, 1, 0);  // bez obwiedni (harness renderuje surowy silnik)
    s.setVolume(1.f);
    const int16_t k[4] = {16384, 3000, 16384, 9000};  // domyslne galki reactor (plugin.json)
    s.setKnobs(k);
    static const int notes[] = {48, 52, 55, 60, 55, 52};
    long n = 0, diff = 0, maxd = 0;
    float buf[2 * 64];
    for (int i = 0; i < 6; i++) {
        s.allOff();
        s.noteOn(notes[i], i & 1 ? 64.f / 127.f : 1.f);  // velocity jak w harnessie: 127 / 64
        for (int b = 0; b < 276; b++) {
            for (int j = 0; j < 128; j++) buf[j] = 0;
            s.render(buf, 64);
            const double* d = s.lastDecimated();
            for (int j = 0; j < 64; j++, n++) {
                long v = lrint(d[j]);
                if (v > 32767) v = 32767;
                if (v < -32768) v = -32768;
                long e = labs(v - ref[n]);
                if (e) diff++;
                if (e > maxd) maxd = e;
            }
        }
    }
    printf("probek: %ld (ref %zu), roznych: %ld, max roznica: %ld\n", n, nref, diff, maxd);
    return diff ? 1 : 0;
}

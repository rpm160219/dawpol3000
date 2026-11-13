// VELOUR: render 8 programow oryginalu (Solina, 48 kHz) z ta sama sekwencja nut — do porownania oryginal (repo
// PicoVintageSynthCollection) z kopia DAW-POL (third_party/pico_solina, z poziomami rejestrow = 1.0).
#include <stdio.h>
#include "solina/solina.h"
int main(int argc, char** argv) {
    FILE* f = fopen(argv[1], "wb");
    static Solina s;
    s.setSampleRate(48000.f);
    float l[256], r[256];
    for (int prog = 0; prog < SOLINA_NPROGRAMS; prog++) {
        s.setProgram(prog);
        s.resetVoices();
        const int chord[5] = {36, 48, 55, 60, 64};
        for (int b = 0; b < 600; b++) {  // ~3,2 s na program: akord, legato, puszczenie, ogon
            if (b == 0) for (int k = 0; k < 5; k++) s.noteOn(chord[k], 100);
            if (b == 150) { s.noteOff(64); s.noteOn(67, 100); s.noteOn(72, 100); }
            if (b == 300) for (int k = 0; k < 128; k++) s.noteOff(k);
            if (b == 420) s.noteOn(84, 100);
            if (b == 480) s.noteOff(84);
            s.processFloat(l, r, 256);
            fwrite(l, 4, 256, f); fwrite(r, 4, 256, f);
        }
    }
    fclose(f);
    return 0;
}

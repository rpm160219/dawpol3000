// LINEA: dekoder ROM (C++) i parser SysEx kontra skrypty oryginalu (d5_make_blob.py, d5_syx_to_patches.py).
// Argumenty: katalog ROM, d5_pcm.bin (z Pythona), plik .syx, plik bajtow patchy (z Pythona: N x 448).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "synth/linea_dsp.h"
#include "io/linea_banks.h"
int main(int argc, char** argv) {
    if (!lineaRomLoad(argv[1])) { printf("ROM: %s\n", lineaRomStatus()); return 1; }
    FILE* f = fopen(argv[2], "rb");
    static int16_t ref[262144];
    if (!f || fread(ref, 2, 262144, f) != 262144) { printf("brak d5_pcm.bin\n"); return 1; }
    fclose(f);
    long d = 0;
    for (int i = 0; i < 262144; i++) if (ref[i] != lineaPcm()[i]) d++;
    printf("LINEA PCM: probek 262144, roznych: %ld (%s)\n", d, lineaRomStatus());
    f = fopen(argv[3], "rb");
    static uint8_t raw[1 << 20];
    const int n = (int)fread(raw, 1, sizeof raw, f);
    fclose(f);
    static uint8_t pat[64][LINEA_PATCH_BYTES];
    int cnt = 0;
    char err[64];
    if (!dp::LineaBanks::parseSyx(raw, n, pat, &cnt, err, sizeof err)) { printf("SYX: %s\n", err); return 1; }
    f = fopen(argv[4], "rb");
    static uint8_t rp[64][LINEA_PATCH_BYTES];
    const int rn = (int)(fread(rp, LINEA_PATCH_BYTES, 64, f));
    fclose(f);
    long dp_ = 0;
    for (int p = 0; p < cnt && p < rn; p++) dp_ += memcmp(pat[p], rp[p], LINEA_PATCH_BYTES) != 0;
    char nm[19];
    lineaPatchName(pat[0], nm);
    printf("LINEA SYX: patchy %d (ref %d), roznych: %ld, pierwszy: %s\n", cnt, rn, dp_ + (cnt != rn), nm);
    // zapis i ponowny odczyt banku (komunikaty DT1 jak D-50)
    dp::LineaBanks::writeSyx("/tmp/claude-1000/linea_rt.syx", pat, cnt);
    f = fopen("/tmp/claude-1000/linea_rt.syx", "rb");
    const int n2 = (int)fread(raw, 1, sizeof raw, f);
    fclose(f);
    static uint8_t pat2[64][LINEA_PATCH_BYTES];
    int cnt2 = 0;
    const bool ok = dp::LineaBanks::parseSyx(raw, n2, pat2, &cnt2, err, sizeof err) && cnt2 == cnt && !memcmp(pat, pat2, sizeof(uint8_t) * LINEA_PATCH_BYTES * cnt);
    printf("LINEA SYX zapis/odczyt: %s\n", ok ? "zgodny" : "BLAD");
    return d || dp_ || !ok;
}

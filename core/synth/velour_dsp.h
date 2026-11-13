// VELOUR: string ensemble (port 1:1 PicoFaceSM, Michi71 — emulacja ARP Solina na podstawie schematow; DSP ensemble,
// filtrow i waveshapera ze string-machine J.-P. Cimalando, Boost 1.0). Organy z dzielnikami: jeden oscylator, wszystkie
// klawisze naraz (bez limitu glosow), 6 rejestrow (kontrabas 16', wiolonczela 8', altowka 8', skrzypce 4', trabka 8',
// rog 4'), obwiednia CRESCENDO / SUSTAIN, ensemble (3 linie BBD: tremolo + chorus), phaser. Dodatek DAW-POL: poziom
// kazdego rejestru 0..100 % (100 % = oryginal bit w bit). Dziala natywnie na 48 kHz (third_party/pico_solina).
#pragma once
#include <stdint.h>

#define VELOUR_PARAMS 26
// kolejnosc parametrow = enum SolinaParam oryginalu
enum {
    VL_CONTRABASS, VL_CELLO, VL_VIOLA, VL_VIOLIN, VL_TRUMPET, VL_HORN, VL_BASS_VOL, VL_CRESCENDO, VL_SUSTAIN, VL_VOLUME,
    VL_TUNE, VL_ENSEMBLE, VL_TREM_RATE, VL_TREM_DEPTH, VL_CHORUS_RATE, VL_CHORUS_DEPTH, VL_ENS_TONE, VL_ENS_WIDTH,
    VL_PHASER, VL_PHASER_RATE, VL_PHASER_COLOR, VL_TONE_LP, VL_TONE_HP, VL_TONE_SHELF, VL_FORMANT, VL_SHAPER
};
#define VELOUR_PROGRAMS 8

struct VelourCore;
VelourCore* velourCreate(float sampleRate);
void velourDestroy(VelourCore* v);
void velourSetParams(VelourCore* v, const int16_t p[VELOUR_PARAMS]);  // slowa 0..32767 (zmienione tylko przeliczane)
void velourNoteOn(VelourCore* v, int pitch);
void velourNoteOff(VelourCore* v, int pitch);
void velourAllOff(VelourCore* v);
void velourRender(VelourCore* v, float* l, float* r, int n);  // nadpisuje l, r
// ekran
int velourActiveKeys(const VelourCore* v);
float velourEnv(const VelourCore* v);      // najwieksza obwiednia klawisza (0..1)
float velourBassEnv(const VelourCore* v);
float velourWordToFloat(int16_t w);
// programy fabryczne oryginalu: nazwa i slowa parametrow
const char* velourProgramName(int i);
void velourProgramWords(int i, int16_t out[VELOUR_PARAMS]);

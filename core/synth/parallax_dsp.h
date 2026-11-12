// Silnik PARALLAX — wlasny silnik DAW-POL (clean room: zbudowany od zera wg publicznego opisu i zrzutu ekranu
// wtyczki "dimension" z Opus / LizardMan: "klasyczny silnik subtraktywny z filtrem w stylu TB-303 i unisonem w stylu
// Juno"; bez zadnego kodu ani pliku binarnego oryginalu). Galki: WAVEFORM (plynnie PULSE -> SQUARE -> SAW -> SAW+SUB
// -> NOISE, PolyBLEP), UNISON (3 rozstrojone oscylatory na glos), FILTER FREQ (20 Hz .. 20 kHz), RES (drabinka
// 4-biegunowa z nasyceniem na wejsciu i gornoprzepustowym filtrem w petli rezonansu, jak w 303; samooscylacja przy
// koncu zakresu). Glos 96 kHz, bloki 128 probek (gospodarz: ADSR, obwiednia filtra na CUTOFF, MOD ENV, 2 LFO).
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ParallaxCore;
struct ParallaxCore* parallaxCreate(void);
void parallaxNoteOn(struct ParallaxCore* p, int voice);
void parallaxRender(struct ParallaxCore* p, int voice, int16_t* out, const int16_t* knobs, uint32_t inc, float rate);
float parallaxCutoffHz(int knob);        // FILTER FREQ w Hz
int parallaxWaveName(int knob, char* out, int n);  // nazwa fali (albo przejscia) do odczytu galki
// Ekran: jeden okres sygnalu po filtrze (n probek, -1..1) przy 220 Hz; detune = przesuniecie glosu unison (0..1)
void parallaxScope(const int16_t knobs[4], float* out, int n, float detune, uint32_t seed);

#ifdef __cplusplus
}
#endif

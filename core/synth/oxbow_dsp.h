// Oryginalny DSP silnika OXBOW (Hundekraft/kennel, port OberVave zvensona; third_party/kennel_oxbow/plugins/oxbow/
// oxbow.c bez zmian): polifoniczny syntezator w stylu OB-X / OB-8. Glos: dwa oscylatory (pila, puls, oba; OSC 2 z
// hard sync, X-MOD do OSC 1, kwinta), szum, przester, filtr SVF (LP12, HP12, BP12, NOTCH, LP24, SEM), obwiednia
// filtra z krzywymi RC, LFO, "vintage" (wlasne odchylki kazdej karty glosu i powolny dryf). Wewnatrz 22,05 kHz.
// Galki (0..32767): 0 CUTOFF, 1 RESONANCE, 2 ENV AMOUNT (obwiednia filtra, srodek = 0), 3 DETUNE (OSC 2, do +1/2 tonu).
// Strona 2: knobs[5] WAVE (SAW, PULSE, BOTH, SYNC, X-MOD, FIFTH), [6] FILTER, [7] DRIVE.
// oxbow.c jest w C; stan sciezki (zmienne pliku) podmieniany przy przelaczaniu sciezek (instancja = sciezka).
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct OxbowCore;
struct OxbowCore* oxbowCreate(void);
void oxbowNoteOn(struct OxbowCore* p, int voice, const int16_t* knobs, int vel);
void oxbowNoteOff(struct OxbowCore* p, int voice);
void oxbowRender(struct OxbowCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel);
// dla ekranu: bloki, blok ostatniego dzwieku glosu, obwiednia filtra glosu (Q15)
struct OxbowView { uint32_t blocks; uint32_t vblk[6]; int32_t venv[6]; };
void oxbowView(const struct OxbowCore* p, struct OxbowView* v);

#ifdef __cplusplus
}
#endif

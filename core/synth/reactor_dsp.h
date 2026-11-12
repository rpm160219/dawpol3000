// Oryginalny DSP silnika REACTOR z op1r (Hundekraft, GPL-3.0-or-later; third_party/op1r_reactor/plugins/reactor/
// reactor.c bez zmian): cztery sinusowe "prety" w petli modulacji fazy ze sprzezeniem przez "chlodziwo"; kazdy glos
// ma temperature rdzenia — po przekroczeniu stanu krytycznego petla, przester i "ucieczka" (pret o pol tonu nizej,
// opadanie stroju) wchodza mocniej.
// Galki (0..32767): 0 COOLING (filtr w petli + szybkosc chlodzenia), 1 RADIATION (szum i trzaski Geigera),
// 2 RODS (prety 2..4 wysuwane po kolei), 3 HEAT (sprzezenie, przester, zrodlo ciepla). knobs[7] = trym presetu.
// reactor.c jest w C; stan glosu (rx_voice) i licznik blokow podmieniane przed wywolaniem (instancja = sciezka).
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ReactorCore;
struct ReactorCore* reactorCreate(void);
void reactorNoteOn(struct ReactorCore* p, int voice, const int16_t* knobs, int vel);
void reactorNoteOff(struct ReactorCore* p, int voice);
void reactorRender(struct ReactorCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel);
// dla ekranu (jak w oryginale czyta rx_voice): temperatura (Q16, 1.0 = krytyczna), stan krytyczny, rozblysk,
// trzask Geigera, blok ostatniego renderu glosu; bloki
struct ReactorView { uint32_t blocks; int32_t temp[6], crit[6], flare[6], click[6]; uint32_t last[6]; };
void reactorView(const struct ReactorCore* p, struct ReactorView* v);

#ifdef __cplusplus
}
#endif

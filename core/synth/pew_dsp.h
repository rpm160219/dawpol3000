// Oryginalny DSP silnika PEW z op1r (Hundekraft, GPL-3.0-or-later; third_party/op1r_pew/pew.c bez zmian):
// organy z dziewiecioma suwakami (drawbar) przez glosnik rotacyjny (Leslie). Stan wspolny dla glosow
// (tablice fal, wirniki, perkusja z pojedynczym wyzwalaniem) — stad jedna instancja na sciezke.
// Galki (0..32767): 0 REGISTER (rejestracje 008000000 -> 888888888), 1 PERCUSSION (+ klik klawisza),
// 2 DRIVE / CHORUS (skaner wibrato, potem przester lampowy), 3 LESLIE (stop / wolno / szybko).
// knobs[5..7] = wysuniecie suwakow niskich / srodkowych / wysokich (strona 2 w oryginale).
#pragma once
#include <stdint.h>

namespace dp {

struct PewCore;
PewCore* pewCreate();  // poza watkiem audio (przy setKind)
void pewNoteOn(PewCore* p, int voice);
void pewNoteOff(PewCore* p, int voice);
// Blok 128 probek glosu; glosy w kolejnosci rosnacej w bloku (jak na OP-1)
void pewRender(PewCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel);

// Stan dla ekranu (odczyt z watku UI, wartosci jak volatile w oryginale)
struct PewView {
    uint32_t blocks, horn, drum, strikes;
    int32_t speed;
    uint8_t held[6];
};
void pewView(const PewCore* p, PewView* v);
// Poziomy 9 suwakow (Q12, 8 = 32768) z galki REGISTER i strony 2 — te same, ktore gra dzwiek (pew_macro.h)
void pewRegistration(int32_t blue, const int16_t* push, int32_t out[9]);

}  // namespace dp

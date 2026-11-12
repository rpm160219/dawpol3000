// Silnik FELT — wlasny model fizyczny fortepianu z filcem (DAW-POL, bez sampli). Na glos do 3 strun jako falowody
// (opoznienie z interpolacja, strata zalezna od czestotliwosci, 2 filtry wszechprzepustowe = sztywnosc struny /
// niealikwotowosc, strojenie poprawione o opoznienia fazowe filtrow), mlotek = impuls kontaktu (czas i jasnosc z
// sily uderzenia i grubosci filcu), grzebien miejsca uderzenia (1/8 struny), plyta rezonansowa = 4 rezonatory,
// mechanika (stuk klawisza, szelest filcu, opadanie tlumika). Galki: FELT (filc), CORDA (una corda -> tre corde z
// rozstrojeniem: dudnienie i dwuetapowe wybrzmienie), DECAY (wybrzmienie strun), NOISE (odglosy mechaniki).
// Glos liczony przy 48 kHz, interpolowany do 96 kHz gospodarza.
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct FeltCore;
struct FeltCore* feltCreate(void);
void feltNoteOn(struct FeltCore* p, int voice);
void feltNoteOff(struct FeltCore* p, int voice);   // tlumik opada (wolane przy kazdym bloku dla puszczonych glosow)
void feltRender(struct FeltCore* p, int voice, int16_t* out, const int16_t* knobs, uint32_t inc, float rate, int vel);
float feltDecaySeconds(int knob);           // wybrzmienie przy C4
int feltStrings(int knob);                  // liczba strun 1..3
// ekran
float feltMotion(const struct FeltCore* p);       // drgania strun 0..1
uint32_t feltStrikes(const struct FeltCore* p);   // licznik uderzen
int feltHeld(const struct FeltCore* p);           // trzymane klawisze (tlumik w gorze)

#ifdef __cplusplus
}
#endif

// Silnik LOUNGE — wlasny silnik DAW-POL (clean room, wg publicznego opisu silnika KEYS z "OP-1 Mods" Justina Joe:
// "tine electric piano, reed i clav, bez sampli; BARK zamiast dynamiki, TONE przesuwa przetwornik, DECAY wybrzmienie,
// TYPE instrument" — bez kodu ani latki oryginalu). Modele: TINE — zab (mody 1, dudnienie listwy tonowej, dzwonek
// ~7,1x) przez nieliniowy przetwornik magnetyczny (szczekanie przy mocnym uderzeniu, TONE = przesuniecie przetwornika
// wzgledem zeba); REED — jezyczek (mody 1 i 6,27) przez asymetryczny przetwornik elektrostatyczny, TONE = jasnosc;
// CLAV — struna Karplus-Strong szarpana stycznikiem, TONE = polozenie przetwornika (grzebien). Glos liczony przy 48 kHz,
// interpolowany do 96 kHz gospodarza; rodzaj instrumentu zapamietany przy uderzeniu.
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct LoungeCore;
struct LoungeCore* loungeCreate(void);
void loungeNoteOn(struct LoungeCore* p, int voice);
void loungeRender(struct LoungeCore* p, int voice, int16_t* out, const int16_t* knobs, uint32_t inc, float rate, int vel);
int loungeType(int knob);                  // 0 TINE, 1 REED, 2 CLAV
float loungeDecaySeconds(int knob);        // czas wybrzmienia przy C4
float loungeMotion(const struct LoungeCore* p);  // ekran: drgania (0..1) najglosniejszego glosu
uint32_t loungeStrikes(const struct LoungeCore* p);  // licznik uderzen (ekran: mlotek)

#ifdef __cplusplus
}
#endif

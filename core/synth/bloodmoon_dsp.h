// Oryginalny silnik BLOODMOON z op1r (Nola Drift / SammiBabe666, kennel; GPL-3.0-or-later; third_party/op1r_bloodmoon/
// bloodmoon.c + moon.c bez zmian): gotyckie organy — podstawa + 2., 3., 4. harmoniczna z jednej fazy — i "nawiedzony"
// dzwon na ataku (dwa nieharmoniczne skladniki: mala tercja 1,1875x i 2,40625x), wibrato wchodzace po ~0,1 s.
// Galki: 0 BLOOD (bogactwo harmonicznych), 1 ECLIPSE (glosnosc i dlugosc dzwonu), 2 HAUNTING (glebokosc wibrata i
// chwiejnosc), 3 MOONRISE (tempo wibrata 0,15..9 Hz). Glosnosc z ADSR gospodarza. Stan glosu = slot w instancji (self);
// ziarno fazy wibrata (bm_seed) podmieniane per instancja. Tu tez ekran oryginalu (ksiezyc za katedra).
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct BloodCore;
struct BloodCore* bloodCreate(void);
void bloodNoteOn(struct BloodCore* p, int voice, const int16_t* knobs);
void bloodNoteOff(struct BloodCore* p, int voice);
void bloodRender(struct BloodCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel);

typedef void (*BmwArtFn)(void* user, const int32_t* const* m, const uint8_t* visible);  // 9 grup jak w SVG
void bloodScreenUpdate(void* user, BmwArtFn art_fn, const int16_t live[4], int dt_ms);

#ifdef __cplusplus
}
#endif

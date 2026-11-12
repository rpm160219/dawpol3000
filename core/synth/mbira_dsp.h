// Oryginalny DSP silnika MBIRA z op1r (Hundekraft, GPL-3.0-or-later; third_party/op1r_mbira/mbira.c bez zmian):
// pianino kciukowe — synteza modalna: stalowy jezyczek = 4 zanikajace skladowe + krotki klik, przez drewniane
// pudlo, opcjonalnie brzeczace kapsle.
// Galki (0..32767): 0 STRIKE (miekki kciuk .. twardy paznokiec), 1 TINE (drewno .. stal .. szklo),
// 2 BOX (rezonans pudla), 3 BUZZ (kapsle). Strona 2: knobs[5] RING (1,6..8 s), [6] DOUBLE (drugi jezyczek
// do +12 c), [7] WAH (dlon nad otworem, 400 Hz .. 1,5 kHz przy 5 Hz).
// mbira.c jest w C; stan sciezki (hot_t pod mb_home_at + mb_g) podmieniany przed wywolaniem (instancja = sciezka).
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct MbiraCore;
struct MbiraCore* mbiraCreate(void);
void mbiraNoteOn(struct MbiraCore* p, int voice, const int16_t* knobs, int vel);
void mbiraRender(struct MbiraCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel);
// dla ekranu: bloki, poziom podstawowej skladowej glosow (Q15)
struct MbiraView { uint32_t blocks; int32_t level[6]; };
void mbiraView(const struct MbiraCore* p, struct MbiraView* v);

#ifdef __cplusplus
}
#endif

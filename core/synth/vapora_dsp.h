// Oryginalny DSP silnika VAPORA z op1r (AiAAAA, GPL-3.0-or-later; third_party/op1r_vapora/vapora.c + vapora_dsp.c bez
// zmian): pad PADsynth — dwa odczyty z 14 rodzin widm (CHARACTER), morfing par tablic, drugi oscylator w interwale,
// filtr dwubiegunowy, LFO ruchu. Galki (0..32767): 0 MORPH, 1 THICKNESS (poziom i rozstrojenie drugiego oscylatora),
// 2 BRIGHTNESS (filtr), 3 MOTION (glebokosc LFO). Strona 2: knobs[5] CHARACTER (14 widm), [6] INTERVAL (-OCT .. +OCT),
// [7] SPEED (0,02..2 Hz). Brak wlasnego wybrzmienia (note_off oryginalu = tylko ekran) — glosnosc z ADSR gospodarza.
// vapora.c jest w C; stan glosu (voices[i], ticks, serial) podmieniany przed wywolaniem (instancja = sciezka).
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct VaporaCore;
struct VaporaCore* vaporaCreate(void);
void vaporaNoteOn(struct VaporaCore* p, int voice, const int16_t* knobs);
void vaporaNoteOff(struct VaporaCore* p, int voice);  // tylko ekran (bramka "trzymania" fali)
void vaporaRender(struct VaporaCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel);
// dla ekranu (watek UI): skrzynki z migawkami glosow (ps_mailbox[6]) i numery trzymanych nut (uint32_t[6])
const void* vaporaMail(const struct VaporaCore* p);
const volatile uint32_t* vaporaHeld(const struct VaporaCore* p);

#ifdef __cplusplus
}
#endif

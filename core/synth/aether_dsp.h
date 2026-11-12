// Oryginalny silnik AETHER z op1r (pcppcp, GPL-3.0-or-later; third_party/op1r_aether/aether.c bez zmian): "przechylany
// teremin" — sinus .. trzcina (parzyste harmoniczne + miekki przester); przechylenie przod / tyl wygina wysokosc,
// uniesienie prawego boku scisza, potrzasanie daje wibrato. Galki: 0 ORBIT (zakres wygiecia 1..24 poltony), 1 FORM
// (barwa), 2 SPACE (wibrato z potrzasania), 3 DEPTH (sciszanie przechyleniem). Strona 2: knobs[5] DEAD ZONE (0..10 st.),
// [6] TILT RANGE (20..60 st.), [7] VIB RATE (3..8 Hz); 0 = 8192 jak w oryginale. RG35XX nie ma czujnika ruchu:
// wrapper podaje oryginalowi wirtualny czujnik z przechylenia strony TILT (PITCH TILT -1..1 = -60..+60 st. przod / tyl,
// VOL TILT 0..1 = 0..30 st. prawego boku, SHAKE 0..1 = drganie osi Z). "Dom" (kat przy nucie) = poziomo.
// Stan glosu oryginalu (st[6], ae_cur) w zmiennych pliku — podmieniany per instancja.
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct AetherCore;
struct AetherCore* aetherCreate(void);
void aetherNoteOn(struct AetherCore* p, int voice, const int16_t* knobs);
// env = obwiednia glosnosci (Q15, tylko ekran); tilt: pitch -1..1, vol 0..1, shake 0..1
void aetherRender(struct AetherCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel,
                  int env, float tiltPitch, float tiltVol, float shake);

typedef void (*AewLineFn)(void* user, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* rgba);
typedef void (*AewArtFn)(void* user, const int32_t* const* m, const uint8_t* visible);  // grupy jak aether_art.h
void aetherScreenUpdate(void* user, AewLineFn line_fn, AewArtFn art_fn, const struct AetherCore* core,
                        const int16_t knobs[8], const int16_t live[4], int page2, int dt_ms);

#ifdef __cplusplus
}
#endif

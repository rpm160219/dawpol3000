// Silnik OUTATIME z paczki zrodel Hundekraft 3.1.4 (kennel; dzwiek = ota_dsp.c / ota_tables.h z opus.outatime,
// LizardMan245, GPL-3.0; klej outatime.c Hundekraft) — bez zmian: "syntezator podrozy w czasie" — trzy oscylatory na
// glos (ramiona kondensatora strumienia), YEAR 1955..2015 zmienia fale (trojkat -> pila -> zawijany sinus), SPEED 0..88
// mph dryf ramion po latach, POWER 0..1,21 GW przester z wyrownaniem glosnosci, TRAIL zaplon: kazda nuta otwiera
// rezonansowy filtr, ktory sie wypala. Strona 2: knobs[5] SPREAD, [6] TONE, [7] FLUX (ensemble). Glosnosc z ADSR
// gospodarza, glide i tryby gry jak w OP-1. Stan glosow (ota_home) per instancja; dane ekranu w instancji.
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct OutatimeCore;
struct OutatimeCore* outatimeCreate(void);
void outatimeNoteOn(struct OutatimeCore* p, int voice, const int16_t* knobs);
void outatimeNoteOff(struct OutatimeCore* p, int voice);
void outatimeRender(struct OutatimeCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel);

typedef void (*OtwLineFn)(void* user, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* rgba);
typedef void (*OtwArtFn)(void* user);
void outatimeScreenUpdate(void* user, OtwLineFn line_fn, OtwArtFn art_fn, const struct OutatimeCore* core,
                          const int16_t knobs[8], const int16_t live[4], int page2, int dt_ms);

int ota_year(int knob);    // 1955..2015 (ota_dsp.c)
int ota_mph(int knob);     // 0..88
int ota_gw100(int knob);   // gigawaty x 100: 0..121

#ifdef __cplusplus
}
#endif

// Oryginalny silnik HOLLOW SAINT z op1r (SammiBabe666 / kennel, GPL-3.0-or-later; third_party/op1r_hollowsaint/
// hollow_saint.c bez zmian, wersja kennel z flagami plugin.json): "nawiedzona" lira korbowa — struna smyczkowa (saw /
// puls / pusty sinus + szum smyczka, formant nosowy), burdon (pryma + kwinta, jeden wlasciciel na akord), brzeczacy
// mostek ("pies") w rytm uderzen kola zsynchronizowanych z tempem, katedra (pudlo, wspolny pogłos, dzwon) i tasma Lo-Fi
// Relic. Galki: 0 DRONE, 1 STRINGS, 2 WHEEL BUZZ, 3 CATHEDRAL; strona 2: knobs[5] klucz burdonu (FOLLOW, C..B),
// [6] wiek tasmy RELIC, [7] tempo uderzen (steady, 1 bar .. 1/16); RELIC wl./wyl. (w OP-1: strona 2 zielonej galki).
// Stan oryginalu (glosy, wspolny etap, burdon) w zmiennych pliku — kazda instancja (sciezka) ma wlasna kopie,
// podmieniana przy zmianie instancji. Silnik czyta obwiednie glosnosci glosu (u nas AMP ENV) i zegar taktu.
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct HollowCore;
struct HollowCore* hollowCreate(void);
void hollowNoteOn(struct HollowCore* p, int voice, const int16_t* knobs);
void hollowNoteOff(struct HollowCore* p, int voice);
// env = obwiednia glosnosci glosu (Q15); relic = tasma wlaczona; beatLen / beatNext = dlugosc taktu i czas do
// nastepnego (1/16384 probki glosu przy polowie czestotliwosci glosu), beatStep = 0x100000 gdy transport gra, 0 stoi
void hollowRender(struct HollowCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel,
                  int env, int relic, int32_t beatLen, int32_t beatNext, int32_t beatStep);

int hollowDroneKey(int16_t w);      // 0 = FOLLOW, 1..12 = C..B
int hollowStrokeZone(int16_t w);    // 0..6: steady, 1 bar, 1/2, 1/4, 1/8, 1/8T, 1/16

typedef void (*HswLineFn)(void* user, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* rgba);
typedef void (*HswArtFn)(void* user, const int32_t* const* m, const uint8_t* visible);  // na grupe (43, jak w SVG)
void hollowScreenUpdate(void* user, HswArtFn art_fn, const struct HollowCore* core, const int16_t knobs[8],
                        const int16_t live[4], int relic, int dt_ms);

#ifdef __cplusplus
}
#endif

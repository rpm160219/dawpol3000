// Efekty op1r (Hundekraft, pcppcp, Maiko) jako inserty / efekty AUX miksera DAW-POL: oryginalny kod bez zmian (tryb
// OP1_HOST), wiele niezaleznych egzemplarzy (oryginal ma 3 stale pozycje: synth, drum, master — core/fx/fxw.h). Blok 64
// probek int16 przy 44,1 kHz jak w OP-1 (konwersja czestotliwosci: core/audio/mixer.cpp). Slowa galek w[8]: 0..3 strona 1,
// 5..7 strona 2. Efekty mono (TAPE, STUTTER, SNOWGLOBE) mikser uruchamia jako dwa egzemplarze: lewy i prawy kanal.
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define FX_TELEMETRY_BYTES 256
typedef struct FxImpl {
    int stereo;                                                          // 1: stereo(l, r); 0: mono na kanal
    void* (*create)(void);
    void (*mono)(void* inst, int16_t* io, const int16_t* w);
    void (*stereoFn)(void* inst, int16_t* l, int16_t* r, const int16_t* w);
    void (*telemetry)(void* inst, void* out);                            // dla ekranu (<= FX_TELEMETRY_BYTES)
    int (*scSource)(const int16_t* w);                                   // ENTE: zrodlo sidechain (0 = wlasne wejscie)
    void (*destroy)(void* inst);
} FxImpl;
const FxImpl* fxImpl(int fx);   // fx = FxId (seq/project.h); 0 albo nieznany: NULL
// zegar tempa (dla efektow z synchronizacja): dlugosc beatu i czas do nastepnego od poczatku bloku (1/16384 probki 44,1 kHz)
void fxSetBeat(int32_t len, int32_t next, int running);
// sidechain ENTE: 64 probki zrodla dla biezacego bloku (NULL = brak)
void fxSetSidechain(const int16_t* sc);

// stare wejscia (testy zgodnosci)
void* fxJuniChorusCreate(void);
void fxJuniChorusProcess(void* inst, int16_t* l, int16_t* r, const int16_t* w);
void* fxSisterCreate(void);
void fxSisterProcess(void* inst, int16_t* l, int16_t* r, const int16_t* w);
void fxJuniChorusTelemetry(void* out);
void fxSisterTelemetry(void* inst, void* out);
#ifdef __cplusplus
}
#endif

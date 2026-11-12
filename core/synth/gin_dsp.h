// Silnik GIN (perkusja) z paczki zrodel Hundekraft 3.1.4 (kennel, plugins/gin): syntezator bebnow SquareWave Studio na
// libpo32 Erica Lewisa (MIT) — 24 dzwieki w zestawie, kazdy z dwoma koncami A/B (po 21 parametrow) i MORPH miedzy nimi,
// 8 zestawow fabrycznych (na podstawie zestawow CNTR RNDM). Dzwiek liczy oryginalny kod bez zmian: przygotowanie
// wspolczynnikow (gin_prepare.c, tonic_voice.c, po32_synth.c), glos staloprzecinkowy 44,1 kHz (gin_integer.c), FIR 2x do
// 88,2 kHz (gin_resample.c), adapter 12 glosow OP-1 z wyciszaniem tego samego klawisza i limitem 4 glosow syntezy
// (gin_adapter.c), stopien glosnosci kennel (gin_fw_level.h). Od DAW-POL: przydzial 12 odbiornikow, klawisz = nuta,
// zapis zestawu, konwersja 88,2 -> 48 kHz (interpolacja Catmull-Rom) i dynamika (velocity < 127 = przygotowanie z
// prawdziwa dynamika libpo32; 127 = jak w OP-1).
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GIN_KIT_SOUNDS 24
#define GIN_PARAMS 21
// Stan jednego dzwieku zestawu (zapisywany w projekcie): konce A i B (parametry 0..1 w kolejnosci TONIC_PARAMETERS),
// MORPH 0..1000, edytowana strona (0 = A, 1 = B).
typedef struct { float ep[2][GIN_PARAMS]; uint16_t morph; uint8_t side, pad; } GinSound;

struct GinCore;
struct GinCore* ginCreate(void);
void ginSetKit(struct GinCore* p, int kit, const GinSound* sounds);  // zestaw fabryczny + biezace dzwieki (24)
void ginNoteOn(struct GinCore* p, int note, int vel127);
void ginAllOff(struct GinCore* p);
int ginRender(struct GinCore* p, float* out, int n);   // dodaje n probek 48 kHz (mono, skala 1 = pelna); zwraca liczbe glosow
void ginRenderNative(struct GinCore* p, int32_t out[128]);  // jeden blok 88,2 kHz (suma glosow po stopniu glosnosci) — test
int ginKeyOfNote(int note);           // klawisz zestawu 0..23: C2 (36) .. B3 (59), kazdy dzwiek na stale; inne nuty = -1 (cisza)
uint32_t ginHits(const struct GinCore* p);   // licznik uderzen (ekran: wybor dzwieku ostatnio uderzonego klawisza)
int ginLastKey(const struct GinCore* p);

// Zestawy fabryczne
int ginFactoryCount(void);
const char* ginFactoryName(int kit);
const char* ginSoundName(int kit, int sound);
int ginKeySound(int kit, int key);     // indeks dzwieku na klawiszu
void ginFactorySound(int kit, int sound, GinSound* out);

#ifdef __cplusplus
}
#endif

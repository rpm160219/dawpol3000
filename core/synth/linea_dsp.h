// LINEA: synteza LA (port 1:1 PicoFaceD5, Michi71 — natywny silnik LA na danych PCM z ROM-ow D-50: ataki z probek
// sklejone z syntetycznym wybrzmieniem, 7 struktur z modulatorem pierscieniowym, 3 LFO, obwiednia wysokosci, EQ,
// chorus, reverb; 16 glosow jak oryginal). Dziala na 32 kHz (jak maszyna), wyjscie przeliczane na 48 kHz (FIR 2:3).
// ROM-y NIE sa w repo: wczytywane z karty (katalog roms/linea, rozpoznawane po CRC32 jak w oryginale, zdublowane
// zrzuty skladane), PCM dekodowany przy starcie. Patche = surowe 448 bajtow D-50 (siedem blokow po 64).
#pragma once
#include <stdint.h>

#define LINEA_PATCH_BYTES 448

// ROM (wspolny dla wszystkich sciezek)
bool lineaRomLoad(const char* dir);       // szuka plikow w katalogu; true = PCM A+B i EPROM rozpoznane
bool lineaRomReady();
const char* lineaRomStatus();             // komunikat dla ekranu (wersja EPROM albo czego brakuje)
const char* lineaPcmName(int i);          // nazwa probki PCM 0..99 (z tabeli)
const int16_t* lineaPcm();                // zdekodowany PCM (262144 probek 32 kHz) albo NULL

// wyrownanie wydajnosciowe (nie zmienia patcha): przesuniecia CUTOFF (-100..100) i RESONANCE (-30..30) wszystkich partiali
struct LineaPerf { int cut = 0, res = 0; };

struct LineaCore;
LineaCore* lineaCreate(float outRate);
void lineaDestroy(LineaCore* c);
// patch + identyfikator (zmiana id = nowy patch: pelna konfiguracja; ten sam id = edycja bez klikow)
void lineaSetPatch(LineaCore* c, const uint8_t* patch, uint32_t id, const LineaPerf& perf);
void lineaNoteOn(LineaCore* c, int note, float vel);
void lineaNoteOff(LineaCore* c, int note);
void lineaAllOff(LineaCore* c);
void lineaRender(LineaCore* c, float* l, float* r, int n);  // 48 kHz, nadpisuje
// ekran
int lineaVoices(const LineaCore* c);
uint32_t lineaNoteCount(const LineaCore* c);  // licznik uderzen (iskry PCM)
float lineaLastVel(const LineaCore* c);
float lineaOutLevel(const LineaCore* c);                    // szczyt wyjscia (wygladzony)

// patche: nazwa (18 znakow alfabetu panelu D-50) i struktura
void lineaPatchName(const uint8_t* patch, char out[19]);
void lineaSetPatchName(uint8_t* patch, const char* name);
int lineaStructure(const uint8_t* patch, int tone);         // 1..7

// Zapis/odczyt projektu: tekstowy plik .dpp (czytelny, odporny na nowe wersje).
//
//   dawpol 1                         <- wersja formatu
//   name "PROJECT 01"
//   bpm 120
//   master 0.80
//   loop 1 0 8                       <- petla utworu: wlaczona, IN takt, OUT takt (wylacznie)
//   track midi "BASS" color 0 vol 0.80 mute 0 solo 0 last 0 engine wavetable k 16384 8192 20000 8192
//         env 0.005 0.4 0.75 0.3       <- instrument: silnik, 4 galki 0..32767, obwiednia A D S R
//         envamt 1 fenv 0.005 0.3 0 0.3 0.5 lfo1 0 2 0.3 2 lfo2 ...  <- AMOUNT obwiedni glosnosci; obwiednia filtra
//         A D S R AMOUNT; LFO: ksztalt Hz glebokosc cel synchronizacja podzial
//   pattern "BASS A" bars 1
//   loop 0 0 16                      <- (opcjonalnie) petla patternu w krokach
//   n 0 72 36 120                    <- nuta: start (ticki), dlugosc (ticki), wysokosc MIDI, velocity
//   end
//   clip 0 0 0                       <- sciezka, pattern, takt startu
//
// Nieznane linie sa pomijane. Zapis atomowy: plik.tmp -> fsync -> rename.
#pragma once
#include <stdio.h>
#include "seq/project.h"

namespace dp {

bool saveProjectFile(const Project& p, const char* path);
// Wspolny zapis/odczyt instrumentu sciezki (projekt i presety): " engine ... k ... env ... voice ..."
void writeInstrument(FILE* f, const Track& t);
void parseInstrument(Track& t, char** tok, int n, int start);
void writeGinSounds(FILE* f, const Track& t);          // GIN: linie gins (dzwieki zmienione wzgledem zestawu)
bool parseGinSound(Track& t, char** tok, int n);
void writeMix(FILE* f, const Track& t);  // mikser kanalu: linia "mix"
void parseMix(Track& t, char** tok, int n);
int tokenize(char* line, char** tok, int max);  // slowa i "napisy"
const char* kv(char** tok, int n, int from, const char* key);
bool loadProjectFile(Project& out, const char* path, char* err, int errLen);

}  // namespace dp

// Zapis/odczyt projektu: tekstowy plik .dpp (czytelny, odporny na nowe wersje).
//
//   dawpol 1                         <- wersja formatu
//   name "PROJECT 01"
//   bpm 120
//   master 0.80
//   track midi "BASS" color 0 vol 0.80 mute 0 solo 0
//   pattern "BASS A" bars 1
//   n 0 72 36 120                    <- nuta: start (ticki), dlugosc (ticki), wysokosc MIDI, velocity
//   end
//   clip 0 0 0                       <- sciezka, pattern, takt startu
//
// Nieznane linie sa pomijane. Zapis atomowy: plik.tmp -> fsync -> rename.
#pragma once
#include "seq/project.h"

namespace dp {

bool saveProjectFile(const Project& p, const char* path);
bool loadProjectFile(Project& out, const char* path, char* err, int errLen);

}  // namespace dp

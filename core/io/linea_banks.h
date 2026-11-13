// Banki patchy LINEA: pliki SysEx D-50 (zrzut DT1, adres 02-00-00, 448 bajtow na patch, do 64 w banku) w katalogu
// presets/linea/banks. Import: plik .syx z roms/linea albo sysex/ (sprawdzone sumy kontrolne i zakresy jak
// d5_syx_to_patches.py oryginalu) kopiowany jako nowy bank. Zapis: bank USER (komunikaty DT1 po 256 bajtow, jak D-50).
// Pierwsze uruchomienie: .syx z roms/linea importowane automatycznie (FactoryPatches = bank FACTORY).
#pragma once
#include <stdint.h>
#include "synth/linea_dsp.h"

namespace dp {

class LineaBanks {
public:
    static const int MAX_BANKS = 24, MAX_PATCHES = 64, MAX_IMPORT = 32;
    void init(const char* presetRoot = "presets", const char* romDir = "roms/linea", const char* sysexDir = "sysex");
    int count() { ensure(); return n_; }
    const char* name(int b) { ensure(); return b >= 0 && b < n_ ? bank_[b].name : ""; }
    int patches(int b);                                    // liczba patchy w banku (wczytuje przy pierwszym uzyciu)
    const uint8_t* patch(int b, int s);                    // 448 bajtow albo NULL
    const char* patchName(int b, int s);
    int find(const char* bankName);                        // indeks banku po nazwie (-1 = brak)
    // import
    int importList(char out[][64], int max);               // pliki .syx do wczytania (nazwy)
    bool importFile(int i, char* msg, int n);              // wczytuje i zapisuje jako bank
    // zapis do banku USER (slot < 0 = na koncu); zwraca slot albo -1
    int saveUser(const uint8_t* patch, int slot);
    bool removePatch(int b, int s);                        // tylko USER
    bool removeBank(int b);                                // usuwa plik banku
    static bool parseSyx(const uint8_t* raw, int n, uint8_t (*out)[LINEA_PATCH_BYTES], int* count, char* err, int errn);
    static bool writeSyx(const char* path, const uint8_t (*p)[LINEA_PATCH_BYTES], int count);
private:
    struct Bank { char name[24]; char file[64]; bool loaded; int n; uint8_t (*data)[LINEA_PATCH_BYTES]; char names[MAX_PATCHES][19]; };
    Bank bank_[MAX_BANKS];
    int n_ = 0;
    bool ready_ = false;
    char dir_[256] = "", rom_[256] = "", sysex_[256] = "";
    char imp_[MAX_IMPORT][64];      // nazwy plikow do importu
    char impPath_[MAX_IMPORT][320];
    void ensure() { if (!ready_) init(); }
    void scan();
    bool loadBank(int b);
};

LineaBanks& lineaBanks();  // wspolne dla aplikacji

}  // namespace dp

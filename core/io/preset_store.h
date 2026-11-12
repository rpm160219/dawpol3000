// Presety instrumentow: <root>/<silnik>/<NAZWA>.dpr (silnik: "wavetable", "wavetableplus", "taffy") — tekst:
//   dawpol-preset 1
//   name "WARM PAD"
//   category PAD
//   inst engine wavetable k ... env ... voice ...     (ten sam zapis co w projekcie)
// Zestaw fabryczny jest wersjonowany (plik-znacznik _factory_vN): nowe wersje dopisuja tylko brakujace presety,
// niczego nie nadpisujac (presety Patryka i jego zmiany zostaja).
#pragma once
#include "seq/project.h"

namespace dp {

class PresetStore {
public:
    static const int MAX = 256;
    void init(const char* root, const char* engineDir = "wavetable");
    bool ready() const { return root_[0] != 0; }
    int refresh();
    int count() const { return n_; }
    const char* name(int i) const { return names_[i]; }
    int category(int i) const { return cats_[i]; }
    bool load(int i, Track& t) const;                         // tylko parametry instrumentu (nazwa/kolor/glosnosc zostaja)
    bool save(const Track& t, const char* name, int category);  // nadpisuje preset o tej nazwie
    bool remove(int i);
    int find(const char* name) const;                         // indeks albo -1
    static void initInstrument(Track& t);                     // ustawienia INIT (galki wg silnika sciezki)

private:
    char root_[256] = "";
    char dir_[300] = "";
    char eng_[16] = "wavetable";
    char names_[MAX][32];
    char files_[MAX][32];
    uint8_t cats_[MAX];
    int n_ = 0;
    void writeFactory();
    void migrateFromWavetable();
    static const int FACTORY_VERSION = 2;
};

}  // namespace dp

// Magazyn projektow: katalog <root>/<NAZWA>/project.dpp (+ w przyszlosci audio/*.wav).
// Pamieta ostatnio otwarty projekt (<root>/_last.txt).
#pragma once
#include "seq/project.h"

namespace dp {

class ProjectStore {
public:
    static const int MAX = 128;
    void init(const char* root);
    bool ready() const { return root_[0] != 0; }
    int refresh();                                   // wczytuje liste, zwraca liczbe
    int count() const { return n_; }
    const char* display(int i) const { return disp_[i]; }  // nazwa do wyswietlenia
    bool exists(const char* name) const;
    bool save(const Project& p);                     // zapis pod p.name
    bool load(Project& p, int i, char* err, int errLen);
    bool remove(int i);
    void setLast(const char* name);
    bool loadLast(Project& p);
    static void folderName(const char* name, char* out, int n);  // "MY SONG" -> "MY_SONG"
    bool hasMarker(const char* name) const;          // plik-znacznik w katalogu projektow (np. demo juz dodane)
    void setMarker(const char* name);

private:
    char root_[256] = "";
    char dirs_[MAX][32];
    char disp_[MAX][32];
    int n_ = 0;
    void pathOf(const char* folder, char* out, int n) const;
};

}  // namespace dp

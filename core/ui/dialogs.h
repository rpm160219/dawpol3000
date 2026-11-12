// Okna dialogowe w stylu menu (belki, ilustracja, pola, lista, pigulki akcji).
//
// NEW TRACK:   gora/dol = typ (MIDI/AUDIO), L1/R1 = silnik (instrument), X = edycja nazwy, A = utworz, B = anuluj.
//              Po utworzeniu sciezki MIDI od razu otwiera sie NEW PATTERN.
// TRACK (edycja, Select na sciezce w ARRANGE): dwie karty przelaczane L2/R2:
//   TRACK:  X = nazwa, krzyzak = kolor (siatka 4x8 = 32), Y dwa razy = usun sciezke; silnik tylko jako nazwa
//   ENGINE: lista silnikow w jednej kolumnie (gora/dol), po prawej ilustracja i opis (sciezki MIDI)
//   A = OK (zapisuje nazwe, kolor i silnik), B = anuluj.
// NEW PATTERN: gora/dol = nowy albo istniejacy pattern z listy, lewo/prawo (lub niebieski
//              enkoder) = dlugosc w taktach, X = edycja nazwy, A = utworz / uzyj, B = anuluj.
//              Po utworzeniu klip trafia na sciezke i otwiera sie piano roll.
#pragma once
#include "seq/project.h"
#include "ui/encoder.h"
#include "ui/view.h"
#include "ui/widgets.h"

namespace dp {

class TrackDialog : public View {
public:
    void open();              // nowa sciezka
    void openEdit(int track);  // edycja istniejacej
    void update(const Buttons& b, const InputState& in, float dt) override;
    void render(Canvas& c) override;

private:
    int type_ = TRACK_MIDI;
    int edit_ = -1;  // -1 = nowa sciezka
    int color_ = 0;
    int engine_ = ENG_WAVETABLE;
    int card_ = 0;     // karta okna edycji: 0 = TRACK, 1 = ENGINE
    int engSel_ = 0;   // karta ENGINE: 0 WAVETABLE, 1 WAVETABLE+, 2 TEST SAW
    void renderEngineCard(Canvas& c);
    void cardDots(Canvas& c, int card);
    bool confirmDelete_ = false;
    char name_[12] = "";
    bool nameTouched_ = false;
    NameEdit ed_;
    void updateEdit(const Buttons& b);
    void renderEdit(Canvas& c);
};

class PatternDialog : public View {
public:
    void open(int track, int bar);
    void update(const Buttons& b, const InputState& in, float dt) override;
    void render(Canvas& c) override;

private:
    int track_ = 0, bar_ = 0, sel_ = 0, first_ = 0, bars_ = 1;  // sel_ 0 = NEW, 1.. = istniejace
    char name_[12] = "";
    NameEdit ed_;
    RelEncoder encLen_;
};

}  // namespace dp

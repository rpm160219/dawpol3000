// Okna dialogowe w stylu menu (belki, ilustracja, pola, lista, pigulki akcji).
//
// NEW TRACK:   gora/dol = typ (MIDI/AUDIO), X = edycja nazwy, A = utworz, B = anuluj.
//              Po utworzeniu sciezki MIDI od razu otwiera sie NEW PATTERN.
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
    void open();
    void update(const Buttons& b, const InputState& in, float dt) override;
    void render(Canvas& c) override;

private:
    int type_ = TRACK_MIDI;
    char name_[12] = "";
    bool nameTouched_ = false;
    NameEdit ed_;
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

// Ekran PIANO ROLL: edycja nut jednego patternu (1..16 taktow 4/4, siatka szesnastek).
// Widok pokazuje 1 takt; kursor przewija do kolejnych taktow.
//
// Sterowanie:
//   D-pad          kursor (krok / poltony), trzymanie = powtarzanie
//   A              wstaw nute (dlugosc = ostatnio uzyta) / odsluchaj istniejaca
//   B              usun nute pod kursorem
//   X + lewo/prawo dlugosc nuty -/+ szesnastka
//   Y + D-pad      przesun nute (czas / wysokosc)
//   L1 / R1        kursor oktawe w dol / w gore
//   Start          play / stop patternu (w petli, na instrumencie sciezki)
//   L3             pasek narzedzi (App): B UNDO, A REDO, Y IN/OUT (petla w krokach), X LOOP
//   R3             tryb ZAZNACZANIA (dolny pasek zmienia sie w pigulki akcji):
//                    B          zaznacz/odznacz nute; trzymaj B + D-pad = zaznaczenie prostokatem;
//                               B na pustym polu = wyczysc zaznaczenie
//                    A          usun zaznaczone
//                    Y          kopiuj zaznaczone (schowek dziala miedzy patternami)
//                    X          wklej w miejscu kursora (wklejone zostaja zaznaczone)
//                    L1 / R1    przelaczenie warstwy paska (1: edycja, 2: przeksztalcenia)
//                  warstwa 2:
//                    B + gora/dol     TRANSPOSE zaznaczonych o poltony
//                    A + D-pad        MOVE — przesuwanie zaznaczonych (czas i wysokosc)
//                    Y + gora/dol     OCTAVE — o oktawe
//                    X                DUPLICATE — kopia zaznaczonych zaraz za nimi
//                    R3         wyjscie z trybu
//   L2             powrot do ARRANGE
// Enkodery: niebieski = LENGTH, zielony = VELOCITY, bialy = VOLUME sciezki, czerwony = BPM.
#pragma once
#include "seq/pattern.h"
#include "ui/encoder.h"
#include "ui/view.h"

namespace dp {

class PianoRoll : public View {
public:
    void open(int pattern, int track);
    void update(const Buttons& b, const InputState& in, float dt) override;
    void render(Canvas& c) override;
    void setCursor(int step, int pitch) { curStep_ = step; curPitch_ = pitch; scrollToCursor(); }
    void setView(int topPitch) { topPitch_ = topPitch; scrollToCursor(); }
    bool valid();                 // czy pattern/sciezka nadal istnieja (np. po undo)
    int patternIndex() const { return pat_; }
    int cursorStep() const { return curStep_; }
    void exitSelect() { selMode_ = false; bHeld_ = false; clearSel(); }
    void onProjectReplaced();     // po undo/wczytaniu: czysci zaznaczenie
    void setSelectMode(bool on) { selMode_ = on; clearSel(); }  // (testy/headless)
    void selectRect(int s0, int p0, int s1, int p1);             // (testy/headless)

private:
    int pat_ = 0, track_ = 0;
    RelEncoder encLen_, encVel_, encTempo_;
    int curStep_ = 0, curPitch_ = 60, topPitch_ = 67;
    int lastLen_ = 2 * TICKS_PER_STEP, lastVel_ = 100;
    int previewPitch_ = -1;
    float previewLeft_ = 0.f;
    // Zaznaczanie
    bool selMode_ = false;
    bool sel_[Pattern::MAX_NOTES] = {};
    bool bHeld_ = false, bMoved_ = false;
    int selLayer_ = 0;  // 0 = SELECT/DELETE/COPY/PASTE, 1 = TRANSPOSE/MOVE/OCTAVE/DUPLICATE
    int anchorStep_ = 0, anchorPitch_ = 0;
    struct Clipboard { Note notes[Pattern::MAX_NOTES]; int n = 0; int refPitch = 60; } clip_;
    void clearSel();
    int selCount();
    void removeNote(int i);  // usuwa z patternu, przenosi flage zaznaczenia jak Pattern::remove
    void updateSelect(const Buttons& b);
    void updateTransform(const Buttons& b);
    bool shiftSelection(int dSteps, int dPitch);  // false = kolizja/poza zakresem (nic sie nie zmienia)
    void duplicateSelection();
    void previewTopSelected();
public:
    void setSelectLayer(int l) { selLayer_ = l; }  // (testy/headless)
private:
    void renderSelectBar(Canvas& c);

    Pattern& pat();
    int steps();
    int noteAtCursor();
    void scrollToCursor();
    void preview(int pitch, int vel);
    void stopPreview();
    void setLen(int idx, int lenTicks);
};

}  // namespace dp

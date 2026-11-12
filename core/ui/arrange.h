// Ekran ARRANGE (playlista): sciezki (max 16 MIDI + 4 audio) i klipy patternow na osi taktow.
//
// Sterowanie:
//   D-pad          kursor (sciezka / takt)
//   A              na wierszu "ADD TRACK": nowa sciezka
//                  na klipie: otworz pattern w piano rollu
//                  na pustym polu: wstaw ostatni pattern sciezki (albo okno nowego patternu)
//   Y              nowy pattern w miejscu kursora (okno NEW PATTERN)
//   B              usun klip
//   X (krotko)     sklonuj klip za nim;  X + lewo/prawo = przesun klip
//   Select         ustawienia sciezki (nazwa, kolor, usuwanie)
//   L1 / R1        mute / solo sciezki (male wskazniki M i S przy nazwie)
//   L3             pasek narzedzi (App): B UNDO, A REDO, Y IN/OUT (petla w taktach), X LOOP
//   Start          play / stop calej aranzacji
//   R2             piano roll patternu pod kursorem (albo ostatniego na tej sciezce)
// Enkodery: niebieski = VOLUME sciezki, zielony = ZOOM, bialy = MASTER, czerwony = BPM.
#pragma once
#include "ui/encoder.h"
#include "ui/view.h"

namespace dp {

class ArrangeView : public View {
public:
    void update(const Buttons& b, const InputState& in, float dt) override;
    void render(Canvas& c) override;
    void setCursor(int track, int bar) { curTrack_ = track; curBar_ = bar; }
    // Pattern "podswietlony" kursorem: klip pod kursorem, inaczej ostatni na sciezce. -1 = brak.
    int highlightedPattern(int* track) const;
    void clampCursor();
    int cursorBar() const { return curBar_; }
    int cursorTrack() const { return curTrack_; }

private:
    int curTrack_ = 0, curBar_ = 0, topTrack_ = 0, firstBar_ = 0, zoom_ = 2;
    bool xMoved_ = false;
    RelEncoder encZoom_, encBpm_;
    float barW() const;
    int visibleBars() const;
    void scroll();
};

}  // namespace dp

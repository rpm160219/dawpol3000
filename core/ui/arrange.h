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
//   Start          play / stop calej aranzacji
//   R2             piano roll ostatniego patternu
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

private:
    int curTrack_ = 0, curBar_ = 0, topTrack_ = 0, firstBar_ = 0, zoom_ = 2;
    bool xMoved_ = false;
    RelEncoder encZoom_, encBpm_;
    float barW() const;
    int visibleBars() const;
    void scroll();
};

}  // namespace dp

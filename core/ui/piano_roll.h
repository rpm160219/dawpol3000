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

private:
    int pat_ = 0, track_ = 0;
    RelEncoder encLen_, encVel_, encTempo_;
    int curStep_ = 0, curPitch_ = 60, topPitch_ = 67;
    int lastLen_ = 2 * TICKS_PER_STEP, lastVel_ = 100;
    int previewPitch_ = -1;
    float previewLeft_ = 0.f;

    Pattern& pat();
    int steps();
    int noteAtCursor();
    void scrollToCursor();
    void preview(int pitch, int vel);
    void stopPreview();
    void setLen(int idx, int lenTicks);
};

}  // namespace dp

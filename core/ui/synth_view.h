// Ekran SYNTH (czwarta zakladka): instrument biezacej sciezki.
//
// Sterowanie:
//   D-pad + ABXY   granie nut (8 stopni skali C-dur: lewo, dol, prawo, gora, Y, B, A, X)
//   L1 / R1        oktawa w dol / w gore
//   Select         strona galek: SOUND (4 parametry silnika) <-> ENVELOPE (attack, decay, sustain, release)
//   Start          play / stop aranzacji
//   L2             powrot do PIANO ROLL
// Enkodery (strona SOUND, WAVETABLE): niebieski = WAVE, zielony = DETUNE, bialy = CUTOFF, czerwony = RESONANCE.
// Enkodery (strona ENVELOPE): niebieski = ATTACK, zielony = DECAY, bialy = SUSTAIN, czerwony = RELEASE.
#pragma once
#include "ui/view.h"
#include "ui/wavetable_screen.h"

namespace dp {

class SynthView : public View {
public:
    void open(int track);
    int track() const { return track_; }
    void update(const Buttons& b, const InputState& in, float dt) override;
    void render(Canvas& c) override;
    void setPage(int p) { page_ = p; }  // (testy/headless)

private:
    int track_ = 0, page_ = 0, octave_ = 4;
    int held_[8] = {-1, -1, -1, -1, -1, -1, -1, -1};  // nuta grana przez przycisk i
    float dt_ = 0.016f;
    WavetableScreen wt_;
    void releaseAll();
};

}  // namespace dp

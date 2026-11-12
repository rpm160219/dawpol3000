// Ekran SYNTH (czwarta zakladka): instrument biezacej sciezki.
//
// Sterowanie:
//   D-pad + ABXY   granie nut (8 stopni skali C-dur: lewo, dol, prawo, gora, Y, B, A, X)
//   L1 / R1        oktawa w dol / w gore
//   Select         nastepna strona galek: SOUND -> AMP ENV -> FILTER ENV -> LFO 1 -> LFO 2
//   Start          play / stop aranzacji
//   L2             powrot do PIANO ROLL
// Enkodery (strona SOUND, WAVETABLE): niebieski = WAVE, zielony = DETUNE, bialy = CUTOFF, czerwony = RESONANCE.
// AMP ENV: ATTACK, DECAY, SUSTAIN, RELEASE. FILTER ENV: ATTACK, DECAY, SUSTAIN, AMOUNT (-100..+100, srodek = 0).
// LFO 1/2: RATE (0,05..20 Hz), DEPTH, SHAPE (sinus/trojkat/pila/prostokat/losowy), DEST (cel modulacji).
#pragma once
#include "ui/encoder.h"
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
    static const int PAGES = 5;
    int track_ = 0, page_ = 0, octave_ = 4;
    RelEncoder stepShape_[2], stepDest_[2];
    int held_[8] = {-1, -1, -1, -1, -1, -1, -1, -1};  // nuta grana przez przycisk i
    float dt_ = 0.016f;
    WavetableScreen wt_;
    void releaseAll();
};

}  // namespace dp

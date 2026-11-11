// Ekran PIANO ROLL: edycja nut jednego patternu (1 takt 4/4, siatka szesnastek).
//
// Sterowanie:
//   D-pad          kursor (krok / poltony), trzymanie = powtarzanie
//   A              wstaw nute (dlugosc = ostatnio uzyta) / odsluchaj istniejaca
//   B              usun nute pod kursorem
//   X + lewo/prawo dlugosc nuty -/+ szesnastka
//   Y + D-pad      przesun nute (czas / wysokosc)
//   L1 / R1        kursor oktawe w dol / w gore
//   Start          play / stop
//   Menu + Select  wyjscie
// Enkodery: niebieski = LENGTH, zielony = VELOCITY, bialy = VOLUME, czerwony = TEMPO.
#pragma once
#include "audio/engine.h"
#include "gfx/canvas.h"
#include "seq/pattern.h"
#include "ui/buttons.h"
#include "ui/encoder.h"
#include "ui/input.h"

namespace dp {

class PianoRoll {
public:
    void init(AudioEngine* e);
    void update(const InputState& in, float dt);
    void render(Canvas& c);
    bool quitRequested() const { return quit_; }
    void loadDemo();                       // przykladowa melodia (testy, headless)
    void setCursor(int step, int pitch) { curStep_ = step; curPitch_ = pitch; scrollToCursor(); }

private:
    AudioEngine* eng_ = nullptr;
    Pattern pat_;
    Buttons btn_;
    RelEncoder encLen_, encVel_, encTempo_;
    int curStep_ = 0, curPitch_ = 60, topPitch_ = 67;
    int lastLen_ = 2 * TICKS_PER_STEP, lastVel_ = 100;
    float volume_ = 0.7f, tempo_ = 120.f;
    bool dirty_ = true, quit_ = false;
    int previewPitch_ = -1;
    float previewLeft_ = 0.f;

    int noteAtCursor() const { return pat_.find(curStep_ * TICKS_PER_STEP, curPitch_); }
    void scrollToCursor();
    void preview(int pitch, int vel);
    void setLen(int idx, int lenTicks);
    void publish();
};

}  // namespace dp

// App: wlasciciel projektu i ekranow. Kieruje wejscie do biezacego ekranu, po zmianach
// publikuje migawke projektu do silnika audio.
#pragma once
#include "audio/engine.h"
#include "seq/project.h"
#include "ui/arrange.h"
#include "ui/dialogs.h"
#include "ui/piano_roll.h"

namespace dp {

class App {
public:
    void init(AudioEngine* e);
    void update(const InputState& in, float dt);
    void render(Canvas& c);
    bool quitRequested() const { return quit_; }

    Project& project() { return prj_; }
    AudioEngine* engine() { return eng_; }
    void markDirty() { dirty_ = true; }

    // Nawigacja
    void showArrange();
    void openPattern(int pattern, int track);
    void openNewTrack();
    void openTrackEdit(int track);
    void openNewPattern(int track, int bar);
    void togglePlaySong();

    void loadDemo();  // przykladowy projekt (headless, pierwsze uruchomienie)

    ArrangeView arrange;
    PianoRoll roll;
    TrackDialog trackDlg;
    PatternDialog patDlg;

private:
    AudioEngine* eng_ = nullptr;
    Project prj_;
    Buttons btn_;
    View* cur_ = &arrange;
    View* next_ = nullptr;
    bool dirty_ = true, quit_ = false;
    int lastPattern_ = -1, lastTrack_ = 0;
    void publish();
};

}  // namespace dp

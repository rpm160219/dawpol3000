// App: wlasciciel projektu i ekranow. Kieruje wejscie do biezacego ekranu, po zmianach
// publikuje migawke projektu do silnika audio. Zakladki (L2/R2): PROJECT | ARRANGE | PIANO ROLL.
#pragma once
#include "audio/engine.h"
#include "io/project_store.h"
#include "seq/project.h"
#include "seq/undo_stack.h"
#include "ui/arrange.h"
#include "ui/dialogs.h"
#include "ui/piano_roll.h"
#include "ui/project_view.h"

namespace dp {

class App {
public:
    void init(AudioEngine* e);
    // Projekty na karcie: wczytuje ostatni (albo demo, gdy brak). Bez tego — tylko demo, bez zapisu.
    void openStore(const char* root);
    void shutdown();  // autozapis przy wyjsciu (gdy sa zmiany)
    void update(const InputState& in, float dt);
    void render(Canvas& c);
    bool quitRequested() const { return quit_; }

    Project& project() { return prj_; }
    AudioEngine* engine() { return eng_; }
    void markDirty() { dirty_ = true; modified_ = true; edited_ = true; }
    void undo();                       // L3 (klikniecie lewej galki)
    void toast(const char* msg);       // krotki komunikat na srodku dolu ekranu
    ProjectStore& store() { return store_; }
    bool isModified() const { return modified_; }
    bool isOwnSave(const char* name) const;  // czy ta nazwa to plik biezacego projektu
    bool saveProject();
    bool loadProject(int index, char* err, int errLen);
    void newProject();

    // Nawigacja
    void showArrange();
    void showProject();
    void openPattern(int pattern, int track);
    void openNewTrack();
    void openTrackEdit(int track);
    void openNewPattern(int track, int bar);
    void togglePlaySong();

    void loadDemo();  // przykladowy projekt (headless, pierwsze uruchomienie)

    ProjectView projectView;
    ArrangeView arrange;
    PianoRoll roll;
    TrackDialog trackDlg;
    PatternDialog patDlg;

private:
    AudioEngine* eng_ = nullptr;
    Project prj_;
    ProjectStore store_;
    char savedAs_[16] = "";  // nazwa, pod ktora projekt lezy na karcie
    bool modified_ = false;
    // Undo: stan przed biezacym gestem + historia
    UndoStack undo_;
    Project committed_;
    bool edited_ = false, pending_ = false;
    char toast_[40] = "";
    float toastT_ = 0;
    void commitStep();
    void resetHistory();
    void validateViews();
    void afterLoad();
    Buttons btn_;
    View* cur_ = &arrange;
    View* next_ = nullptr;
    bool dirty_ = true, quit_ = false;
    int lastPattern_ = -1, lastTrack_ = 0;
    void publish();
};

}  // namespace dp

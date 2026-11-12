// App: wlasciciel projektu i ekranow. Kieruje wejscie do biezacego ekranu, po zmianach
// publikuje migawke projektu do silnika audio. Zakladki (L2/R2): PROJECT | ARRANGE | PIANO ROLL | SYNTH.
#pragma once
#include "audio/engine.h"
#include "io/preset_store.h"
#include "io/project_store.h"
#include "seq/project.h"
#include "seq/undo_stack.h"
#include "ui/arrange.h"
#include "ui/dialogs.h"
#include "ui/piano_roll.h"
#include "ui/project_view.h"
#include "ui/synth_view.h"

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
    void startSplash(float t = 0.f) { splashT_ = t; }  // ekran startowy z nazwa programu (platform/main_sdl.cpp)

    Project& project() { return prj_; }
    AudioEngine* engine() { return eng_; }
    void markDirty() { dirty_ = true; modified_ = true; edited_ = true; }
    void undo();
    void redo();
    void toast(const char* msg);       // krotki komunikat na srodku dolu ekranu
    ProjectStore& store() { return store_; }
    PresetStore& presets() { return presets_; }
    // presety wg silnika sciezki: WAVETABLE, WAVETABLE+ (osobny katalog), TAFFY
    PresetStore& presetsFor(int engine, bool alt) {
        return engine == ENG_TAFFY ? presetsTf_ : (engine == ENG_PEW ? presetsPw_ : (engine == ENG_ELG ? presetsEl_ : (engine == ENG_EVA ? presetsEv_ : (engine == ENG_MBIRA ? presetsMb_ : (engine == ENG_REACTOR ? presetsRx_ : (engine == ENG_OXBOW ? (alt ? presetsObp_ : presetsOb_) : (engine == ENG_SID ? presetsSid_ : (engine == ENG_MONOLIT ? presetsMn_ : (engine == ENG_VAPORA ? presetsVp_ : (engine == ENG_SUPERSAW ? presetsSs_ : (engine == ENG_MIXTUR ? presetsMx_ : (engine == ENG_BRASS ? presetsBr_ : (engine == ENG_HOLLOW ? presetsHs_ : (engine == ENG_BLOOD ? presetsBm_ : (engine == ENG_BLOT ? presetsBl_ : (engine == ENG_AETHER ? presetsAe_ : (engine == ENG_ARC ? presetsAr_ : (engine == ENG_DDDD ? presetsDd_ : (engine == ENG_JUNI ? presetsJn_ : (engine == ENG_OUTATIME ? presetsOt_ : (engine == ENG_GIN ? presetsGn_ : (engine == ENG_PARALLAX ? presetsPx_ : (engine == ENG_LOUNGE ? presetsLg_ : (engine == ENG_FELT ? presetsFt_ : (alt ? presetsWp_ : presets_)))))))))))))))))))))))));
    }
    void openPresets(const char* root) {
        presetsWp_.init(root, "wavetableplus");  // najpierw: przenosi presety WAVETABLE+ ze wspolnego katalogu
        presets_.init(root, "wavetable");
        presetsTf_.init(root, "taffy");
        presetsPw_.init(root, "pew");
        presetsEl_.init(root, "elg");
        presetsEv_.init(root, "eva");
        presetsMb_.init(root, "mbira");
        presetsRx_.init(root, "reactor");
        presetsOb_.init(root, "oxbow");
        presetsObp_.init(root, "oxbowplus");
        presetsSid_.init(root, "sid");
        presetsMn_.init(root, "monolit");
        presetsVp_.init(root, "vapora");
        presetsSs_.init(root, "supersaw");
        presetsMx_.init(root, "mixtur");
        presetsBr_.init(root, "brass");
        presetsHs_.init(root, "hollowsaint");
        presetsBm_.init(root, "bloodmoon");
        presetsBl_.init(root, "blot");
        presetsAe_.init(root, "aether");
        presetsAr_.init(root, "arc");
        presetsDd_.init(root, "dddd");
        presetsJn_.init(root, "juni");
        presetsOt_.init(root, "outatime");
        presetsGn_.init(root, "gin");
        presetsPx_.init(root, "parallax");
        presetsLg_.init(root, "lounge");
        presetsFt_.init(root, "felt");
    }
    bool isModified() const { return modified_; }
    bool isOwnSave(const char* name) const;  // czy ta nazwa to plik biezacego projektu
    bool saveProject();
    bool loadProject(int index, char* err, int errLen);
    void newProject();

    // Nawigacja
    void showArrange();
    void showProject();
    void openPattern(int pattern, int track);
    void openSynth(int track);
    void openNewTrack();
    void openTrackEdit(int track);

    void openNewPattern(int track, int bar);
    void togglePlaySong();

    void loadDemo();  // przykladowy projekt (headless, pierwsze uruchomienie)
    void loadDemoSynthwave();   // utwor "SYNTHWAVE" (headless: render WAV)
    void loadDemoAllEngines();  // utwor "ALL ENGINES" (headless: render WAV)

    ProjectView projectView;
    ArrangeView arrange;
    PianoRoll roll;
    TrackDialog trackDlg;
    PatternDialog patDlg;
    SynthView synth;

private:
    AudioEngine* eng_ = nullptr;
    Project prj_;
    ProjectStore store_;
    PresetStore presets_, presetsWp_, presetsTf_, presetsPw_, presetsEl_, presetsEv_, presetsMb_, presetsRx_, presetsOb_, presetsObp_, presetsSid_, presetsMn_, presetsVp_, presetsSs_, presetsMx_, presetsBr_, presetsHs_, presetsBm_, presetsBl_, presetsAe_, presetsAr_, presetsDd_, presetsJn_, presetsOt_, presetsGn_, presetsPx_, presetsLg_, presetsFt_;
    char savedAs_[16] = "";  // nazwa, pod ktora projekt lezy na karcie
    bool modified_ = false;
    // Undo: stan przed biezacym gestem + historia
    UndoStack undo_, redo_;
    // Pasek narzedzi pod L3 (ARRANGE / PIANO ROLL): B UNDO, A REDO, Y IN/OUT, X LOOP
    bool tools_ = false, inPending_ = false;
    void toolsUpdate(const InputState& in, float dt);
    void toolsRender(Canvas& c);
    void toggleLoop();
    void setInOut();
    Project committed_;
    bool edited_ = false, pending_ = false;
    char toast_[64] = "";
    float toastT_ = 0;
    float splashT_ = -1.f;  // < 0 = brak ekranu startowego
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

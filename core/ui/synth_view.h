// Ekran SYNTH (czwarta zakladka): instrument biezacej sciezki.
//
// Sterowanie:
//   D-pad + ABXY   odsluch: 8 stopni skali C-dur od C4 (bez opisu na ekranie)
//   (silnik WAVETABLE / WAVETABLE+ wybiera sie na karcie ENGINE w oknie TRACK: Select na sciezce, R2)
//   Select         nastepna strona galek:
//                    oryginal:    SOUND -> VOICE -> AMP ENV -> FILTER ENV -> MOD ENV -> LFO 1 -> LFO 2 -> PRESETS
//                    PEW:         SOUND -> BARS -> VOICE -> AMP ENV -> MOD ENV -> LFO 1 -> LFO 2 -> PRESETS (bez drugiej obwiedni)
//                    rozszerzony: OSC 1 -> OSC 2 -> FILTER -> VOICE -> AMP ENV -> FILTER ENV -> MOD ENV -> LFO 1 ->
//                                 LFO 2 -> PRESETS
// MOD ENV: ATTACK, DECAY, SUSTAIN, RELEASE; po L3 biala/czerwona galka = DEST (cel jak w LFO) + AMOUNT (-100..+100).
// VOICE: MODE (POLY/MONO), GLIDE (portamento; L3 = synchronizacja z BPM: podzial 1/1 .. 1/128T), UNISON
//        (rozstrojenie stosu w MONO), AGE (dryf "vintage").
// PRESETS: kategorie (lewo) i presety (prawo); lewo/prawo = kolumna, gora/dol = wybor,
//          A = wczytaj, X = zapisz (nazwa, kategoria = zaznaczona), B = usun (dwa razy), Y = INIT.
//   L3             na stronach obwiedni: czerwona galka RELEASE <-> AMOUNT; na OSC 1: LEVEL <-> SUB;
//                  na stronach LFO: synchronizacja z tempem wl/wyl (RATE = podzial 1/1 .. 1/128T)
//   Start          play / stop aranzacji
//   L2             powrot do PIANO ROLL
// Enkodery (strona SOUND, WAVETABLE): niebieski = WAVE, zielony = DETUNE, bialy = CUTOFF, czerwony = RESONANCE.
// AMP ENV / FILTER ENV: ATTACK, DECAY, SUSTAIN, RELEASE; po L3 zamiast RELEASE — AMOUNT
// (AMP i FILTER: -100..+100 %, srodek = 0; AMP ujemny = obwiednia odwrocona).
// LFO 1/2: RATE (0,05..20 Hz), DEPTH, SHAPE (sinus/trojkat/pila/prostokat/losowy), DEST (cel modulacji).
// OSC 1/2: WAVE, INTERVAL (-24..+24 poltonow), DETUNE (-50..+50 centow), LEVEL. FILTER: CUTOFF, RES, MODE, DRIVE.
#pragma once
#include "seq/project.h"
#include "ui/encoder.h"
#include "ui/view.h"
#include "ui/taffy_screen.h"
#include "ui/pew_screen.h"
#include "ui/wavetable_screen.h"
#include "ui/widgets.h"

namespace dp {

class SynthView : public View {
public:
    enum Kind { K_SOUND, K_OSC1, K_OSC2, K_FILTER, K_AMP, K_FENV, K_LFO1, K_LFO2, K_VOICE, K_PRESETS, K_MENV, K_BARS };
    void open(int track);
    int track() const { return track_; }
    void update(const Buttons& b, const InputState& in, float dt) override;
    void render(Canvas& c) override;
    void setPage(int p) { page_ = p; }  // (testy/headless)
    void setAlt(bool a) { alt_ = a; }
    void setPageKind(int kind);  // (testy/headless) strona po rodzaju (Kind) w biezacym trybie

private:
    int track_ = 0, page_ = 0, octave_ = 4;
    bool alt_ = false;  // L3: czerwona galka = AMOUNT
    RelEncoder stepShape_[2], stepDest_[2], stepDiv_[2], stepInt_[2], stepMode_;
    // tryb stron: 0 oryginal (WAVETABLE, TAFFY), 1 WAVETABLE+, 2 PEW (dodatkowa strona BARS)
    static int pageMode(const Track& t) { return t.engine == ENG_WAVETABLE && t.alt ? 1 : (t.engine == ENG_PEW ? 2 : 0); }
    int pageCount(int pm) const { return pm == 1 ? 10 : 8; }
    RelEncoder stepMDest_, stepGDiv_;
    RelEncoder stepVMode_;
    // przegladarka presetow
    int catSel_ = 0, preSel_ = 0, preFirst_ = 0, focus_ = 1;  // catSel_ 0 = ALL, 1.. = kategorie
    int confirm_ = 0;                                       // 1 = usuwanie, 2 = INIT
    bool savePending_ = false;
    char saveName_[16] = "";
    NameEdit ed_;
    int filtered(int* idx, int max);                        // presety w wybranej kategorii
    void updatePresets(const Buttons& b);
    void renderPresets(Canvas& c);
    Kind kind(int pm) const;
    int held_[8] = {-1, -1, -1, -1, -1, -1, -1, -1};  // nuta grana przez przycisk i
    float dt_ = 0.016f;
    WavetableScreen wt_;
    TaffyScreen tf_;
    PewScreen pw_;
    void releaseAll();
};

}  // namespace dp

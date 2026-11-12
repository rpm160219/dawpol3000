// Projekt: sciezki (max 16 MIDI + 4 audio), pula patternow, klipy na playliscie (arrange).
// Caly projekt to zwykla struktura bez wskaznikow — UI kopiuje ja do silnika audio jako migawke.
#pragma once
#include <stdint.h>
#include "seq/pattern.h"

namespace dp {

static const int MAX_MIDI_TRACKS = 16, MAX_AUDIO_TRACKS = 4;
static const int MAX_TRACKS = MAX_MIDI_TRACKS + MAX_AUDIO_TRACKS;
static const int MAX_PATTERNS = 64, MAX_CLIPS = 512, MAX_SONG_BARS = 256;

enum TrackType : uint8_t { TRACK_MIDI, TRACK_AUDIO };
// Silniki (instrumenty) sciezek MIDI
enum EngineType : uint8_t { ENG_WAVETABLE, ENG_SAW, ENG_COUNT };
const char* engineName(int e);
// LFO: ksztalty i cele modulacji
enum LfoShape : uint8_t { LFO_SINE, LFO_TRI, LFO_SAW, LFO_SQUARE, LFO_RANDOM, LFO_SHAPES };
enum LfoDest : uint8_t {
    DEST_WAVE, DEST_DETUNE, DEST_CUTOFF, DEST_RESO, DEST_PITCH, DEST_VOLUME,
    // tryb rozszerzony: fala, wysokosc i glosnosc kazdego oscylatora
    DEST_W1, DEST_W2, DEST_P1, DEST_P2, DEST_LV1, DEST_LV2,  // (WAVE = obie fale naraz)
    LFO_DESTS
};
// Tryby filtra (tryb rozszerzony WAVETABLE): LP24 = oryginalny ladder
enum FilterMode : uint8_t { FLT_LP24, FLT_LP12, FLT_BP, FLT_HP, FLT_MODES };
const char* filterModeName(int m);
// Tryb rozszerzony WAVETABLE (R3): dwa niezalezne oscylatory + sub + tryby filtra
struct WtOsc {
    int16_t wave = 16384;  // 0..32767 (OSC 1 korzysta z knob[0])
    int8_t semi = 0;       // interwal -24..+24 poltonow
    float cents = 0.f;     // -50..+50
    float level = 1.f;     // 0..1
};
struct WtExt {
    WtOsc osc[2];
    float sub = 0.f;       // sinus oktawe nizej od OSC 1, 0..1
    uint8_t fmode = FLT_LP24;
    float drive = 0.f;     // dodatkowy drive 0..1
    WtExt() { osc[0].cents = -4.f; osc[1].cents = 4.f; }
};
const char* lfoShapeName(int s);
const char* lfoDestName(int d);
// Podzialy synchronizacji LFO z tempem: 1/1 .. 1/128 i triole (T)
static const int LFO_DIVS = 15;
const char* lfoDivName(int i);
float lfoDivBeats(int i);  // dlugosc cyklu w cwiercnutach
struct Lfo {
    uint8_t shape = LFO_SINE, dest = DEST_CUTOFF;
    float rate = 2.f;    // Hz (0.05 .. 20) gdy bez synchronizacji
    float depth = 0.f;   // 0..1
    bool sync = false;   // synchronizacja z tempem (faza przypieta do pozycji w utworze)
    uint8_t div = 3;     // podzial (indeks, domyslnie 1/4)
};

struct Track {
    uint8_t type = TRACK_MIDI;
    uint8_t color = 0;       // indeks w pal::track (0..31)
    char name[12] = "TRACK";
    float volume = 0.8f;
    bool mute = false, solo = false;
    // Instrument (sciezki MIDI): silnik, 4 galki (0..32767), obwiednia A/D/R w sekundach + sustain 0..1
    uint8_t engine = ENG_WAVETABLE;
    int16_t knob[4] = {16384, 8192, 20000, 8192};
    float env[4] = {0.005f, 0.4f, 0.75f, 0.3f};
    float envAmt = 1.f;   // -1..1: jak obwiednia ksztaltuje glosnosc (1 = normalnie, 0 = stala, ujemne = odwrocona)
    // Obwiednia filtra: A, D, S, R + AMOUNT (-1..1, ulamek zakresu CUTOFF)
    float fenv[4] = {0.005f, 0.3f, 0.f, 0.3f};
    float fenvAmt = 0.f;
    Lfo lfo[2];
    bool alt = false;  // tryb rozszerzony WAVETABLE (R3 w SYNTH); false = oryginal 1:1
    WtExt ext;
    int lastPattern = -1;    // ostatnio uzyty pattern (A na pustym polu wstawia go)
};

struct Clip {
    uint8_t track;
    uint8_t pattern;
    uint16_t startBar;
};

struct Project {
    char name[16] = "PROJECT 01";
    float bpm = 120.f;
    float masterVol = 0.8f;
    // Petla utworu (takty, OUT wylacznie). OUT <= IN = caly utwor.
    // Wylaczona: utwor gra raz od poczatku i sie zatrzymuje.
    bool loopOn = true;
    int loopIn = 0, loopOut = 0;
    int nTracks = 0;
    Track tracks[MAX_TRACKS];
    int nPatterns = 0;
    Pattern patterns[MAX_PATTERNS];
    int nClips = 0;
    Clip clips[MAX_CLIPS];

    int countTracks(int type) const;
    bool canAddTrack(int type) const;
    int addTrack(int type, const char* name);         // indeks albo -1
    void removeTrack(int t);                          // usuwa tez jej klipy
    bool audible(int t) const;                        // uwzglednia mute i solo
    int addPattern(const char* name, int bars);       // indeks albo -1
    int clipBars(const Clip& c) const { return patterns[c.pattern].bars(); }
    int clipAt(int track, int bar) const;             // klip obejmujacy takt albo -1
    bool fits(int track, int startBar, int bars, int ignore = -1) const;
    int addClip(int track, int pattern, int startBar);  // -1 gdy nachodzi / brak miejsca
    void removeClip(int i);
    int songBars() const;                              // koniec ostatniego klipu (min 1)
    // Zakres odtwarzania utworu w tickach [start, end) wg petli.
    void songRange(int& start, int& end) const;
    void nextPatternName(char* out, int n) const;
    void nextTrackName(int type, char* out, int n) const;
};

}  // namespace dp

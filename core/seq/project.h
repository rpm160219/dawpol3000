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

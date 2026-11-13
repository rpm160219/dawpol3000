// Projekt: sciezki (max 16 MIDI + 4 audio), pula patternow, klipy na playliscie (arrange).
// Caly projekt to zwykla struktura bez wskaznikow — UI kopiuje ja do silnika audio jako migawke.
#pragma once
#include "synth/gin_dsp.h"
#include <stdint.h>
#include "seq/pattern.h"

namespace dp {

static const int MAX_MIDI_TRACKS = 16, MAX_AUDIO_TRACKS = 4;
static const int MAX_TRACKS = MAX_MIDI_TRACKS + MAX_AUDIO_TRACKS;
static const int MAX_PATTERNS = 64, MAX_CLIPS = 512, MAX_SONG_BARS = 256;

enum TrackType : uint8_t { TRACK_MIDI, TRACK_AUDIO };
// Silniki (instrumenty) sciezek MIDI
enum EngineType : uint8_t { ENG_WAVETABLE, ENG_SAW, ENG_TAFFY, ENG_PEW, ENG_ELG, ENG_EVA, ENG_MBIRA, ENG_REACTOR, ENG_OXBOW, ENG_SID, ENG_MONOLIT, ENG_VAPORA, ENG_SUPERSAW, ENG_MIXTUR, ENG_BRASS, ENG_HOLLOW, ENG_BLOOD, ENG_BLOT, ENG_AETHER, ENG_ARC, ENG_DDDD, ENG_JUNI, ENG_OUTATIME, ENG_GIN, ENG_PARALLAX, ENG_LOUNGE, ENG_FELT, ENG_GRAND, ENG_COUNT };
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
// OXBOW+ (rozszerzony OXBOW): dwa niezalezne oscylatory i ustawienia, ktore w oryginale sa stalymi.
// Domyslne = neutralny glos oryginalu (wtedy OXBOW+ brzmi jak OXBOW z WAVE = SAW).
struct ObExt {
    uint8_t wave[2] = {1, 1};     // 0 OFF, 1 SAW, 2 PULSE, 3 BOTH
    int8_t semi[2] = {0, 0};      // interwal -24..+24 poltonow
    int8_t cents1 = 0;            // strojenie OSC 1 -50..+50 centow (OSC 2: galka DETUNE)
    uint8_t level[2] = {94, 78};  // 0..126 (oryginal: 110^2 >> 7, 100^2 >> 7)
    uint8_t pw[2] = {64, 64};     // szerokosc pulsu 1..127 (64 = 50 %)
    uint8_t noise = 0;            // 0..126
    bool sync = false;            // hard sync OSC 2 do OSC 1
    uint8_t xmod = 0;             // X-MOD OSC 2 -> OSC 1, 0..127
    int8_t envPitch = 0;          // obwiednia filtra -> wysokosc OSC 2, -64..63 (+-24 poltony)
    uint8_t keytrk = 64, velflt = 40, vint = 30;  // sledzenie klawiatury (128 = 100 %), velocity -> filtr, VINTAGE
    uint8_t ftype = 0;            // LP12, HP12, BP12, NOTCH, LP24, SEM
    uint8_t drive = 0;            // 0..127
};
const char* lfoShapeName(int s);
const char* lfoDestName(int d);
const char* destNameFor(int engine, int d);  // TAFFY: WAVE, DCW, ENV, RES; PEW: REG, PERC, DRV, LES; EVA: PRES, FREQ, OXY, COM; MBIRA: STRK, TINE, BOX, BUZZ; REACTOR: COOL, RAD, RODS, HEAT; OXBOW: CUT, RES, ENV, DTUN; SID: WAVE, MOD, CUT, RES; MONOLIT: SHPE, CUT, CONT, HEAT; VAPORA: MRPH, THCK, BRT, MOTN; SUPERSAW: DTUN, MRPH, SUB, DRV
// Podzialy synchronizacji LFO z tempem: 1/1 .. 1/128 i triole (T)
static const int LFO_DIVS = 15;
const char* lfoDivName(int i);
float lfoDivBeats(int i);  // dlugosc cyklu w cwiercnutach
float glideSeconds(const struct Track& t, float bpm);
void engineDefaultKnobs(int engine, int16_t k[4]);  // domyslne galki brzmienia silnika (plugin.json oryginalu)  // czas portamento (z synchronizacja albo bez)
void engineDefaultPush(int engine, int16_t p[3]);
void ginLoadKit(struct Track& t, int kit);       // GIN: dzwieki zestawu fabrycznego
void ginEnsure(struct Track& t);                 // GIN: zestaw 0, gdy sciezka nie ma jeszcze zestawu
bool ginSoundEdited(const struct Track& t, int s);  // GIN: dzwiek rozni sie od fabrycznego  // strona 2 nowego silnika (HOLLOW SAINT: 0, 8192, 21064; inne 0)
struct Lfo {
    uint8_t shape = LFO_SINE, dest = DEST_CUTOFF;
    float rate = 2.f;    // Hz (0.05 .. 20) gdy bez synchronizacji
    float depth = 0.f;   // 0..1
    bool sync = false;   // synchronizacja z tempem (faza przypieta do pozycji w utworze)
    uint8_t div = 3;     // podzial (indeks, domyslnie 1/4)
};

// Kategorie presetow (ikona + nazwa w przegladarce)
enum PresetCat : uint8_t { CAT_BASS, CAT_LEAD, CAT_PAD, CAT_KEYS, CAT_PLUCK, CAT_BELL, CAT_FX, CAT_DRUMS, CAT_OTHER, PRESET_CATS };
const char* presetCatName(int c);

// ---- MIKSER ----
static const int MIX_AUX = 6;      // szyny AUX (wysylki A..F)
static const int MIX_INSERTS = 4;  // inserty na kanal
enum FxId : uint8_t { FX_NONE, FX_JUNI_CHORUS, FX_SISTER, FX_SNOWGLOBE, FX_RACKVERB, FX_TAPE, FX_FATHER, FX_STUTTER, FX_CRUSH, FX_ENTE, FX_COUNT };  // efekty (inserty i AUX)
enum CompType : uint8_t { COMP_OPTO, COMP_VCA, COMP_FET, COMP_TYPES };  // modele kompresora kanalu (w stylu LA-2A, dbx 160, 1176)
// EQ 3-pasmowy: LOW = polka niska, MID = dzwon, HIGH = polka wysoka; wzmocnienia w dB, czestotliwosci w Hz
struct MixEq {
    bool on = true, max = false;  // max: MAX EQ (szerszy zakres +-18 dB i lekkie nasycenie)
    float gain[3] = {0.f, 0.f, 0.f};
    float freq[3] = {100.f, 1000.f, 10000.f};
    float q[3] = {.7f, 1.f, .7f};
    float out = 0.f;  // dB
    bool flat() const { return !max && gain[0] == 0.f && gain[1] == 0.f && gain[2] == 0.f && out == 0.f; }
};
// Kompresor kanalu: p[] 0..1, znaczenie wg modelu (zob. core/audio/mixer.cpp)
//   OPTO: PEAK REDUCTION, GAIN, LIMIT (>= .5), MIX     VCA: THRESHOLD, RATIO, OUTPUT, OVEREASY (>= .5)
//   FET: INPUT, OUTPUT, ATTACK, RELEASE, RATIO (5 krokow: 4 8 12 20 ALL)
struct MixComp {
    bool on = false;
    uint8_t type = COMP_OPTO;
    float p[COMP_TYPES][5] = {{.35f, .4f, 0.f, 1.f, 0.f}, {.6f, .3f, .5f, 1.f, 0.f}, {.4f, .5f, .5f, .5f, 0.f}};
};
struct FxSlot { uint8_t fx = FX_NONE; bool bypass = false; int16_t w[8] = {}; };  // bypass: efekt omijany (stan zostaje)  // efekt + slowa galek (0..3 strona 1, 5..7 strona 2, jak OP-1)
struct AuxBus { FxSlot fx; MixEq eq; float level = .8f; };
// Kompresor szyny master (w stylu SSL G): prog, ratio 2/4/10, atak 0,1..30 ms, release 0,1..1,2 s / AUTO, makeup
struct BusComp { bool on = false; float thresh = .6f; uint8_t ratio = 1, attack = 3, release = 4; float makeup = 0.f; };

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
    // Obwiednia modulacji (MOD ENV): A, D, S, R + AMOUNT (-1..1) i cel (jak w LFO)
    float menv[4] = {0.001f, 0.2f, 0.f, 0.2f};
    float menvAmt = 0.f;
    uint8_t menvDest = DEST_PITCH;
    Lfo lfo[2];
    bool alt = false;  // tryb rozszerzony WAVETABLE (R3 w SYNTH); false = oryginal 1:1
    WtExt ext;
    ObExt obx;  // OXBOW+ (alt przy silniku OXBOW)
    int16_t push[3] = {0, 0, 0};  // PEW: wysuniecie suwakow niskich / srodkowych / wysokich (0..32767); EVA: push[2] = oktawa sygnalu (slowo 7); MBIRA: RING, DOUBLE, WAH (slowa 5..7); REACTOR: push[2] = trym presetu (slowo 7); OXBOW: WAVE, FILTER, DRIVE (slowa 5..7); SID: push[2] = trym presetu (slowo 7); MONOLIT: BODY, VEL, AGE (slowa 5..7); VAPORA: CHARACTER, INTERVAL, SPEED (slowa 5..7; 0 = 8192 jak w oryginale); FELT: DETUNE, WORN, BODY; GRAND: UNISON, GROWL, BOARD; OUTATIME: SPREAD, TONE, FLUX; JUNI: RESONANCE, HPF, PWM RATE; DDDD: FILTER (LP -> BP -> HP), SPREAD, RATE; ARC: RATIO (0 = 1:1), COUPLING, BRIGHTNESS (0 = neutralnie); AETHER: DEAD ZONE, TILT RANGE, VIB RATE (slowa 5..7; 0 = 8192); BLOT: WAVE, ACCENT (0 = 20000), DRIVE (slowa 5..7); HOLLOW SAINT: klucz burdonu, wiek tasmy, tempo uderzen (slowa 5..7; domyslnie 0, 8192, 21064); MIXTUR: F1, F2, RES (slowa 5..7; 0 = 8192 jak w oryginale); SUPERSAW: filtr DAW-POL — push[0] = zamkniecie (0 = otwarty, CUTOFF = 32767 - push[0]), push[1] = RES, push[2] = typ 0 LP24, 1 LP12, 2 BP, 3 HP
    // Glosy: POLY/MONO, portamento (s), unison (rozstrojenie stosu w MONO, centy 0..50), AGE 0..1 (dryf "vintage")
    bool mono = false;
    float glide = 0.f, unison = 0.f, age = 0.f;
    bool glideSync = false;  // GLIDE zsynchronizowany z tempem (czas = podzial nutowy przy biezacym BPM)
    uint8_t glideDiv = 7;    // indeks podzialu (jak w LFO), domyslnie 1/16
    bool movSync = false;    // ELG: ruch (MOVEMENT) zsynchronizowany z tempem — cykl = podzial movDiv
    uint8_t movDiv = 0;      // indeks podzialu (jak w LFO), domyslnie 1/1
    int8_t octave = 0;       // OCTAVE instrumentu -3..+3 (wszystkie silniki; przesuwa wysokosc dzwieku)
    int16_t tilt[3] = {0, 0, 0};  // AETHER: wirtualne przechylenie — PITCH (-32767..32767 = przod / tyl), VOL (prawy bok), SHAKE
    bool relic = false;      // HOLLOW SAINT: tasma Lo-Fi Relic wlaczona (w OP-1: strona 2 zielonej galki)
    uint8_t sun = 0;         // VAPORA: wzor slonca na ekranie 0..6 (tylko obraz)
    int8_t ginKit = -1;      // GIN: zestaw fabryczny 0..7 (-1 = jeszcze nie ustawiony: zestaw 0)
    GinSound gin[GIN_KIT_SOUNDS];  // GIN: biezace dzwieki zestawu (konce A/B, MORPH, strona edycji)
    char preset[16] = "";  // nazwa ostatnio wczytanego/zapisanego presetu (tylko do wyswietlania)
    int lastPattern = -1;    // ostatnio uzyty pattern (A na pustym polu wstawia go)
    // mikser kanalu: panorama -1..1, EQ, kompresor, 2 inserty, 6 wysylek AUX (po tlumiku)
    float pan = 0.f;
    MixEq eq;
    MixComp comp;
    FxSlot ins[MIX_INSERTS];
    float send[MIX_AUX] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
};

struct Clip {
    uint8_t track;
    uint8_t pattern;
    uint16_t startBar;
};

struct Project {
    Project();  // domyslny mikser: AUX A = SISTER (poglos), wysylki 0
    char name[16] = "PROJECT 01";
    float bpm = 120.f;
    float masterVol = 0.8f;
    // Petla utworu (takty, OUT wylacznie). OUT <= IN = caly utwor.
    // Wylaczona: utwor gra raz od poczatku i sie zatrzymuje.
    bool loopOn = true;
    int loopIn = 0, loopOut = 0;
    int nTracks = 0;
    Track tracks[MAX_TRACKS];
    AuxBus aux[MIX_AUX];  // powroty AUX A..F
    BusComp bus;          // kompresor szyny master
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

// Zestawy fabryczne presetow (po jednej linii na preset, w zapisie instrumentu z pliku projektu — parseInstrument).
// Klucze: k WAVE DTUN|DCW CUT|ENV RES | env A D S R | envamt X | fenv A D S R AMOUNT | menv A D S R AMOUNT CEL |
//         lfo1/lfo2 KSZTALT HZ GLEB CEL SYNC PODZIAL | voice MONO GLIDE UNISON AGE | gsync 1 PODZIAL |
//         (WAVETABLE+) alt 1, o1/o2 FALA INTERWAL CENTY POZIOM, sub X, fmode M, drv D.
// Fale WAVETABLE: SINE 0, WARM 4681, TRI 9362, SAW 14043, SQUARE 18724, PULSE 23405, VOCAL 28086, DIGITAL 32767.
// Ksztalty TAFFY: SAW 0, SQUARE 10923, PULSE 21846, SAW-PULSE 32767. Cele: 0 WAVE 1 DTUN/DCW 2 CUT/ENV 3 RES 4 PTCH
// 5 VOL 6 W1 7 W2 8 P1 9 P2 10 LV1 11 LV2. Ksztalty LFO: 0 SINE 1 TRI 2 SAW 3 SQR 4 RND. Podzialy: 0 1/1 1 1/2 3 1/4 5 1/8 7 1/16.
// ver = wersja zestawu w danym katalogu; na karcie dopisywane sa tylko presety z wersji nowszych niz znacznik.
#include "io/preset_store.h"

namespace dp {

struct FactoryPreset { const char* name; int cat; int ver; const char* inst; };

static const FactoryPreset kWavetable[] = {
    {"INIT", CAT_OTHER, 1, ""},
    {"ACID", CAT_BASS, 1, "k 14000 0 5000 26000 voice 1 0.09 0 0 fenv 0.002 0.18 0 0.1 0.55 env 0.005 0.25 0.6 0.05"},
    {"VOCAL LEAD", CAT_LEAD, 1, "k 28000 6000 22000 12000 voice 1 0.06 0 0 lfo1 0 5.5 0.012 4 0 0"},
    {"WARM PAD", CAT_PAD, 1, "k 4700 22000 16000 6000 env 0.6 1.5 0.8 1.5 lfo1 0 0.3 0.15 0 0 0 voice 0 0 0 0.3"},
    {"GLASS KEYS", CAT_KEYS, 1, "k 23400 4000 24000 8000 env 0.002 0.9 0.25 0.5"},
    {"PLUCK", CAT_PLUCK, 1, "k 14000 8192 6000 10000 env 0.001 0.35 0 0.25 fenv 0.001 0.15 0 0.2 0.6"},
    {"WOBBLE", CAT_FX, 1, "k 16000 8192 10000 20000 voice 1 0 0 0 lfo1 0 2 0.45 2 1 5"},
    // ---- wersja 3: 30 nowych ----
    {"ROUND BASS", CAT_BASS, 3, "k 2000 3000 12000 6000 voice 1 0.02 0 0 env 0.003 0.5 0.7 0.1"},
    {"FILTER BASS", CAT_BASS, 3, "k 14043 2000 7000 18000 voice 1 0 0 0 fenv 0.002 0.3 0.1 0.1 0.45 env 0.002 0.4 0.7 0.08"},
    {"PLUCK BASS", CAT_BASS, 3, "k 18724 0 9000 12000 fenv 0.001 0.12 0 0.1 0.5 env 0.001 0.3 0.3 0.08"},
    {"MONO SAW BASS", CAT_BASS, 3, "k 14043 10000 14000 8000 voice 1 0.05 0 0.2 env 0.003 0.3 0.8 0.1"},
    {"DEEP SINE", CAT_BASS, 3, "k 0 0 32767 0 voice 1 0.02 0 0 env 0.005 0.5 0.9 0.15"},
    {"RUBBER BASS", CAT_BASS, 3, "k 9362 4000 10000 20000 voice 1 0.07 0 0 menv 0.001 0.08 0 0.05 0.12 4 fenv 0.002 0.25 0.2 0.1 0.35 env 0.002 0.3 0.7 0.1"},
    {"SINE LEAD", CAT_LEAD, 3, "k 0 2000 32767 0 voice 1 0.08 0 0 lfo1 0 5.5 0.012 4 0 0 env 0.01 0.3 0.9 0.3"},
    {"SQUARE LEAD", CAT_LEAD, 3, "k 18724 8000 20000 10000 voice 1 0.05 0 0 env 0.005 0.3 0.85 0.25"},
    {"SOFT LEAD", CAT_LEAD, 3, "k 4681 10000 16000 6000 env 0.08 0.5 0.8 0.4 lfo1 0 4.5 0.01 4 0 0"},
    {"SCREAMER", CAT_LEAD, 3, "k 14043 12000 22000 28000 voice 1 0.04 0 0 lfo1 0 6 0.008 4 0 0 env 0.005 0.3 0.9 0.2"},
    {"WHISTLE", CAT_LEAD, 3, "k 0 0 18000 30000 voice 1 0.1 0 0 env 0.05 0.3 0.8 0.4"},
    {"VOX LEAD", CAT_LEAD, 3, "k 28086 9000 24000 14000 voice 1 0.06 0 0.2 lfo1 1 0.6 0.15 0 0 0 env 0.02 0.4 0.85 0.3"},
    {"DARK PAD", CAT_PAD, 3, "k 9362 20000 10000 8000 env 1 2 0.85 2 voice 0 0 0 0.4"},
    {"GLASS PAD", CAT_PAD, 3, "k 23405 16000 26000 12000 env 0.8 1.5 0.7 2 lfo1 0 0.2 0.2 0 0 0"},
    {"SLOW SWELL", CAT_PAD, 3, "k 14043 18000 8000 10000 env 2.5 2 0.9 2.5 fenv 2 2 0.6 2 0.5"},
    {"CHOIR", CAT_PAD, 3, "k 28086 24000 18000 9000 env 0.6 1.5 0.85 1.8 lfo1 0 4.8 0.006 4 0 0 voice 0 0 0 0.3"},
    {"SHIMMER PAD", CAT_PAD, 3, "k 32767 26000 28000 6000 env 1.2 2 0.8 2.5 lfo1 1 0.15 0.3 0 0 0 lfo2 0 7 0.15 5 0 0"},
    {"DRONE", CAT_PAD, 3, "k 4681 30000 12000 14000 env 2 1 1 3 lfo1 0 0.08 0.3 2 0 0 voice 0 0 0 0.6"},
    {"E-PIANO", CAT_KEYS, 3, "k 4681 6000 20000 4000 env 0.002 1.5 0.2 0.5 fenv 0.002 0.6 0.2 0.3 0.25"},
    {"SOFT KEYS", CAT_KEYS, 3, "k 9362 8000 16000 2000 env 0.005 1 0.4 0.6"},
    {"ORGAN", CAT_KEYS, 3, "k 4681 4000 26000 0 env 0.003 0.2 1 0.08 lfo1 0 6.5 0.1 5 0 0"},
    {"CLAV", CAT_KEYS, 3, "k 23405 2000 14000 16000 fenv 0.001 0.2 0 0.1 0.5 env 0.001 0.5 0.1 0.15"},
    {"HARP", CAT_PLUCK, 3, "k 9362 3000 18000 4000 env 0.001 1.2 0 0.8 fenv 0.001 0.4 0 0.4 0.3"},
    {"KALIMBA", CAT_PLUCK, 3, "k 0 0 22000 10000 env 0.001 0.6 0 0.5 menv 0.001 0.02 0 0.02 0.08 4"},
    {"MUTED PLUCK", CAT_PLUCK, 3, "k 14043 4000 6000 8000 env 0.001 0.18 0 0.1 fenv 0.001 0.08 0 0.05 0.4"},
    {"TUBULAR", CAT_BELL, 3, "k 32767 4000 24000 12000 env 0.001 2.5 0 2 lfo1 0 4 0.004 4 0 0"},
    {"MUSIC BOX", CAT_BELL, 3, "k 0 0 30000 16000 env 0.001 0.9 0 0.7 menv 0.001 0.03 0 0.03 0.05 4"},
    {"SIREN", CAT_FX, 3, "k 18724 8192 22000 8000 lfo1 1 0.5 0.4 4 0 0 env 0.01 0.3 1 0.3"},
    {"LASER", CAT_FX, 3, "k 14043 0 26000 22000 menv 0.001 0.25 0 0.1 0.9 4 env 0.001 0.3 0 0.1"},
    {"RANDOM BLIPS", CAT_FX, 3, "k 23405 8192 20000 18000 lfo1 4 8 0.25 4 1 7 env 0.002 0.2 0.5 0.1"},
};

static const FactoryPreset kWavetablePlus[] = {
    {"INIT", CAT_OTHER, 1, "alt 1"},
    {"SUB BASS", CAT_BASS, 1, "alt 1 k 9400 8192 9000 6000 o2 16384 -12 4 0.6 sub 0.7 voice 1 0.04 0 0 env 0.002 0.3 0.7 0.08"},
    {"SUPERSAW", CAT_LEAD, 1, "alt 1 k 14000 8192 26000 4000 o2 14000 0 4 1 voice 1 0.03 22 0 env 0.005 0.4 0.9 0.25"},
    {"OLD PROPHET", CAT_PAD, 1, "alt 1 k 14000 8192 15000 9000 o1 16384 0 -7 1 o2 18700 0 7 1 env 0.25 1 0.7 0.8 fenv 0.3 1.2 0.2 0.8 0.3 voice 0 0 0 0.7"},
    {"DIGI BELL", CAT_BELL, 1, "alt 1 k 32767 8192 28000 8192 o2 0 19 4 0.5 env 0.001 1.8 0 1.5"},
    {"HOOVER", CAT_LEAD, 1, "alt 1 k 14043 8192 24000 6000 o1 16384 0 -9 1 o2 14043 -12 9 0.8 fmode 1 voice 1 0 30 0 gsync 1 7 menv 0.001 0.25 0 0.2 -0.18 4 env 0.005 0.4 0.9 0.3"},
    {"REESE", CAT_BASS, 1, "alt 1 k 14043 8192 11000 7000 o1 16384 0 -14 1 o2 14043 0 14 1 sub 0.5 voice 1 0.03 0 0 lfo1 0 0.25 0.08 7 0 0 env 0.003 0.4 1 0.1"},
    {"KICK DRUM", CAT_FX, 1, "alt 1 k 0 8192 20000 0 o1 16384 0 0 1 o2 16384 0 4 0 sub 0.6 drv 0.4 voice 1 0 0 0 menv 0.001 0.07 0 0.05 1 4 env 0.001 0.35 0 0.1"},
    {"SYNC PLUCK", CAT_PLUCK, 1, "alt 1 k 18724 8192 9000 14000 o2 14043 12 4 0.7 fmode 2 fenv 0.001 0.18 0 0.2 0.6 env 0.001 0.4 0 0.3"},
    {"SOFT STRINGS", CAT_PAD, 1, "alt 1 k 14043 8192 17000 3000 o1 16384 0 -8 1 o2 14043 0 8 1 fmode 1 voice 0 0 0 0.5 env 0.5 1.5 0.85 1.4 lfo1 0 5.2 0.008 4 0 0"},
    {"GATED KEYS", CAT_KEYS, 1, "alt 1 k 4681 8192 19000 5000 o2 23405 12 4 0.4 env 0.01 0.4 0.9 0.4 lfo1 3 2 0.85 5 1 7"},
    {"PORTA LEAD", CAT_LEAD, 1, "alt 1 k 28086 8192 21000 9000 o2 14043 7 4 0.6 fmode 1 voice 1 0 0 0.3 gsync 1 5 env 0.005 0.4 0.85 0.3 lfo1 0 5 0.01 4 0 0"},
    {"DRIFT PAD", CAT_PAD, 1, "alt 1 k 4681 8192 14000 8000 o2 28086 7 4 0.7 voice 0 0 0 1 env 0.8 2 0.8 2 lfo1 1 0.12 0.25 6 0 0 lfo2 0 2 0.2 7 1 0"},
    {"BELL TREE", CAT_BELL, 1, "alt 1 k 32767 8192 4000 5000 o1 16384 0 -4 0.45 o2 0 24 4 0.35 fmode 3 env 0.001 1.4 0 1.6 menv 0.001 0.6 0 0.2 0.4 6"},
    {"NOISE SWEEP", CAT_FX, 1, "alt 1 k 32767 8192 6000 9000 o1 16384 0 -40 0.3 o2 32767 19 40 0.25 fmode 3 lfo1 2 2 0.6 2 1 1 env 0.2 0.4 0.6 0.8"},
    // ---- wersja 2: 30 nowych ----
    {"FIFTH BASS", CAT_BASS, 2, "alt 1 k 14043 8192 9000 8000 o2 14043 7 4 0.6 sub 0.5 voice 1 0.02 0 0 env 0.003 0.4 0.7 0.1"},
    {"OCTAVE BASS", CAT_BASS, 2, "alt 1 k 18724 8192 10000 6000 o2 18724 -12 4 0.8 voice 1 0.03 0 0 env 0.002 0.3 0.8 0.1"},
    {"GROWL", CAT_BASS, 2, "alt 1 k 28086 8192 9000 20000 o2 14043 0 10 1 drv 0.6 voice 1 0 0 0 lfo1 0 3 0.3 6 1 7 env 0.003 0.3 0.9 0.1"},
    {"DUB BASS", CAT_BASS, 2, "alt 1 k 0 8192 14000 4000 o2 4681 -12 4 0.5 sub 0.8 voice 1 0.04 0 0 menv 0.001 0.1 0 0.1 0.2 4 env 0.002 0.6 0.4 0.2"},
    {"BP BASS", CAT_BASS, 2, "alt 1 k 14043 8192 12000 16000 fmode 2 o2 23405 0 6 0.7 sub 0.6 voice 1 0 0 0 fenv 0.002 0.2 0.1 0.1 0.4 env 0.002 0.3 0.7 0.1"},
    {"SUB WOBBLE", CAT_BASS, 2, "alt 1 k 14043 8192 8000 18000 sub 0.7 voice 1 0 0 0 lfo1 0 2 0.4 2 1 6 env 0.003 0.3 0.9 0.1"},
    {"TWIN LEAD", CAT_LEAD, 2, "alt 1 k 18724 8192 22000 8000 o1 0 0 -6 1 o2 14043 0 6 0.9 voice 1 0.05 0 0 env 0.005 0.3 0.85 0.3"},
    {"BRASS LEAD", CAT_LEAD, 2, "alt 1 k 14043 8192 12000 6000 o2 14043 0 7 1 fenv 0.05 0.4 0.4 0.3 0.4 env 0.04 0.4 0.8 0.3"},
    {"OCTAVE LEAD", CAT_LEAD, 2, "alt 1 k 23405 8192 24000 9000 o2 14043 12 4 0.6 voice 1 0.06 0 0 lfo1 0 5.5 0.01 4 0 0"},
    {"SAW STACK", CAT_LEAD, 2, "alt 1 k 14043 8192 25000 4000 o2 14043 0 15 1 voice 1 0.02 30 0 env 0.005 0.4 0.9 0.3"},
    {"BP LEAD", CAT_LEAD, 2, "alt 1 k 28086 8192 20000 20000 fmode 2 o2 18724 7 4 0.7 voice 1 0.05 0 0 drv 0.3"},
    {"MONO FIFTHS", CAT_LEAD, 2, "alt 1 k 14043 8192 20000 10000 o2 14043 7 4 1 voice 1 0.08 12 0 env 0.005 0.4 0.85 0.3"},
    {"WIDE PAD", CAT_PAD, 2, "alt 1 k 14043 8192 18000 5000 o1 0 0 -12 1 o2 14043 0 12 1 env 0.8 2 0.85 2 voice 0 0 0 0.4"},
    {"STRINGS PLUS", CAT_PAD, 2, "alt 1 k 14043 8192 16000 3000 o1 0 0 -10 1 o2 14043 12 10 0.5 fmode 1 env 0.5 1.5 0.85 1.6 lfo1 0 5 0.006 4 0 0"},
    {"VOCAL PAD", CAT_PAD, 2, "alt 1 k 28086 8192 20000 8000 o2 23405 0 8 0.6 env 0.7 1.5 0.8 2 lfo1 1 0.25 0.2 6 0 0"},
    {"MOTION PAD", CAT_PAD, 2, "alt 1 k 9362 8192 14000 12000 o2 28086 7 4 0.6 env 1 2 0.85 2 lfo1 0 0.2 0.3 6 0 0 lfo2 1 0.13 0.3 7 0 0"},
    {"EVOLVER", CAT_PAD, 2, "alt 1 k 4681 8192 12000 14000 o2 32767 12 4 0.5 env 1.5 2 0.8 2.5 lfo1 2 0.1 0.4 6 1 0 lfo2 0 0.3 0.2 2 0 0 voice 0 0 0 0.5"},
    {"AIR PAD", CAT_PAD, 2, "alt 1 k 32767 8192 9000 6000 fmode 3 o1 16384 0 -5 0.5 o2 28086 12 5 0.4 env 1.2 2 0.8 2.5 lfo1 0 0.15 0.25 2 0 0"},
    {"STAB", CAT_KEYS, 2, "alt 1 k 14043 8192 18000 12000 o2 14043 0 8 1 fenv 0.001 0.15 0 0.1 0.4 env 0.001 0.3 0 0.15"},
    {"CHORD ORGAN", CAT_KEYS, 2, "alt 1 k 4681 8192 24000 0 o2 4681 7 2 0.7 sub 0.4 env 0.003 0.2 1 0.08"},
    {"POLY KEYS", CAT_KEYS, 2, "alt 1 k 9362 8192 20000 6000 o2 18724 12 4 0.4 env 0.003 1 0.4 0.5 fenv 0.003 0.5 0.3 0.3 0.2"},
    {"SYNTH PIANO", CAT_KEYS, 2, "alt 1 k 4681 8192 22000 4000 o2 23405 12 4 0.3 env 0.002 1.6 0.15 0.6 fenv 0.002 0.8 0.1 0.4 0.3 voice 0 0 0 0.2"},
    {"BP PLUCK", CAT_PLUCK, 2, "alt 1 k 14043 8192 12000 18000 fmode 2 o2 23405 12 4 0.6 fenv 0.001 0.2 0 0.1 0.5 env 0.001 0.4 0 0.25"},
    {"STAB PLUCK", CAT_PLUCK, 2, "alt 1 k 18724 8192 10000 14000 o2 14043 -12 4 0.6 fenv 0.001 0.12 0 0.08 0.6 env 0.001 0.25 0 0.12"},
    {"DIGI PLUCK", CAT_PLUCK, 2, "alt 1 k 32767 8192 18000 10000 o2 28086 19 4 0.4 fmode 1 env 0.001 0.5 0 0.3 menv 0.001 0.2 0 0.1 0.4 6"},
    {"BELL FIFTH", CAT_BELL, 2, "alt 1 k 0 8192 26000 8000 o2 32767 7 4 0.4 env 0.001 2 0 1.8"},
    {"CRYSTAL", CAT_BELL, 2, "alt 1 k 32767 8192 12000 8000 o2 0 24 4 0.5 fmode 3 env 0.001 1.5 0 1.5 lfo1 0 6 0.004 4 0 0"},
    {"RISER", CAT_FX, 2, "alt 1 k 14043 8192 8000 18000 o2 14043 7 30 1 env 3 1 1 0.5 fenv 3 1 1 0.5 0.7 menv 4 1 1 1 0.5 4"},
    {"ZAP", CAT_FX, 2, "alt 1 k 32767 8192 24000 20000 menv 0.001 0.15 0 0.1 1 4 env 0.001 0.2 0 0.1"},
    {"RANDOM SH", CAT_FX, 2, "alt 1 k 18724 8192 18000 22000 fmode 2 lfo1 4 8 0.35 2 1 7 env 0.002 0.3 0.9 0.2"},
};

static const FactoryPreset kTaffy[] = {
    {"INIT", CAT_OTHER, 1, ""},
    {"CZ BRASS", CAT_LEAD, 1, "k 0 9000 22000 0 env 0.03 0.6 0.7 0.25 fenv 0.04 0.5 0.3 0.3 0.2"},
    {"CZ BASS", CAT_BASS, 1, "k 10923 6000 26000 0 voice 1 0.03 0 0 env 0.002 0.4 0.6 0.08"},
    {"DIGI PIANO", CAT_KEYS, 1, "k 32767 4000 20000 0 env 0.002 1.2 0.1 0.6"},
    {"SQUARE ORGAN", CAT_KEYS, 1, "k 10923 12000 0 0 env 0.005 0.3 1 0.1"},
    {"PULSE PLUCK", CAT_PLUCK, 1, "k 21846 2000 30000 0 env 0.001 0.3 0 0.25"},
    {"BUBBLEGUM PAD", CAT_PAD, 1, "k 16384 8000 6000 12000 voice 0 0 0 0.4 env 0.7 1.5 0.8 1.5 lfo1 0 0.2 0.2 0 0 0"},
    {"CZ BELLS", CAT_BELL, 1, "k 32767 14000 16000 18000 env 0.001 1.6 0 1.4"},
    {"RESO SWEEP", CAT_FX, 1, "k 0 3000 0 26000 lfo1 1 2 0.4 1 1 1 env 0.005 0.4 0.9 0.5"},
    {"SYNC LEAD", CAT_LEAD, 1, "k 0 7000 12000 20000 voice 1 0 15 0 gsync 1 7 lfo1 0 5.5 0.01 4 0 0"},
    {"WOBBLE CZ", CAT_FX, 1, "k 10923 4000 0 8000 voice 1 0 0 0 lfo1 0 2 0.5 1 1 5"},
    // ---- wersja 2: 30 nowych ----
    {"CZ SUB", CAT_BASS, 2, "k 0 1000 8000 0 voice 1 0.02 0 0 env 0.002 0.4 0.8 0.1"},
    {"SAW BASS CZ", CAT_BASS, 2, "k 0 9000 18000 0 voice 1 0.03 0 0 env 0.002 0.35 0.7 0.08"},
    {"RESO BASS", CAT_BASS, 2, "k 0 4000 10000 20000 voice 1 0 0 0 fenv 0.002 0.25 0.1 0.1 0.3 env 0.002 0.3 0.7 0.1"},
    {"PLUCKY BASS", CAT_BASS, 2, "k 10923 3000 30000 0 env 0.001 0.25 0.2 0.08"},
    {"MONO CZ", CAT_BASS, 2, "k 21846 6000 12000 4000 voice 1 0.06 0 0.2 env 0.003 0.4 0.8 0.1"},
    {"DCW BASS", CAT_BASS, 2, "k 10923 2000 0 0 voice 1 0 0 0 fenv 0.002 0.3 0 0.1 0.6 env 0.002 0.4 0.6 0.1"},
    {"CZ LEAD", CAT_LEAD, 2, "k 0 12000 16000 6000 voice 1 0.05 0 0 lfo1 0 5.5 0.01 4 0 0 env 0.005 0.3 0.85 0.25"},
    {"PULSE LEAD", CAT_LEAD, 2, "k 21846 10000 12000 0 voice 1 0.04 0 0 env 0.005 0.3 0.85 0.3"},
    {"RESO LEAD", CAT_LEAD, 2, "k 0 8000 8000 24000 voice 1 0.06 0 0 lfo1 1 0.4 0.15 1 0 0"},
    {"SOFT CZ", CAT_LEAD, 2, "k 10923 4000 6000 0 env 0.05 0.5 0.8 0.4"},
    {"GLIDE CZ", CAT_LEAD, 2, "k 32767 9000 10000 8000 voice 1 0 0 0 gsync 1 5 env 0.005 0.3 0.9 0.2"},
    {"BRIGHT LEAD", CAT_LEAD, 2, "k 0 20000 20000 14000 voice 1 0.03 12 0 env 0.005 0.3 0.9 0.25"},
    {"CZ STRINGS", CAT_PAD, 2, "k 0 7000 4000 0 env 0.5 1.5 0.85 1.5 voice 0 0 0 0.4 lfo1 0 5 0.006 4 0 0"},
    {"CZ CHOIR", CAT_PAD, 2, "k 21846 6000 4000 10000 env 0.7 1.5 0.8 1.8 lfo1 1 0.3 0.15 3 0 0"},
    {"GLASS CZ", CAT_PAD, 2, "k 32767 10000 8000 16000 env 0.6 2 0.7 2"},
    {"SLOW DCW", CAT_PAD, 2, "k 10923 2000 0 0 env 1 1.5 0.9 2 fenv 2.5 2 0.6 2 0.6"},
    {"AGED PAD", CAT_PAD, 2, "k 0 6000 4000 8000 voice 0 0 0 1 env 0.9 2 0.8 2.2"},
    {"RESO PAD", CAT_PAD, 2, "k 10923 4000 6000 22000 env 1 2 0.8 2 lfo1 0 0.15 0.2 3 0 0"},
    {"CZ EP", CAT_KEYS, 2, "k 32767 3000 18000 4000 env 0.002 1.4 0.15 0.5"},
    {"CZ ORGAN", CAT_KEYS, 2, "k 10923 16000 0 0 env 0.003 0.2 1 0.08 lfo1 0 6 0.1 5 0 0"},
    {"CLAVI CZ", CAT_KEYS, 2, "k 21846 8000 26000 8000 env 0.001 0.4 0.1 0.12"},
    {"HARPSI", CAT_KEYS, 2, "k 0 14000 22000 6000 env 0.001 0.9 0 0.35"},
    {"CZ HARP", CAT_PLUCK, 2, "k 32767 4000 24000 0 env 0.001 1.2 0 0.8"},
    {"RESO PLUCK", CAT_PLUCK, 2, "k 0 2000 14000 24000 env 0.001 0.35 0 0.25 fenv 0.001 0.2 0 0.1 0.4"},
    {"KOTO", CAT_PLUCK, 2, "k 21846 6000 28000 6000 env 0.001 0.8 0 0.4 menv 0.001 0.04 0 0.03 0.04 4"},
    {"CZ CHIME", CAT_BELL, 2, "k 32767 12000 12000 20000 env 0.001 2 0 1.8"},
    {"GLOCK", CAT_BELL, 2, "k 21846 14000 20000 26000 env 0.001 1 0 0.9"},
    {"CZ ZAP", CAT_FX, 2, "k 0 16000 30000 18000 menv 0.001 0.18 0 0.1 1 4 env 0.001 0.25 0 0.1"},
    {"SWEEPER", CAT_FX, 2, "k 10923 2000 0 20000 lfo1 2 0.5 0.6 1 1 1 env 0.01 0.4 1 0.4"},
    {"STEPS", CAT_FX, 2, "k 21846 8000 0 12000 lfo1 4 4 0.4 1 1 7 env 0.002 0.3 0.9 0.2"},
};

// PEW: k REGISTER PERC DRIVE LESLIE; REGISTER 0 = 008000000, 8192 = 888000000, 16384 = 888800000,
// 24576 = 888808008, 32767 = 888888888; PERC <3277 off, do 14746 miekka 2., do 24576 ostra 3., wyzej klik;
// DRIVE do 14746 = skaner/chorus, wyzej przester; LESLIE <10923 stop, <21845 wolno, wyzej szybko.
// push NISKIE SRODKOWE WYSOKIE = wysuniecie grup suwakow.
static const FactoryPreset kPew[] = {
    {"INIT", CAT_OTHER, 1, ""},
    {"JAZZ CLUB", CAT_KEYS, 1, "k 8192 20000 7000 16384 env 0.002 0.3 1 0.08"},
    {"GOSPEL", CAT_KEYS, 1, "k 16384 12000 12000 28000 env 0.002 0.3 1 0.1"},
    {"FULL ORGAN", CAT_KEYS, 1, "k 32767 0 20000 28000 env 0.002 0.3 1 0.1"},
    {"FLUTE 8", CAT_KEYS, 1, "k 0 0 4000 16384 env 0.01 0.3 1 0.12"},
    {"BALLAD", CAT_KEYS, 1, "k 4000 9000 10000 16384 env 0.01 0.3 1 0.2"},
    {"SMOKY BAR", CAT_KEYS, 1, "k 8192 18000 14000 5000 env 0.002 0.3 1 0.08"},
    {"ROCK ORGAN", CAT_KEYS, 1, "k 24576 22000 26000 28000 env 0.002 0.3 1 0.08"},
    {"GROWL ORGAN", CAT_KEYS, 1, "k 32767 26000 32767 28000 env 0.002 0.3 1 0.08"},
    {"CLICKY", CAT_KEYS, 1, "k 8192 31000 6000 16384 env 0.001 0.3 1 0.06"},
    {"CHURCH", CAT_PAD, 1, "k 32767 0 3000 5000 env 0.15 0.5 1 0.8 push 20000 0 10000"},
    {"CATHEDRAL", CAT_PAD, 1, "k 24576 0 12000 16384 env 0.6 1 1 2 push 32767 8000 16000"},
    {"SOFT CHORALE", CAT_PAD, 1, "k 4000 0 14000 16384 env 0.4 1 1 1.5"},
    {"SWELL PAD", CAT_PAD, 1, "k 16384 0 12000 16384 env 1.5 1 1 2 lfo1 0 0.2 0.15 2 0 0"},
    {"DRONE BOX", CAT_PAD, 1, "k 32767 0 20000 5000 env 2 1 1 3 push 32767 0 0 voice 0 0 0 0.5"},
    {"ORGAN LEAD", CAT_LEAD, 1, "k 24576 20000 22000 28000 voice 1 0.04 0 0 env 0.002 0.3 1 0.1"},
    {"WHISTLE BAR", CAT_LEAD, 1, "k 0 0 9000 28000 voice 1 0.06 0 0 push 0 0 32767 env 0.01 0.3 1 0.15"},
    {"SCREAM", CAT_LEAD, 1, "k 32767 24000 32767 28000 voice 1 0.03 0 0 env 0.002 0.3 1 0.1"},
    {"GLIDE ORGAN", CAT_LEAD, 1, "k 16384 14000 16000 16384 voice 1 0 0 0 gsync 1 7 env 0.002 0.3 1 0.1"},
    {"PEDAL BASS", CAT_BASS, 1, "k 8192 0 0 5000 voice 1 0.02 0 0 push 32767 0 0 env 0.003 0.3 1 0.08"},
    {"WALKING BASS", CAT_BASS, 1, "k 4000 16000 6000 5000 voice 1 0 0 0 env 0.002 0.4 0.4 0.08 push 24000 0 0"},
    {"DIRTY BASS", CAT_BASS, 1, "k 8192 10000 28000 5000 voice 1 0.02 0 0 push 32767 0 0 env 0.002 0.3 1 0.08"},
    {"PERC STAB", CAT_PLUCK, 1, "k 4000 24000 8000 16384 env 0.001 0.3 0 0.1"},
    {"CHIME BARS", CAT_BELL, 1, "k 24576 20000 0 16384 env 0.001 1 0 0.8 push 0 0 32767"},
    {"MUSIC HALL", CAT_BELL, 1, "k 0 22000 6000 28000 env 0.001 0.8 0.2 0.6 push 0 20000 32767"},
    {"TREMOLO", CAT_KEYS, 1, "k 16384 0 4000 28000 env 0.005 0.3 1 0.1 lfo1 0 6 0.3 5 0 0"},
    {"BRAKE", CAT_FX, 1, "k 24576 0 16000 0 env 0.002 0.3 1 0.3 lfo1 3 0.2 0.5 3 1 0"},
    {"SPIN UP", CAT_FX, 1, "k 16384 0 10000 16384 env 0.002 0.3 1 0.3 menv 2 1 1 1 0.4 3"},
    {"WARP ORGAN", CAT_FX, 1, "k 24576 10000 20000 28000 env 0.002 0.3 1 0.3 lfo1 1 0.3 0.04 4 0 0 voice 0 0 0 1"},
    {"TAPE ORGAN", CAT_OTHER, 1, "k 8192 14000 10000 16384 env 0.003 0.3 1 0.15 voice 0 0 0 0.7"},
    {"PIPE SOLO", CAT_LEAD, 1, "k 0 0 2000 5000 voice 1 0.05 0 0 env 0.04 0.3 1 0.2 push 0 32767 0"},
};

// ELG: k INTERVAL DETUNE TIMBRE MOVEMENT. INTERVAL co 512 jednostek powtarza 5 stopni: 256 UNISON, 768 FIFTH,
// 1280 OCTAVE, 1792 OCT+5TH, 2304 2 OCT. DETUNE 0..32767 = 0..~20 c; TIMBRE = filtr 40 Hz..6 kHz + przester;
// MOVEMENT = predkosc i glebokosc ruchu; msync 1 PODZIAL = ruch w tempie. Tylko obwiednia glosnosci (sam silnik).
static const FactoryPreset kElg[] = {
    {"INIT", CAT_OTHER, 1, ""},
    {"NORTHERN LIGHTS", CAT_PAD, 2, "k 1280 9000 20000 12000 env 1.5 1 1 3"},
    {"TUNDRA", CAT_PAD, 2, "k 768 6000 14000 8000 env 2 1 1 3.5"},
    {"MOOSE CALL", CAT_LEAD, 2, "k 768 4000 24000 6000 env 0.08 0.5 0.9 0.6"},
    {"DEEP DRONE", CAT_PAD, 2, "k 256 3000 9000 5000 env 2.5 1 1 4"},
    {"FROST", CAT_PAD, 2, "k 2304 14000 26000 14000 env 1.2 1 0.9 3"},
    {"FJORD", CAT_PAD, 2, "k 1792 10000 17000 10000 env 1.8 1 1 3"},
    {"AURORA PULSE", CAT_PAD, 2, "k 1280 8000 20000 22000 msync 1 3 env 0.8 1 1 2"},
    {"SLOW BREATH", CAT_PAD, 2, "k 768 7000 15000 18000 msync 1 0 env 1.5 1 1 3"},
    {"WIDE OCTAVES", CAT_PAD, 2, "k 2304 20000 18000 9000 env 1 1 1 2.5"},
    {"DARK FOREST", CAT_PAD, 2, "k 256 12000 7000 15000 env 2 1 1 3"},
    {"GROWL BASS", CAT_BASS, 2, "k 256 5000 22000 3000 env 0.005 0.4 0.8 0.15"},
    {"FIFTH BASS", CAT_BASS, 2, "k 768 3000 16000 2000 env 0.005 0.3 0.9 0.12"},
    {"SUB DRONE", CAT_BASS, 2, "k 256 1000 8000 4000 env 0.05 0.5 1 0.8"},
    {"WOBBLE BASS", CAT_BASS, 2, "k 1280 4000 18000 26000 msync 1 5 env 0.005 0.3 1 0.15"},
    {"FUZZ BASS", CAT_BASS, 2, "k 1280 8000 30000 2000 env 0.003 0.3 0.9 0.1"},
    {"SAW LEAD", CAT_LEAD, 2, "k 1280 10000 26000 4000 env 0.01 0.3 0.9 0.3"},
    {"OCTAVE LEAD", CAT_LEAD, 2, "k 1280 3000 23000 7000 env 0.02 0.4 0.85 0.4"},
    {"HOWL", CAT_LEAD, 2, "k 1792 16000 28000 16000 env 0.1 0.5 0.9 0.8"},
    {"DRIFTER", CAT_LEAD, 2, "k 768 22000 20000 20000 env 0.05 0.4 0.9 0.6"},
    {"SNOW KEYS", CAT_KEYS, 2, "k 2304 6000 21000 5000 env 0.005 1.2 0.3 0.8"},
    {"ICE ORGAN", CAT_KEYS, 2, "k 1280 2000 17000 3000 env 0.005 0.3 1 0.1"},
    {"RUNE PLUCK", CAT_PLUCK, 2, "k 768 5000 25000 6000 env 0.001 0.5 0 0.4"},
    {"ANTLER PLUCK", CAT_PLUCK, 2, "k 1792 9000 22000 4000 env 0.001 0.35 0 0.25"},
    {"MOON BELL", CAT_BELL, 2, "k 2304 2000 30000 10000 env 0.001 2 0 2"},
    {"STORM", CAT_FX, 2, "k 1792 30000 32767 32767 env 0.8 1 1 2"},
    {"SHIVER", CAT_FX, 2, "k 2304 26000 24000 30000 msync 1 9 env 0.3 1 1 1"},
    {"WIND", CAT_FX, 2, "k 256 32767 11000 28000 env 1.5 1 1 2.5"},
    {"HEARTBEAT", CAT_FX, 2, "k 256 2000 12000 24000 msync 1 3 env 0.01 0.4 1 0.3"},
    {"RITUAL", CAT_OTHER, 2, "k 768 11000 19000 17000 msync 1 1 env 1 1 1 3"},
    {"CAMPFIRE", CAT_OTHER, 2, "k 1280 7000 13000 11000 env 0.6 1 1 2"},
};

// EVA: 8 presetow oryginalu (third_party/op1r_eva/presets.py): galki 1:1; obwiednia OP-1 (0..32767) przeliczona
// w przyblizeniu na sekundy; efekty (spring/delay), LFO i oktawa oryginalu pominiete (jeszcze ich nie ma).
static const FactoryPreset kEva[] = {
    {"INIT", CAT_OTHER, 1, ""},
    {"SPACE CHOIR", CAT_PAD, 1, "k 20000 16384 12000 3000 env 0.05 0.6 0.92 0.9"},
    {"TEARS IN RAIN", CAT_PAD, 1, "k 11000 4000 22000 6000 env 0.15 0.8 0.92 1.2"},
    {"ENSEMBLE AH", CAT_PAD, 1, "k 24000 15500 28000 0 env 0.04 0.6 0.88 0.8"},
    {"EE HALO", CAT_PAD, 1, "k 26000 31000 18000 2000 env 0.03 0.4 0.8 1"},
    {"DISTANT OO", CAT_PAD, 1, "k 3500 1500 15000 9000 env 0.2 0.8 0.92 1.2"},
    {"RADIO CHOIR", CAT_PAD, 1, "k 20000 14000 10000 17000 env 0.05 0.6 0.85 0.7"},
    {"HOUSTON", CAT_FX, 1, "k 19000 11000 6000 23000 env 0.02 0.35 0.8 0.45"},
    {"LOST SIGNAL", CAT_FX, 1, "k 16000 21000 20000 30000 env 0.03 0.6 0.8 0.8"},
    // ---- wersja 2: z MOD ENV i LFO (cele: 0 PRES, 1 FREQ, 2 OXY, 3 COM, 4 PTCH, 5 VOL) ----
    {"VOWEL SWEEP", CAT_PAD, 2, "k 20000 0 14000 2000 env 0.1 0.8 0.9 1.2 menv 2 1 1 1.5 0.95 1"},
    {"TALKING CHOIR", CAT_PAD, 2, "k 22000 16384 12000 2000 env 0.05 0.6 0.9 1 lfo1 1 0.3 0.45 1 0 0"},
    {"BREATHING", CAT_PAD, 2, "k 16000 9000 18000 3000 env 0.3 1 0.9 1.5 lfo1 0 0.2 0.35 2 0 0 lfo2 0 0.2 0.15 5 0 0"},
    {"OO TO EE", CAT_PAD, 2, "k 18000 0 14000 1000 env 0.2 1 0.9 1.4 lfo1 1 1 0.9 1 1 0"},
    {"PULSAR", CAT_PAD, 2, "k 24000 20000 16000 4000 env 0.05 0.5 0.9 0.9 lfo1 3 2 0.5 5 1 5"},
    {"DRIFTING CREW", CAT_PAD, 2, "k 14000 12000 22000 5000 env 0.4 1 0.9 2 lfo1 4 0.4 0.03 4 0 0 lfo2 0 0.13 0.3 1 0 0"},
    {"SOLAR WIND", CAT_PAD, 2, "k 12000 26000 28000 9000 env 1 1 0.9 2.5 lfo1 0 0.08 0.6 2 0 0"},
    {"SWELL CHOIR", CAT_PAD, 2, "k 6000 14000 16000 2000 env 0.8 1 1 1.5 menv 1.5 1 1 1.5 0.7 0"},
    {"STARGAZER", CAT_PAD, 2, "k 26000 30000 14000 1500 env 0.1 0.8 0.85 1.6 lfo1 0 4.5 0.02 4 0 0 lfo2 1 0.15 0.4 1 0 0"},
    {"SIGNAL FADE", CAT_FX, 2, "k 18000 16000 12000 6000 env 0.05 0.8 0.9 1 lfo1 1 0.1 0.8 3 0 0"},
    {"MISSION CONTROL", CAT_FX, 2, "k 20000 12000 8000 18000 env 0.02 0.4 0.85 0.6 lfo1 4 3 0.35 3 1 7"},
    {"ORBIT", CAT_FX, 2, "k 22000 18000 18000 14000 env 0.1 0.6 0.9 1 lfo1 0 0.25 0.5 3 1 0 lfo2 0 0.25 0.6 1 1 0"},
    {"RE-ENTRY", CAT_FX, 2, "k 24000 22000 20000 4000 env 0.01 2 0.6 1 menv 0.01 2.5 0 1 0.85 3"},
    {"GATED CHOIR", CAT_KEYS, 2, "k 26000 16384 10000 2000 env 0.01 0.4 0.9 0.4 lfo1 3 2 0.9 5 1 7"},
    {"CHOIR STAB", CAT_KEYS, 2, "k 30000 18000 8000 3000 env 0.005 0.5 0 0.5 menv 0.001 0.3 0 0.3 0.6 0"},
    {"AH PLUCK", CAT_PLUCK, 2, "k 32767 16384 6000 2000 env 0.002 0.6 0 0.5 menv 0.001 0.25 0 0.2 -0.8 1"},
    {"SPACE LEAD", CAT_LEAD, 2, "k 28000 24000 6000 4000 env 0.02 0.4 0.9 0.5 lfo1 0 5.5 0.015 4 0 0 menv 0.001 0.15 0 0.1 -0.06 4"},
    {"VOX LEAD", CAT_LEAD, 2, "k 30000 12000 8000 2000 env 0.01 0.3 0.9 0.4 menv 0.4 0.6 0.4 0.5 0.4 1"},
    {"COMET", CAT_BELL, 2, "k 32767 31000 20000 2000 env 0.002 1.8 0 2 menv 0.001 1 0 1 -0.5 1"},
    {"STATION BASS", CAT_BASS, 2, "k 24000 4000 4000 8000 env 0.005 0.5 0.7 0.3 menv 0.001 0.3 0 0.2 0.5 0"},
};

// Zestaw dla katalogu silnika: wskaznik i liczba pozycji
const FactoryPreset* factoryList(const char* eng, int* n, int* engine) {
    if (!__builtin_strcmp(eng, "taffy")) { *n = sizeof kTaffy / sizeof kTaffy[0]; *engine = ENG_TAFFY; return kTaffy; }
    if (!__builtin_strcmp(eng, "eva")) { *n = sizeof kEva / sizeof kEva[0]; *engine = ENG_EVA; return kEva; }
    if (!__builtin_strcmp(eng, "elg")) { *n = sizeof kElg / sizeof kElg[0]; *engine = ENG_ELG; return kElg; }
    if (!__builtin_strcmp(eng, "pew")) { *n = sizeof kPew / sizeof kPew[0]; *engine = ENG_PEW; return kPew; }
    if (!__builtin_strcmp(eng, "wavetableplus")) { *n = sizeof kWavetablePlus / sizeof kWavetablePlus[0]; *engine = ENG_WAVETABLE; return kWavetablePlus; }
    *n = sizeof kWavetable / sizeof kWavetable[0];
    *engine = ENG_WAVETABLE;
    return kWavetable;
}

}  // namespace dp

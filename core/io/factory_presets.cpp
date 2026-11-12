// Zestawy fabryczne presetow (po jednej linii na preset, w zapisie instrumentu z pliku projektu — parseInstrument).
// Klucze: k WAVE DTUN|DCW CUT|ENV RES | env A D S R | envamt X | fenv A D S R AMOUNT | menv A D S R AMOUNT CEL |
//         lfo1/lfo2 KSZTALT HZ GLEB CEL SYNC PODZIAL | voice MONO GLIDE UNISON AGE | gsync 1 PODZIAL |
//         (WAVETABLE+) alt 1, o1/o2 FALA INTERWAL CENTY POZIOM, sub X, fmode M, drv D.
// Fale WAVETABLE: SINE 0, WARM 4681, TRI 9362, SAW 14043, SQUARE 18724, PULSE 23405, VOCAL 28086, DIGITAL 32767.
// Ksztalty TAFFY: SAW 0, SQUARE 10923, PULSE 21846, SAW-PULSE 32767. Cele: 0 WAVE 1 DTUN/DCW 2 CUT/ENV 3 RES 4 PTCH
// 5 VOL 6 W1 7 W2 8 P1 9 P2 10 LV1 11 LV2. Ksztalty LFO: 0 SINE 1 TRI 2 SAW 3 SQR 4 RND. Podzialy: 0 1/1 1 1/2 3 1/4 5 1/8 7 1/16.
// ver = wersja zestawu w danym katalogu; na karcie dopisywane sa tylko presety z wersji nowszych niz znacznik.
#include "io/preset_store.h"
#include "io/project_file.h"
#include "seq/demo_song.h"
#include <stdio.h>

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
    {"SYNC LEAD", CAT_LEAD, 1, "k 23222 2581 20642 3072 push 19114 2304 2600 env 0.001 0.79 0.763 0.407"},
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

// MBIRA: 8 presetow oryginalu (third_party/op1r_mbira/presets.py): galki i strona 2 (push RING DOUBLE WAH) 1:1;
// obwiednia OP-1 przeliczona w przyblizeniu; efekty i oktawa oryginalu pominiete.
static const FactoryPreset kMbira[] = {
    {"INIT", CAT_OTHER, 1, ""},
    {"KALIMBA", CAT_PLUCK, 1, "k 14000 16384 12000 0 env 0.001 0.8 0.92 0.6"},
    {"THUMB WAH", CAT_PLUCK, 1, "k 16000 16384 14000 6000 push 8000 6000 20000 env 0.001 0.8 0.92 0.6"},
    {"GLASS RAIN", CAT_BELL, 1, "k 26000 30000 6000 0 push 16000 12000 0 env 0.001 1 0.8 0.8"},
    {"MARIMBA STEPS", CAT_PLUCK, 1, "k 12000 2000 20000 9000 env 0.001 0.6 0.75 0.4"},
    {"MUTED WOOD", CAT_PLUCK, 1, "k 9000 2000 5000 0 env 0.001 0.8 0.75 0.15"},
    {"STEEL DUET", CAT_PLUCK, 1, "k 14500 16384 9000 0 push 5000 6000 0 env 0.001 0.7 0.78 0.35"},
    {"BOTTLE CAPS", CAT_PLUCK, 1, "k 19000 16000 17000 16000 env 0.001 0.6 0.7 0.2"},
    {"GLASS DUSK", CAT_BELL, 1, "k 10000 30000 4000 0 push 10000 0 0 env 0.001 1 0.92 0.45"},
    // ---- wersja 2: z MOD ENV, LFO i strona 2 (push RING DOUBLE WAH; cele 0 STRK 1 TINE 2 BOX 3 BUZZ 4 PTCH 5 VOL) ----
    {"LONG KALIMBA", CAT_PLUCK, 2, "k 14000 16384 14000 0 push 26000 4000 0 env 0.001 1 0.95 1.5"},
    {"DETUNED TINES", CAT_PLUCK, 2, "k 15000 17000 12000 0 push 14000 22000 0 env 0.001 0.8 0.9 0.8"},
    {"WAH KALIMBA", CAT_PLUCK, 2, "k 16000 16384 12000 0 push 10000 0 30000 env 0.001 0.8 0.9 0.6"},
    {"MORPH TINE", CAT_PLUCK, 2, "k 14000 4000 12000 0 push 16000 0 0 env 0.001 0.8 0.92 0.8 menv 0.001 0.6 0 0.5 0.7 1"},
    {"WOBBLE TINE", CAT_PLUCK, 2, "k 14000 16384 12000 0 push 18000 0 0 env 0.001 0.8 0.92 0.8 lfo1 0 5 0.012 4 0 0"},
    {"RATTLE STEPS", CAT_PLUCK, 2, "k 18000 16000 14000 8000 env 0.001 0.6 0.8 0.3 lfo1 3 2 0.5 3 1 7"},
    {"SOFT THUMBS", CAT_KEYS, 2, "k 4000 16384 18000 0 push 20000 6000 0 env 0.002 1 0.92 1"},
    {"HARD NAILS", CAT_PLUCK, 2, "k 32767 18000 8000 0 env 0.001 0.5 0.8 0.3 menv 0.001 0.15 0 0.1 -0.5 0"},
    {"WOOD MARIMBA", CAT_PLUCK, 2, "k 11000 1000 22000 0 push 6000 0 0 env 0.001 0.5 0.75 0.3"},
    {"LOG DRUM", CAT_FX, 2, "k 9000 0 30000 0 env 0.001 0.4 0.6 0.2 menv 0.001 0.06 0 0.05 -0.15 4"},
    {"STEEL PAN", CAT_BELL, 2, "k 22000 20000 10000 0 push 14000 10000 6000 env 0.001 0.9 0.85 0.7"},
    {"GLASS HARP", CAT_BELL, 2, "k 8000 32000 4000 0 push 30000 12000 0 env 0.05 1 0.95 2"},
    {"ICE CHIMES", CAT_BELL, 2, "k 28000 31000 2000 0 push 26000 18000 0 env 0.001 1.5 0.9 2 lfo1 0 0.3 0.15 5 0 0"},
    {"MUSIC BOX TINE", CAT_BELL, 2, "k 24000 24000 6000 0 push 12000 3000 0 env 0.001 0.8 0.85 0.8"},
    {"TALKING BOX", CAT_FX, 2, "k 16000 16384 16000 4000 push 14000 0 24000 env 0.001 0.8 0.9 0.6 lfo1 1 3 0.4 2 0 0"},
    {"BUZZ SWELL", CAT_FX, 2, "k 16000 14000 14000 0 env 0.001 1 0.9 0.8 menv 0.6 0.6 1 0.6 0.8 3"},
    {"GATED TINES", CAT_KEYS, 2, "k 16000 16384 12000 0 push 24000 6000 0 env 0.001 1 0.95 0.5 lfo1 3 2 0.85 5 1 7"},
    {"WARPED", CAT_FX, 2, "k 14000 16384 12000 4000 push 20000 16000 0 env 0.001 1 0.9 1 lfo1 4 1 0.08 4 0 0"},
    {"DEEP BOX BASS", CAT_BASS, 2, "k 10000 3000 30000 0 env 0.001 0.6 0.85 0.3"},
    {"BRIGHT PLINK", CAT_PLUCK, 2, "k 30000 20000 8000 2000 push 4000 0 0 env 0.001 0.4 0.6 0.25"},
};

// REACTOR: 11 presetow oryginalu (third_party/op1r_reactor/plugins/reactor/presets.py): galki i trym (slowo 7 =
// push[2]) 1:1; obwiednia OP-1 przeliczona w przyblizeniu; efekty i oktawa (-1 w trzech basach) pominiete.
static const FactoryPreset kReactor[] = {
    {"INIT", CAT_OTHER, 1, ""},
    {"CHINA SYNDROME", CAT_BASS, 1, "k 2500 4000 32767 16000 push 0 0 8697 env 0.001 0.4 0.8 0.2"},
    {"BREEDER", CAT_PAD, 1, "k 9000 3000 20000 13500 push 0 0 7689 env 0.5 0.8 0.92 0.8"},
    {"IGNITION", CAT_KEYS, 1, "k 20000 1500 18000 9000 push 0 0 8354 env 0.001 0.5 0.45 0.25"},
    {"TURBINE", CAT_LEAD, 1, "k 6000 5000 28000 20000 push 0 0 8578 env 0.02 0.45 0.8 0.4"},
    {"COOLANT LEAK", CAT_LEAD, 1, "k 6000 3000 14000 13000 push 0 0 7850 env 0.01 0.5 0.8 0.3"},
    {"GEIGER LEAD", CAT_LEAD, 1, "k 18000 26000 12000 10000 push 0 0 7932 env 0.001 0.4 0.75 0.15"},
    {"HALF-LIFE PAD", CAT_PAD, 1, "k 12000 5000 24000 16000 push 0 0 7914 env 0.6 0.8 0.85 0.8"},
    {"CORE BASS", CAT_BASS, 1, "k 14000 2000 32767 12000 push 0 0 7796 env 0.001 0.3 0.45 0.1"},
    {"MELTDOWN", CAT_FX, 1, "k 0 20000 32767 32767 push 0 0 8829 env 0.2 0.6 0.92 0.6"},
    {"CONTROL RODS", CAT_BELL, 1, "k 24000 0 26000 4000 push 0 0 9929 env 0.001 0.7 0 0.4"},
    {"SCRAM", CAT_FX, 1, "k 8000 8000 30000 24000 push 0 0 8687 env 0.001 0.25 0.2 0.08"},
};

// OXBOW: 16 presetow oryginalu (third_party/kennel_oxbow/plugins/oxbow/presets.py): galki i strona 2 (push WAVE FILTER
// DRIVE) 1:1; obwiednia OP-1 przeliczona w przyblizeniu (czas ~ 8 s x (v/32767)^1,5); efekty pominiete.
static const FactoryPreset kOxbow[] = {
    {"INIT", CAT_OTHER, 1, ""},
    {"POLY BRASS", CAT_KEYS, 1, "k 14191 3871 26834 3584 push 2730 2304 1600 env 0.009 0.79 0.702 0.341"},
    {"SYNC LEAD", CAT_LEAD, 1, "k 23222 2581 20642 3072 push 19114 2304 2600 env 0.001 0.211 0.763 0.07"},
    {"PWM STRING", CAT_PAD, 1, "k 19352 1291 19094 4608 push 8192 256 0 env 0.449 1.451 0.855 1.556"},
    {"BELL XMOD", CAT_BELL, 1, "k 25802 5161 24254 3072 push 24576 1280 600 env 0.001 1.057 0.183 1.057"},
    {"FAT BASS", CAT_BASS, 1, "k 10321 7741 28124 3072 push 2730 2304 3500 env 0.001 0.707 0.458 0.179"},
    {"SWEEP PAD", CAT_PAD, 1, "k 7741 5161 32767 5120 push 13653 2304 0 env 0.707 1.556 0.793 1.773"},
    {"BP QUACK", CAT_PLUCK, 1, "k 12901 18062 31994 3072 push 8192 1280 500 env 0.001 0.55 0.0 0.159"},
    {"BRASS STAB", CAT_KEYS, 1, "k 15481 6451 29414 4096 push 13653 256 1200 env 0.003 0.707 0.671 0.279"},
    {"HOLLOW PAD", CAT_PAD, 1, "k 18062 5161 16514 6144 push 8192 1792 0 env 0.55 1.556 0.793 1.556"},
    {"SQUARE LD", CAT_LEAD, 1, "k 23222 5161 23222 2560 push 8192 256 1000 env 0.001 0.707 0.855 0.407"},
    {"SOFT KEYS", CAT_KEYS, 1, "k 15481 2581 25544 3072 push 2730 256 0 env 0.006 0.876 0.397 0.477"},
    {"CLAV PLUCK", CAT_PLUCK, 1, "k 14191 10321 32767 1536 push 8192 1280 2200 env 0.001 0.341 0.0 0.103"},
    {"WOBBLE BS", CAT_BASS, 1, "k 9031 12901 16514 3072 push 13653 2304 2800 env 0.001 0.876 0.549 0.279"},
    {"FIFTHS LD", CAT_LEAD, 1, "k 21932 3871 23222 3072 push 30037 256 600 env 0.001 0.965 0.793 0.477"},
    {"VINTAGE PD", CAT_PAD, 1, "k 14191 3871 24254 4608 push 13653 256 0 env 0.477 1.556 0.824 1.556"},
    {"XMOD GROWL", CAT_BASS, 1, "k 12901 5161 29414 3072 push 24576 2304 4500 env 0.003 0.965 0.671 0.407"},
};

// OXBOW+ (rozszerzony OXBOW): INIT = neutralny glos oryginalu (obwiednia filtra ustawia initInstrument), 30 presetow.
// obx: w1 w2 semi1 semi2 cents1 lvl1 lvl2 pw1 pw2 noise sync xmod env>p2 keytrk vel vintage ftype drive
// (fale 0 OFF 1 SAW 2 PULSE 3 BOTH; filtry 0 LP12 1 HP12 2 BP12 3 NOTCH 4 LP24 5 SEM); fenv = obwiednia filtra OXBOW
// (AMOUNT = galka ENV AMOUNT: k[2], 16384 = 0); cele LFO: 6 PW1, 7 PW2, 8 P1, 9 P2, 10 LV1, 11 LV2.
static const FactoryPreset kOxbowPlus[] = {
    {"INIT", CAT_OTHER, 1, "alt 1"},
    {"OB BASS", CAT_BASS, 1, "alt 1 k 9000 8000 26000 3000 obx 1 2 0 -12 0 100 90 64 40 0 0 0 0 64 40 30 4 20 fenv 0.001 0.3 0.1 0.2 0 env 0.002 0.3 0.9 0.1 voice 1 0.03 0 0"},
    {"SYNC BASS", CAT_BASS, 1, "alt 1 k 12000 6000 24000 0 obx 1 1 0 7 0 90 100 64 64 0 1 0 40 64 40 30 0 10 fenv 0.001 0.25 0.05 0.2 0 env 0.002 0.3 0.9 0.1 voice 1 0 0 0"},
    {"UNISON BASS", CAT_BASS, 1, "alt 1 k 10000 5000 25000 2000 obx 1 1 0 -12 0 100 80 64 64 0 0 0 0 64 40 40 4 30 fenv 0.001 0.35 0.15 0.2 0 env 0.003 0.3 0.9 0.1 voice 1 0.02 18 0"},
    {"PULSE SUB", CAT_BASS, 1, "alt 1 k 8000 4000 22000 0 obx 2 2 0 -12 0 90 100 40 64 0 0 0 0 64 40 30 4 0 fenv 0.001 0.2 0.2 0.2 0 env 0.002 0.3 0.9 0.08 voice 1 0.03 0 0"},
    {"XMOD BASS", CAT_BASS, 1, "alt 1 k 11000 7000 25000 1000 obx 1 1 0 -12 0 100 60 64 64 0 0 60 0 64 40 30 0 25 fenv 0.001 0.3 0.1 0.2 0 env 0.002 0.3 0.9 0.1 voice 1 0 0 0"},
    {"FUNK BASS", CAT_BASS, 1, "alt 1 k 9000 16000 29000 0 obx 2 1 0 0 0 100 70 48 64 0 0 0 0 64 60 30 2 15 fenv 0.001 0.18 0 0.12 0 env 0.002 0.25 0.7 0.08 voice 1 0 0 0"},
    {"SYNC SCREAM", CAT_LEAD, 1, "alt 1 k 16000 9000 26000 0 obx 1 1 0 12 0 80 110 64 64 0 1 0 50 64 40 30 0 60 fenv 0.01 0.6 0.3 0.4 0 env 0.005 0.3 0.9 0.25 voice 1 0.06 0 0"},
    {"FIFTH LEAD", CAT_LEAD, 1, "alt 1 k 18000 7000 22000 4000 obx 1 1 0 7 0 100 90 64 64 0 0 0 0 64 40 30 0 10 fenv 0.005 0.4 0.5 0.3 0 env 0.005 0.3 0.9 0.3 voice 1 0.05 0 0 lfo1 0 5.5 0.012 4 0 0"},
    {"UNISON LEAD", CAT_LEAD, 1, "alt 1 k 17000 6000 22000 3000 obx 1 1 0 0 -6 100 100 64 64 0 0 0 0 64 40 40 4 20 fenv 0.005 0.5 0.5 0.3 0 env 0.005 0.3 0.9 0.3 voice 1 0.04 25 0"},
    {"PWM LEAD", CAT_LEAD, 1, "alt 1 k 16000 8000 22000 2000 obx 2 2 0 0 0 100 90 64 64 0 0 0 0 64 40 30 0 10 fenv 0.005 0.5 0.5 0.3 0 env 0.005 0.3 0.9 0.3 voice 1 0.05 0 0 lfo1 1 4 0.5 6 0 0 lfo2 1 3.3 0.5 7 0 0"},
    {"XMOD LEAD", CAT_LEAD, 1, "alt 1 k 18000 9000 24000 0 obx 1 1 0 0 0 100 70 64 64 0 0 80 0 64 40 30 0 30 fenv 0.005 0.4 0.4 0.3 0 env 0.005 0.3 0.9 0.3 voice 1 0.04 0 0"},
    {"PORTA LEAD", CAT_LEAD, 1, "alt 1 k 15000 9000 23000 3000 obx 1 2 0 -12 0 100 60 50 64 0 0 0 0 64 40 30 4 25 fenv 0.01 0.5 0.6 0.3 0 env 0.005 0.3 0.9 0.3 voice 1 0.15 0 0.2"},
    {"OB STRINGS", CAT_PAD, 1, "alt 1 k 15000 4000 20000 12000 obx 1 1 0 0 -8 100 100 64 64 0 0 0 0 64 30 80 4 0 fenv 0.7 1.2 0.7 1.5 0 env 0.5 1.5 0.85 1.5 voice 0 0 0 0.5"},
    {"PWM PAD", CAT_PAD, 1, "alt 1 k 14000 5000 20000 6000 obx 2 2 0 0 -5 100 100 64 64 0 0 0 0 64 30 50 4 0 fenv 0.8 1.2 0.6 1.5 0 env 0.6 1.5 0.85 1.8 lfo1 0 0.25 0.6 6 0 0 lfo2 0 0.18 0.6 7 0 0"},
    {"SEM PAD", CAT_PAD, 1, "alt 1 k 16000 10000 21000 8000 obx 1 3 0 12 0 100 70 64 48 0 0 0 0 64 30 50 5 10 fenv 0.6 1 0.6 1.4 0 env 0.5 1.5 0.85 1.6 voice 0 0 0 0.4"},
    {"NOTCH PAD", CAT_PAD, 1, "alt 1 k 17000 12000 18000 7000 obx 1 2 0 7 0 100 80 64 64 0 0 0 0 64 30 50 3 0 fenv 0.5 1 0.6 1.5 0 env 0.6 1.5 0.85 1.8 lfo1 1 0.12 0.2 0 0 0"},
    {"VINTAGE PAD", CAT_PAD, 1, "alt 1 k 15000 6000 20000 10000 obx 1 1 0 0 -10 100 100 64 64 0 0 0 0 64 30 127 4 0 fenv 0.8 1.2 0.7 1.6 0 env 0.7 1.5 0.85 2 voice 0 0 0 1"},
    {"NOISE WIND", CAT_FX, 1, "alt 1 k 27000 9000 16384 0 obx 0 0 0 0 0 0 0 64 64 126 0 0 0 64 0 30 0 0 fenv 1 1 1 1.5 0 env 0.4 1 1 1.5 lfo1 0 0.12 0.18 0 0 0"},
    {"OB BRASS", CAT_KEYS, 1, "alt 1 k 12000 6000 27000 6000 obx 1 1 0 0 -4 100 100 64 64 0 0 0 0 64 50 40 4 10 fenv 0.08 0.7 0.55 0.4 0 env 0.04 0.4 0.85 0.35 voice 0 0 0 0.3"},
    {"BRASS SECTION", CAT_KEYS, 1, "alt 1 k 13000 5000 26000 10000 obx 1 1 0 0 -10 100 100 64 64 0 0 0 0 64 50 60 0 0 fenv 0.12 0.8 0.6 0.5 0 env 0.06 0.5 0.85 0.4 voice 0 0 0 0.5"},
    {"POLY KEYS", CAT_KEYS, 1, "alt 1 k 15000 5000 24000 3000 obx 2 1 0 0 0 90 90 56 64 0 0 0 0 64 50 30 4 0 fenv 0.002 0.7 0.2 0.4 0 env 0.002 1 0.4 0.4"},
    {"OCTAVE KEYS", CAT_KEYS, 1, "alt 1 k 16000 4000 23000 2000 obx 1 2 0 12 0 100 60 64 40 0 0 0 0 64 50 30 0 0 fenv 0.002 0.6 0.3 0.4 0 env 0.002 0.9 0.5 0.4"},
    {"OB PLUCK", CAT_PLUCK, 1, "alt 1 k 9000 8000 30000 3000 obx 1 1 0 0 0 100 90 64 64 0 0 0 0 64 60 30 4 0 fenv 0.001 0.25 0 0.2 0 env 0.001 0.5 0 0.3"},
    {"SYNC PLUCK", CAT_PLUCK, 1, "alt 1 k 13000 6000 28000 0 obx 1 1 0 12 0 80 100 64 64 0 1 0 45 64 60 30 0 15 fenv 0.001 0.2 0 0.15 0 env 0.001 0.4 0 0.25"},
    {"HP PLUCK", CAT_PLUCK, 1, "alt 1 k 9000 12000 26000 2000 obx 2 1 0 12 0 100 70 48 64 0 0 0 0 64 60 30 1 0 fenv 0.001 0.3 0 0.2 0 env 0.001 0.45 0 0.3"},
    {"XMOD BELL", CAT_BELL, 1, "alt 1 k 22000 4000 20000 0 obx 0 1 0 19 0 100 90 64 64 0 0 100 0 64 40 30 0 0 fenv 0.001 1.2 0.1 1 0 env 0.001 1.8 0 1.5"},
    {"METAL", CAT_BELL, 1, "alt 1 k 20000 8000 24000 0 obx 1 2 0 17 0 90 90 64 30 0 1 70 20 64 40 30 0 10 fenv 0.001 0.8 0.1 0.8 0 env 0.001 1.4 0 1.2"},
    {"SWEEPER", CAT_FX, 1, "alt 1 k 12000 18000 16384 4000 obx 1 1 0 0 -8 100 100 64 64 0 0 0 0 64 40 40 0 20 fenv 0.01 0.5 0.5 0.5 0 env 0.01 0.5 1 0.6 lfo1 2 0.25 0.6 0 1 0"},
    {"LASER", CAT_FX, 1, "alt 1 k 20000 12000 20000 0 obx 1 1 0 12 0 60 100 64 64 0 1 0 -64 64 40 30 0 0 fenv 0.001 0.35 0 0.2 0 env 0.001 0.4 0 0.2 menv 0.001 0.3 0 0.2 -0.5 4"},
    {"NOISE HIT", CAT_FX, 1, "alt 1 k 28000 6000 30000 0 obx 0 0 0 0 0 0 0 64 64 126 0 0 0 32 40 30 0 0 fenv 0.001 0.12 0 0.1 0 env 0.001 0.2 0 0.15"},
};

// SID: 8 presetow oryginalu (third_party/op1r_sid/presets.py; fala = srodek pasma 512, obwiednia OP-1 w przyblizeniu,
// efekty i oktawa pominiete) + wersja 2: 12 z MOD ENV i LFO (cele: 0 WAVE 1 MOD 2 CUT 3 RES 4 PTCH 5 VOL).
static const FactoryPreset kSid[] = {
    {"INIT", CAT_OTHER, 1, ""},
    {"ROUND BASS", CAT_BASS, 1, "k 1280 16384 23000 0 env 0.001 0.477 0.855 0.169"},
    {"CHIP BASS", CAT_BASS, 1, "k 1280 10000 25200 6500 env 0.001 0.407 0.58 0.169"},
    {"SAW LEAD", CAT_LEAD, 1, "k 768 6500 29200 5500 env 0.001 0.707 0.671 0.279"},
    {"PULSE KEYS", CAT_KEYS, 1, "k 1280 18000 27800 3500 env 0.001 0.707 0.305 0.407"},
    {"SOFT TRIANGLE", CAT_PAD, 1, "k 256 10000 28000 2500 env 0.407 0.79 0.671 0.707"},
    {"SYNC BITE", CAT_LEAD, 1, "k 2304 10923 30000 6000 env 0.001 0.55 0.366 0.222"},
    {"RING BELL", CAT_BELL, 1, "k 2816 16384 31000 3500 env 0.001 0.79 0.0 0.627"},
    {"NOISE HATS", CAT_FX, 1, "k 1792 0 31500 3000 env 0.001 0.407 0.0 0.078"},
    {"PWM BASS", CAT_BASS, 2, "k 1280 8000 24000 4000 env 0.002 0.4 0.8 0.15 lfo1 1 0.6 0.35 1 0 0 push 0 0 6800"},
    {"FILTER PLUCK", CAT_PLUCK, 2, "k 768 8000 18000 9000 env 0.001 0.4 0 0.3 menv 0.001 0.3 0 0.2 0.35 2"},
    {"WOBBLE BASS", CAT_BASS, 2, "k 768 4000 22000 12000 env 0.002 0.3 0.9 0.1 lfo1 0 2 0.3 2 1 5"},
    {"ARP SYNC", CAT_LEAD, 2, "k 2304 10923 29000 5000 env 0.002 0.3 0.8 0.2 lfo1 3 4 0.25 1 1 7"},
    {"SYNC SWEEP", CAT_LEAD, 2, "k 2304 6000 30000 4000 env 0.005 0.4 0.85 0.3 menv 0.01 1.2 0 0.4 0.6 1"},
    {"RING DRONE", CAT_PAD, 2, "k 2816 20000 28000 6000 env 0.6 1 0.9 1.5 lfo1 0 0.2 0.15 1 0 0"},
    {"CHIP STRINGS", CAT_PAD, 2, "k 768 22000 26000 3000 env 0.4 1 0.85 1.2 lfo1 0 5 0.01 4 0 0"},
    {"VIBRATO LEAD", CAT_LEAD, 2, "k 1280 14000 28500 4000 env 0.01 0.3 0.85 0.3 lfo1 0 6 0.012 4 1 7"},
    {"ZAP", CAT_FX, 2, "k 1280 16384 30000 6000 env 0.001 0.2 0 0.1 menv 0.001 0.15 0 0.1 1 4"},
    {"SNARE", CAT_FX, 2, "k 1792 0 27000 12000 env 0.001 0.15 0 0.1 push 0 0 13000"},
    {"KICK", CAT_FX, 2, "k 256 0 26000 4000 env 0.001 0.25 0 0.1 menv 0.001 0.08 0 0.05 0.6 4 push 0 0 14000"},
    {"GLITCH NOISE", CAT_FX, 2, "k 1792 16000 24000 16000 env 0.002 0.3 0.8 0.2 lfo1 4 8 0.4 2 1 7"},
};

// MONOLIT: 12 presetow oryginalu (third_party/op1r_monolit/presets.py; galki i strona 2 BODY/VEL/AGE 1:1, obwiednia OP-1
// w przyblizeniu; efekty pominiete) + wersja 2: 20 z MONO/glide/unison i modulacja; wersja 3: oktawa -1 czterech basow
// jak w oryginale (OCTAVE instrumentu; zastepuje wczesniejsze wersje tych presetow).
static const FactoryPreset kMonolit[] = {
    {"INIT", CAT_OTHER, 1, ""},
    {"LADDER BASS", CAT_BASS, 3, "k 8192 7000 6000 12000 push 3277 12000 4000 env 0.001 0.965 0.793 0.169 oct -1"},
    {"PLUCK BASS", CAT_BASS, 3, "k 4000 3000 9000 20000 push 3277 20000 0 env 0.001 1.349 0.549 0.121 oct -1"},
    {"SUB DROP", CAT_BASS, 3, "k 8192 10000 2000 4000 push 29491 6000 0 env 0.001 1.773 0.916 0.341 oct -1"},
    {"LADDER PING", CAT_PLUCK, 3, "k 0 1500 3000 31000 push 3277 16000 0 env 0.001 1.152 0.366 0.121 oct -1"},
    {"ACID LINE", CAT_BASS, 1, "k 0 5000 11000 28000 push 3277 24000 2000 env 0.001 0.79 0.61 0.222"},
    {"FAT LEAD", CAT_LEAD, 1, "k 24576 18000 4000 14000 push 22938 8000 9000 env 0.001 1.152 0.916 0.477"},
    {"FIFTH LEAD", CAT_LEAD, 1, "k 24576 15000 8000 18000 push 16384 10000 6000 env 0.001 1.152 0.855 0.477"},
    {"BRIGHT STAB", CAT_KEYS, 1, "k 24576 22000 7000 8000 push 22938 16000 4000 env 0.001 0.477 0.488 0.222"},
    {"BRASS SWELL", CAT_KEYS, 1, "k 16384 9000 24000 9000 push 9830 14000 6000 env 0.078 1.152 0.855 0.79"},
    {"WORN TAPE LEAD", CAT_LEAD, 1, "k 16384 13000 12000 10000 push 9830 8000 30000 env 0.001 1.152 0.855 0.627"},
    {"WIDE PAD", CAT_PAD, 1, "k 32767 12000 30000 6000 push 22938 6000 20000 env 0.627 1.773 0.732 1.773"},
    {"NIGHT DRONE", CAT_PAD, 1, "k 32767 6000 32767 22000 push 29491 0 26000 env 1.152 2.73 1.0 2.73"},
    {"MOOG BASS", CAT_BASS, 2, "k 8192 6000 5000 14000 push 3277 14000 3000 env 0.002 0.4 0.9 0.1 voice 1 0.03 0 0"},
    {"GLIDE BASS", CAT_BASS, 2, "k 4000 5000 8000 18000 push 3277 12000 2000 env 0.002 0.4 0.9 0.1 voice 1 0.12 0 0"},
    {"UNISON BASS", CAT_BASS, 2, "k 24576 6000 6000 12000 push 3277 10000 4000 env 0.002 0.4 0.9 0.1 voice 1 0.03 14 0"},
    {"ACID GLIDE", CAT_BASS, 2, "k 0 4000 12000 29000 push 3277 26000 2000 env 0.002 0.3 0.8 0.08 voice 1 0.09 0 0"},
    {"WOBBLE BASS", CAT_BASS, 2, "k 8192 5000 2000 20000 push 3277 8000 2000 env 0.002 0.3 0.9 0.1 voice 1 0 0 0 lfo1 0 2 0.3 1 1 5"},
    {"REESE", CAT_BASS, 2, "k 16384 7000 3000 10000 push 9830 6000 12000 env 0.003 0.4 0.9 0.15 voice 1 0.02 10 0"},
    {"SYNC LEAD", CAT_LEAD, 2, "k 24576 16000 7000 16000 push 22938 12000 6000 env 0.005 0.3 0.9 0.25 voice 1 0.06 0 0 lfo1 0 5.5 0.012 4 0 0"},
    {"SCREAMER", CAT_LEAD, 2, "k 16384 14000 6000 31000 push 16384 12000 4000 env 0.005 0.3 0.9 0.25 voice 1 0.05 0 0"},
    {"UNISON LEAD", CAT_LEAD, 2, "k 32767 17000 5000 12000 push 22938 8000 8000 env 0.005 0.3 0.9 0.3 voice 1 0.05 20 0"},
    {"PORTA LEAD", CAT_LEAD, 2, "k 24576 15000 9000 14000 push 16384 10000 6000 env 0.01 0.3 0.9 0.3 voice 1 0.18 0 0"},
    {"HEAT LEAD", CAT_LEAD, 2, "k 16384 12000 8000 20000 push 22938 10000 4000 env 0.005 0.3 0.9 0.3 voice 1 0.06 0 0 lfo1 1 0.3 0.3 3 0 0"},
    {"FILTER SWEEP", CAT_PAD, 2, "k 32767 6000 20000 16000 push 22938 6000 15000 env 0.5 1 0.9 1.5 lfo1 0 0.1 0.3 1 0 0"},
    {"PULSING PAD", CAT_PAD, 2, "k 32767 10000 26000 10000 push 16384 6000 18000 env 0.4 1 0.9 1.5 lfo1 3 2 0.6 5 1 5"},
    {"SLOW STACK", CAT_PAD, 2, "k 32767 9000 30000 8000 push 22938 4000 22000 env 1 1.5 0.9 2"},
    {"PLUCK STAB", CAT_PLUCK, 2, "k 24576 8000 4000 16000 push 22938 18000 3000 env 0.001 0.4 0 0.3"},
    {"RESO PING", CAT_PLUCK, 2, "k 0 2000 3000 30000 push 3277 20000 0 env 0.001 0.6 0 0.4 lfo1 4 6 0.15 1 1 7"},
    {"POLY KEYS", CAT_KEYS, 2, "k 16384 12000 9000 8000 push 22938 16000 5000 env 0.003 0.8 0.4 0.4"},
    {"BRASS", CAT_KEYS, 2, "k 24576 8000 22000 10000 push 9830 14000 8000 env 0.05 0.5 0.8 0.3"},
    {"ZAP", CAT_FX, 2, "k 0 14000 4000 26000 push 3277 0 0 env 0.001 0.2 0 0.1 menv 0.001 0.18 0 0.1 1 4"},
    {"SELF OSC", CAT_FX, 2, "k 0 9000 16384 32767 push 3277 0 0 env 0.01 0.4 1 0.5 lfo1 0 0.2 0.2 1 0 0"},
};

// VAPORA: 15 presetow oryginalu (third_party/op1r_vapora/data/presets.json: galki i strona 2 CHARACTER/INTERVAL/SPEED
// 1:1, oktawa jak w recipes.json, obwiednia z preview_envelope_ms; efekty OP-1 pominiete) + 15 nowych z FILTER ENV
// (na BRIGHTNESS), MOD ENV i LFO. CHARACTER: slowo n*256 (n = 1..14) -> widmo (n + 10) % 14; INTERVAL: i*3277+1638.
static const FactoryPreset kVapora[] = {
    {"INIT", CAT_OTHER, 1, "push 8192 8192 8192"},
    {"ASTEROID SWARM", CAT_PAD, 1, "k 0 2500 30000 25000 push 8448 8192 16000 env 3.7 1 0.92 2.5"},
    {"BREATH CHOIR", CAT_PAD, 1, "k 1700 16000 28500 11500 push 8704 16384 7000 env 0.65 1 0.75 2.5"},
    {"CAVERN FLUTE", CAT_LEAD, 1, "k 5500 24500 32000 25500 push 8960 31129 28000 env 1.5 1 0.92 2.5"},
    {"DEEP PRESSURE", CAT_PAD, 1, "k 5500 24500 32000 4000 push 9216 31129 2200 env 5.2 1 0.92 2.5"},
    {"PINK SURF", CAT_PAD, 1, "k 0 22000 20000 0 push 9472 8192 8192 env 3.7 1 0.92 2.5"},
    {"PORCELAIN ORBIT", CAT_BELL, 1, "k 5500 24500 32000 25500 push 9728 8192 28000 env 1.5 1 0.79 2.5"},
    {"SUB LANTERN", CAT_BASS, 1, "k 0 0 18000 0 push 9984 1638 8192 env 0.005 1.8 0.7 2.5 oct -1"},
    {"A FIFTH TIDE", CAT_PAD, 1, "k 19000 28000 20500 14000 push 8192 16384 6500 env 1.5 1 0.92 2.5"},
    {"HOLY DUST", CAT_PAD, 1, "k 14000 28500 29000 19000 push 10496 16384 8500 env 3.933 2.2 0.92 3.5"},
    {"XFIN", CAT_PAD, 1, "k 22000 29500 24500 19000 push 8192 16384 5500 env 3.933 2.2 0.92 3.5"},
    {"ICE CHAPEL", CAT_PAD, 1, "k 12000 30000 27000 20000 push 11520 31129 7500 env 4.1 2.2 0.92 3.5 oct -1"},
    {"LAST LIGHT", CAT_PAD, 1, "k 11500 29000 16000 18000 push 10240 16384 5000 env 3.266 2.2 0.92 3.5"},
    {"MOON CHOIR", CAT_PAD, 1, "k 27000 28000 21000 18000 push 11520 14745 4500 env 3.766 2.2 0.92 3.5"},
    {"STONE HALL", CAT_PAD, 1, "k 2800 24500 23500 12500 push 10496 8192 6000 env 3.6 2.2 0.92 3.5 oct -3"},
    {"TIDAL ARC", CAT_PAD, 1, "k 28000 30000 27500 22000 push 11264 16384 6500 env 3.433 2.2 0.92 3.5"},
    {"SUNRISE SWEEP", CAT_PAD, 1, "k 12000 26000 6000 10000 push 1024 18023 8192 env 2 1 0.9 3 fenv 3 2 0.6 3 0.6"},
    {"NIGHT TIDE", CAT_PAD, 1, "k 8000 20000 16000 14000 push 512 8192 6000 env 2.5 1 0.9 3.5 lfo1 0 0.08 0.4 0 0 0"},
    {"GLASS BELLS", CAT_BELL, 1, "k 20000 22000 12000 6000 push 768 31131 12000 env 0.005 2.5 0 2.5 fenv 0.001 1.2 0 1 0.5"},
    {"VELVET KEYS", CAT_KEYS, 1, "k 10000 18000 12000 8000 push 3072 14746 10000 env 0.01 1.5 0.4 1.2 fenv 0.001 0.8 0.2 0.8 0.4"},
    {"SUB PULSE", CAT_BASS, 1, "k 4000 6000 14000 2000 push 2816 1638 8192 env 0.01 0.3 0.9 0.3 lfo1 3 2 0.7 5 1 5 oct -1"},
    {"AURORA DRIFT", CAT_PAD, 1, "k 14000 26000 18000 12000 push 256 27854 7000 env 3 2 0.9 4 lfo1 1 0.05 0.3 2 0 0 lfo2 0 0.11 0.4 0 0 0"},
    {"NEBULA STRINGS", CAT_PAD, 1, "k 16000 20000 22000 10000 push 3584 31131 9000 env 0.8 1 0.85 2.5 menv 2 2 0.3 2 0.5 1"},
    {"BREATH LEAD", CAT_LEAD, 1, "k 6000 12000 20000 6000 push 1536 8192 9000 env 0.05 0.5 0.8 0.4 lfo1 0 5 0.01 4 0 0"},
    {"FLUTE WHISPER", CAT_LEAD, 1, "k 4000 14000 16000 8000 push 1792 31131 12000 env 0.15 0.5 0.8 0.6 fenv 0.2 0.6 0.5 0.6 0.3"},
    {"PORCELAIN RAIN", CAT_PLUCK, 1, "k 12000 20000 22000 10000 push 2560 18023 16000 env 0.002 1.2 0 1.2 lfo1 4 6 0.3 0 0 0"},
    {"PRESSURE DROP", CAT_FX, 1, "k 8000 24000 24000 16000 push 2048 4915 14000 env 0.01 2 0 2 menv 0.001 1.5 0 1 -0.5 4"},
    {"SWARM RISE", CAT_FX, 1, "k 6000 22000 4000 14000 push 1280 24577 12000 env 2 1 1 2.5 fenv 4 0.1 1 2 0.7 lfo1 2 0.25 0.3 3 0 0"},
    {"CATHEDRAL SWELL", CAT_PAD, 1, "k 6000 26000 8000 14000 push 3328 1638 6000 env 2.5 2 0.9 4 fenv 4 2 0.7 4 0.5 oct -1"},
    {"OCEAN CHORD", CAT_KEYS, 1, "k 18000 24000 20000 10000 push 512 21300 8192 env 0.02 1 0.6 1.5 lfo1 0 1 0.3 5 1 3"},
    {"SURF GATE", CAT_FX, 1, "k 4000 22000 22000 8000 push 2304 8192 8192 env 0.01 0.5 1 0.5 lfo1 3 4 0.9 5 1 7"},
};

// SUPERSAW: oryginal (op1r, przyklad pcppcp) nie ma presetow — 24 wlasne: k DETUNE MORPH SUB DRIVE, MONO/glide, LFO.
// Wersja 2: bez UNISON i AGE (usuniete z VOICE, decyzja Patryka) — zastepuje presety, ktore ich uzywaly.
// Wersja 3: filtr DAW-POL (push ZAMKNIECIE RES TYP, fenv na CUTOFF, cel LFO 6 = CUT) — 9 presetow z filtrem.
static const FactoryPreset kSupersaw[] = {
    {"INIT", CAT_OTHER, 1, ""},
    {"TRANCE LEAD", CAT_LEAD, 2, "k 20000 0 8000 8000 voice 1 0.04 0 0 env 0.005 0.3 0.85 0.3"},
    {"HOOVER", CAT_LEAD, 2, "k 28000 6000 20000 14000 voice 1 0.15 0 0 env 0.02 0.3 0.9 0.4 lfo1 0 5.5 0.015 4 0 0"},
    {"PWM LEAD", CAT_LEAD, 1, "k 12000 16000 6000 8000 voice 1 0.05 0 0 env 0.01 0.3 0.9 0.3 lfo1 1 0.8 0.4 1 0 0"},
    {"SCREAM LEAD", CAT_LEAD, 2, "k 22000 10000 8000 32767 voice 1 0.06 0 0 env 0.005 0.3 0.9 0.3"},
    {"GLIDE LEAD", CAT_LEAD, 2, "k 16000 4000 10000 10000 voice 1 0.2 0 0 env 0.01 0.3 0.9 0.3"},
    {"SUPER PAD", CAT_PAD, 2, "k 20000 0 10000 4000 env 1 1 0.9 2 lfo1 0 0.2 0.15 0 0 0"},
    {"STACK PAD", CAT_PAD, 2, "k 26000 4000 6000 4000 env 1.5 1 0.9 2.5"},
    {"SAW STRINGS", CAT_PAD, 1, "k 12000 0 4000 2000 env 0.4 1 0.85 1.2 lfo1 0 4.5 0.006 4 0 0"},
    {"DRIFT PAD", CAT_PAD, 2, "k 22000 8000 12000 4000 env 2 1 0.9 3 lfo1 4 0.3 0.2 0 0 0"},
    {"TRANCE GATE", CAT_PAD, 1, "k 20000 0 8000 6000 env 0.005 0.3 1 0.3 lfo1 3 4 0.9 5 1 7"},
    {"HYPER CHORD", CAT_PAD, 2, "k 32767 0 10000 6000 env 0.01 0.4 0.8 0.5"},
    {"SQUARE ORGAN", CAT_KEYS, 1, "k 4000 32767 16000 3000 env 0.005 0.1 1 0.08"},
    {"BRASS STAB", CAT_KEYS, 1, "k 10000 2000 12000 12000 env 0.04 0.3 0.6 0.25 menv 0.001 0.08 0 0.05 -0.03 4"},
    {"CHORD STAB", CAT_KEYS, 1, "k 18000 0 8000 10000 env 0.002 0.35 0 0.25"},
    {"OCTAVE KEYS", CAT_KEYS, 1, "k 8000 0 32767 6000 env 0.003 0.6 0.5 0.4"},
    {"DIRTY KEYS", CAT_KEYS, 1, "k 12000 6000 12000 24000 env 0.003 0.4 0.6 0.3 lfo1 0 6 0.3 5 0 0"},
    {"SAW PLUCK", CAT_PLUCK, 1, "k 14000 0 6000 6000 env 0.001 0.25 0 0.2"},
    {"SUB BASS", CAT_BASS, 1, "k 2000 0 32767 4000 voice 1 0.02 0 0 env 0.003 0.3 0.9 0.1 oct -1"},
    {"REESE BASS", CAT_BASS, 2, "k 10000 0 20000 16000 voice 1 0 0 0 env 0.005 0.3 0.9 0.15 lfo1 1 0.3 0.3 0 0 0 oct -1"},
    {"FUZZ BASS", CAT_BASS, 1, "k 4000 20000 24000 32767 voice 1 0.03 0 0 env 0.002 0.3 0.9 0.1 oct -1"},
    {"SQUARE BASS", CAT_BASS, 1, "k 3000 32767 20000 10000 voice 1 0.05 0 0 env 0.002 0.3 0.8 0.1 oct -1"},
    {"WOBBLE BASS", CAT_BASS, 1, "k 6000 16000 24000 20000 voice 1 0 0 0 env 0.002 0.3 0.9 0.1 lfo1 0 2 0.5 1 1 5 oct -1"},
    {"RISER", CAT_FX, 1, "k 32767 0 0 8000 env 3 0.1 1 1 menv 4 0.1 1 1 0.5 4"},
    {"SIREN", CAT_FX, 1, "k 10000 32767 0 12000 env 0.05 0.1 1 0.5 lfo1 1 0.5 0.3 4 0 0"},
    {"FILTER PLUCK", CAT_PLUCK, 3, "k 16000 0 8000 6000 push 26000 8000 0 env 0.001 0.6 0 0.4 fenv 0.001 0.3 0 0.3 0.6"},
    {"TRANCE PLUCK", CAT_PLUCK, 3, "k 22000 0 6000 8000 push 22000 12000 0 env 0.001 0.5 0 0.4 fenv 0.001 0.25 0 0.25 0.55"},
    {"SWEEP PAD", CAT_PAD, 3, "k 20000 0 10000 4000 push 20000 10000 0 env 1 1 0.9 2 lfo1 1 0.08 0.35 6 0 0"},
    {"PUMP PAD", CAT_PAD, 3, "k 20000 0 8000 6000 push 18000 14000 0 env 0.005 0.3 1 0.3 lfo1 2 4 0.5 6 1 5"},
    {"THIN AIR", CAT_PAD, 3, "k 24000 0 0 4000 push 12000 8000 3 env 0.8 1 0.9 2"},
    {"ACID SAW", CAT_BASS, 3, "k 4000 0 8000 16000 push 24000 26000 1 voice 1 0.06 0 0 env 0.002 0.3 0.9 0.1 fenv 0.001 0.2 0 0.15 0.5 oct -1"},
    {"WARM BASS", CAT_BASS, 3, "k 6000 0 24000 6000 push 20000 4000 0 voice 1 0.03 0 0 env 0.003 0.3 0.9 0.1 oct -1"},
    {"WAH LEAD", CAT_LEAD, 3, "k 16000 6000 8000 12000 push 16000 22000 2 voice 1 0.05 0 0 env 0.01 0.3 0.9 0.3 lfo1 0 3 0.3 6 0 0"},
    {"FILTER RISE", CAT_FX, 3, "k 32767 0 4000 8000 push 30000 18000 0 env 2 0.1 1 1 fenv 4 0.1 1 1 0.9"},
};

// MIXTUR: oryginal (op1r, ultralazr) nie ma presetow — 13 wlasnych z samych galek silnika i obwiedni (bez dodatkow).
// k = undertony A..D (slowo (n*32768 + 16384) / 25, OFF = 655); push F1 F2 RES (F1 = 180 Hz * 2^(4,5 w), F2 = 500 Hz *
// 2^(3,5 w), w = slowo / 32768; samogloski wg typowych formantow).
static const FactoryPreset kMixtur[] = {
    {"INIT", CAT_OTHER, 1, ""},
    {"BIRDS", CAT_PAD, 1, "k 1966 3277 4588 7209 push 8192 8192 20000 env 0.02 0.4 0.9 0.6"},
    {"AH CHOIR", CAT_PAD, 1, "k 1966 3277 4588 5898 push 14287 10650 12000 env 0.3 0.5 0.9 1.2"},
    {"OO PIPE", CAT_KEYS, 1, "k 1966 3277 655 655 push 5378 7477 10000 env 0.01 0.2 1 0.15"},
    {"EE REED", CAT_LEAD, 1, "k 1966 4588 655 655 push 4260 20614 16000 env 0.02 0.3 0.9 0.3"},
    {"MINOR CHORD", CAT_KEYS, 1, "k 5898 7209 8519 655 push 12118 7010 8192 env 0.005 0.6 0.6 0.5"},
    {"SUB TOWER", CAT_BASS, 1, "k 1966 3277 5898 11141 push 6000 6000 6000 env 0.005 0.3 0.9 0.2"},
    {"DARK ORGAN", CAT_KEYS, 1, "k 1966 3277 5898 655 push 6000 9000 6000 env 0.005 0.1 1 0.1"},
    {"GLASS DIVIDERS", CAT_BELL, 1, "k 1966 9830 12451 16384 push 22000 24000 26000 env 0.002 1.5 0 1.5"},
    {"VOWEL LEAD", CAT_LEAD, 1, "k 1966 3277 655 655 push 11344 17666 30000 env 0.01 0.3 0.9 0.3"},
    {"TRAUTONIUM", CAT_LEAD, 1, "k 1966 3277 4588 655 push 9000 12000 14000 env 0.03 0.3 0.9 0.25"},
    {"DEEP DRONE", CAT_PAD, 1, "k 4588 7209 9830 13762 push 4000 5000 20000 env 1.5 1 1 3"},
    {"BUZZ BASS", CAT_BASS, 1, "k 1966 3277 655 655 push 16000 20000 4000 env 0.003 0.4 0.7 0.15 oct -1"},
    {"THROAT PAD", CAT_PAD, 1, "k 3277 4588 5898 7209 push 8000 6000 24000 env 0.8 1 0.9 2"},
};

// BRASSOPHONE: oryginal (op1r, nvmsocool) nie ma presetow — 12 wlasnych z samych galek i obwiedni (obwiednia AMP ENV
// prowadzi tu tez barwe i podjazd wysokosci). k TIMBRE SWEEP VIBRATO VIB-RATE.
static const FactoryPreset kBrass[] = {
    {"INIT", CAT_OTHER, 1, ""},
    {"TRUMPET", CAT_LEAD, 1, "k 26000 22000 8000 14000 env 0.04 0.3 0.8 0.15"},
    {"FLUGEL", CAT_LEAD, 1, "k 32767 12000 10000 10000 env 0.08 0.4 0.75 0.35"},
    {"MUTED TRUMPET", CAT_LEAD, 1, "k 32767 8000 4000 12000 env 0.02 0.2 0.7 0.12"},
    {"FANFARE", CAT_LEAD, 1, "k 10000 28000 6000 15000 env 0.015 0.3 0.85 0.2"},
    {"SINE WHISTLE", CAT_LEAD, 1, "k 32767 0 16000 16000 env 0.05 0.2 1 0.3"},
    {"SOFT HORNS", CAT_PAD, 1, "k 30000 10000 6000 9000 env 0.25 0.5 0.85 0.6"},
    {"SWELL SECTION", CAT_PAD, 1, "k 30000 30000 6000 8000 env 1.2 0.5 0.9 1"},
    {"BRASS STAB", CAT_KEYS, 1, "k 22000 26000 0 0 env 0.01 0.25 0.3 0.15"},
    {"TUBA", CAT_BASS, 1, "k 28000 14000 0 0 env 0.03 0.3 0.8 0.12 oct -1"},
    {"HORN PLUCK", CAT_PLUCK, 1, "k 32767 32767 0 0 env 0.002 0.4 0 0.3"},
    {"WOBBLE HORN", CAT_FX, 1, "k 20000 24000 32767 26000 env 0.05 0.3 0.9 0.4"},
};

// HOLLOW SAINT: 8 presetow oryginalu (third_party/op1r_hollowsaint/presets.py: galki, klucz burdonu, wiek tasmy, tempo
// uderzen i oktawa 1:1; ADSR OP-1 -> sekundy (v / 8192)^1,5; efekty i tremolo OP-1 pominiete; tasma RELIC wylaczona jak
// w zapisie oryginalu) + 6 wlasnych z wlaczona tasma RELIC i innymi rytmami kola. push KLUCZ WIEK TEMPO, relic 1 = tasma.
static const FactoryPreset kHollow[] = {
    {"INIT", CAT_OTHER, 1, "push 0 8192 21064"},
    {"HOLLOW SAINT", CAT_PAD, 1, "k 16384 16384 14745 13107 push 1260 8192 21065 env 0.071 1.152 1.0 0.506"},
    {"MIDNIGHT VIGIL", CAT_PAD, 1, "k 22937 8192 3277 27852 push 8822 19660 7021 env 0.291 1.152 1.0 1.556"},
    {"HOWLING DOG", CAT_LEAD, 1, "k 18022 23592 28180 7209 push 1260 14745 30427 env 0.025 1.152 1.0 0.222"},
    {"ASH PROCESSION", CAT_KEYS, 1, "k 20316 18022 19660 16384 push 21425 16384 16384 env 0.086 1.152 1.0 0.627"},
    {"BONE FLUTE", CAT_LEAD, 1, "k 9175 1966 0 20316 push 1260 11468 21065 env 0.159 1.152 0.916 0.965 oct 1"},
    {"CRYPT DRONE", CAT_PAD, 1, "k 32767 12451 9830 23592 push 3780 22937 11702 env 0.367 1.152 1.0 1.773 oct -1"},
    {"PLAGUE DANCE", CAT_LEAD, 1, "k 14745 28180 21626 4587 push 1260 27852 25746 env 0.015 1.152 1.0 0.159"},
    {"DROWNED BELL", CAT_BELL, 1, "k 13107 11468 5898 32767 push 26466 31129 11702 env 0.121 1.152 1.0 2.234"},
    {"WORN HYMN", CAT_PAD, 1, "k 20000 12000 6000 24000 push 1260 20000 2340 env 0.4 1.2 1.0 1.8 relic 1"},
    {"TAVERN TAPE", CAT_LEAD, 1, "k 16000 22000 20000 8000 push 1260 26000 21065 env 0.03 1.152 1.0 0.25 relic 1"},
    {"GHOST RECORDING", CAT_PAD, 1, "k 26000 6000 0 30000 push 3780 32767 2340 env 1 1.5 1.0 2.5 relic 1"},
    {"PILGRIM DRONE", CAT_PAD, 1, "k 32767 9000 14000 20000 push 13867 8192 16384 env 0.5 1.2 1.0 2 oct -1"},
    {"RATTLE DANCE", CAT_LEAD, 1, "k 10000 26000 32767 6000 push 1260 8192 30427 env 0.01 1.152 1.0 0.2"},
    {"DUSTY BELL", CAT_BELL, 1, "k 0 8000 0 32767 push 1260 28000 2340 env 0.05 1.5 0.8 2.5 relic 1"},
};

// BLOODMOON: 8 presetow oryginalu (presets/*.aif autorki: galki i oktawa 1:1, ADSR OP-1 -> sekundy (v / 8192)^1,5;
// efekty OP-1 pominiete).
static const FactoryPreset kBlood[] = {
    {"INIT", CAT_OTHER, 1, ""},
    {"BLOOD RITE", CAT_PAD, 1, "k 32767 26000 16000 12000 env 0.001 1.851 0.61 1.773"},
    {"CATHEDRAL", CAT_KEYS, 1, "k 14000 20000 7000 9000 env 0.048 0.722 0.793 1.773"},
    {"CRYPT DRONE", CAT_PAD, 1, "k 30000 0 9000 2500 env 0.627 0.965 1.0 2.472 oct -1"},
    {"FUNERAL BELL", CAT_BELL, 1, "k 9000 32767 2500 4000 env 0.001 1.152 0.183 2.234"},
    {"HOLLOW SAINT", CAT_KEYS, 1, "k 0 12000 3500 6000 env 0.019 0.722 0.732 1.152"},
    {"MOON TREMOR", CAT_LEAD, 1, "k 18000 16000 32767 30000 env 0.001 0.722 0.916 0.627"},
    {"SUNKEN PIPES", CAT_PAD, 1, "k 26000 4000 13000 1500 env 0.125 0.722 1.0 2.234 oct -1"},
    {"WITCH CHOIR", CAT_PAD, 1, "k 20000 9000 22000 7500 env 0.222 0.722 0.855 1.938"},
};

// BLOT: oryginal (op1r, pcppcp) nie ma presetow — 14 wlasnych: k CUTOFF RES ENV-MOD DECAY, push WAVE ACCENT DRIVE,
// MONO + GLIDE jako slide 303 (akcent: nuty z velocity >= 100).
static const FactoryPreset kBlot[] = {
    {"INIT", CAT_OTHER, 1, ""},
    {"ACID CLASSIC", CAT_BASS, 1, "k 7000 26000 20000 9000 voice 1 0.06 0 0 env 0.002 0.3 1 0.05"},
    {"SQUELCH", CAT_BASS, 1, "k 6000 31000 24000 12000 voice 1 0.06 0 0 env 0.002 0.3 1 0.05 push 0 26000 6000"},
    {"SQUARE ACID", CAT_BASS, 1, "k 8000 27000 18000 10000 voice 1 0.06 0 0 env 0.002 0.3 1 0.05 push 32767 20000 0"},
    {"DEEP 303", CAT_BASS, 1, "k 4000 18000 14000 16000 voice 1 0.05 0 0 env 0.002 0.3 1 0.08 oct -1"},
    {"RUBBER BASS", CAT_BASS, 1, "k 5000 12000 22000 6000 voice 1 0.03 0 0 env 0.002 0.3 1 0.06"},
    {"DIRTY ACID", CAT_BASS, 1, "k 9000 28000 22000 10000 voice 1 0.06 0 0 env 0.002 0.3 1 0.05 push 0 24000 26000"},
    {"LONG SWEEP", CAT_LEAD, 1, "k 6000 27000 26000 30000 voice 1 0.1 0 0 env 0.003 0.5 1 0.2"},
    {"SCREAM LEAD", CAT_LEAD, 1, "k 14000 30000 20000 18000 voice 1 0.08 0 0 env 0.003 0.3 1 0.15 push 16384 26000 32767"},
    {"POLY ACID", CAT_KEYS, 1, "k 9000 24000 18000 9000 env 0.002 0.3 0.7 0.15"},
    {"BLIP", CAT_PLUCK, 1, "k 10000 22000 26000 2000 env 0.001 0.2 0 0.08"},
    {"FAT SQUARE", CAT_BASS, 1, "k 12000 16000 10000 12000 voice 1 0.04 0 0 env 0.002 0.3 1 0.08 push 32767 20000 16000 oct -1"},
    {"WARM PAD", CAT_PAD, 1, "k 14000 10000 6000 32767 env 0.4 1 0.8 1.2"},
    {"ZAP", CAT_FX, 1, "k 2000 30000 32767 3000 env 0.001 0.15 0 0.08"},
    {"RESO DRONE", CAT_FX, 1, "k 12000 32000 8000 32767 env 0.5 1 1 2 push 16384 20000 10000"},
};

// Zestaw dla katalogu silnika: wskaznik i liczba pozycji
const FactoryPreset* factoryList(const char* eng, int* n, int* engine) {
    if (!__builtin_strcmp(eng, "taffy")) { *n = sizeof kTaffy / sizeof kTaffy[0]; *engine = ENG_TAFFY; return kTaffy; }
    if (!__builtin_strcmp(eng, "blot")) { *n = sizeof kBlot / sizeof kBlot[0]; *engine = ENG_BLOT; return kBlot; }
    if (!__builtin_strcmp(eng, "bloodmoon")) { *n = sizeof kBlood / sizeof kBlood[0]; *engine = ENG_BLOOD; return kBlood; }
    if (!__builtin_strcmp(eng, "hollowsaint")) { *n = sizeof kHollow / sizeof kHollow[0]; *engine = ENG_HOLLOW; return kHollow; }
    if (!__builtin_strcmp(eng, "brass")) { *n = sizeof kBrass / sizeof kBrass[0]; *engine = ENG_BRASS; return kBrass; }
    if (!__builtin_strcmp(eng, "mixtur")) { *n = sizeof kMixtur / sizeof kMixtur[0]; *engine = ENG_MIXTUR; return kMixtur; }
    if (!__builtin_strcmp(eng, "supersaw")) { *n = sizeof kSupersaw / sizeof kSupersaw[0]; *engine = ENG_SUPERSAW; return kSupersaw; }
    if (!__builtin_strcmp(eng, "vapora")) { *n = sizeof kVapora / sizeof kVapora[0]; *engine = ENG_VAPORA; return kVapora; }
    if (!__builtin_strcmp(eng, "monolit")) { *n = sizeof kMonolit / sizeof kMonolit[0]; *engine = ENG_MONOLIT; return kMonolit; }
    if (!__builtin_strcmp(eng, "sid")) { *n = sizeof kSid / sizeof kSid[0]; *engine = ENG_SID; return kSid; }
    if (!__builtin_strcmp(eng, "oxbowplus")) { *n = sizeof kOxbowPlus / sizeof kOxbowPlus[0]; *engine = ENG_OXBOW; return kOxbowPlus; }
    if (!__builtin_strcmp(eng, "oxbow")) { *n = sizeof kOxbow / sizeof kOxbow[0]; *engine = ENG_OXBOW; return kOxbow; }
    if (!__builtin_strcmp(eng, "reactor")) { *n = sizeof kReactor / sizeof kReactor[0]; *engine = ENG_REACTOR; return kReactor; }
    if (!__builtin_strcmp(eng, "mbira")) { *n = sizeof kMbira / sizeof kMbira[0]; *engine = ENG_MBIRA; return kMbira; }
    if (!__builtin_strcmp(eng, "eva")) { *n = sizeof kEva / sizeof kEva[0]; *engine = ENG_EVA; return kEva; }
    if (!__builtin_strcmp(eng, "elg")) { *n = sizeof kElg / sizeof kElg[0]; *engine = ENG_ELG; return kElg; }
    if (!__builtin_strcmp(eng, "pew")) { *n = sizeof kPew / sizeof kPew[0]; *engine = ENG_PEW; return kPew; }
    if (!__builtin_strcmp(eng, "wavetableplus")) { *n = sizeof kWavetablePlus / sizeof kWavetablePlus[0]; *engine = ENG_WAVETABLE; return kWavetablePlus; }
    *n = sizeof kWavetable / sizeof kWavetable[0];
    *engine = ENG_WAVETABLE;
    return kWavetable;
}

// Instrument z presetu fabrycznego (katalog silnika + nazwa) — np. do utworow demo. false = brak presetu.
bool factoryInstrument(const char* eng, const char* name, Track& t) {
    int n = 0, engine = ENG_WAVETABLE;
    const FactoryPreset* list = factoryList(eng, &n, &engine);
    for (int i = 0; i < n; i++) {
        if (__builtin_strcmp(list[i].name, name)) continue;
        t.engine = (uint8_t)engine;
        t.alt = false;
        PresetStore::initInstrument(t);
        static char line[2048];
        char* tok[128];
        snprintf(line, sizeof line, "%s", list[i].inst);
        parseInstrument(t, tok, tokenize(line, tok, 128), 0);
        snprintf(t.preset, sizeof t.preset, "%s", name);
        return true;
    }
    return false;
}

}  // namespace dp

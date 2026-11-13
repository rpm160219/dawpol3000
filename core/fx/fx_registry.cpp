#include "fx/fx_registry.h"
#include <stdio.h>
#include <string.h>

namespace dp {

#define CONT8 {0, 0, 0, 0, 0, 0, 0, 0}
static const FxInfo kFx[FX_COUNT] = {
    {"NONE", "-", FXC_NONE, {"NO EFFECT.", "", "", ""}, {"", "", "", ""}, {"", "", "", ""}, {0, 0, 0, 0, 0, 0, 0, 0}, CONT8, CONT8, {}},
    {"JUNI CHORUS", "CHORUS", FXC_INSERT,
     {"PORT OF THE OP1R JUNI CHORUS BY", "HUNDEKRAFT, BIT-EXACT. EARLY-80S", "BBD ENSEMBLE: MODES I, II, I+II,", "DEPTH, DARKNESS OF THE WET, MIX."},
     {"MODE", "DEPTH", "TONE", "MIX"}, {"", "", "", ""}, {4608, 27852, 6000, 13239, 0, 0, 0, 0}, CONT8, CONT8, {}},
    {"SISTER", "SISTER", FXC_AUX,
     {"PORT OF THE OP1R SISTER BY HUNDEKRAFT,", "BIT-EXACT. A CATHEDRAL REVERB WHOSE", "TAIL SINGS (CHOIR), RINGS IN TUNE", "(PIPES) OR SWELLS BACKWARDS."},
     {"SPACE", "VOICE", "SWELL", "MIX"}, {"", "KEY", "", ""}, {19661, 6554, 0, 6620, 0, 0, 16384, 0}, CONT8, CONT8, {}},
    {"SNOWGLOBE", "SNOW", FXC_AUX,
     {"PORT OF THE OP1R SNOWGLOBE BY", "HUNDEKRAFT, BIT-EXACT. SHIMMER REVERB:", "THE TAIL CLIMBS AN OCTAVE AT A TIME.", "SHAKE AT THE TOP FREEZES THE TAIL."},
     {"SNOWFALL", "SHIMMER", "SIZE", "SHAKE"}, {"", "", "", ""}, {16384, 16384, 16384, 6620, 0, 0, 0, 0}, CONT8, CONT8, {}},
    {"RACKVERB", "RACK", FXC_AUX,
     {"PORT OF THE OP1R RACKVERB BY HUNDEKRAFT,", "BIT-EXACT. LATE-80S RACK REVERBS AND", "EFFECTS: UNITS VERB I, VERB II, FEX,", "QUAD; OWN PROGRAMS, ONE PER STEP."},
     {"FEEDBACK", "CUTOFF", "MIX", "PROGRAM"}, {"SIZE", "MODE", "UNIT", ""}, {0, 31763, 16384, 4424, 0, 32767, 8192, 4096},
     {FXK_CONT, FXK_CONT, FXK_CONT, FXK_MID, FXK_CONT, FXK_CONT, FXK_MID, FXK_MID}, {0, 0, 0, 100, 0, 0, 2, 4},
     {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "AUTH|CLEAN", "VERB I|VERB II|FEX|QUAD"}},
    {"TAPE", "TAPE", FXC_AUX,
     {"PORT OF THE OP1R TAPE BY PCPPCP,", "BIT-EXACT. TAPE ECHO: AGE DARKENS THE", "REPEATS, WOW AND FLUTTER ON THE HEAD,", "EXTRA HEADS AND TEMPO SYNC ON PAGE 2."},
     {"AGE", "FEEDBACK", "TIME", "WOW"}, {"HEADS", "MIX", "SYNC", ""}, {12000, 14000, 12000, 6000, 0, 16384, 16384, 0},
     {FXK_CONT, FXK_CONT, FXK_CONT, FXK_CONT, FXK_CONT, FXK_MID, FXK_CONT, FXK_INT}, {0, 0, 0, 0, 0, 5, 0, 10},
     {nullptr, nullptr, nullptr, nullptr, nullptr, "1|2|3|DOTTED|SWING", nullptr, "FREE|1/16|1/8T|1/16D|1/8|1/4T|1/8D|1/4|1/4D|1/2"}},
    {"FATHER", "FATHER", FXC_AUX,
     {"PORT OF THE OP1R FATHER BY HUNDEKRAFT,", "BIT-EXACT. AMBIENCE REVERB: DISTANCE,", "A DURATION GATE ON THE TAIL, COLOUR", "OF THE ROOM; CLASSIC MODE ON PAGE 2."},
     {"DISTANCE", "GATE", "COLOR", "MIX"}, {"", "", "CLASSIC", ""}, {16384, 0, 18022, 6620, 0, 0, 0, 0},
     {FXK_CONT, FXK_CONT, FXK_CONT, FXK_CONT, FXK_CONT, FXK_CONT, FXK_CONT, FXK_MID}, {0, 0, 0, 0, 0, 0, 0, 2},
     {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "OFF|ON"}},
    {"STUTTER", "STUTTER", FXC_INSERT,
     {"PORT OF THE OP1R STUTTER BY", "HUNDEKRAFT, BIT-EXACT. BEAT-LOCKED", "STUTTER: REPEAT, BRAKE, REVERSE, GATE;", "REPEATS AND FREEZE ON PAGE 2."},
     {"RATE", "MODE", "SHAPE", "CHANCE"}, {"REPEATS", "", "", "FREEZE"}, {17152, 2304, 8000, 4965, 0, 13568, 0, 0},
     {FXK_MID, FXK_MID, FXK_CONT, FXK_CONT, FXK_INT, FXK_SEL512, FXK_CONT, FXK_CONT}, {6, 4, 0, 0, 2, 6, 0, 0},
     {"1/4|1/8|1/8T|1/16|1/16T|1/32", "REPEAT|BRAKE|REVERSE|GATE", nullptr, nullptr, "OFF|ON", "2|3|4|6|8|16", nullptr, nullptr}},
    {"CRUSH", "CRUSH", FXC_INSERT,
     {"PORT OF CRUSH (MAIKO #250, REBUILT BY", "HUNDEKRAFT WITH THE OWNER'S CONSENT),", "BIT-EXACT. BIT DEPTH, SAMPLE RATE", "REDUCTION, DRIVE AND MIX."},
     {"BITS", "RATE", "DRIVE", "MIX"}, {"", "", "", ""}, {10922, 21239, 8192, 32767, 0, 0, 0, 0}, CONT8, CONT8, {}},
    {"ENTE", "ENTE", FXC_INSERT,
     {"PORT OF THE OP1R ENTE BY PCPPCP,", "BIT-EXACT. SIDECHAIN COMPRESSOR: SELF,", "TRACKS 1..4 (T1..T4), ALL TRACKS (TAPE)", "OR A TEMPO PULSE (BEAT) DUCKS THE SOUND."},
     {"THRESHOLD", "RATIO", "RELEASE", "SOURCE"}, {"ATTACK", "MAKEUP", "SC HPF", ""}, {32767, 19840, 12000, 0, 0, 4000, 0, 9000},
     {FXK_CONT, FXK_CONT, FXK_CONT, FXK_LIST, FXK_CONT, FXK_CONT, FXK_CONT, FXK_CONT}, {0, 0, 0, 16, 0, 0, 0, 0},
     {nullptr, nullptr, nullptr, "SELF|T1|T2|T3|T4|TAPE|IN|INST|BEAT 1/1|BEAT 1/2|BEAT 1/4|BEAT 1/8|BEAT 1/16|BEAT 1/4T|BEAT 1/8T|BEAT 1/16T",
      nullptr, nullptr, nullptr, nullptr}},
};

const FxInfo& fxInfo(int fx) { return kFx[fx >= 0 && fx < FX_COUNT ? fx : 0]; }

void fxDefaults(FxSlot& s, int fx) {
    s.fx = (uint8_t)(fx >= 0 && fx < FX_COUNT ? fx : FX_NONE);
    s.bypass = false;
    for (int i = 0; i < 8; i++) s.w[i] = kFx[s.fx].def[i];
}

int fxKnobWord(int page, int k) { return page ? (k < 3 ? 5 + k : 4) : k; }

int fxStepOf(const FxInfo& fi, int word, int16_t v) {
    const int n = fi.steps[word] ? fi.steps[word] : 1, x = v < 0 ? 0 : v;
    int i = 0;
    switch (fi.kind[word]) {
    case FXK_MID: i = (x * n) >> 15; break;
    case FXK_LIST: i = (x * (n - 1) + 16383) / 32767; break;
    case FXK_INT: i = x; break;
    case FXK_SEL512: i = (x >> 9) % n; break;
    default: return 0;
    }
    return i < 0 ? 0 : (i >= n ? n - 1 : i);
}

int16_t fxWordOfStep(const FxInfo& fi, int word, int i) {
    const int n = fi.steps[word] ? fi.steps[word] : 1;
    if (i < 0) i = 0;
    if (i >= n) i = n - 1;
    switch (fi.kind[word]) {
    case FXK_MID: return (int16_t)(((2 * i + 1) * 32768) / (2 * n));
    case FXK_LIST: return (int16_t)(n > 1 ? i * 32767 / (n - 1) : 0);
    case FXK_INT: return (int16_t)i;
    case FXK_SEL512: return (int16_t)(i * 512);
    default: return 0;
    }
}

void fxValueText(const FxInfo& fi, int word, int16_t v, char* out, int n) {
    if (fi.kind[word] == FXK_CONT) { snprintf(out, n, "%d", ((v < 0 ? 0 : v) * 99 + 16383) / 32767); return; }
    const int i = fxStepOf(fi, word, v);
    const char* l = fi.labels[word];
    if (!l) { snprintf(out, n, "%d", i + 1); return; }
    for (int k = 0; k < i && l; k++) { l = strchr(l, '|'); if (l) l++; }
    if (!l) { snprintf(out, n, "%d", i + 1); return; }
    int m = 0;
    while (l[m] && l[m] != '|' && m < n - 1) { out[m] = l[m]; m++; }
    out[m] = 0;
}

int fxListOf(int cls, int* out, int max) {
    int m = 0;
    for (int i = 1; i < FX_COUNT && m < max; i++) if (kFx[i].cls == cls) out[m++] = i;
    return m;
}

}  // namespace dp

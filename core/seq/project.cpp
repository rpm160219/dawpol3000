#include "seq/project.h"
#include <stdio.h>
#include <string.h>

namespace dp {

const char* engineName(int e) {
    switch (e) {
    case ENG_WAVETABLE: return "WAVETABLE";
    case ENG_SAW: return "TEST SAW";
    case ENG_TAFFY: return "TAFFY";
    case ENG_PEW: return "PEW";
    case ENG_ELG: return "ELG";
    case ENG_EVA: return "EVA";
    case ENG_MBIRA: return "MBIRA";
    case ENG_REACTOR: return "REACTOR";
    case ENG_OXBOW: return "OXBOW";
    case ENG_SID: return "SID";
    case ENG_MONOLIT: return "MONOLIT";
    case ENG_VAPORA: return "VAPORA";
    case ENG_SUPERSAW: return "SUPERSAW";
    case ENG_MIXTUR: return "MIXTUR";
    case ENG_BRASS: return "BRASSOPHONE";
    case ENG_HOLLOW: return "HOLLOW SAINT";
    case ENG_BLOOD: return "BLOODMOON";
    case ENG_BLOT: return "BLOT";
    case ENG_AETHER: return "AETHER";
    case ENG_ARC: return "ARC";
    case ENG_DDDD: return "DDDD";
    case ENG_JUNI: return "JUNI";
    case ENG_OUTATIME: return "OUTATIME";
    case ENG_GIN: return "GIN";
    case ENG_PARALLAX: return "PARALLAX";
    default: return "?";
    }
}

const char* lfoShapeName(int s) {
    static const char* n[LFO_SHAPES] = {"SINE", "TRI", "SAW", "SQR", "RND"};
    return s >= 0 && s < LFO_SHAPES ? n[s] : "?";
}
const char* lfoDestName(int d) {
    static const char* n[LFO_DESTS] = {"WAVE", "DTUN", "CUT", "RES", "PTCH", "VOL", "W1", "W2", "P1", "P2", "LV1", "LV2"};
    return d >= 0 && d < LFO_DESTS ? n[d] : "?";
}

static const struct { const char* name; float beats; } kDivs[LFO_DIVS] = {
    {"1/1", 4.f},          {"1/2", 2.f},          {"1/2T", 4.f / 3.f},   {"1/4", 1.f},     {"1/4T", 2.f / 3.f},
    {"1/8", 0.5f},         {"1/8T", 1.f / 3.f},   {"1/16", 0.25f},       {"1/16T", 1.f / 6.f}, {"1/32", 0.125f},
    {"1/32T", 1.f / 12.f}, {"1/64", 1.f / 16.f},  {"1/64T", 1.f / 24.f}, {"1/128", 1.f / 32.f}, {"1/128T", 1.f / 48.f},
};
const char* destNameFor(int engine, int d) {
    static const char* tf[4] = {"WAVE", "DCW", "ENV", "RES"};
    static const char* pw[4] = {"REG", "PERC", "DRV", "LES"};
    if (engine == ENG_TAFFY && d >= 0 && d < 4) return tf[d];
    static const char* ev[4] = {"PRES", "FREQ", "OXY", "COM"};
    if (engine == ENG_PEW && d >= 0 && d < 4) return pw[d];
    static const char* mbn[4] = {"STRK", "TINE", "BOX", "BUZZ"};
    if (engine == ENG_EVA && d >= 0 && d < 4) return ev[d];
    static const char* rxn[4] = {"COOL", "RAD", "RODS", "HEAT"};
    if (engine == ENG_MBIRA && d >= 0 && d < 4) return mbn[d];
    static const char* obn[4] = {"CUT", "RES", "ENV", "DTUN"};
    if (engine == ENG_REACTOR && d >= 0 && d < 4) return rxn[d];
    static const char* sdn[4] = {"WAVE", "MOD", "CUT", "RES"};
    static const char* mnn[4] = {"SHPE", "CUT", "CONT", "HEAT"};
    if (engine == ENG_SID && d >= 0 && d < 4) return sdn[d];
    if (engine == ENG_MONOLIT && d >= 0 && d < 4) return mnn[d];
    static const char* vpn[4] = {"MRPH", "THCK", "BRT", "MOTN"};
    if (engine == ENG_VAPORA && d >= 0 && d < 4) return vpn[d];
    static const char* ssn[4] = {"DTUN", "MRPH", "SUB", "DRV"};
    if (engine == ENG_SUPERSAW && d >= 0 && d < 4) return ssn[d];
    static const char* pxn[4] = {"WAVE", "UNI", "CUT", "RES"};
    if (engine == ENG_PARALLAX && d >= 0 && d < 4) return pxn[d];
    static const char* otn[4] = {"YEAR", "SPD", "PWR", "TRL"};
    if (engine == ENG_OUTATIME && d >= 0 && d < 4) return otn[d];
    static const char* jnn[4] = {"TONE", "SHPE", "SWP", "CHOR"};
    if (engine == ENG_JUNI && d >= 0 && d < 4) return jnn[d];
    static const char* ddn[4] = {"SRC", "FBK", "DSTR", "HAND"};
    if (engine == ENG_DDDD && d >= 0 && d < 4) return ddn[d];
    static const char* acn[4] = {"CUR", "CARB", "HISS", "STRK"};
    if (engine == ENG_ARC && d >= 0 && d < 4) return acn[d];
    if (engine == ENG_ARC && d == DEST_W1) return "RAT";  // strona 2: proporcja 2. lampy, sprzezenie, jasnosc
    if (engine == ENG_ARC && d == DEST_W2) return "CPL";
    if (engine == ENG_ARC && d == DEST_P1) return "BRT";
    static const char* aen[4] = {"ORBT", "FORM", "SPCE", "DPTH"};
    if (engine == ENG_AETHER && d >= 0 && d < 4) return aen[d];
    if (engine == ENG_AETHER && d == DEST_W1) return "PTLT";  // wirtualne przechylenie
    if (engine == ENG_AETHER && d == DEST_W2) return "VTLT";
    if (engine == ENG_AETHER && d == DEST_P1) return "SHAK";
    static const char* bln[4] = {"CUT", "RES", "ENV", "DEC"};
    if (engine == ENG_BLOT && d >= 0 && d < 4) return bln[d];
    static const char* bmn[4] = {"BLD", "ECL", "HNT", "MOON"};
    if (engine == ENG_BLOOD && d >= 0 && d < 4) return bmn[d];
    static const char* hsn[4] = {"DRON", "STR", "BUZZ", "CATH"};
    if (engine == ENG_HOLLOW && d >= 0 && d < 4) return hsn[d];
    static const char* brn[4] = {"TMBR", "SWP", "VIB", "VRT"};
    if (engine == ENG_BRASS && d >= 0 && d < 4) return brn[d];
    static const char* mxn[4] = {"UT A", "UT B", "UT C", "UT D"};
    if (engine == ENG_MIXTUR && d >= 0 && d < 4) return mxn[d];
    if (engine == ENG_MIXTUR && d == DEST_W1) return "F1";  // formanty (strona 2)
    if (engine == ENG_MIXTUR && d == DEST_W2) return "F2";
    if (engine == ENG_MIXTUR && d == DEST_P1) return "RES";
    if (engine == ENG_SUPERSAW && d == DEST_W1) return "CUT";  // filtr DAW-POL
    if (engine == ENG_SUPERSAW && d == DEST_W2) return "RES";
    if (engine == ENG_OXBOW && d >= 0 && d < 4) return obn[d];
    if (engine == ENG_OXBOW && d == DEST_W1) return "PW1";  // OXBOW+: szerokosc pulsu zamiast fali
    if (engine == ENG_OXBOW && d == DEST_W2) return "PW2";
    return lfoDestName(d);
}

const char* presetCatName(int c) {
    static const char* n[PRESET_CATS] = {"BASS", "LEAD", "PAD", "KEYS", "PLUCK", "BELL", "FX", "DRUMS", "OTHER"};
    return c >= 0 && c < PRESET_CATS ? n[c] : "OTHER";
}

const char* filterModeName(int m) {
    static const char* n[FLT_MODES] = {"LP24", "LP12", "BP", "HP"};
    return m >= 0 && m < FLT_MODES ? n[m] : "?";
}

const char* lfoDivName(int i) { return i >= 0 && i < LFO_DIVS ? kDivs[i].name : "?"; }
float lfoDivBeats(int i) { return i >= 0 && i < LFO_DIVS ? kDivs[i].beats : 1.f; }

void engineDefaultKnobs(int engine, int16_t k[4]) {
    static const int16_t wt[4] = {16384, 8192, 20000, 8192}, tf[4] = {16384, 8192, 0, 8192}, pw[4] = {16384, 12000, 8000, 16384}, ev[4] = {20000, 16384, 12000, 3000}, mb[4] = {14000, 16384, 12000, 0}, rx[4] = {16384, 3000, 16384, 9000}, ob[4] = {11869, 5677, 23738, 2560}, sd[4] = {6656, 16384, 32136, 8192}, mn[4] = {16384, 14745, 11469, 9830}, vp[4] = {9000, 26000, 18500, 12500}, ss[4] = {8000, 0, 12000, 6000}, mx[4] = {1966, 3277, 4588, 7209}, br[4] = {19660, 16384, 3932, 7536}, hs[4] = {16384, 16384, 12000, 12000}, bm[4] = {14000, 20000, 7000, 9000}, bl[4] = {9000, 22000, 18000, 9000}, ae[4] = {15671, 11469, 16384, 32767}, ac[4] = {12000, 6000, 5000, 9000}, dd[4] = {0, 24000, 20000, 8000}, jn[4] = {18000, 16384, 11000, 9800}, ot[4] = {16549, 6620, 8274, 11584}, px[4] = {16384, 0, 22000, 6000};
    const int16_t* s = engine == ENG_TAFFY ? tf : engine == ENG_PEW ? pw : engine == ENG_EVA ? ev : engine == ENG_MBIRA ? mb : engine == ENG_REACTOR ? rx : engine == ENG_OXBOW ? ob : engine == ENG_SID ? sd : engine == ENG_MONOLIT ? mn : engine == ENG_VAPORA ? vp : engine == ENG_SUPERSAW ? ss : engine == ENG_MIXTUR ? mx : engine == ENG_BRASS ? br : engine == ENG_HOLLOW ? hs : engine == ENG_BLOOD ? bm : engine == ENG_BLOT ? bl : engine == ENG_AETHER ? ae : engine == ENG_ARC ? ac : engine == ENG_DDDD ? dd : engine == ENG_JUNI ? jn : engine == ENG_OUTATIME ? ot : engine == ENG_PARALLAX ? px : wt;
    for (int i = 0; i < 4; i++) k[i] = s[i];
}

void engineDefaultPush(int engine, int16_t p[3]) {
    p[0] = p[1] = p[2] = 0;
    if (engine == ENG_HOLLOW) { p[1] = 8192; p[2] = 21064; }  // plugin.json page2_defaults: FOLLOW, wiek 25 %, 1/8
}

float glideSeconds(const Track& t, float bpm) {
    if (!t.glideSync) return t.glide;
    return lfoDivBeats(t.glideDiv) * 60.f / (bpm > 1 ? bpm : 120.f);
}

int Project::countTracks(int type) const {
    int n = 0;
    for (int i = 0; i < nTracks; i++) if (tracks[i].type == type) n++;
    return n;
}

bool Project::canAddTrack(int type) const {
    if (nTracks >= MAX_TRACKS) return false;
    return countTracks(type) < (type == TRACK_MIDI ? MAX_MIDI_TRACKS : MAX_AUDIO_TRACKS);
}

int Project::addTrack(int type, const char* nm) {
    if (!canAddTrack(type)) return -1;
    Track& t = tracks[nTracks];
    t = Track();
    t.type = (uint8_t)type;
    t.color = (uint8_t)(nTracks % 32);
    snprintf(t.name, sizeof t.name, "%s", nm);
    return nTracks++;
}

void Project::removeTrack(int t) {
    if (t < 0 || t >= nTracks) return;
    for (int i = 0; i < nClips;) {
        if (clips[i].track == t) { removeClip(i); continue; }
        if (clips[i].track > t) clips[i].track--;
        i++;
    }
    for (int i = t; i < nTracks - 1; i++) tracks[i] = tracks[i + 1];
    nTracks--;
}

bool Project::audible(int t) const {
    if (t < 0 || t >= nTracks || tracks[t].mute) return false;
    bool anySolo = false;
    for (int i = 0; i < nTracks; i++) if (tracks[i].solo) anySolo = true;
    return !anySolo || tracks[t].solo;
}

int Project::addPattern(const char* nm, int bars) {
    if (nPatterns >= MAX_PATTERNS) return -1;
    Pattern& p = patterns[nPatterns];
    p.clear();
    snprintf(p.name, sizeof p.name, "%s", nm);
    p.setBars(bars);
    return nPatterns++;
}

int Project::clipAt(int track, int bar) const {
    for (int i = 0; i < nClips; i++) {
        const Clip& c = clips[i];
        if (c.track == track && bar >= c.startBar && bar < c.startBar + clipBars(c)) return i;
    }
    return -1;
}

bool Project::fits(int track, int start, int bars, int ignore) const {
    if (start < 0 || start + bars > MAX_SONG_BARS) return false;
    for (int i = 0; i < nClips; i++) {
        if (i == ignore || clips[i].track != track) continue;
        int s = clips[i].startBar, e = s + clipBars(clips[i]);
        if (start < e && start + bars > s) return false;
    }
    return true;
}

int Project::addClip(int track, int pattern, int start) {
    if (nClips >= MAX_CLIPS || pattern < 0 || pattern >= nPatterns) return -1;
    if (!fits(track, start, patterns[pattern].bars())) return -1;
    clips[nClips] = {(uint8_t)track, (uint8_t)pattern, (uint16_t)start};
    tracks[track].lastPattern = pattern;
    return nClips++;
}

void Project::removeClip(int i) {
    if (i < 0 || i >= nClips) return;
    clips[i] = clips[--nClips];
}

int Project::songBars() const {
    int e = 1;
    for (int i = 0; i < nClips; i++) {
        int ce = clips[i].startBar + clipBars(clips[i]);
        if (ce > e) e = ce;
    }
    return e;
}

void Project::songRange(int& start, int& end) const {
    if (loopOn && loopOut > loopIn) { start = loopIn * TICKS_PER_BAR; end = loopOut * TICKS_PER_BAR; }
    else { start = 0; end = songBars() * TICKS_PER_BAR; }
}

void Project::nextPatternName(char* out, int n) const { snprintf(out, n, "PATTERN %02d", nPatterns + 1); }

void Project::nextTrackName(int type, char* out, int n) const {
    snprintf(out, n, "%s %02d", type == TRACK_MIDI ? "MIDI" : "AUDIO", countTracks(type) + 1);
}

}  // namespace dp

namespace dp {
void ginLoadKit(Track& t, int kit) {
    if (kit < 0 || kit >= ginFactoryCount()) kit = 0;
    t.ginKit = (int8_t)kit;
    for (int i = 0; i < GIN_KIT_SOUNDS; i++) ginFactorySound(kit, i, &t.gin[i]);
}
void ginEnsure(Track& t) { if (t.ginKit < 0) ginLoadKit(t, 0); }
bool ginSoundEdited(const Track& t, int s) {
    GinSound f;
    ginFactorySound(t.ginKit < 0 ? 0 : t.ginKit, s, &f);
    return t.gin[s].morph != 0 || t.gin[s].side != 0 || memcmp(f.ep, t.gin[s].ep, sizeof f.ep) != 0;
}
}  // namespace dp

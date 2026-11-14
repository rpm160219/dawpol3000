#include <ctype.h>
#include "io/project_file.h"
#include "synth/polivoks_presets.h"
#include "gfx/canvas.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

namespace dp {

static const int FORMAT_VERSION = 1;


static void putStr(FILE* f, const char* s) {
    fputc('"', f);
    for (; *s; s++) if (*s != '"' && *s != '\n') fputc(*s, f);
    fputc('"', f);
}

void writeInstrument(FILE* f, const Track& t) {
    fprintf(f, " engine %s k %d %d %d %d env %.4f %.4f %.4f %.4f",
            t.engine == ENG_PROPHET ? "prophet5" : t.engine == ENG_POLIVOKS ? "polivoks" : t.engine == ENG_SAW ? "saw" : (t.engine == ENG_TAFFY ? "taffy" : (t.engine == ENG_PEW ? "pew" : (t.engine == ENG_ELG ? "elg" : (t.engine == ENG_EVA ? "eva" : (t.engine == ENG_MBIRA ? "mbira" : (t.engine == ENG_REACTOR ? "reactor" : (t.engine == ENG_OXBOW ? "oxbow" : (t.engine == ENG_SID ? "sid" : (t.engine == ENG_MONOLIT ? "monolit" : (t.engine == ENG_VAPORA ? "vapora" : (t.engine == ENG_SUPERSAW ? "supersaw" : (t.engine == ENG_MIXTUR ? "mixtur" : (t.engine == ENG_BRASS ? "brass" : (t.engine == ENG_HOLLOW ? "hollowsaint" : (t.engine == ENG_BLOOD ? "bloodmoon" : (t.engine == ENG_BLOT ? "blot" : (t.engine == ENG_AETHER ? "aether" : (t.engine == ENG_ARC ? "arc" : (t.engine == ENG_DDDD ? "dddd" : (t.engine == ENG_JUNI ? "juni" : (t.engine == ENG_OUTATIME ? "outatime" : (t.engine == ENG_GIN ? "gin" : (t.engine == ENG_PARALLAX ? "parallax" : (t.engine == ENG_LOUNGE ? "lounge" : (t.engine == ENG_FELT ? "felt" : (t.engine == ENG_GRAND ? "grand" : (t.engine == ENG_VELOUR ? "velour" : (t.engine == ENG_LINEA ? "linea" : "wavetable")))))))))))))))))))))))))))), t.knob[0],
            t.knob[1], t.knob[2], t.knob[3], t.env[0], t.env[1], t.env[2], t.env[3]);
    fprintf(f, " envamt %.4f fenv %.4f %.4f %.4f %.4f %.4f", t.envAmt, t.fenv[0], t.fenv[1], t.fenv[2], t.fenv[3],
            t.fenvAmt);
    for (int l = 0; l < 2; l++)
        fprintf(f, " lfo%d %d %.4f %.4f %d %d %d", l + 1, t.lfo[l].shape, t.lfo[l].rate, t.lfo[l].depth, t.lfo[l].dest,
                t.lfo[l].sync, t.lfo[l].div);
    const WtExt& x = t.ext;
    fprintf(f, " alt %d o1 %d %d %.2f %.4f o2 %d %d %.2f %.4f sub %.4f fmode %d drv %.4f", t.alt, x.osc[0].wave,
            x.osc[0].semi, x.osc[0].cents, x.osc[0].level, x.osc[1].wave, x.osc[1].semi, x.osc[1].cents, x.osc[1].level,
            x.sub, x.fmode, x.drive);
    if (t.engine == ENG_OXBOW && t.alt) {
        const ObExt& o = t.obx;
        fprintf(f, " obx %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", o.wave[0], o.wave[1], o.semi[0], o.semi[1],
                o.cents1, o.level[0], o.level[1], o.pw[0], o.pw[1], o.noise, o.sync, o.xmod, o.envPitch, o.keytrk, o.velflt,
                o.vint, o.ftype, o.drive);
    }
    if (t.engine == ENG_PEW || t.engine == ENG_EVA || t.engine == ENG_MBIRA || t.engine == ENG_REACTOR || t.engine == ENG_OXBOW || t.engine == ENG_SID || t.engine == ENG_MONOLIT || t.engine == ENG_VAPORA || t.engine == ENG_SUPERSAW || t.engine == ENG_MIXTUR || t.engine == ENG_HOLLOW || t.engine == ENG_BLOT || t.engine == ENG_AETHER || t.engine == ENG_ARC || t.engine == ENG_DDDD || t.engine == ENG_JUNI || t.engine == ENG_OUTATIME || t.engine == ENG_FELT || t.engine == ENG_GRAND) fprintf(f, " push %d %d %d", t.push[0], t.push[1], t.push[2]);
    if (t.octave) fprintf(f, " oct %d", t.octave);
    if (t.engine == ENG_HOLLOW && t.relic) fprintf(f, " relic 1");
    if (t.engine == ENG_AETHER && (t.tilt[0] | t.tilt[1] | t.tilt[2])) fprintf(f, " tilt %d %d %d", t.tilt[0], t.tilt[1], t.tilt[2]);
    if (t.engine == ENG_VAPORA && t.sun) fprintf(f, " vsun %d", t.sun);
    if (t.engine == ENG_ELG) fprintf(f, " msync %d %d", t.movSync, t.movDiv);
    if (t.engine == ENG_GIN) fprintf(f, " gink %d", t.ginKit < 0 ? 0 : t.ginKit);
    if (t.engine == ENG_VELOUR) {  // VELOUR: 26 parametrow oryginalu
        fprintf(f, " velr");
        for (int i = 0; i < VELOUR_PARAMS; i++) fprintf(f, " %d", t.velour[i]);
    }
    fprintf(f, " voice %d %.4f %.3f %.4f gsync %d %d", t.mono, t.glide, t.unison, t.age, t.glideSync, t.glideDiv);
    fprintf(f, " menv %.4f %.4f %.4f %.4f %.4f %d", t.menv[0], t.menv[1], t.menv[2], t.menv[3], t.menvAmt, t.menvDest);
    if (t.engine==ENG_PROPHET) {
        int16_t defaults[PROPHET_PARAMS];
        if (!t.prophetSet) prophetPreset(0,defaults);
        fprintf(f," pfive %d",PROPHET_VERSION);
        for (int i=0;i<PROPHET_PARAMS;++i) fprintf(f," %d",t.prophetSet?t.prophet[i]:defaults[i]);
    }
    if (t.engine==ENG_POLIVOKS) {
        int16_t defaults[POLIVOKS_NUM_PARAMS];
        if (!t.polivoksSet) polivoksLoadPreset(0,defaults);
        const int16_t* pv=t.polivoksSet ? t.polivoks : defaults;
        fprintf(f," pvox %d",POLIVOKS_PARAM_VERSION);
        for (int i=0;i<POLIVOKS_NUM_PARAMS;++i) fprintf(f," %d",pv[i]);
    }
    if (t.preset[0]) { fprintf(f, " preset "); putStr(f, t.preset); }
}

// GIN: dzwieki zmienione wzgledem zestawu fabrycznego — osobne linie "gins i morph strona A(21) B(21)".
// LINEA: linia "lnea slot cut res bank hex(448 bajtow)" — patch sciezki razem z edycjami
void writeLinea(FILE* f, const Track& t) {
    if (t.engine != ENG_LINEA || !t.lineaSet) return;
    char bank[24];  // nazwa banku bez spacji (jeden token): spacja -> '_'
    snprintf(bank, sizeof bank, "%s", t.lineaBank[0] ? t.lineaBank : "-");
    for (char* q = bank; *q; q++) if (*q == ' ') *q = '_';
    fprintf(f, "lnea %d %d %d %s ", t.lineaSlot, t.lineaCut, t.lineaRes, bank);
    for (int i = 0; i < LINEA_PATCH_BYTES; i++) fprintf(f, "%02x", t.linea[i]);
    fputc('\n', f);
}
bool parseLinea(Track& t, char** tok, int n) {
    if (n < 6 || strcmp(tok[0], "lnea") || strlen(tok[5]) < 2 * LINEA_PATCH_BYTES) return false;
    t.lineaSlot = (int16_t)atoi(tok[1]);
    t.lineaCut = (int8_t)atoi(tok[2]);
    t.lineaRes = (int8_t)atoi(tok[3]);
    snprintf(t.lineaBank, sizeof t.lineaBank, "%s", strcmp(tok[4], "-") ? tok[4] : "");
    for (char* q = t.lineaBank; *q; q++) if (*q == '_') *q = ' ';
    for (int i = 0; i < LINEA_PATCH_BYTES; i++) {
        unsigned v = 0;
        sscanf(tok[5] + 2 * i, "%2x", &v);
        t.linea[i] = (uint8_t)(v & 0x7F);
    }
    static uint32_t idc = 0x40000000u;
    t.lineaId = ++idc;
    t.lineaSet = true;
    return true;
}

void writeGinSounds(FILE* f, const Track& t) {
    if (t.engine != ENG_GIN || t.ginKit < 0) return;
    for (int s = 0; s < GIN_KIT_SOUNDS; s++) {
        if (!ginSoundEdited(t, s)) continue;
        const GinSound& g = t.gin[s];
        fprintf(f, "gins %d %d %d", s, g.morph, g.side);
        for (int e = 0; e < 2; e++)
            for (int i = 0; i < GIN_PARAMS; i++) fprintf(f, " %.9g", g.ep[e][i]);
        fputc('\n', f);
    }
}

bool parseGinSound(Track& t, char** tok, int n) {
    if (n < 4 + 2 * GIN_PARAMS || strcmp(tok[0], "gins")) return false;
    const int s = atoi(tok[1]);
    if (s < 0 || s >= GIN_KIT_SOUNDS) return false;
    GinSound& g = t.gin[s];
    const int m = atoi(tok[2]);
    g.morph = (uint16_t)(m < 0 ? 0 : m > 1000 ? 1000 : m);
    g.side = atoi(tok[3]) ? 1 : 0;
    for (int e = 0; e < 2; e++)
        for (int i = 0; i < GIN_PARAMS; i++) {
            float x = (float)atof(tok[4 + e * GIN_PARAMS + i]);
            g.ep[e][i] = x < 0 ? 0 : (x > 1 ? 1 : x);
        }
    return true;
}

// ---- mikser: linie "mix" (po linii sciezki), "aux i ...", "bus ..." ----
static void writeEq(FILE* f, const MixEq& e) {
    fprintf(f, " eq %d %d %.2f %.2f %.2f %.1f %.1f %.1f %.3f %.3f %.3f %.2f", e.on, e.max, e.gain[0], e.gain[1], e.gain[2], e.freq[0],
            e.freq[1], e.freq[2], e.q[0], e.q[1], e.q[2], e.out);
}
static void writeFx(FILE* f, const FxSlot& s) {
    fprintf(f, s.bypass ? " %db" : " %d", s.fx);  // "b" = bypass
    for (int i = 0; i < 8; i++) fprintf(f, " %d", s.w[i]);
}
void writeMix(FILE* f, const Track& t) {
    fprintf(f, "mix pan %.3f", t.pan);
    writeEq(f, t.eq);
    fprintf(f, " cmp %d %d", t.comp.on, t.comp.type);
    for (int m = 0; m < COMP_TYPES; m++)
        for (int i = 0; i < 5; i++) fprintf(f, " %.3f", t.comp.p[m][i]);
    fprintf(f, " ins");
    for (int k = 0; k < MIX_INSERTS; k++) writeFx(f, t.ins[k]);
    fprintf(f, " snd");
    for (int k = 0; k < MIX_AUX; k++) fprintf(f, " %.3f", t.send[k]);
    fputc('\n', f);
}
static void writeMixGlobal(FILE* f, const Project& p) {
    for (int a = 0; a < MIX_AUX; a++) {
        fprintf(f, "aux %d %.3f fx", a, p.aux[a].level);
        writeFx(f, p.aux[a].fx);
        writeEq(f, p.aux[a].eq);
        fputc('\n', f);
    }
    fprintf(f, "bus %d %.3f %d %d %d %.2f\n", p.bus.on, p.bus.thresh, p.bus.ratio, p.bus.attack, p.bus.release, p.bus.makeup);
}
static int parseEq(MixEq& e, char** tok, int n, int i) {  // tok[i] == "eq"
    if (i + 12 >= n) return n;
    e.on = atoi(tok[i + 1]) != 0; e.max = atoi(tok[i + 2]) != 0;
    for (int k = 0; k < 3; k++) e.gain[k] = (float)atof(tok[i + 3 + k]);
    for (int k = 0; k < 3; k++) e.freq[k] = (float)atof(tok[i + 6 + k]);
    for (int k = 0; k < 3; k++) e.q[k] = (float)atof(tok[i + 9 + k]);
    e.out = (float)atof(tok[i + 12]);
    return i + 13;
}
static int parseFx(FxSlot& s, char** tok, int n, int i) {  // tok[i] = numer efektu
    if (i + 8 >= n) return n;
    const int fx = atoi(tok[i]);
    s.fx = (uint8_t)(fx >= 0 && fx < FX_COUNT ? fx : FX_NONE);
    s.bypass = strchr(tok[i], 'b') != nullptr;
    for (int k = 0; k < 8; k++) s.w[k] = (int16_t)atoi(tok[i + 1 + k]);
    return i + 9;
}
void parseMix(Track& t, char** tok, int n) {
    for (int i = 1; i < n;) {
        if (!strcmp(tok[i], "pan") && i + 1 < n) { t.pan = (float)atof(tok[i + 1]); i += 2; }
        else if (!strcmp(tok[i], "eq")) i = parseEq(t.eq, tok, n, i);
        else if (!strcmp(tok[i], "cmp") && i + 2 + COMP_TYPES * 5 < n) {
            t.comp.on = atoi(tok[i + 1]) != 0;
            const int ty = atoi(tok[i + 2]);
            t.comp.type = (uint8_t)(ty >= 0 && ty < COMP_TYPES ? ty : 0);
            for (int m = 0; m < COMP_TYPES; m++)
                for (int k = 0; k < 5; k++) t.comp.p[m][k] = (float)atof(tok[i + 3 + m * 5 + k]);
            i += 3 + COMP_TYPES * 5;
        } else if (!strcmp(tok[i], "ins")) {
            i++;
            // stare pliki maja 2 inserty: konczymy na pierwszym slowie, ktore nie jest liczba (np. "snd")
            for (int k = 0; k < MIX_INSERTS && i < n && (isdigit((unsigned char)tok[i][0]) || tok[i][0] == '-'); k++) i = parseFx(t.ins[k], tok, n, i);
        } else if (!strcmp(tok[i], "snd") && i + MIX_AUX < n) {
            for (int k = 0; k < MIX_AUX; k++) t.send[k] = (float)atof(tok[i + 1 + k]);
            i += 1 + MIX_AUX;
        } else i++;
    }
}

bool saveProjectFile(const Project& p, const char* path) {
    char tmp[512];
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE* f = fopen(tmp, "w");
    if (!f) return false;
    fprintf(f, "# DAW-POL 3000 project\ndawpol %d\nname ", FORMAT_VERSION);
    putStr(f, p.name);
    fprintf(f, "\nbpm %.2f\nmaster %.3f\nloop %d %d %d\n\n", p.bpm, p.masterVol, p.loopOn, p.loopIn, p.loopOut);
    for (int i = 0; i < p.nTracks; i++) {
        const Track& t = p.tracks[i];
        fprintf(f, "track %s ", t.type == TRACK_MIDI ? "midi" : "audio");
        putStr(f, t.name);
        fprintf(f, " color %d vol %.3f mute %d solo %d last %d", t.color, t.volume, t.mute, t.solo, t.lastPattern);
        if (t.type == TRACK_MIDI) writeInstrument(f, t);
        fputc('\n', f);
        if (t.type == TRACK_MIDI) { writeGinSounds(f, t); writeLinea(f, t); }
        writeMix(f, t);
    }
    writeMixGlobal(f, p);
    fputc('\n', f);
    for (int i = 0; i < p.nPatterns; i++) {
        const Pattern& pt = p.patterns[i];
        fprintf(f, "pattern ");
        putStr(f, pt.name);
        fprintf(f, " bars %d\n", pt.bars());
        if (pt.loopOn || pt.loopOut) fprintf(f, "loop %d %d %d\n", pt.loopOn, pt.loopIn, pt.loopOut);
        for (int k = 0; k < pt.count; k++) {
            const Note& n = pt.notes[k];
            fprintf(f, "n %d %d %d %d\n", n.start, n.len, n.pitch, n.vel);
        }
        fprintf(f, "end\n\n");
    }
    for (int i = 0; i < p.nClips; i++)
        fprintf(f, "clip %d %d %d\n", p.clips[i].track, p.clips[i].pattern, p.clips[i].startBar);
    bool ok = fflush(f) == 0;
    ok &= fsync(fileno(f)) == 0;
    ok &= fclose(f) == 0;
    if (!ok) { unlink(tmp); return false; }
    return rename(tmp, path) == 0;
}

// Prosty tokenizer: slowa i "napisy w cudzyslowie".
int tokenize(char* line, char** tok, int max) {
    int n = 0;
    char* p = line;
    while (*p && n < max) {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (!*p || *p == '#') break;
        if (*p == '"') {
            tok[n++] = ++p;
            while (*p && *p != '"') p++;
        } else {
            tok[n++] = p;
            while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') p++;
        }
        if (*p) *p++ = 0;
    }
    return n;
}

// Wartosc po kluczu w linii "klucz wartosc klucz wartosc ...".
const char* kv(char** tok, int n, int from, const char* key) {
    for (int i = from; i + 1 < n; i++) if (!strcmp(tok[i], key)) return tok[i + 1];
    return nullptr;
}

void parseInstrument(Track& t, char** tok, int n, int start) {
    const char* v;
    if ((v = kv(tok, n, start, "engine")))
        t.engine = !strcmp(v,"prophet5") ? ENG_PROPHET : !strcmp(v,"polivoks") ? ENG_POLIVOKS : !strcmp(v, "saw") ? ENG_SAW : (!strcmp(v, "taffy") ? ENG_TAFFY : (!strcmp(v, "pew") ? ENG_PEW : (!strcmp(v, "elg") ? ENG_ELG : (!strcmp(v, "eva") ? ENG_EVA : (!strcmp(v, "mbira") ? ENG_MBIRA : (!strcmp(v, "reactor") ? ENG_REACTOR : (!strcmp(v, "oxbow") ? ENG_OXBOW : (!strcmp(v, "sid") ? ENG_SID : (!strcmp(v, "monolit") ? ENG_MONOLIT : (!strcmp(v, "vapora") ? ENG_VAPORA : (!strcmp(v, "supersaw") ? ENG_SUPERSAW : (!strcmp(v, "mixtur") ? ENG_MIXTUR : (!strcmp(v, "brass") ? ENG_BRASS : (!strcmp(v, "hollowsaint") ? ENG_HOLLOW : (!strcmp(v, "bloodmoon") ? ENG_BLOOD : (!strcmp(v, "blot") ? ENG_BLOT : (!strcmp(v, "aether") ? ENG_AETHER : (!strcmp(v, "arc") ? ENG_ARC : (!strcmp(v, "dddd") ? ENG_DDDD : (!strcmp(v, "juni") ? ENG_JUNI : (!strcmp(v, "outatime") ? ENG_OUTATIME : (!strcmp(v, "gin") ? ENG_GIN : (!strcmp(v, "parallax") ? ENG_PARALLAX : (!strcmp(v, "lounge") ? ENG_LOUNGE : (!strcmp(v, "felt") ? ENG_FELT : (!strcmp(v, "grand") ? ENG_GRAND : (!strcmp(v, "velour") ? ENG_VELOUR : (!strcmp(v, "linea") ? ENG_LINEA : ENG_WAVETABLE))))))))))))))))))))))))))));
    // pary "klucz wartosci..." — kazdy klucz sprawdza, czy ma dosc wartosci za soba
    for (int i = start; i < n; i++) {
                const int left = n - 1 - i;  // ile tokenow po kluczu
                if (!strcmp(tok[i], "k") && left >= 4)
                    for (int j = 0; j < 4; j++) { int x = atoi(tok[i + 1 + j]); t.knob[j] = (int16_t)(x < 0 ? 0 : x > 32767 ? 32767 : x); }
                if (!strcmp(tok[i], "env") && left >= 4)
                    for (int j = 0; j < 4; j++) t.env[j] = (float)atof(tok[i + 1 + j]);
                if (!strcmp(tok[i], "envamt") && left >= 1) t.envAmt = (float)atof(tok[i + 1]);
                if (!strcmp(tok[i], "fenv") && left >= 4) {
                    // nowy zapis: A D S R AMOUNT; pierwsza wersja (A D S AMOUNT, bez R) — rozpoznawana po 4 liczbach
                    bool five = left >= 5 && (tok[i + 5][0] == '-' || tok[i + 5][0] == '.' || (tok[i + 5][0] >= '0' && tok[i + 5][0] <= '9'));
                    for (int j = 0; j < 3; j++) t.fenv[j] = (float)atof(tok[i + 1 + j]);
                    if (five) { t.fenv[3] = (float)atof(tok[i + 4]); t.fenvAmt = (float)atof(tok[i + 5]); }
                    else { t.fenv[3] = t.env[3]; t.fenvAmt = (float)atof(tok[i + 4]); }
                }
                if ((!strcmp(tok[i], "lfo1") || !strcmp(tok[i], "lfo2")) && left >= 4) {
                    Lfo& L = t.lfo[tok[i][3] - '1'];
                    L.shape = (uint8_t)(atoi(tok[i + 1]) % LFO_SHAPES);
                    L.rate = (float)atof(tok[i + 2]);
                    L.depth = (float)atof(tok[i + 3]);
                    L.dest = (uint8_t)(atoi(tok[i + 4]) % LFO_DESTS);
                    if (left >= 6 && tok[i + 5][0] >= '0' && tok[i + 5][0] <= '9') {  // sync, podzial (nowsze zapisy)
                        L.sync = atoi(tok[i + 5]) != 0;
                        L.div = (uint8_t)(atoi(tok[i + 6]) % LFO_DIVS);
                    }
                }
                if (!strcmp(tok[i], "alt") && left >= 1) t.alt = atoi(tok[i + 1]) != 0;
                if ((!strcmp(tok[i], "o1") || !strcmp(tok[i], "o2")) && left >= 4) {
                    WtOsc& o = t.ext.osc[tok[i][1] - '1'];
                    int w = atoi(tok[i + 1]), sm = atoi(tok[i + 2]);
                    o.wave = (int16_t)(w < 0 ? 0 : w > 32767 ? 32767 : w);
                    o.semi = (int8_t)(sm < -24 ? -24 : sm > 24 ? 24 : sm);
                    o.cents = (float)atof(tok[i + 3]);
                    o.level = (float)atof(tok[i + 4]);
                }
                if (!strcmp(tok[i], "sub") && left >= 1) t.ext.sub = (float)atof(tok[i + 1]);
                if (!strcmp(tok[i], "fmode") && left >= 1) t.ext.fmode = (uint8_t)(atoi(tok[i + 1]) % FLT_MODES);
                if (!strcmp(tok[i], "drv") && left >= 1) t.ext.drive = (float)atof(tok[i + 1]);
                if (!strcmp(tok[i], "obx") && left >= 18) {
                    int v[18];
                    for (int j = 0; j < 18; j++) v[j] = atoi(tok[i + 1 + j]);
                    auto cl = [](int x, int lo, int hi) { return x < lo ? lo : x > hi ? hi : x; };
                    ObExt& o = t.obx;
                    o.wave[0] = (uint8_t)cl(v[0], 0, 3); o.wave[1] = (uint8_t)cl(v[1], 0, 3);
                    o.semi[0] = (int8_t)cl(v[2], -24, 24); o.semi[1] = (int8_t)cl(v[3], -24, 24);
                    o.cents1 = (int8_t)cl(v[4], -50, 50);
                    o.level[0] = (uint8_t)cl(v[5], 0, 126); o.level[1] = (uint8_t)cl(v[6], 0, 126);
                    o.pw[0] = (uint8_t)cl(v[7], 1, 127); o.pw[1] = (uint8_t)cl(v[8], 1, 127);
                    o.noise = (uint8_t)cl(v[9], 0, 126); o.sync = v[10] != 0; o.xmod = (uint8_t)cl(v[11], 0, 127);
                    o.envPitch = (int8_t)cl(v[12], -64, 63); o.keytrk = (uint8_t)cl(v[13], 0, 128);
                    o.velflt = (uint8_t)cl(v[14], 0, 127); o.vint = (uint8_t)cl(v[15], 0, 127);
                    o.ftype = (uint8_t)cl(v[16], 0, 5); o.drive = (uint8_t)cl(v[17], 0, 127);
                }
                if (!strcmp(tok[i], "msync") && left >= 2) {
                    t.movSync = atoi(tok[i + 1]) != 0;
                    t.movDiv = (uint8_t)(atoi(tok[i + 2]) % LFO_DIVS);
                }
                if (!strcmp(tok[i], "push") && left >= 3)
                    for (int j = 0; j < 3; j++) { int v = atoi(tok[i + 1 + j]); t.push[j] = (int16_t)(v < 0 ? 0 : v > 32767 ? 32767 : v); }
                if (!strcmp(tok[i], "tilt") && left >= 3)
                    for (int j = 0; j < 3; j++) { int v = atoi(tok[i + 1 + j]); t.tilt[j] = (int16_t)(v < -32767 ? -32767 : v > 32767 ? 32767 : v); }
                if (!strcmp(tok[i], "relic") && left >= 1) t.relic = atoi(tok[i + 1]) != 0;
                if (!strcmp(tok[i], "oct") && left >= 1) { int v = atoi(tok[i + 1]); t.octave = (int8_t)(v < -3 ? -3 : v > 3 ? 3 : v); }
                if (!strcmp(tok[i], "gink") && left >= 1) ginLoadKit(t, atoi(tok[i + 1]));
                if (!strcmp(tok[i],"pfive") && left>=PROPHET_V1_PARAMS+1) {
                    const int version=atoi(tok[i+1]);
                    const int count=version==1?PROPHET_V1_PARAMS:version==PROPHET_VERSION?PROPHET_PARAMS:0;
                    if(count && left>=count+1){
                        for (int j=0;j<count;++j) { int x=atoi(tok[i+2+j]); t.prophet[j]=(int16_t)(x<0?0:x>32767?32767:x); }
                        if(version==1)t.prophet[P5_REVISION]=0;
                        t.prophetSet=true;
                    }
                }
                if (!strcmp(tok[i],"pvox") && left>=35) {
                    int version=atoi(tok[i+1]);
                    int count=version==1 ? 34 : version==2 ? POLIVOKS_V2_PARAMS : version==POLIVOKS_PARAM_VERSION ? POLIVOKS_NUM_PARAMS : 0;
                    if (count && left>=count+1) {
                        int16_t words[POLIVOKS_NUM_PARAMS]={};
                        for (int j=0;j<count;++j) { int x=atoi(tok[i+2+j]); words[j]=(int16_t)(x<0 ? 0 : x>32767 ? 32767 : x); }
                        if (version==1) polivoksMigrateV1(words,t.polivoks);
                        else if(version==2) polivoksMigrateV2(words,t.polivoks);
                        else memcpy(t.polivoks,words,sizeof(t.polivoks));
                        t.polivoksSet=true;
                    }
                }
                if (!strcmp(tok[i], "velr") && left >= VELOUR_PARAMS) {  // VELOUR
                    for (int j = 0; j < VELOUR_PARAMS; j++) { int x = atoi(tok[i + 1 + j]); t.velour[j] = (int16_t)(x < 0 ? 0 : x > 32767 ? 32767 : x); }
                    t.velourSet = true;
                }  // GIN: zestaw (linie gins = zmiany)
                if (!strcmp(tok[i], "vsun") && left >= 1) t.sun = (uint8_t)(atoi(tok[i + 1]) % 7);
                if (!strcmp(tok[i], "gsync") && left >= 2) {
                    t.glideSync = atoi(tok[i + 1]) != 0;
                    t.glideDiv = (uint8_t)(atoi(tok[i + 2]) % LFO_DIVS);
                }
                if (!strcmp(tok[i], "menv") && left >= 6) {
                    for (int j = 0; j < 4; j++) t.menv[j] = (float)atof(tok[i + 1 + j]);
                    t.menvAmt = (float)atof(tok[i + 5]);
                    t.menvDest = (uint8_t)(atoi(tok[i + 6]) % LFO_DESTS);
                }
                if (!strcmp(tok[i], "preset") && left >= 1) snprintf(t.preset, sizeof t.preset, "%s", tok[i + 1]);
                if (!strcmp(tok[i], "voice") && left >= 4) {
                    t.mono = atoi(tok[i + 1]) != 0;
                    t.glide = (float)atof(tok[i + 2]);
                    t.unison = (float)atof(tok[i + 3]);
                    t.age = (float)atof(tok[i + 4]);
                }
            }
}

bool loadProjectFile(Project& out, const char* path, char* err, int errLen) {
    FILE* f = fopen(path, "r");
    if (!f) { snprintf(err, errLen, "CANNOT OPEN FILE"); return false; }
    static Project p;  // duzy obiekt — nie na stosie
    p = Project();
    p.nTracks = p.nPatterns = p.nClips = 0;
    char line[2048];
    char* tok[192];
    int version = 0, curPat = -1;
    while (fgets(line, sizeof line, f)) {
        int n = tokenize(line, tok, 192);
        if (n == 0) continue;
        const char* k = tok[0];
        if (!strcmp(k, "dawpol") && n > 1) version = atoi(tok[1]);
        else if (!strcmp(k, "name") && n > 1) snprintf(p.name, sizeof p.name, "%s", tok[1]);
        else if (!strcmp(k, "bpm") && n > 1) p.bpm = (float)atof(tok[1]);
        else if (!strcmp(k, "master") && n > 1) p.masterVol = (float)atof(tok[1]);
        else if (!strcmp(k, "track") && n > 2 && p.nTracks < MAX_TRACKS) {
            Track& t = p.tracks[p.nTracks++];
            t = Track();
            t.type = !strcmp(tok[1], "audio") ? TRACK_AUDIO : TRACK_MIDI;
            snprintf(t.name, sizeof t.name, "%s", tok[2]);
            const char* v;
            if ((v = kv(tok, n, 3, "color"))) t.color = (uint8_t)(atoi(v) % pal::TRACK_COLORS);
            if ((v = kv(tok, n, 3, "vol"))) t.volume = (float)atof(v);
            if ((v = kv(tok, n, 3, "mute"))) t.mute = atoi(v) != 0;
            if ((v = kv(tok, n, 3, "solo"))) t.solo = atoi(v) != 0;
            if ((v = kv(tok, n, 3, "last"))) t.lastPattern = atoi(v);
            parseInstrument(t, tok, n, 3);
        } else if (!strcmp(k, "mix") && p.nTracks > 0) {
            parseMix(p.tracks[p.nTracks - 1], tok, n);
        } else if (!strcmp(k, "aux") && n > 3) {
            const int a = atoi(tok[1]);
            if (a >= 0 && a < MIX_AUX) {
                p.aux[a].level = (float)atof(tok[2]);
                int i = 3;
                if (i < n && !strcmp(tok[i], "fx")) i = parseFx(p.aux[a].fx, tok, n, i + 1);
                if (i < n && !strcmp(tok[i], "eq")) parseEq(p.aux[a].eq, tok, n, i);
            }
        } else if (!strcmp(k, "bus") && n > 6) {
            p.bus.on = atoi(tok[1]) != 0; p.bus.thresh = (float)atof(tok[2]);
            p.bus.ratio = (uint8_t)(atoi(tok[3]) % 3); p.bus.attack = (uint8_t)(atoi(tok[4]) % 6); p.bus.release = (uint8_t)(atoi(tok[5]) % 5);
            p.bus.makeup = (float)atof(tok[6]);
        } else if (!strcmp(k, "lnea") && p.nTracks > 0) {
            parseLinea(p.tracks[p.nTracks - 1], tok, n);
        } else if (!strcmp(k, "gins") && p.nTracks > 0) {
            parseGinSound(p.tracks[p.nTracks - 1], tok, n);
        } else if (!strcmp(k, "pattern") && n > 1 && p.nPatterns < MAX_PATTERNS) {
            curPat = p.nPatterns++;
            Pattern& pt = p.patterns[curPat];
            pt.clear();
            snprintf(pt.name, sizeof pt.name, "%s", tok[1]);
            const char* v = kv(tok, n, 2, "bars");
            pt.setBars(v ? atoi(v) : 1);
        } else if (!strcmp(k, "n") && n > 4 && curPat >= 0) {
            Note nt;
            nt.start = (uint16_t)atoi(tok[1]);
            nt.len = (uint16_t)atoi(tok[2]);
            nt.pitch = (uint8_t)(atoi(tok[3]) & 127);
            nt.vel = (uint8_t)atoi(tok[4]);
            if (nt.vel < 1) nt.vel = 1;
            if (nt.vel > 127) nt.vel = 127;
            p.patterns[curPat].add(nt);
        } else if (!strcmp(k, "end")) curPat = -1;
        else if (!strcmp(k, "loop") && n > 3) {
            if (curPat >= 0) {
                Pattern& pt = p.patterns[curPat];
                pt.loopOn = atoi(tok[1]) != 0;
                pt.loopIn = (uint16_t)atoi(tok[2]);
                pt.loopOut = (uint16_t)atoi(tok[3]);
            } else {
                p.loopOn = atoi(tok[1]) != 0;
                p.loopIn = atoi(tok[2]);
                p.loopOut = atoi(tok[3]);
            }
        }
        else if (!strcmp(k, "clip") && n > 3 && p.nClips < MAX_CLIPS) {
            int t = atoi(tok[1]), pt = atoi(tok[2]), b = atoi(tok[3]);
            if (t >= 0 && t < p.nTracks && pt >= 0 && pt < p.nPatterns && b >= 0 && b < MAX_SONG_BARS)
                p.clips[p.nClips++] = {(uint8_t)t, (uint8_t)pt, (uint16_t)b};
        }
    }
    fclose(f);
    if (version < 1) { snprintf(err, errLen, "NOT A DAW-POL PROJECT"); return false; }
    if (version > FORMAT_VERSION) { snprintf(err, errLen, "NEWER FORMAT (V%d)", version); return false; }
    for (int i = 0; i < p.nTracks; i++)
        if (p.tracks[i].lastPattern >= p.nPatterns) p.tracks[i].lastPattern = -1;
    if (p.bpm < 40 || p.bpm > 240) p.bpm = 120;
    out = p;
    return true;
}

}  // namespace dp

#include "io/project_file.h"
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
        if (t.type == TRACK_MIDI)
            fprintf(f, " engine %s k %d %d %d %d env %.4f %.4f %.4f %.4f", t.engine == ENG_SAW ? "saw" : "wavetable",
                    t.knob[0], t.knob[1], t.knob[2], t.knob[3], t.env[0], t.env[1], t.env[2], t.env[3]);
        if (t.type == TRACK_MIDI) {
            fprintf(f, " fenv %.4f %.4f %.4f %.4f", t.fenv[0], t.fenv[1], t.fenv[2], t.fenv[3]);
            for (int l = 0; l < 2; l++)
                fprintf(f, " lfo%d %d %.4f %.4f %d", l + 1, t.lfo[l].shape, t.lfo[l].rate, t.lfo[l].depth, t.lfo[l].dest);
        }
        fputc('\n', f);
    }
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
static int tokenize(char* line, char** tok, int max) {
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
static const char* kv(char** tok, int n, int from, const char* key) {
    for (int i = from; i + 1 < n; i++) if (!strcmp(tok[i], key)) return tok[i + 1];
    return nullptr;
}

bool loadProjectFile(Project& out, const char* path, char* err, int errLen) {
    FILE* f = fopen(path, "r");
    if (!f) { snprintf(err, errLen, "CANNOT OPEN FILE"); return false; }
    static Project p;  // duzy obiekt — nie na stosie
    p = Project();
    p.nTracks = p.nPatterns = p.nClips = 0;
    char line[512];
    char* tok[24];
    int version = 0, curPat = -1;
    while (fgets(line, sizeof line, f)) {
        int n = tokenize(line, tok, 24);
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
            if ((v = kv(tok, n, 3, "engine"))) t.engine = !strcmp(v, "saw") ? ENG_SAW : ENG_WAVETABLE;
            for (int i = 3; i + 4 < n; i++) {
                if (!strcmp(tok[i], "k"))
                    for (int j = 0; j < 4; j++) { int x = atoi(tok[i + 1 + j]); t.knob[j] = (int16_t)(x < 0 ? 0 : x > 32767 ? 32767 : x); }
                if (!strcmp(tok[i], "env"))
                    for (int j = 0; j < 4; j++) t.env[j] = (float)atof(tok[i + 1 + j]);
                if (!strcmp(tok[i], "fenv"))
                    for (int j = 0; j < 4; j++) t.fenv[j] = (float)atof(tok[i + 1 + j]);
                if (!strcmp(tok[i], "lfo1") || !strcmp(tok[i], "lfo2")) {
                    Lfo& L = t.lfo[tok[i][3] - '1'];
                    L.shape = (uint8_t)(atoi(tok[i + 1]) % LFO_SHAPES);
                    L.rate = (float)atof(tok[i + 2]);
                    L.depth = (float)atof(tok[i + 3]);
                    L.dest = (uint8_t)(atoi(tok[i + 4]) % LFO_DESTS);
                }
            }
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

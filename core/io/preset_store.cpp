#include "io/preset_store.h"
#include "io/fs.h"
#include "io/project_file.h"
#include "io/project_store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

namespace dp {

void PresetStore::initInstrument(Track& t) {
    Track d;
    t.engine = d.engine;
    memcpy(t.knob, d.knob, sizeof t.knob);
    memcpy(t.env, d.env, sizeof t.env);
    t.envAmt = d.envAmt;
    memcpy(t.fenv, d.fenv, sizeof t.fenv);
    t.fenvAmt = d.fenvAmt;
    memcpy(t.menv, d.menv, sizeof t.menv);
    t.menvAmt = d.menvAmt;
    t.menvDest = d.menvDest;
    t.lfo[0] = d.lfo[0];
    t.lfo[1] = d.lfo[1];
    t.alt = d.alt;
    t.ext = d.ext;
    t.mono = d.mono;
    t.glide = d.glide;
    t.unison = d.unison;
    t.age = d.age;
    t.glideSync = d.glideSync;
    t.glideDiv = d.glideDiv;
}

void PresetStore::init(const char* root) {
    snprintf(root_, sizeof root_, "%s", root);
    snprintf(dir_, sizeof dir_, "%s/wavetable", root_);
    fs::makeDirs(dir_);
    refresh();
    writeFactory();  // dopisuje brakujace presety z nowszych wersji zestawu
    refresh();
}

int PresetStore::refresh() {
    if (!ready()) return n_ = 0;
    int nf = fs::listFiles(dir_, ".dpr", files_, MAX);
    n_ = 0;
    for (int i = 0; i < nf; i++) {
        char path[400], line[256];
        snprintf(path, sizeof path, "%s/%s.dpr", dir_, files_[i]);
        FILE* f = fopen(path, "r");
        if (!f) continue;
        char nm[32];
        snprintf(nm, sizeof nm, "%s", files_[i]);
        int cat = CAT_OTHER;
        while (fgets(line, sizeof line, f)) {
            char* tok[8];
            int n = tokenize(line, tok, 8);
            if (n >= 2 && !strcmp(tok[0], "name")) snprintf(nm, sizeof nm, "%s", tok[1]);
            if (n >= 2 && !strcmp(tok[0], "category"))
                for (int c = 0; c < PRESET_CATS; c++) if (!strcmp(tok[1], presetCatName(c))) cat = c;
            if (n >= 1 && !strcmp(tok[0], "inst")) break;
        }
        fclose(f);
        if (n_ != i) memcpy(files_[n_], files_[i], 32);
        snprintf(names_[n_], 32, "%s", nm);
        cats_[n_] = (uint8_t)cat;
        n_++;
    }
    return n_;
}

int PresetStore::find(const char* name) const {
    char a[64], b[64];
    ProjectStore::folderName(name, a, sizeof a);
    for (int i = 0; i < n_; i++) {
        ProjectStore::folderName(names_[i], b, sizeof b);
        if (!strcmp(a, b)) return i;
    }
    return -1;
}

bool PresetStore::load(int i, Track& t) const {
    if (i < 0 || i >= n_) return false;
    char path[400];
    snprintf(path, sizeof path, "%s/%s.dpr", dir_, files_[i]);
    FILE* f = fopen(path, "r");
    if (!f) return false;
    static char line[2048];
    char* tok[128];
    bool ok = false;
    while (fgets(line, sizeof line, f)) {
        int n = tokenize(line, tok, 128);
        if (n >= 1 && !strcmp(tok[0], "inst")) {
            Track tmp = t;
            initInstrument(tmp);  // brakujace pola = INIT
            parseInstrument(tmp, tok, n, 1);
            t = tmp;
            ok = true;
        }
    }
    fclose(f);
    return ok;
}

bool PresetStore::save(const Track& t, const char* name, int category) {
    if (!ready()) return false;
    char fn[64], path[400], tmp[420];
    ProjectStore::folderName(name, fn, sizeof fn);
    snprintf(path, sizeof path, "%s/%s.dpr", dir_, fn);
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE* f = fopen(tmp, "w");
    if (!f) return false;
    fprintf(f, "dawpol-preset 1\nname \"%s\"\ncategory %s\ninst", name, presetCatName(category));
    writeInstrument(f, t);
    fputc('\n', f);
    bool ok = fflush(f) == 0 && fsync(fileno(f)) == 0;
    ok &= fclose(f) == 0;
    if (!ok || rename(tmp, path) != 0) { unlink(tmp); return false; }
    refresh();
    return true;
}

bool PresetStore::remove(int i) {
    if (i < 0 || i >= n_) return false;
    char path[400];
    snprintf(path, sizeof path, "%s/%s.dpr", dir_, files_[i]);
    bool ok = unlink(path) == 0;
    refresh();
    return ok;
}

// Pozycje fal w galce WAVE (0..32767): SINE, WARM, TRI, SAW, SQUARE, PULSE, VOCAL, DIGITAL
static const int16_t W_SINE = 0, W_WARM = 4681, W_TRI = 9362, W_SAW = 14043, W_SQR = 18724, W_PULSE = 23405,
                     W_VOCAL = 28086, W_DIGI = 32767;

// Zestaw fabryczny. ver = wersja zestawu, w ktorej preset doszedl.
void PresetStore::writeFactory() {
    char mark[400];
    snprintf(mark, sizeof mark, "%s/_factory_v%d", dir_, FACTORY_VERSION);
    if (fs::exists(mark)) return;
    struct F { const char* name; int cat; int ver; void (*set)(Track&); };
    static const F list[] = {
        {"INIT", CAT_OTHER, 1, [](Track&) {}},
        {"SUB BASS", CAT_BASS, 1, [](Track& t) {
             t.alt = true; t.knob[0] = 9400; t.knob[2] = 9000; t.knob[3] = 6000; t.ext.osc[1].semi = -12;
             t.ext.osc[1].level = 0.6f; t.ext.sub = 0.7f; t.mono = true; t.glide = 0.04f;
             t.env[0] = 0.002f; t.env[1] = 0.3f; t.env[2] = 0.7f; t.env[3] = 0.08f; }},
        {"ACID", CAT_BASS, 1, [](Track& t) {
             t.knob[0] = 14000; t.knob[1] = 0; t.knob[2] = 5000; t.knob[3] = 26000; t.mono = true; t.glide = 0.09f;
             t.fenv[0] = 0.002f; t.fenv[1] = 0.18f; t.fenv[2] = 0.f; t.fenv[3] = 0.1f; t.fenvAmt = 0.55f;
             t.env[1] = 0.25f; t.env[2] = 0.6f; t.env[3] = 0.05f; }},
        {"SUPERSAW", CAT_LEAD, 1, [](Track& t) {
             t.alt = true; t.knob[0] = 14000; t.knob[2] = 26000; t.knob[3] = 4000; t.ext.osc[1].wave = 14000;
             t.mono = true; t.unison = 22.f; t.glide = 0.03f; t.env[2] = 0.9f; t.env[3] = 0.25f; }},
        {"VOCAL LEAD", CAT_LEAD, 1, [](Track& t) {
             t.knob[0] = 28000; t.knob[1] = 6000; t.knob[2] = 22000; t.knob[3] = 12000; t.mono = true; t.glide = 0.06f;
             t.lfo[0].shape = LFO_SINE; t.lfo[0].rate = 5.5f; t.lfo[0].depth = 0.012f; t.lfo[0].dest = DEST_PITCH; }},
        {"WARM PAD", CAT_PAD, 1, [](Track& t) {
             t.knob[0] = 4700; t.knob[1] = 22000; t.knob[2] = 16000; t.knob[3] = 6000; t.env[0] = 0.6f;
             t.env[1] = 1.5f; t.env[2] = 0.8f; t.env[3] = 1.5f; t.lfo[0].shape = LFO_SINE; t.lfo[0].rate = 0.3f;
             t.lfo[0].depth = 0.15f; t.lfo[0].dest = DEST_WAVE; t.age = 0.3f; }},
        {"OLD PROPHET", CAT_PAD, 1, [](Track& t) {
             t.alt = true; t.knob[0] = 14000; t.knob[2] = 15000; t.knob[3] = 9000; t.ext.osc[1].wave = 18700;
             t.ext.osc[0].cents = -7; t.ext.osc[1].cents = 7; t.env[0] = 0.25f; t.env[1] = 1.f; t.env[2] = 0.7f;
             t.env[3] = 0.8f; t.fenv[0] = 0.3f; t.fenv[1] = 1.2f; t.fenv[2] = 0.2f; t.fenv[3] = 0.8f; t.fenvAmt = 0.3f;
             t.age = 0.7f; }},
        {"GLASS KEYS", CAT_KEYS, 1, [](Track& t) {
             t.knob[0] = 23400; t.knob[1] = 4000; t.knob[2] = 24000; t.knob[3] = 8000; t.env[0] = 0.002f;
             t.env[1] = 0.9f; t.env[2] = 0.25f; t.env[3] = 0.5f; }},
        {"PLUCK", CAT_PLUCK, 1, [](Track& t) {
             t.knob[0] = 14000; t.knob[2] = 6000; t.knob[3] = 10000; t.env[0] = 0.001f; t.env[1] = 0.35f;
             t.env[2] = 0.f; t.env[3] = 0.25f; t.fenv[0] = 0.001f; t.fenv[1] = 0.15f; t.fenv[2] = 0.f; t.fenv[3] = 0.2f;
             t.fenvAmt = 0.6f; }},
        {"DIGI BELL", CAT_BELL, 1, [](Track& t) {
             t.alt = true; t.knob[0] = 32767; t.knob[2] = 28000; t.ext.osc[1].wave = 0; t.ext.osc[1].semi = 19;
             t.ext.osc[1].level = 0.5f; t.env[0] = 0.001f; t.env[1] = 1.8f; t.env[2] = 0.f; t.env[3] = 1.5f; }},
        {"WOBBLE", CAT_FX, 1, [](Track& t) {
             t.knob[0] = 16000; t.knob[2] = 10000; t.knob[3] = 20000; t.mono = true; t.lfo[0].shape = LFO_SINE;
             t.lfo[0].sync = true; t.lfo[0].div = 5; t.lfo[0].depth = 0.45f; t.lfo[0].dest = DEST_CUTOFF; }},
        // ---- wersja 2: presety trybu rozszerzonego (2 OSC, sub, tryby filtra, MONO/UNISON, GLIDE, AGE, MOD ENV) ----
        {"HOOVER", CAT_LEAD, 2, [](Track& t) {
             t.alt = true; t.knob[0] = W_SAW; t.ext.osc[1].wave = W_SAW; t.ext.osc[1].semi = -12; t.ext.osc[1].level = 0.8f;
             t.ext.osc[0].cents = -9; t.ext.osc[1].cents = 9; t.knob[2] = 24000; t.knob[3] = 6000; t.ext.fmode = FLT_LP12;
             t.mono = true; t.unison = 30.f; t.glideSync = true; t.glideDiv = 7;
             t.menv[0] = 0.001f; t.menv[1] = 0.25f; t.menv[2] = 0.f; t.menv[3] = 0.2f; t.menvAmt = -0.18f; t.menvDest = DEST_PITCH;
             t.env[2] = 0.9f; t.env[3] = 0.3f; }},
        {"REESE", CAT_BASS, 2, [](Track& t) {
             t.alt = true; t.knob[0] = W_SAW; t.ext.osc[1].wave = W_SAW; t.ext.osc[0].cents = -14; t.ext.osc[1].cents = 14;
             t.ext.sub = 0.5f; t.knob[2] = 11000; t.knob[3] = 7000; t.mono = true; t.glide = 0.03f;
             t.lfo[0].shape = LFO_SINE; t.lfo[0].rate = 0.25f; t.lfo[0].depth = 0.08f; t.lfo[0].dest = DEST_W2;
             t.env[0] = 0.003f; t.env[2] = 1.f; t.env[3] = 0.1f; }},
        {"KICK DRUM", CAT_FX, 2, [](Track& t) {
             t.alt = true; t.knob[0] = W_SINE; t.ext.osc[1].level = 0.f; t.ext.osc[0].cents = 0; t.ext.sub = 0.6f;
             t.knob[2] = 20000; t.knob[3] = 0; t.mono = true; t.ext.drive = 0.4f;
             t.menv[0] = 0.001f; t.menv[1] = 0.07f; t.menv[2] = 0.f; t.menv[3] = 0.05f; t.menvAmt = 1.f; t.menvDest = DEST_PITCH;
             t.env[0] = 0.001f; t.env[1] = 0.35f; t.env[2] = 0.f; t.env[3] = 0.1f; }},
        {"SYNC PLUCK", CAT_PLUCK, 2, [](Track& t) {
             t.alt = true; t.knob[0] = W_SQR; t.ext.osc[1].wave = W_SAW; t.ext.osc[1].semi = 12; t.ext.osc[1].level = 0.7f;
             t.ext.fmode = FLT_BP; t.knob[2] = 9000; t.knob[3] = 14000;
             t.fenv[0] = 0.001f; t.fenv[1] = 0.18f; t.fenv[2] = 0.f; t.fenv[3] = 0.2f; t.fenvAmt = 0.6f;
             t.env[0] = 0.001f; t.env[1] = 0.4f; t.env[2] = 0.f; t.env[3] = 0.3f; }},
        {"SOFT STRINGS", CAT_PAD, 2, [](Track& t) {
             t.alt = true; t.knob[0] = W_SAW; t.ext.osc[1].wave = W_SAW; t.ext.osc[0].cents = -8; t.ext.osc[1].cents = 8;
             t.ext.fmode = FLT_LP12; t.knob[2] = 17000; t.knob[3] = 3000; t.age = 0.5f;
             t.env[0] = 0.5f; t.env[1] = 1.5f; t.env[2] = 0.85f; t.env[3] = 1.4f;
             t.lfo[0].shape = LFO_SINE; t.lfo[0].rate = 5.2f; t.lfo[0].depth = 0.008f; t.lfo[0].dest = DEST_PITCH; }},
        {"GATED KEYS", CAT_KEYS, 2, [](Track& t) {
             t.alt = true; t.knob[0] = W_WARM; t.ext.osc[1].wave = W_PULSE; t.ext.osc[1].semi = 12; t.ext.osc[1].level = 0.4f;
             t.knob[2] = 19000; t.knob[3] = 5000; t.env[0] = 0.01f; t.env[2] = 0.9f; t.env[3] = 0.4f;
             t.lfo[0].shape = LFO_SQUARE; t.lfo[0].sync = true; t.lfo[0].div = 7; t.lfo[0].depth = 0.85f; t.lfo[0].dest = DEST_VOLUME; }},
        {"PORTA LEAD", CAT_LEAD, 2, [](Track& t) {
             t.alt = true; t.knob[0] = W_VOCAL; t.ext.osc[1].wave = W_SAW; t.ext.osc[1].semi = 7; t.ext.osc[1].level = 0.6f;
             t.ext.fmode = FLT_LP12; t.knob[2] = 21000; t.knob[3] = 9000; t.mono = true; t.glideSync = true; t.glideDiv = 5;
             t.age = 0.3f; t.env[2] = 0.85f;
             t.lfo[0].shape = LFO_SINE; t.lfo[0].rate = 5.f; t.lfo[0].depth = 0.01f; t.lfo[0].dest = DEST_PITCH; }},
        {"DRIFT PAD", CAT_PAD, 2, [](Track& t) {
             t.alt = true; t.knob[0] = W_WARM; t.ext.osc[1].wave = W_VOCAL; t.ext.osc[1].semi = 7; t.ext.osc[1].level = 0.7f;
             t.knob[2] = 14000; t.knob[3] = 8000; t.age = 1.f; t.env[0] = 0.8f; t.env[1] = 2.f; t.env[2] = 0.8f; t.env[3] = 2.f;
             t.lfo[0].shape = LFO_TRI; t.lfo[0].rate = 0.12f; t.lfo[0].depth = 0.25f; t.lfo[0].dest = DEST_W1;
             t.lfo[1].shape = LFO_SINE; t.lfo[1].sync = true; t.lfo[1].div = 0; t.lfo[1].depth = 0.2f; t.lfo[1].dest = DEST_W2; }},
        {"BELL TREE", CAT_BELL, 2, [](Track& t) {
             t.alt = true; t.knob[0] = W_DIGI; t.ext.osc[0].level = 0.45f; t.ext.osc[1].wave = W_SINE; t.ext.osc[1].semi = 24;
             t.ext.osc[1].level = 0.35f; t.ext.fmode = FLT_HP; t.knob[2] = 4000; t.knob[3] = 5000;
             t.env[0] = 0.001f; t.env[1] = 1.4f; t.env[2] = 0.f; t.env[3] = 1.6f;
             t.menv[0] = 0.001f; t.menv[1] = 0.6f; t.menv[2] = 0.f; t.menvAmt = 0.4f; t.menvDest = DEST_W1; }},
        {"NOISE SWEEP", CAT_FX, 2, [](Track& t) {
             t.alt = true; t.knob[0] = W_DIGI; t.ext.osc[1].wave = W_DIGI; t.ext.osc[1].semi = 19; t.ext.osc[0].cents = -40;
             t.ext.osc[1].cents = 40; t.ext.osc[0].level = 0.3f; t.ext.osc[1].level = 0.25f; t.ext.fmode = FLT_HP;
             t.knob[2] = 6000; t.knob[3] = 9000;
             t.lfo[0].shape = LFO_SAW; t.lfo[0].sync = true; t.lfo[0].div = 1; t.lfo[0].depth = 0.6f; t.lfo[0].dest = DEST_CUTOFF;
             t.env[0] = 0.2f; t.env[2] = 0.6f; t.env[3] = 0.8f; }},
    };
    // pierwszy start (pusty katalog): caly zestaw; pozniej: tylko presety z nowszych wersji, ktorych brakuje
    bool fresh = n_ == 0;
    char old[400];
    int had = 0;
    for (int v = FACTORY_VERSION - 1; v >= 1 && !had; v--) {
        snprintf(old, sizeof old, "%s/_factory_v%d", dir_, v);
        if (fs::exists(old)) had = v;
    }
    if (!fresh && had == 0) had = 1;  // katalog sprzed wersjonowania = zestaw v1
    for (const F& p : list) {
        if (!fresh && p.ver <= had) continue;
        if (find(p.name) >= 0) continue;  // nie nadpisuj
        Track t;
        initInstrument(t);
        p.set(t);
        save(t, p.name, p.cat);
    }
    FILE* f = fopen(mark, "w");
    if (f) fclose(f);
}

}  // namespace dp

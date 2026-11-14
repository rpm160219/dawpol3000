#include "io/preset_store.h"
#include "synth/oxbow_plus_dsp.h"
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
    engineDefaultKnobs(t.engine, t.knob);  // silnik zostaje (INIT w obrebie silnika sciezki)
    memcpy(t.env, d.env, sizeof t.env);
    t.envAmt = d.envAmt;
    memcpy(t.fenv, d.fenv, sizeof t.fenv);
    t.fenvAmt = d.fenvAmt;
    memcpy(t.menv, d.menv, sizeof t.menv);
    t.menvAmt = d.menvAmt;
    t.menvDest = d.menvDest;
    t.lfo[0] = d.lfo[0];
    t.lfo[1] = d.lfo[1];
    t.ext = d.ext;
    engineDefaultPush(t.engine, t.push);
    t.relic = d.relic;
    for (int i = 0; i < 3; i++) t.tilt[i] = d.tilt[i];
    t.movSync = d.movSync;
    t.obx = d.obx;
    if (t.engine == ENG_OXBOW && t.alt) {  // OXBOW+: obwiednia filtra jak neutralny glos oryginalu (A 20, D 70, S 40, R 60)
        t.fenv[0] = oxbowPlusAttackSeconds(20);
        t.fenv[1] = oxbowPlusDecaySeconds(70);
        t.fenv[2] = 40.f / 127.f;
        t.fenv[3] = oxbowPlusDecaySeconds(60);
    }
    t.movDiv = d.movDiv;
    if (t.engine==ENG_POLIVOKS) { t.polivoksSet=false; polivoksEnsure(t); }
    if (t.engine == ENG_GIN) ginLoadKit(t, 0);  // GIN: INIT = pierwszy zestaw
    if (t.engine == ENG_VELOUR) { t.velourSet = false; velourEnsure(t); }  // VELOUR: INIT = FULL STRINGS
    t.mono = d.mono;
    t.glide = d.glide;
    t.unison = d.unison;
    t.age = d.age;
    t.glideSync = d.glideSync;
    t.glideDiv = d.glideDiv;
    t.octave = d.octave;  // wzor slonca VAPORA (t.sun) zostaje — to tylko obraz
}

void PresetStore::init(const char* root, const char* engineDir) {
    snprintf(root_, sizeof root_, "%s", root);
    snprintf(eng_, sizeof eng_, "%s", engineDir);
    snprintf(dir_, sizeof dir_, "%s/%s", root_, eng_);
    fs::makeDirs(dir_);
    if (!strcmp(eng_, "wavetableplus")) migrateFromWavetable();
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
    char* tok[192];
    bool ok = false;
    static Track tmp;
    while (fgets(line, sizeof line, f)) {
        int n = tokenize(line, tok, 192);
        if (n >= 1 && !strcmp(tok[0], "inst")) {
            tmp = t;
            initInstrument(tmp);  // brakujace pola = INIT
            parseInstrument(tmp, tok, n, 1);
            ok = true;
        } else if (ok && n >= 1) parseGinSound(tmp, tok, n);  // GIN: zmienione dzwieki zestawu
    }
    if (ok) t = tmp;
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
    writeGinSounds(f, t);
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

struct FactoryPreset { const char* name; int cat; int ver; const char* inst; };
const FactoryPreset* factoryList(const char* eng, int* n, int* engine);  // factory_presets.cpp

// Przenosi presety WAVETABLE+ (alt 1) z katalogu WAVETABLE do wlasnego (rozdzielenie presetow, raz).
void PresetStore::migrateFromWavetable() {
    char src[300], mark[400];
    snprintf(src, sizeof src, "%s/wavetable", root_);
    snprintf(mark, sizeof mark, "%s/_split_done", dir_);
    if (fs::exists(mark)) return;
    static char names[MAX][32];
    int n = fs::listFiles(src, ".dpr", names, MAX);
    for (int i = 0; i < n; i++) {
        char from[400], to[400], line[2048];
        snprintf(from, sizeof from, "%s/%s.dpr", src, names[i]);
        FILE* f = fopen(from, "r");
        if (!f) continue;
        bool plus = false;
        while (fgets(line, sizeof line, f)) if (!strncmp(line, "inst", 4) && strstr(line, " alt 1")) plus = true;
        fclose(f);
        snprintf(to, sizeof to, "%s/%s.dpr", dir_, names[i]);
        if (plus && !fs::exists(to)) rename(from, to);
    }
    FILE* m = fopen(mark, "w");
    if (m) fclose(m);
}

// Zestaw fabryczny: dopisuje presety z wersji nowszych niz najwyzszy znacznik _factory_vN. Preset o tej samej nazwie
// nadpisywany tylko, gdy jego wpis ma wersje nowsza niz znacznik (poprawka presetu fabrycznego, np. MONOLIT v3: oktawa).
void PresetStore::writeFactory() {
    int n = 0, engine = ENG_WAVETABLE;
    const FactoryPreset* list = factoryList(eng_, &n, &engine);
    int had = 0, top = 0;
    for (int i = 0; i < n; i++) if (list[i].ver > top) top = list[i].ver;
    for (int v = top; v >= 1 && !had; v--) {
        char m[400];
        snprintf(m, sizeof m, "%s/_factory_v%d", dir_, v);
        if (fs::exists(m)) had = v;
    }
    if (had >= top) return;
    static char line[2048];
    char* tok[192];
    for (int i = 0; i < n; i++) {
        const FactoryPreset& p = list[i];
        if (p.ver <= had || (find(p.name) >= 0 && had == 0)) continue;  // nowsza wersja istniejacego presetu fabrycznego go zastepuje
        Track t;
        t.engine = (uint8_t)engine;
        t.alt = !strcmp(eng_, "wavetableplus") || !strcmp(eng_, "oxbowplus");  // tryb rozszerzony przed INIT
        initInstrument(t);
        snprintf(line, sizeof line, "%s", p.inst);
        int nt = tokenize(line, tok, 192);
        parseInstrument(t, tok, nt, 0);
        save(t, p.name, p.cat);
    }
    char mark[400];
    snprintf(mark, sizeof mark, "%s/_factory_v%d", dir_, top);
    FILE* f = fopen(mark, "w");
    if (f) fclose(f);
}

}  // namespace dp

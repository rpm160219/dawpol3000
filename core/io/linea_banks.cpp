#include "io/linea_banks.h"
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

namespace dp {

LineaBanks& lineaBanks() { static LineaBanks b; return b; }

static bool endsSyx(const char* f) { const size_t n = strlen(f); return n > 4 && !strcasecmp(f + n - 4, ".syx"); }
static void mkdirs(const char* path) {
    char tmp[300];
    snprintf(tmp, sizeof tmp, "%s", path);
    for (char* p = tmp + 1; *p; p++) if (*p == '/') { *p = 0; mkdir(tmp, 0755); *p = '/'; }
    mkdir(tmp, 0755);
}
static uint8_t* slurp(const char* path, int* n) {
    FILE* f = fopen(path, "rb");
    if (!f) return nullptr;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > (4 << 20)) { fclose(f); return nullptr; }
    uint8_t* b = (uint8_t*)malloc((size_t)sz);
    if (b && fread(b, 1, (size_t)sz, f) != (size_t)sz) { free(b); b = nullptr; }
    fclose(f);
    *n = (int)sz;
    return b;
}
// nazwa banku z nazwy pliku: wielkie litery, bez rozszerzenia; "factory" w nazwie = FACTORY
static void bankName(const char* file, char* out, int n) {
    char b[64];
    snprintf(b, sizeof b, "%s", file);
    char* dot = strrchr(b, '.');
    if (dot) *dot = 0;
    int k = 0;
    for (const char* p = b; *p && k < n - 1; p++) out[k++] = (char)(isalnum((unsigned char)*p) ? toupper((unsigned char)*p) : (*p == '-' ? '-' : ' '));
    out[k] = 0;
}

// --- SysEx D-50 (jak d5_syx_to_patches.py: DT1, sumy kontrolne, 448 bajtow na patch od 02-00-00, kontrola zakresow)
bool LineaBanks::parseSyx(const uint8_t* raw, int n, uint8_t (*out)[LINEA_PATCH_BYTES], int* count, char* err, int errn) {
    static uint8_t mem[0x8000 + MAX_PATCHES * LINEA_PATCH_BYTES];
    static uint8_t have[sizeof mem];
    memset(have, 0, sizeof have);
    int msgs = 0;
    for (int i = 0; i < n;) {
        int a = i;
        while (a < n && raw[a] != 0xF0) a++;
        if (a >= n) break;
        int b = a;
        while (b < n && raw[b] != 0xF7) b++;
        if (b >= n) break;
        const uint8_t* m = raw + a;
        const int len = b - a + 1;
        i = b + 1;
        if (len < 10 || m[1] != 0x41 || m[3] != 0x14 || m[4] != 0x12) continue;  // tylko DT1 D-50
        const int addr = m[5] * 16384 + m[6] * 128 + m[7];
        const int dn = len - 10;
        int total = m[5] + m[6] + m[7];
        for (int k = 0; k < dn; k++) total += m[8 + k];
        if ((128 - total % 128) % 128 != m[len - 2]) { snprintf(err, errn, "CHECKSUM ERROR AT %02X-%02X-%02X", m[5], m[6], m[7]); return false; }
        for (int k = 0; k < dn; k++) {
            const int ad = addr + k;
            if (ad >= 0 && ad < (int)sizeof mem) { mem[ad] = m[8 + k]; have[ad] = 1; }
        }
        msgs++;
    }
    if (!msgs) { snprintf(err, errn, "NO D-50 DATA IN THIS FILE"); return false; }
    int found = 0;
    while (found < MAX_PATCHES) {
        const int base = 0x8000 + found * LINEA_PATCH_BYTES;
        bool all = true;
        for (int k = 0; k < LINEA_PATCH_BYTES && all; k++) all = have[base + k];
        if (!all) break;
        memcpy(out[found], mem + base, LINEA_PATCH_BYTES);
        found++;
    }
    if (!found) { snprintf(err, errn, "NO PATCH MEMORY AT 02-00-00"); return false; }
    struct Chk { const char* l; int blk, off, hi; };
    static const Chk chk[] = {{"STRUCTURE", 2, 10, 6}, {"STRUCTURE", 5, 10, 6}, {"CHORUS TYPE", 2, 42, 7}, {"KEY MODE", 6, 18, 8},
                              {"REVERB TYPE", 6, 30, 31}, {"PCM WAVE", 0, 7, 99}, {"WAVEFORM", 0, 6, 1}, {"TVF RESONANCE", 0, 14, 30}};
    for (const Chk& c : chk)
        for (int p = 0; p < found; p++)
            if (out[p][c.blk * 64 + c.off] > c.hi) { snprintf(err, errn, "%s OUT OF RANGE - NOT A D-50 BANK", c.l); return false; }
    *count = found;
    return true;
}

bool LineaBanks::writeSyx(const char* path, const uint8_t (*p)[LINEA_PATCH_BYTES], int count) {
    char tmp[340];
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE* f = fopen(tmp, "wb");
    if (!f) return false;
    const int total = count * LINEA_PATCH_BYTES;
    for (int off = 0; off < total; off += 256) {
        const int len = total - off < 256 ? total - off : 256;
        const int addr = 0x8000 + off;
        uint8_t h[8] = {0xF0, 0x41, 0x00, 0x14, 0x12, (uint8_t)((addr >> 14) & 0x7F), (uint8_t)((addr >> 7) & 0x7F), (uint8_t)(addr & 0x7F)};
        fwrite(h, 1, 8, f);
        int sum = h[5] + h[6] + h[7];
        for (int k = 0; k < len; k++) {
            const uint8_t v = p[(off + k) / LINEA_PATCH_BYTES][(off + k) % LINEA_PATCH_BYTES] & 0x7F;
            fputc(v, f);
            sum += v;
        }
        fputc((128 - sum % 128) % 128, f);
        fputc(0xF7, f);
    }
    const bool ok = fclose(f) == 0;
    if (!ok || rename(tmp, path) != 0) { unlink(tmp); return false; }
    return true;
}

void LineaBanks::init(const char* presetRoot, const char* romDir, const char* sysexDir) {
    snprintf(dir_, sizeof dir_, "%s/linea/banks", presetRoot);
    snprintf(rom_, sizeof rom_, "%s", romDir);
    snprintf(sysex_, sizeof sysex_, "%s", sysexDir);
    mkdirs(dir_);
    ready_ = true;
    scan();
    if (n_ == 0) {  // pierwsze uruchomienie: banki z roms/linea
        char names[MAX_IMPORT][64];
        const int ni = importList(names, MAX_IMPORT);
        char msg[64];
        for (int i = 0; i < ni; i++) importFile(i, msg, sizeof msg);
    }
}

void LineaBanks::scan() {
    for (int i = 0; i < n_; i++) free(bank_[i].data);
    n_ = 0;
    DIR* d = opendir(dir_);
    if (!d) return;
    struct dirent* e;
    while ((e = readdir(d)) && n_ < MAX_BANKS) {
        if (!endsSyx(e->d_name)) continue;
        Bank& b = bank_[n_++];
        memset(&b, 0, sizeof b);
        snprintf(b.file, sizeof b.file, "%s", e->d_name);
        bankName(e->d_name, b.name, sizeof b.name);
    }
    closedir(d);
    // kolejnosc: FACTORY, potem alfabetycznie, USER na koncu
    auto rank = [](const Bank& b) { return !strcmp(b.name, "FACTORY") ? 0 : (!strcmp(b.name, "USER") ? 2 : 1); };
    for (int i = 1; i < n_; i++)
        for (int j = i; j > 0; j--) {
            Bank &x = bank_[j - 1], &y = bank_[j];
            const int rx = rank(x), ry = rank(y);
            if (rx > ry || (rx == ry && strcmp(x.name, y.name) > 0)) { Bank t = x; x = y; y = t; } else break;
        }
}

bool LineaBanks::loadBank(int b) {
    if (b < 0 || b >= n_) return false;
    Bank& k = bank_[b];
    if (k.loaded) return k.n > 0;
    k.loaded = true;
    char path[340];
    snprintf(path, sizeof path, "%s/%s", dir_, k.file);
    int n = 0;
    uint8_t* raw = slurp(path, &n);
    if (!raw) return false;
    k.data = (uint8_t(*)[LINEA_PATCH_BYTES])malloc(sizeof(uint8_t) * LINEA_PATCH_BYTES * MAX_PATCHES);
    char err[64];
    int cnt = 0;
    if (k.data && parseSyx(raw, n, k.data, &cnt, err, sizeof err)) {
        k.n = cnt;
        for (int s = 0; s < cnt; s++) lineaPatchName(k.data[s], k.names[s]);
    }
    free(raw);
    return k.n > 0;
}

int LineaBanks::patches(int b) { ensure(); loadBank(b); return b >= 0 && b < n_ ? bank_[b].n : 0; }
const uint8_t* LineaBanks::patch(int b, int s) { ensure(); if (!loadBank(b) || s < 0 || s >= bank_[b].n) return nullptr; return bank_[b].data[s]; }
const char* LineaBanks::patchName(int b, int s) { ensure(); if (!loadBank(b) || s < 0 || s >= bank_[b].n) return ""; return bank_[b].names[s]; }
int LineaBanks::find(const char* nm) { ensure(); for (int i = 0; i < n_; i++) if (!strcmp(bank_[i].name, nm)) return i; return -1; }

int LineaBanks::importList(char out[][64], int max) {
    int k = 0;
    const char* dirs[2] = {rom_, sysex_};
    for (const char* dd : dirs) {
        DIR* d = opendir(dd);
        if (!d) continue;
        struct dirent* e;
        while ((e = readdir(d)) && k < max && k < MAX_IMPORT) {
            if (!endsSyx(e->d_name)) continue;
            snprintf(imp_[k], sizeof imp_[k], "%s", e->d_name);
            snprintf(impPath_[k], sizeof impPath_[k], "%s/%s", dd, e->d_name);
            snprintf(out[k], 64, "%s", e->d_name);
            k++;
        }
        closedir(d);
    }
    return k;
}

bool LineaBanks::importFile(int i, char* msg, int n) {
    if (i < 0 || i >= MAX_IMPORT || !impPath_[i][0]) { snprintf(msg, n, "NO FILE"); return false; }
    int len = 0;
    uint8_t* raw = slurp(impPath_[i], &len);
    if (!raw) { snprintf(msg, n, "CANNOT READ FILE"); return false; }
    static uint8_t tmp[MAX_PATCHES][LINEA_PATCH_BYTES];
    int cnt = 0;
    const bool ok = parseSyx(raw, len, tmp, &cnt, msg, n);
    free(raw);
    if (!ok) return false;
    char nm[24];
    bankName(imp_[i], nm, sizeof nm);
    if (strstr(nm, "FACTORY")) snprintf(nm, sizeof nm, "FACTORY");
    char path[340];
    snprintf(path, sizeof path, "%s/%s.syx", dir_, nm);
    if (!writeSyx(path, tmp, cnt)) { snprintf(msg, n, "CANNOT WRITE BANK"); return false; }
    scan();
    snprintf(msg, n, "BANK %s: %d PATCHES", nm, cnt);
    return true;
}

int LineaBanks::saveUser(const uint8_t* p, int slot) {
    ensure();
    static uint8_t tmp[MAX_PATCHES][LINEA_PATCH_BYTES];
    int cnt = 0;
    const int b = find("USER");
    if (b >= 0 && loadBank(b)) { cnt = bank_[b].n; memcpy(tmp, bank_[b].data, sizeof(uint8_t) * LINEA_PATCH_BYTES * cnt); }
    if (slot < 0 || slot > cnt) slot = cnt;
    if (slot >= MAX_PATCHES) return -1;
    memcpy(tmp[slot], p, LINEA_PATCH_BYTES);
    if (slot == cnt) cnt++;
    char path[340];
    snprintf(path, sizeof path, "%s/USER.syx", dir_);
    if (!writeSyx(path, tmp, cnt)) return -1;
    scan();
    return slot;
}

bool LineaBanks::removePatch(int b, int s) {
    ensure();
    if (b < 0 || b >= n_ || strcmp(bank_[b].name, "USER") || !loadBank(b) || s < 0 || s >= bank_[b].n) return false;
    static uint8_t tmp[MAX_PATCHES][LINEA_PATCH_BYTES];
    int cnt = 0;
    for (int i = 0; i < bank_[b].n; i++) if (i != s) memcpy(tmp[cnt++], bank_[b].data[i], LINEA_PATCH_BYTES);
    char path[340];
    snprintf(path, sizeof path, "%s/%s", dir_, bank_[b].file);
    if (cnt == 0) { unlink(path); scan(); return true; }
    const bool ok = writeSyx(path, tmp, cnt);
    scan();
    return ok;
}

bool LineaBanks::removeBank(int b) {
    ensure();
    if (b < 0 || b >= n_) return false;
    char path[340];
    snprintf(path, sizeof path, "%s/%s", dir_, bank_[b].file);
    const bool ok = unlink(path) == 0;
    scan();
    return ok;
}

}  // namespace dp

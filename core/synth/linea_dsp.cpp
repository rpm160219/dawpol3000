// LINEA — opakowanie silnika PicoFaceD5 (zob. linea_dsp.h). Logika poziomow, limitu glosow i SysEx wg D5_Bridge.cpp
// oryginalu (third_party/pico_d50/tools), bez warstwy sprzetu Pico.
#include "synth/linea_dsp.h"
#include <dirent.h>
#include <initializer_list>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include "d5_engine/d5_patch_map.h"

// ---------------------------------------------------------------- ROM
namespace {
const uint32_t PCM_A_CRC = 0x1461C0FB, PCM_B_CRC = 0xE50599BF, PCM_COMBINED_CRC = 0xE2AED2D9;
struct ProM { uint32_t crc; const char* v; };
const ProM kProm[] = {{0xE92C69F9, "V2.22"}, {0xCCBA4E46, "V1.06"}, {0x3E72BDF0, "V2.21"}, {0x5DB7C340, "V2.20"},
                      {0xD1387C54, "V2.10"}, {0x7FC199C5, "V1.10"}, {0xD871451E, "V1.04"}};
int16_t* g_pcm = nullptr;          // 262144 probek
int16_t* g_loopRam = nullptr;
char g_status[64] = "ROM NOT LOADED";
bool g_ready = false;

uint32_t crc32(const uint8_t* d, size_t n) {
    static uint32_t t[256];
    static bool init = false;
    if (!init) {
        for (uint32_t i = 0; i < 256; i++) { uint32_t c = i; for (int k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1; t[i] = c; }
        init = true;
    }
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) c = t[(c ^ d[i]) & 255] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}
// zdublowany zrzut (identyczne polowki) -> jedna kopia (jak _fold w d5_rom.py)
size_t fold(const uint8_t* d, size_t n) {
    while (n >= 2 && memcmp(d, d + n / 2, n / 2) == 0) n /= 2;
    return n;
}
// dekodowanie slowa PCM: permutacja bitow + znak i 15-bitowy log2 (d5_rom.py: _reorder_bits, _build_lut, int(v*32767))
int16_t decodeWord(uint16_t raw) {
    uint32_t o = raw & 0x8000;
    o |= (raw << 8) & 0x4000;
    o |= (raw >> 1) & 0x3F80;
    o |= (raw << 1) & 0x007E;
    o |= (raw >> 7) & 0x0001;
    const double amp = pow(2.0, ((double)(o & 0x7FFF) - 32767.0) / 2048.0);
    double v = (o & 0x8000) ? -amp : amp;
    long x = (long)(v * 32767.0);  // int() w Pythonie: obciecie do zera
    if (x > 32767) x = 32767;
    if (x < -32767) x = -32767;
    return (int16_t)x;
}
uint8_t* readFile(const char* path, size_t* n) {
    FILE* f = fopen(path, "rb");
    if (!f) return nullptr;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > (2 << 20)) { fclose(f); return nullptr; }
    uint8_t* b = (uint8_t*)malloc((size_t)sz);
    if (b && fread(b, 1, (size_t)sz, f) != (size_t)sz) { free(b); b = nullptr; }
    fclose(f);
    *n = (size_t)sz;
    return b;
}
}  // namespace

bool lineaRomLoad(const char* dir) {
    if (g_ready) return true;
    DIR* d = opendir(dir);
    if (!d) { snprintf(g_status, sizeof g_status, "NO ROM FOLDER %s", dir); return false; }
    uint8_t *a = nullptr, *b = nullptr, *comb = nullptr;
    size_t na = 0, nb = 0, nc = 0;
    bool prom = false;
    const char* ver = "UNKNOWN";
    struct dirent* e;
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.') continue;
        char path[512];
        snprintf(path, sizeof path, "%s/%s", dir, e->d_name);
        size_t n = 0;
        uint8_t* buf = readFile(path, &n);
        if (!buf) continue;
        n = fold(buf, n);
        const uint32_t c = crc32(buf, n);
        if (c == PCM_A_CRC && !a) { a = buf; na = n; continue; }
        if (c == PCM_B_CRC && !b) { b = buf; nb = n; continue; }
        if (c == PCM_COMBINED_CRC && !comb) { comb = buf; nc = n; continue; }
        if (n == 65536 && memmem(buf, n, "Marmba", 6)) {  // EPROM programu (tabela nazw PCM)
            prom = true;
            for (const ProM& p : kProm) if (p.crc == c) ver = p.v;
        }
        free(buf);
    }
    closedir(d);
    if (!a && comb) { a = comb; na = nc / 2; b = comb + nc / 2; nb = nc / 2; }
    if (!a || !b || !prom) {
        snprintf(g_status, sizeof g_status, "MISSING %s%s%s IN ROMS/LINEA", !a ? "PCM A " : "", !b ? "PCM B " : "", !prom ? "EPROM" : "");
        if (comb) free(comb); else { free(a); free(b); }
        return false;
    }
    g_pcm = (int16_t*)malloc(sizeof(int16_t) * 262144);
    if (!g_pcm) return false;
    size_t k = 0;
    for (size_t i = 0; i + 1 < na && k < 262144; i += 2) g_pcm[k++] = decodeWord((uint16_t)(a[i] << 8 | a[i + 1]));
    for (size_t i = 0; i + 1 < nb && k < 262144; i += 2) g_pcm[k++] = decodeWord((uint16_t)(b[i] << 8 | b[i + 1]));
    while (k < 262144) g_pcm[k++] = 0;
    if (comb) free(comb); else { free(a); free(b); }
    g_loopRam = (int16_t*)malloc(sizeof(int16_t) * d5::loop_ram_words());
    if (g_loopRam) d5::install_loop_ram(g_pcm, g_loopRam, d5::loop_ram_words());
    snprintf(g_status, sizeof g_status, "ROM OK  EPROM %s", ver);
    g_ready = true;
    return true;
}
bool lineaRomReady() { return g_ready; }
const char* lineaRomStatus() { return g_status; }
const char* lineaPcmName(int i) { return d5::kPcmSamples[i < 0 || i >= d5::kPcmCount ? 0 : i].name; }
const int16_t* lineaPcm() { return g_pcm; }

// ---------------------------------------------------------------- nazwy patchy
static const char kChars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890-";
void lineaPatchName(const uint8_t* p, char out[19]) {
    int end = 0;
    for (int k = 0; k < 18; k++) {
        const int v = p[6 * 64 + k] & 0x3F;
        out[k] = v < (int)sizeof kChars - 1 ? kChars[v] : ' ';
        if (out[k] != ' ') end = k + 1;
    }
    out[end] = 0;
}
void lineaSetPatchName(uint8_t* p, const char* name) {
    for (int k = 0; k < 18; k++) {
        const char ch = *name ? *name++ : ' ';
        const char* f = strchr(kChars, ch ? ch : ' ');
        p[6 * 64 + k] = (uint8_t)(f ? f - kChars : 0);
    }
}
int lineaStructure(const uint8_t* p, int tone) { return p[(tone ? 5 : 2) * 64 + 10] % 7 + 1; }

// ---------------------------------------------------------------- silnik
namespace {
// FIR 32 -> 48 kHz: interpolacja x3 (96 kHz), filtr do 15 kHz, decymacja /2; 3 fazy po 24 wspolczynniki
const int FIR_TAPS = 72, FIR_PH = 24;
float g_fir[3][FIR_PH];
bool g_firReady = false;
void firInit() {
    if (g_firReady) return;
    const double fc = 15000.0 / 96000.0;  // znormalizowana czestotliwosc graniczna
    double h[FIR_TAPS], sum = 0;
    for (int n = 0; n < FIR_TAPS; n++) {
        const double x = n - (FIR_TAPS - 1) / 2.0;
        const double s = x == 0 ? 2 * fc : sin(2 * M_PI * fc * x) / (M_PI * x);
        const double w = 0.42 - 0.5 * cos(2 * M_PI * n / (FIR_TAPS - 1)) + 0.08 * cos(4 * M_PI * n / (FIR_TAPS - 1));
        h[n] = s * w;
        sum += h[n];
    }
    for (int n = 0; n < FIR_TAPS; n++) g_fir[n % 3][n / 3] = (float)(h[n] / sum * 3.0);  // wzmocnienie x3 (interpolacja)
    g_firReady = true;
}
}  // namespace

struct LineaCore {
    d5::Patch patch;
    uint8_t temp[LINEA_PATCH_BYTES];   // patch z przesunieciami wydajnosciowymi
    uint8_t src[LINEA_PATCH_BYTES];    // ostatnio ustawiony (bez przesuniec)
    uint32_t id = 0xFFFFFFFFu;
    LineaPerf perf;
    bool configured = false;
    bool whole = false;
    float baseVolume = 1.f;
    uint8_t held[128] = {};
    int active = 0;
    uint32_t notes = 0;
    float lastVel = 0, outLv = 0;
    // resampler
    float hl[FIR_PH * 2] = {}, hr[FIR_PH * 2] = {};
    int hp = 0;      // pozycja zapisu w historii
    int phase = 0;   // indeks wyjscia mod 3 (2m mod 3 = faza)
};

LineaCore* lineaCreate(float) {
    firInit();
    return new (std::nothrow) LineaCore();
}
void lineaDestroy(LineaCore* c) { delete c; }

static void applyLevels(LineaCore* c) {
    const uint8_t* pb = d5::patch_block(c->temp, d5::kBlkPatch);
    const uint8_t* uc = d5::patch_block(c->temp, d5::kBlkUpperCommon);
    c->patch.set_volume(c->baseVolume * 80 * 0.01f);  // jak D5_Bridge: glosnosc panelu 80
    c->patch.set_reverb_balance((pb[31] > 100 ? 100 : pb[31]) * 0.01f);
    c->patch.set_chorus_balance((uc[45] > 100 ? 100 : uc[45]) * 0.01f);
    c->patch.set_master_cents(0.f);
}

void lineaSetPatch(LineaCore* c, const uint8_t* p, uint32_t id, const LineaPerf& perf) {
    if (!c || !g_ready) return;
    if (c->configured && id == c->id && perf.cut == c->perf.cut && perf.res == c->perf.res && !memcmp(p, c->src, LINEA_PATCH_BYTES)) return;
    const bool fresh = !c->configured || id != c->id;
    memcpy(c->src, p, LINEA_PATCH_BYTES);
    memcpy(c->temp, p, LINEA_PATCH_BYTES);
    c->perf = perf;
    if (perf.cut || perf.res)  // CUTOFF / RESONANCE wszystkich czterech partiali (bajty 13 i 14)
        for (int blk : {d5::kBlkUpperP1, d5::kBlkUpperP2, d5::kBlkLowerP1, d5::kBlkLowerP2}) {
            uint8_t* q = c->temp + blk * 64;
            int cu = q[13] + perf.cut, re = q[14] + perf.res;
            q[13] = (uint8_t)(cu < 0 ? 0 : (cu > 100 ? 100 : cu));
            q[14] = (uint8_t)(re < 0 ? 0 : (re > 30 ? 30 : re));
        }
    d5::PatchSpec spec = d5::patch_from_bytes(c->temp, g_pcm);
    c->baseVolume = spec.volume;
    c->whole = spec.key_mode == d5::KeyMode::kWhole;
    if (fresh) {
        lineaAllOff(c);
        c->patch.configure(spec, 32000.f);
        c->configured = true;
    } else c->patch.reconfigure(spec);
    c->id = id;
    applyLevels(c);
}

void lineaNoteOn(LineaCore* c, int note, float vel) {
    if (!c || !c->configured || note < 0 || note > 127) return;
    const int limit = c->whole ? 16 : 8;  // polifonia maszyny: 16 przy jednym tonie, 8 + 8 przy dwoch
    if (c->active >= limit && !c->held[note])
        for (int n = 0; n < 128; ++n)
            if (c->held[n]) { c->patch.note_off(n); c->held[n] = 0; --c->active; break; }
    c->patch.note_on(note, vel);
    if (!c->held[note]) ++c->active;
    c->held[note] = 1;
    c->notes++;
    c->lastVel = vel;
}
void lineaNoteOff(LineaCore* c, int note) {
    if (!c || note < 0 || note > 127) return;
    c->patch.note_off(note);
    if (c->held[note]) { c->held[note] = 0; if (c->active > 0) --c->active; }
}
void lineaAllOff(LineaCore* c) {
    if (!c) return;
    for (int n = 0; n < 128; ++n) if (c->held[n]) { c->patch.note_off(n); c->held[n] = 0; }
    c->active = 0;
}

void lineaRender(LineaCore* c, float* l, float* r, int n) {
    if (!c || !c->configured || !g_ready) { for (int i = 0; i < n; i++) l[i] = r[i] = 0.f; return; }
    float pk = 0;
    for (int i = 0; i < n; i++) {
        // wyjscie m = pozycja 2m na siatce 96 kHz; wejscie k = pozycja 3k. n = floor(2m/3) (najnowsze wejscie),
        // r = 2m - 3n (faza: 0, 2, 1, 0, 2, 1 ...); y = sum_i x[n-i] h[r + 3i]. Nowe wejscie przy r = 0 i r = 1.
        const int ph = c->phase;
        if (ph != 2) {
            float a, b;
            c->patch.next_stereo(a, b);
            c->hp = (c->hp + FIR_PH - 1) % FIR_PH;
            c->hl[c->hp] = c->hl[c->hp + FIR_PH] = a;
            c->hr[c->hp] = c->hr[c->hp + FIR_PH] = b;
        }
        const float* k = g_fir[ph];
        const float* xl = c->hl + c->hp;
        const float* xr = c->hr + c->hp;
        float sl = 0, sr = 0;
        for (int j = 0; j < FIR_PH; j++) { sl += k[j] * xl[j]; sr += k[j] * xr[j]; }
        l[i] = sl; r[i] = sr;
        const float m = fabsf(sl) > fabsf(sr) ? fabsf(sl) : fabsf(sr);
        if (m > pk) pk = m;
        c->phase = (ph + 2) % 3;
    }
    c->outLv = pk > c->outLv ? pk : c->outLv * .9f;
}

int lineaVoices(const LineaCore* c) { return c ? c->patch.voices_sounding() : 0; }
uint32_t lineaNoteCount(const LineaCore* c) { return c ? c->notes : 0; }
float lineaLastVel(const LineaCore* c) { return c ? c->lastVel : 0.f; }
float lineaOutLevel(const LineaCore* c) { return c ? c->outLv : 0.f; }

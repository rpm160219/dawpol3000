// SYNTH: strony silnika LINEA (synteza LA, port PicoFaceD5). Select = nastepna strona:
//   PLAY     generatywna konstelacja patcha; D-pad lewo/prawo = patch, gora/dol = bank; lewa galka = WEKTOR (X: balans
//            partiali obu tonow, Y: balans tonow Upper/Lower — parametry patcha); prawa galka X = CUTOFF, Y = RESONANCE
//            (przesuniecie wszystkich TVF, nie zmienia patcha); ABXY = odsluch; L1/R1 = OCTAVE
//   COMMON / WG / TVF / TVA / P-ENV+LFO / EFFECTS — edycja w stylu D-50: wyswietlacz LCD, 4 parametry wiersza na
//            galkach (krokowo, zakresy z MIDI implementation D-50), D-pad gora/dol = wiersz, L1/R1 = ton UPPER / LOWER,
//            L3 = partial 1 / 2
//   BANKS    banki SysEx: lewo = banki (+ IMPORT SYSEX), prawo = patche; A = wczytaj / importuj, X = zapisz biezacy
//            patch do banku USER, B = usun (2 x; patch w USER albo bank)
#include "ui/synth_view.h"
#include <initializer_list>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "audio/engine.h"
#include "gfx/font.h"
#include "gfx/icons.h"
#include "io/linea_banks.h"
#include "ui/app.h"

namespace dp {

namespace {
const Color AMBER_C = rgb(255, 170, 40);
enum Blk { BP = 0, BC = 1, BPA = 2 };  // partial, common (tonu), patch
enum Fmt { F_NUM, F_B50, F_B7, F_B12, F_PITCH, F_KF, F_LFOM, F_PENVM, F_BENDM, F_WAVE, F_PCM, F_LSEL, F_STRUCT, F_LWAVE,
           F_LSYNC, F_KEYM, F_ONOFF, F_PLUS1, F_EQLF, F_EQHF, F_BIASPT, F_PMUTE };
struct LP { uint8_t blk, off, max; const char* name; uint8_t fmt; };
const LP X_ = {0, 0, 0, "", F_NUM};
// strony edycji: wiersze po 4 parametry
const LP kCommon[][4] = {
    {{BC, 10, 6, "STRUCTURE", F_STRUCT}, {BC, 46, 3, "PARTIALS", F_PMUTE}, {BC, 47, 100, "BALANCE", F_NUM}, {BPA, 33, 100, "TONE BAL", F_NUM}},
    {{BPA, 18, 8, "KEY MODE", F_KEYM}, {BPA, 32, 100, "VOLUME", F_NUM}, {BPA, 26, 12, "BEND RANGE", F_NUM}, {BC, 22, 100, "PMOD DEPTH", F_NUM}},
    {{BC, 23, 100, "LEVER", F_NUM}, {BC, 24, 100, "AFTERTOUCH", F_NUM}, X_, X_},
};
const LP kWg[][4] = {
    {{BP, 0, 72, "COARSE", F_PITCH}, {BP, 1, 100, "FINE", F_B50}, {BP, 2, 16, "KEYFOLLOW", F_KF}, {BP, 3, 3, "LFO MODE", F_LFOM}},
    {{BP, 6, 1, "WAVEFORM", F_WAVE}, {BP, 7, 99, "PCM WAVE", F_PCM}, {BP, 4, 2, "P-ENV MODE", F_PENVM}, {BP, 5, 2, "BEND MODE", F_BENDM}},
    {{BP, 8, 100, "PULSE WIDTH", F_NUM}, {BP, 9, 14, "PW VELO", F_B7}, {BP, 10, 5, "PW LFO", F_LSEL}, {BP, 11, 100, "PW LFO DEPTH", F_NUM}},
    {{BP, 12, 14, "PW AFTER", F_B7}, X_, X_, X_},
};
const LP kTvf[][4] = {
    {{BP, 13, 100, "CUTOFF", F_NUM}, {BP, 14, 30, "RESONANCE", F_NUM}, {BP, 15, 14, "KEYFOLLOW", F_KF}, {BP, 18, 100, "ENV DEPTH", F_NUM}},
    {{BP, 22, 100, "T1", F_NUM}, {BP, 23, 100, "T2", F_NUM}, {BP, 24, 100, "T3", F_NUM}, {BP, 25, 100, "T4", F_NUM}},
    {{BP, 26, 100, "T5", F_NUM}, {BP, 27, 100, "L1", F_NUM}, {BP, 28, 100, "L2", F_NUM}, {BP, 29, 100, "L3", F_NUM}},
    {{BP, 30, 100, "SUSTAIN", F_NUM}, {BP, 31, 1, "END", F_NUM}, {BP, 19, 100, "ENV VELO", F_NUM}, {BP, 20, 4, "DEPTH KF", F_NUM}},
    {{BP, 21, 4, "TIME KF", F_NUM}, {BP, 16, 127, "BIAS POINT", F_BIASPT}, {BP, 17, 14, "BIAS LEVEL", F_B7}, {BP, 34, 14, "AFTER", F_B7}},
    {{BP, 32, 5, "LFO", F_LSEL}, {BP, 33, 100, "LFO DEPTH", F_NUM}, X_, X_},
};
const LP kTva[][4] = {
    {{BP, 35, 100, "LEVEL", F_NUM}, {BP, 36, 100, "VELOCITY", F_B50}, {BP, 37, 127, "BIAS POINT", F_BIASPT}, {BP, 38, 12, "BIAS LEVEL", F_NUM}},
    {{BP, 39, 100, "T1", F_NUM}, {BP, 40, 100, "T2", F_NUM}, {BP, 41, 100, "T3", F_NUM}, {BP, 42, 100, "T4", F_NUM}},
    {{BP, 43, 100, "T5", F_NUM}, {BP, 44, 100, "L1", F_NUM}, {BP, 45, 100, "L2", F_NUM}, {BP, 46, 100, "L3", F_NUM}},
    {{BP, 47, 100, "SUSTAIN", F_NUM}, {BP, 48, 1, "END", F_NUM}, {BP, 49, 4, "VELO KF", F_NUM}, {BP, 50, 4, "TIME KF", F_NUM}},
    {{BP, 51, 5, "LFO", F_LSEL}, {BP, 52, 100, "LFO DEPTH", F_NUM}, {BP, 53, 14, "AFTER", F_B7}, X_},
};
const LP kPenv[][4] = {
    {{BC, 13, 50, "P-ENV T1", F_NUM}, {BC, 14, 50, "T2", F_NUM}, {BC, 15, 50, "T3", F_NUM}, {BC, 16, 50, "T4", F_NUM}},
    {{BC, 17, 100, "L0", F_B50}, {BC, 18, 100, "L1", F_B50}, {BC, 19, 100, "L2", F_B50}, {BC, 20, 100, "SUSTAIN", F_B50}},
    {{BC, 21, 100, "END", F_B50}, {BC, 11, 2, "VELO", F_NUM}, {BC, 12, 4, "TIME KF", F_NUM}, X_},
    {{BC, 25, 3, "LFO1 WAVE", F_LWAVE}, {BC, 26, 100, "RATE", F_NUM}, {BC, 27, 100, "DELAY", F_NUM}, {BC, 28, 2, "SYNC", F_LSYNC}},
    {{BC, 29, 3, "LFO2 WAVE", F_LWAVE}, {BC, 30, 100, "RATE", F_NUM}, {BC, 31, 100, "DELAY", F_NUM}, {BC, 32, 2, "SYNC", F_LSYNC}},
    {{BC, 33, 3, "LFO3 WAVE", F_LWAVE}, {BC, 34, 100, "RATE", F_NUM}, {BC, 35, 100, "DELAY", F_NUM}, {BC, 36, 2, "SYNC", F_LSYNC}},
};
const LP kFx[][4] = {
    {{BC, 37, 15, "EQ LOW FREQ", F_EQLF}, {BC, 38, 24, "LOW GAIN", F_B12}, {BC, 39, 21, "HIGH FREQ", F_EQHF}, {BC, 40, 8, "HIGH Q", F_NUM}},
    {{BC, 41, 24, "HIGH GAIN", F_B12}, {BC, 42, 7, "CHORUS TYPE", F_PLUS1}, {BC, 43, 100, "CHORUS RATE", F_NUM}, {BC, 44, 100, "DEPTH", F_NUM}},
    {{BC, 45, 100, "CHORUS BAL", F_NUM}, {BPA, 30, 31, "REVERB TYPE", F_PLUS1}, {BPA, 31, 100, "REVERB BAL", F_NUM}, X_},
};
struct Pg { const char* name; const LP (*rows)[4]; int n; };
const Pg kPages[6] = {{"COMMON", kCommon, 3}, {"WG", kWg, 4}, {"TVF", kTvf, 6}, {"TVA", kTva, 5}, {"P-ENV / LFO", kPenv, 6}, {"EFFECTS", kFx, 3}};
const int PG_PLAY = 0, PG_BANKS = 7;
const int kBtn[8] = {B_LEFT, B_DOWN, B_RIGHT, B_UP, B_Y, B_B, B_A, B_X};
const int kMaj[4] = {0, 4, 7, 12};
const float kEqLo[16] = {63, 75, 88, 105, 125, 150, 175, 210, 250, 300, 350, 420, 500, 600, 700, 840};
const float kEqHi[22] = {250, 300, 350, 420, 500, 600, 700, 840, 1000, 1200, 1400, 1700, 2000, 2400, 2800, 3400, 4000, 4800, 5700, 6700, 8000, 9500};
int blkIndex(const LP& lp, int tone, int part) { return lp.blk == BP ? (tone ? 3 : 0) + part : (lp.blk == BC ? (tone ? 5 : 2) : 6); }
void fmtVal(const LP& lp, int v, char* o, int n) {
    static const char* nn[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    static const char* kf[17] = {"-1", "-1/2", "-1/4", "0", "1/8", "1/4", "3/8", "1/2", "5/8", "3/4", "7/8", "1", "5/4", "3/2", "2", "S1", "S2"};
    switch (lp.fmt) {
    case F_B50: snprintf(o, n, "%+d", v - 50); break;
    case F_B7: snprintf(o, n, "%+d", v - 7); break;
    case F_B12: snprintf(o, n, "%+d DB", v - 12); break;
    case F_PITCH: snprintf(o, n, "%s%d", nn[(v + 24) % 12], (v + 24) / 12 - 1); break;
    case F_KF: snprintf(o, n, "%s", kf[v > 16 ? 16 : v]); break;
    case F_LFOM: { static const char* m[4] = {"OFF", "(+)", "(-)", "A&L"}; snprintf(o, n, "%s", m[v & 3]); } break;
    case F_PENVM: { static const char* m[3] = {"OFF", "(+)", "(-)"}; snprintf(o, n, "%s", m[v > 2 ? 2 : v]); } break;
    case F_BENDM: { static const char* m[3] = {"OFF", "KF", "NORM"}; snprintf(o, n, "%s", m[v > 2 ? 2 : v]); } break;
    case F_WAVE: snprintf(o, n, "%s", v ? "SAW" : "SQU"); break;
    case F_PCM: snprintf(o, n, "%d %s", v + 1, lineaPcmName(v)); break;
    case F_LSEL: { static const char* m[6] = {"+1", "-1", "+2", "-2", "+3", "-3"}; snprintf(o, n, "%s", m[v > 5 ? 5 : v]); } break;
    case F_STRUCT: { static const char* m[7] = {"1 S+S", "2 SxS", "3 P+S", "4 PxS", "5 SxP", "6 P+P", "7 PxP"}; snprintf(o, n, "%s", m[v % 7]); } break;
    case F_LWAVE: { static const char* m[4] = {"TRI", "SAW", "SQU", "RND"}; snprintf(o, n, "%s", m[v & 3]); } break;
    case F_LSYNC: { static const char* m[3] = {"OFF", "ON", "KEY"}; snprintf(o, n, "%s", m[v > 2 ? 2 : v]); } break;
    case F_KEYM: { static const char* m[9] = {"WHOLE", "DUAL", "SPLIT", "WHOLE-S", "DUAL-S", "SPL-US", "SPL-LS", "SEP", "SEP-S"}; snprintf(o, n, "%s", m[v > 8 ? 8 : v]); } break;
    case F_PLUS1: snprintf(o, n, "%d", v + 1); break;
    case F_EQLF: snprintf(o, n, "%.0f HZ", kEqLo[v > 15 ? 15 : v]); break;
    case F_EQHF: { const float f = kEqHi[v > 21 ? 21 : v]; if (f >= 1000) snprintf(o, n, "%.1fK", f / 1000); else snprintf(o, n, "%.0f HZ", f); } break;
    case F_BIASPT: snprintf(o, n, "%c%s%d", v & 64 ? '>' : '<', nn[((v & 63) + 33) % 12], ((v & 63) + 33) / 12 - 1); break;
    case F_PMUTE: { static const char* m[4] = {"--", "1-", "-2", "12"}; snprintf(o, n, "%s", m[v & 3]); } break;
    default: snprintf(o, n, "%d", v); break;
    }
}
// obwiednia w stylu instrukcji: T1..T5, L1..L3, SUSTAIN, END
void drawEnv(Canvas& c, float x, float y, float w, float h, const uint8_t* t5, const uint8_t* l3, int sus, int end, Color col) {
    float tt[5], tot = 0;
    for (int i = 0; i < 5; i++) { tt[i] = 6 + t5[i]; tot += tt[i]; }
    tot += 40;
    float px = x, py = y + h;
    const float lv[5] = {l3[0] / 100.f, l3[1] / 100.f, l3[2] / 100.f, sus / 100.f, end ? 1.f : 0.f};
    float pts[2 * 7];
    int n = 0;
    pts[n * 2] = px; pts[n * 2 + 1] = py; n++;
    for (int i = 0; i < 5; i++) {
        px += tt[i] / tot * w;
        if (i == 4) px += 40 / tot * w;  // podtrzymanie przed T5
        pts[n * 2] = px; pts[n * 2 + 1] = y + h - lv[i == 4 ? 4 : i] * h; n++;
        if (i == 3) { px += 40 / tot * w * 0; }
    }
    c.polyline(pts, n, 1.6f, col);
    for (int i = 0; i < n; i++) c.fillCircle(pts[2 * i], pts[2 * i + 1], 2.f, col);
    static const char* tl[5] = {"T1", "T2", "T3", "T4", "T5"};
    for (int i = 1; i < n && i <= 5; i++) drawText(c, (pts[2 * i] + pts[2 * i - 2]) * .5f, y + h + 6, tl[i - 1], 6, 1.f, rgb(110, 80, 30), ALIGN_CENTER);
}
}  // namespace

void SynthView::updateLinea(const Buttons& b, const InputState& in, float dt, Track& t) {
    AudioEngine* e = app_->engine();
    if (!t.lineaSet) { lineaEnsure(t); if (t.lineaSet) app_->markDirty(); }
    if (b.pressed(B_START)) app_->togglePlaySong();
    LineaBanks& bk = lineaBanks();
    bool ch = false;
    static const int axisOf[4] = {AX_LX, AX_LY, AX_RX, AX_RY};
    static const float sign[4] = {1.f, -1.f, 1.f, -1.f};
    auto audition = [&](const int* btns, int nb, const int* semis) {
        for (int i = 0; i < nb; i++) {
            if (b.pressed(btns[i]) && e) {
                const int pitch = 12 * (octave_ + 1) + semis[i];
                if (held_[i] >= 0) e->send(EC_NOTE_OFF, track_, held_[i]);
                e->send(EC_NOTE_ON, track_, pitch, 0, 0.8f);
                held_[i] = pitch;
            }
            if (b.released(btns[i]) && held_[i] >= 0) { if (e) e->send(EC_NOTE_OFF, track_, held_[i]); held_[i] = -1; }
        }
    };
    if (page_ == PG_PLAY) {
        static const int ab[4] = {B_Y, B_B, B_A, B_X};
        audition(ab, 4, kMaj);
        if (b.pressed(B_L1) && t.octave > -3) { t.octave--; ch = true; }
        if (b.pressed(B_R1) && t.octave < 3) { t.octave++; ch = true; }
        // patch / bank
        int bank = bk.find(t.lineaBank), slot = t.lineaSlot;
        if (bank < 0) { bank = 0; slot = -1; }
        int d = b.repeat(B_RIGHT) ? 1 : (b.repeat(B_LEFT) ? -1 : 0);
        if (d && bk.count() > 0) {
            slot += d;
            if (slot >= bk.patches(bank)) { bank = (bank + 1) % bk.count(); slot = 0; }
            if (slot < 0) { bank = (bank + bk.count() - 1) % bk.count(); slot = bk.patches(bank) - 1; }
            if (lineaLoadPatch(t, bank, slot)) ch = true;
        }
        const int db = b.pressed(B_DOWN) ? 1 : (b.pressed(B_UP) ? -1 : 0);
        if (db && bk.count() > 0) {
            bank = (bank + db + bk.count()) % bk.count();
            if (lineaLoadPatch(t, bank, 0)) ch = true;
        }
        // lewa galka: wektor (balans partiali obu tonow, balans tonow); prawa: CUTOFF / RESONANCE
        const float vx = RelEncoder::delta(in.axis[AX_LX], dt, .7f), vy = RelEncoder::delta(in.axis[AX_LY], dt, .7f);
        if (vx != 0.f) for (int blk : {2, 5}) { int v = t.linea[blk * 64 + 47] + (int)lrintf(vx * 100.f); t.linea[blk * 64 + 47] = (uint8_t)(v < 0 ? 0 : (v > 100 ? 100 : v)); ch = true; }
        if (vy != 0.f) { int v = t.linea[6 * 64 + 33] + (int)lrintf(vy * 100.f); t.linea[6 * 64 + 33] = (uint8_t)(v < 0 ? 0 : (v > 100 ? 100 : v)); ch = true; }
        const int sc = stepLn_[2].steps(in.axis[AX_RX], dt, 30.f), sr = stepLn_[3].steps(-in.axis[AX_RY], dt, 10.f);
        if (sc) { int v = t.lineaCut + sc; t.lineaCut = (int8_t)(v < -100 ? -100 : (v > 100 ? 100 : v)); ch = true; }
        if (sr) { int v = t.lineaRes + sr; t.lineaRes = (int8_t)(v < -30 ? -30 : (v > 30 ? 30 : v)); ch = true; }
    } else if (page_ == PG_BANKS) {
        if (b.pressed(B_L1) || b.pressed(B_R1) || b.pressed(B_LEFT) || b.pressed(B_RIGHT)) { lnFocus_ ^= 1; lnConfirm_ = 0; }
        const int nb = bk.count();
        const bool importRow = lnBank_ == nb;  // ostatni wiersz banku = IMPORT SYSEX
        if (lnFocus_ == 0) {
            if (b.repeat(B_UP) && lnBank_ > 0) { lnBank_--; lnSlot_ = 0; lnFirst_ = 0; lnConfirm_ = 0; }
            if (b.repeat(B_DOWN) && lnBank_ < nb) { lnBank_++; lnSlot_ = 0; lnFirst_ = 0; lnConfirm_ = 0; }
        } else {
            const int np = importRow ? lnImpN_ : bk.patches(lnBank_);
            if (b.repeat(B_UP) && lnSlot_ > 0) { lnSlot_--; lnConfirm_ = 0; }
            if (b.repeat(B_DOWN) && lnSlot_ < np - 1) { lnSlot_++; lnConfirm_ = 0; }
        }
        if (importRow) lnImpN_ = bk.importList(lnImp_, 32);
        if (b.pressed(B_A)) {
            if (importRow) {
                if (lnFocus_ == 1 && lnSlot_ < lnImpN_) {
                    char msg[64];
                    bk.importFile(lnSlot_, msg, sizeof msg);
                    app_->toast(msg);
                }
            } else if (lineaLoadPatch(t, lnBank_, lnSlot_)) { ch = true; app_->toast(t.preset); }
        }
        if (b.pressed(B_X) && t.lineaSet) {  // zapis do banku USER (na koncu albo w zaznaczony slot USER)
            const int ub = bk.find("USER");
            const int slot = bk.saveUser(t.linea, lnBank_ == ub && lnFocus_ == 1 ? lnSlot_ : -1);
            if (slot >= 0) {
                snprintf(t.lineaBank, sizeof t.lineaBank, "USER");
                t.lineaSlot = (int16_t)slot;
                ch = true;
                char m[40];
                snprintf(m, sizeof m, "SAVED TO USER %d", slot + 1);
                app_->toast(m);
            } else app_->toast("SAVE FAILED");
        }
        if (b.pressed(B_B) && !importRow) {
            if (lnConfirm_ == 0) lnConfirm_ = 1;
            else {
                const int ub = bk.find("USER");
                if (lnFocus_ == 1 && lnBank_ == ub) bk.removePatch(lnBank_, lnSlot_);
                else if (lnFocus_ == 0) bk.removeBank(lnBank_);
                lnConfirm_ = 0;
                lnSlot_ = 0;
            }
        }
    } else {  // strony edycji
        const Pg& pg = kPages[page_ - 1];
        if (b.pressed(B_L1)) lnTone_ = 0;
        if (b.pressed(B_R1)) lnTone_ = 1;
        if (b.pressed(B_L3)) lnPart_ ^= 1;
        if (b.repeat(B_UP)) lnRow_ = (lnRow_ + pg.n - 1) % pg.n;
        if (b.repeat(B_DOWN)) lnRow_ = (lnRow_ + 1) % pg.n;
        if (lnRow_ >= pg.n) lnRow_ = 0;
        static const int ab[4] = {B_Y, B_B, B_A, B_X};
        audition(ab, 4, kMaj);
        for (int i = 0; i < 4; i++) {
            const LP& lp = pg.rows[lnRow_][i];
            if (!lp.name[0]) continue;
            const float rate = lp.max > 40 ? 30.f : 6.f;
            const int st = stepLn_[i].steps(in.axis[axisOf[i]] * sign[i], dt, rate);
            if (!st) continue;
            uint8_t& v = t.linea[blkIndex(lp, lnTone_, lnPart_) * 64 + lp.off];
            int x = v + st;
            v = (uint8_t)(x < 0 ? 0 : (x > lp.max ? lp.max : x));
            ch = true;
        }
    }
    if (ch) app_->markDirty();
}

void SynthView::renderLinea(Canvas& c, const Track& t) {
    Project& p = app_->project();
    AudioEngine* e = app_->engine();
    const Color tc = pal::track[t.color % pal::TRACK_COLORS];
    const bool playing = e && e->isPlaying() && e->playMode() == Sequencer::SONG;
    const int tick = playing ? e->playTick() : 0;
    LineaBanks& bk = lineaBanks();
    c.clear(pal::bg);
    char b1[64], b2[64];
    static const uint8_t zero[LINEA_PATCH_BYTES] = {};
    const uint8_t* pt = t.lineaSet ? t.linea : zero;
    if (!lineaRomReady() || !t.lineaSet) {
        lineaLcd(c, 60, 140, 520, 70, !lineaRomReady() ? "LINEA: NO ROM" : "LINEA: NO PATCH BANK",
                 !lineaRomReady() ? lineaRomStatus() : "PUT A D-50 .SYX IN ROMS/LINEA");
    } else if (page_ == PG_PLAY) {
        const int bank = bk.find(t.lineaBank);
        if (t.lineaSlot >= 0 && t.lineaBank[0]) snprintf(b1, sizeof b1, "%s %d-%d", t.lineaBank, bank + 1, t.lineaSlot + 1);
        else snprintf(b1, sizeof b1, "EDITED");
        ln_.drawPlay(c, pt, e ? e->lineaCore(track_) : nullptr, t.lineaCut, t.lineaRes, b1, dt_);
    } else if (page_ == PG_BANKS) {
        const int nb = bk.count();
        const char* items[LineaBanks::MAX_BANKS + 1];
        for (int i = 0; i < nb; i++) items[i] = bk.name(i);
        items[nb] = "+ IMPORT SYSEX";
        for (int i = 0; i <= nb && i < 11; i++) {  // banki (+ IMPORT SYSEX)
            const float y = 60 + i * 26;
            const bool sel = i == lnBank_;
            if (sel) c.fillRoundRect(16, y - 4, 214, 24, 4, lnFocus_ == 0 ? rgb(52, 40, 22) : rgb(30, 26, 20));
            drawTextFit(c, 24, y + 1, items[i], 10, 1.3f, i == nb ? pal::green : (sel ? AMBER_C : pal::tan), 200);
        }
        c.line(240, 56, 240, 330, 1.f, pal::steel);
        if (lnBank_ == nb) {
            lnImpN_ = bk.importList(lnImp_, 32);
            drawText(c, 256, 60, "SYSEX FILES (ROMS/LINEA, SYSEX)", 8, 1.1f, pal::white);
            if (!lnImpN_) drawText(c, 256, 90, "NO .SYX FILES FOUND", 8, 1.1f, pal::dim);
            for (int i = 0; i < lnImpN_ && i < 9; i++) {
                if (i == lnSlot_ && lnFocus_ == 1) c.fillRoundRect(252, 80 + i * 26, 370, 22, 4, rgb(40, 32, 18));
                drawText(c, 260, 86 + i * 26, lnImp_[i], 9, 1.2f, i == lnSlot_ ? AMBER_C : rgb(170, 170, 170));
            }
            drawText(c, 256, 330, "A = IMPORT AS A BANK", 7, 1.f, pal::dim);
        } else {
            const int np = bk.patches(lnBank_);
            if (lnSlot_ >= np) lnSlot_ = np > 0 ? np - 1 : 0;
            if (lnSlot_ < lnFirst_) lnFirst_ = lnSlot_;
            if (lnSlot_ >= lnFirst_ + 9) lnFirst_ = lnSlot_ - 8;
            for (int i = lnFirst_; i < np && i < lnFirst_ + 9; i++) {
                const float y = 60 + (i - lnFirst_) * 28;
                const bool sel = i == lnSlot_ && lnFocus_ == 1;
                if (sel) c.fillRoundRect(252, y - 4, 370, 24, 4, rgb(40, 32, 18));
                snprintf(b1, sizeof b1, "%d-%d", i / 8 + 1, i % 8 + 1);
                drawText(c, 260, y + 2, b1, 8, 1.1f, pal::dim);
                drawText(c, 300, y + 1, bk.patchName(lnBank_, i), 10, 1.3f, sel ? AMBER_C : rgb(190, 190, 190));
                if (!strcmp(bk.name(lnBank_), t.lineaBank) && i == t.lineaSlot) c.fillCircle(612, y + 6, 3, pal::green);
            }
            snprintf(b1, sizeof b1, "%d PATCHES", np);
            drawText(c, 620, 316, b1, 7, 1.f, pal::dim, ALIGN_RIGHT);
        }
        if (lnConfirm_) drawText(c, 320, 336, lnFocus_ == 0 ? "B AGAIN = DELETE THIS BANK" : "B AGAIN = DELETE (USER ONLY)", 8, 1.1f, pal::red, ALIGN_CENTER);
    } else {  // strona edycji w stylu D-50
        const Pg& pg = kPages[page_ - 1];
        if (lnRow_ >= pg.n) lnRow_ = 0;
        const bool partialPage = pg.rows[lnRow_][0].blk == BP;
        const char* tn = lnTone_ ? "LOWER" : "UPPER";
        if (partialPage) snprintf(b1, sizeof b1, "%s %d  %s   ROW %d/%d", tn, lnPart_ + 1, pg.name, lnRow_ + 1, pg.n);
        else snprintf(b1, sizeof b1, "%s  %s   ROW %d/%d", tn, pg.name, lnRow_ + 1, pg.n);
        char vals[4][24];
        b2[0] = 0;
        for (int i = 0; i < 4; i++) {
            const LP& lp = pg.rows[lnRow_][i];
            vals[i][0] = 0;
            if (!lp.name[0]) continue;
            fmtVal(lp, pt[blkIndex(lp, lnTone_, lnPart_) * 64 + lp.off], vals[i], sizeof vals[i]);
        }
        const LP& l0 = pg.rows[lnRow_][0];
        char nm[19];
        lineaPatchName(pt, nm);
        snprintf(b2, sizeof b2, "%s = %s", l0.name, vals[0]);
        lineaLcd(c, 20, 56, 600, 64, b1, b2);
        drawText(c, 620, 128, nm, 8, 1.1f, rgb(150, 110, 40), ALIGN_RIGHT);
        // wiersze strony (lista) i grafika
        for (int r = 0; r < pg.n; r++) {
            char line[96] = "";
            for (int i = 0; i < 4; i++) {
                const LP& lp = pg.rows[r][i];
                if (!lp.name[0]) continue;
                char v[24];
                fmtVal(lp, pt[blkIndex(lp, lnTone_, lnPart_) * 64 + lp.off], v, sizeof v);
                char cell[40];
                snprintf(cell, sizeof cell, "%s %s   ", lp.name, v);
                strncat(line, cell, sizeof line - strlen(line) - 1);
            }
            drawText(c, 24, 146 + r * 16, line, 7, 1.f, r == lnRow_ ? AMBER_C : rgb(110, 110, 110));
        }
        const int pb = (lnTone_ ? 3 : 0) + lnPart_;
        const uint8_t* q = pt + pb * 64;
        if (page_ == 3) drawEnv(c, 360, 236, 250, 70, q + 22, q + 27, q[30], q[31], AMBER_C);       // TVF
        if (page_ == 4) drawEnv(c, 360, 236, 250, 70, q + 39, q + 44, q[47], q[48], AMBER_C);       // TVA
        if (page_ == 1) {  // struktura: bloki partiali i ring
            for (int tone = 0; tone < 2; tone++) {
                const int s = lineaStructure(pt, tone);
                static const char* st[7][2] = {{"S", "S"}, {"S", "S"}, {"P", "S"}, {"P", "S"}, {"S", "P"}, {"P", "P"}, {"P", "P"}};
                const bool ring = s == 2 || s == 4 || s == 5 || s == 7;
                const float y = 230 + tone * 50, x = 380;
                drawText(c, x - 70, y + 8, tone ? "LOWER" : "UPPER", 8, 1.1f, rgb(150, 110, 40));
                for (int k = 0; k < 2; k++) {
                    c.strokeRoundRect(x + k * 120, y, 70, 30, 4, 1.4f, AMBER_C);
                    drawText(c, x + 35 + k * 120, y + 9, st[s - 1][k][0] == 'P' ? "PCM" : "SYNTH", 9, 1.2f, AMBER_C, ALIGN_CENTER);
                }
                c.line(x + 70, y + 15, x + 120, y + 15, 1.4f, AMBER_C);
                drawText(c, x + 95, y - 2, ring ? "RING" : "+", 8, 1.1f, ring ? pal::red : AMBER_C, ALIGN_CENTER);
            }
        }
    }
    c.fillRect(0, 0, 640, 44, pal::bg);
    screenHeader(c, ICON_KEYS, tc, "LINEA", t.name, tc, tick / TICKS_PER_BAR + 1, (tick % TICKS_PER_BAR) / PPQ + 1, playing, p.bpm);
    c.fillRect(0, 338, 640, 142, pal::bg);
    static const char* kPg[8] = {"PLAY", "COMMON", "WG", "TVF", "TVA", "P-ENV / LFO", "EFFECTS", "BANKS"};
    drawText(c, 20, 342, kPg[page_ < 8 ? page_ : 0], 13, 1.5f, pal::white);
    for (int i = 0; i < 8; i++) {
        const float x = 24 + i * 12;
        if (i == page_) c.fillCircle(x, 366, 2.6f, pal::white);
        else c.circle(x, 366, 2.2f, 1.f, rgb(90, 90, 90));
    }
    drawText(c, 24 + 8 * 12 + 4, 362, "SELECT = NEXT PAGE", 7, 1.f, rgb(80, 80, 80));
    const char* hint = page_ == PG_PLAY ? "D-PAD = PATCH / BANK   ABXY = PLAY   L1/R1 = OCTAVE"
                     : page_ == PG_BANKS ? "A = LOAD   X = SAVE TO USER   B = DELETE   L/R = COLUMN"
                                         : "UP/DOWN = ROW   L1/R1 = UPPER/LOWER   L3 = PARTIAL";
    drawText(c, 20, 384, hint, 7, 1.f, rgb(80, 80, 80));
    if (t.octave) { snprintf(b1, sizeof b1, "OCT %+d", t.octave); drawText(c, 380, 342, b1, 9, 1.2f, pal::white, ALIGN_RIGHT); }
    const char* labels[4] = {"", "", "", ""};
    char v[4][24] = {"", "", "", ""};
    const char* vals[4] = {v[0], v[1], v[2], v[3]};
    float pos[4] = {0, 0, 0, 0};
    if (page_ == PG_PLAY && t.lineaSet) {
        labels[0] = "PARTIAL BAL"; labels[1] = "TONE BAL"; labels[2] = "CUTOFF"; labels[3] = "RESONANCE";
        const int pbal = (pt[2 * 64 + 47] + pt[5 * 64 + 47]) / 2, tbal = pt[6 * 64 + 33];
        snprintf(v[0], 24, "%d", pbal); snprintf(v[1], 24, "%d", tbal);
        snprintf(v[2], 24, "%+d", t.lineaCut); snprintf(v[3], 24, "%+d", t.lineaRes);
        pos[0] = pbal / 100.f; pos[1] = tbal / 100.f; pos[2] = (t.lineaCut + 100) / 200.f; pos[3] = (t.lineaRes + 30) / 60.f;
    } else if (page_ >= 1 && page_ <= 6 && t.lineaSet) {
        const Pg& pg = kPages[page_ - 1];
        for (int i = 0; i < 4; i++) {
            const LP& lp = pg.rows[lnRow_ < pg.n ? lnRow_ : 0][i];
            if (!lp.name[0]) continue;
            labels[i] = lp.name;
            const int x = pt[blkIndex(lp, lnTone_, lnPart_) * 64 + lp.off];
            fmtVal(lp, x, v[i], sizeof v[i]);
            pos[i] = lp.max ? x / (float)lp.max : 0.f;
        }
    }
    encoderBar(c, labels, vals, pos);
}

}  // namespace dp

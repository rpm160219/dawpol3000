#include "synth/wavetable_synth.h"
#include "seq/project.h"
#include "synth/taffy_dsp.h"
#include "synth/pew_dsp.h"
#include "synth/elg_dsp.h"
#include "synth/eva_dsp.h"
#include "synth/mbira_dsp.h"
#include "synth/reactor_dsp.h"
#include "synth/oxbow_dsp.h"
#include "synth/oxbow_plus_dsp.h"
#include "synth/sid_dsp_wrap.h"
#include "synth/monolit_dsp.h"
#include "synth/vapora_dsp.h"
#include "synth/supersaw_dsp.h"
#include "synth/mixtur_dsp.h"
#include "synth/brass_dsp.h"
#include "synth/hollowsaint_dsp.h"
#include "synth/bloodmoon_dsp.h"
#include "synth/blot_dsp.h"
#include "synth/aether_dsp.h"
#include "synth/arc_dsp.h"
#include "synth/dddd_dsp.h"
#include "synth/juni_dsp.h"
#include "synth/outatime_dsp.h"
#include "synth/gin_dsp.h"
#include "synth/parallax_dsp.h"
#include "synth/lounge_dsp.h"
#include "synth/felt_dsp.h"
#include "synth/grand_dsp.h"
#include <math.h>
#include <string.h>

// Oryginalne DSP op1r (C, liczby calkowite). Naglowek definiuje statyczne tablice — tylko w tym pliku.
// WT_DSP_HEADER mozna podmienic w tescie zgodnosci (oryginalna tabela 88,2 kHz).
#ifndef WT_DSP_HEADER
#define WT_DSP_HEADER "../../third_party/op1r_wavetable/wavetable_dsp.h"
#endif
#include WT_DSP_HEADER

namespace dp {

static_assert(sizeof(WavetableState) <= 96, "WavetableState nie miesci sie w buforze glosu");
static inline WavetableState* ST(unsigned char (*st)[96], int i) { return (WavetableState*)st[i]; }

int32_t wavetableShape(int32_t k0, uint32_t ph) { return wavetable_dsp_shape(k0, ph); }

// ---- Tryb rozszerzony ----
// Stan glosu: fazy OSC 1, OSC 2, sub; stopnie ladderu; kontrolki poprzedniego bloku (rampy jak w oryginale).
struct WtExtState {
    uint32_t ph1, ph2, phs;
    int32_t y[4];
    int32_t control[10];
    int ready;
};
static_assert(sizeof(WtExtState) <= 128, "WtExtState nie miesci sie w buforze glosu");
static inline WtExtState* SX(unsigned char (*sx)[128], int i) { return (WtExtState*)sx[i]; }

static inline void wtExtOn(WtExtState* s) {
    s->ready = 0;
    s->ph1 = 0;
    s->ph2 = 0x55555555u;  // jak oryginal: drugi oscylator przesuniety o 1/3 okresu
    s->phs = 0;
    s->y[0] = s->y[1] = s->y[2] = s->y[3] = 0;
}

#ifndef WT_EXT_G_LP12
#define WT_EXT_G_LP12 22938  // 0.70
#endif
#ifndef WT_EXT_G_BP
#define WT_EXT_G_BP 16384    // 0.50
#endif
#ifndef WT_EXT_G_HP
#define WT_EXT_G_HP 16384    // 0.50
#endif

static inline const int16_t* waveA(int32_t pos) { return wt_waves[pos >> 15]; }
static inline const int16_t* waveB(int32_t pos) { return wt_waves[(pos >> 15) + ((pos >> 15) < WT_WAVES - 1)]; }

// Jedna probka: arytmetyka jak wavetable_sample (oryginal), z niezaleznymi oscylatorami i wyjsciem wg trybu filtra.
static inline int16_t wtExtSample(WtExtState* s, uint32_t inc1, uint32_t inc2, const int32_t* c, int mode) {
    const int32_t pos1 = c[0], pos2 = c[1], l1 = c[2], l2 = c[3], ls = c[4];
    const int32_t g = c[5], k = c[6], drive = c[7], makeup = c[8], level = c[9];
    s->ph1 += inc1;
    s->ph2 += inc2;
    s->phs += inc1 >> 1;  // sub: oktawe nizej od OSC 1
    const int32_t o1 = wavetable_osc(waveA(pos1), waveB(pos1), pos1 & 0x7FFF, s->ph1);
    const int32_t o2 = wavetable_osc(waveA(pos2), waveB(pos2), pos2 & 0x7FFF, s->ph2);
    const int32_t os = wavetable_read(wt_waves[0], s->phs);
    int32_t x = (((o1 * l1) >> 15) + ((o2 * l2) >> 15) + ((os * ls) >> 15)) >> 2;  // zapas jak w oryginale
    const int32_t wsum = l1 + l2 + ls;                     // suma poziomow ponad 2 (OSC1+OSC2 jak w oryginale):
    if (wsum > 65534) x = (int32_t)((int64_t)x * 65534 / wsum);  // normalizacja, zeby SUB nie przesterowywal wejscia
    x = wavetable_soft((x * drive) >> 12);
    int32_t* y = s->y;
    const int32_t u = wavetable_soft(x - ((k * y[3]) >> 12));
    y[0] += (g * (u - y[0])) >> 15;
    y[1] += (g * (y[0] - y[1])) >> 15;
    y[2] += (g * (y[1] - y[2])) >> 15;
    y[3] += (g * (y[2] - y[3])) >> 15;
    int32_t sel;
    switch (mode) {  // wyjscia ladderu (mieszanki stopni jak w filtrach wielotrybowych)
    // skale wyjsc dobrane pomiarem tak, by nasycenie soft clipa bylo jak w LP24 (oryginal)
    // + kompensacja rezonansu: przy wysokim RES wczesniejsze stopnie ladderu niosa wiecej energii szczytu
    case 1: sel = (((y[1] * WT_EXT_G_LP12) >> 15) * 4096) / (4096 + ((k * 174) >> 10)); break;            // LP12
    case 2: sel = ((((2 * (y[0] - y[1])) * WT_EXT_G_BP) >> 15) * 4096) / (4096 + ((k * 461) >> 10)); break; // BP
    case 3: sel = ((u - 2 * y[0] + y[1]) * WT_EXT_G_HP) >> 15; break;                                   // HP12
    default: sel = y[3]; break;                      // LP24 (oryginal)
    }
    // czyste wyrownanie glosnosci trybow (za soft clipem): LP12 x1.35, BP x2, HP x1.8; LP24 bez zmian
    static const int32_t kModeGain[4] = {4096, 5530, 8192, 7373};
    const int32_t lv = mode > 0 && mode < 4 ? (level * kModeGain[mode]) >> 12 : level;
    int32_t sample = (wavetable_soft((sel * makeup) >> 12) * lv) >> 12;
    return sample > 32767 ? 32767 : sample < -32768 ? -32768 : (int16_t)sample;
}

void WavetableSynth::init(float outRate) {
    outRate_ = outRate;
    voiceRate_ = outRate * 2.f;
    for (int i = 0; i < VOICES; i++) {
        v_[i] = Voice();
        wavetable_dsp_on(ST(st_, i));
        v_[i].statP = rnd();  // AGE: staly charakter kazdego glosu (jak rozrzut elementow w analogu)
        v_[i].statC = rnd();
    }
    nStack_ = 0;
    lastPitch_ = -1;
    if (!pew_) pew_ = pewCreate();
    if (!elg_) elg_ = elgCreate();
    if (!eva_) eva_ = evaCreate();
    if (!mbira_) mbira_ = mbiraCreate();
    if (!reactor_) reactor_ = reactorCreate();
    if (!oxbow_) oxbow_ = oxbowCreate();
    if (!oxp_) oxp_ = oxbowPlusCreate();
    if (!sid_) sid_ = sidCreate();
    if (!mnl_) mnl_ = monolitCreate();
    if (!vpr_) vpr_ = vaporaCreate();
    if (!ssw_) ssw_ = supersawCreate();
    if (!mxt_) mxt_ = mixturCreate();
    if (!brs_) brs_ = brassCreate();
    if (!hls_) hls_ = hollowCreate();
    if (!bld_) bld_ = bloodCreate();
    if (!blt_) blt_ = blotCreate();
    if (!aet_) aet_ = aetherCreate();
    if (!arc_) arc_ = arcCreate();
    if (!ddd_) ddd_ = ddddCreate();
    if (!jun_) jun_ = juniCreate();
    if (!ota_) ota_ = outatimeCreate();
    if (!gin_) gin_ = ginCreate();
    if (!prx_) prx_ = parallaxCreate();
    if (!lng_) lng_ = loungeCreate();
    if (!flt_) flt_ = feltCreate();
    if (!grd_) grd_ = grandCreate();
    if (!vel_) vel_ = velourCreate(outRate);  // VELOUR: natywnie w czestotliwosci wyjscia
    // FIR z harness/audio_harness.c (op1r): okno Blackmana, sinc polpasmowy
    for (int n = 0; n < TAPS; n++) {
        double x = n - (TAPS - 1) / 2.0;
        double w = 0.42 - 0.5 * cos(2 * M_PI * n / (TAPS - 1)) + 0.08 * cos(4 * M_PI * n / (TAPS - 1));
        h_[n] = (x == 0 ? 0.5 : sin(M_PI * x / 2) / (M_PI * x)) * w;
    }
    for (int n = 0; n < 2 * TAPS; n++) z_[n] = 0;
    zp_ = 0;
    fifoPos_ = BLOCK / 2;
    volK_ = 1.f - expf(-1.f / (0.01f * outRate));
    fade_ = 1.f / (0.002f * voiceRate_);  // ~2 ms
    gateK_ = 1.f / (0.003f * voiceRate_); // ~3 ms
    for (int i = 0; i < 4; i++) modKnob_[i].store(knobs_[i]);
    modKnob_[4].store(wave2_);
    for (int i = 0; i < VOICES; i++) wtExtOn(SX(sx_, i));
    setEnvelope(0.005f, 0.4f, 0.75f, 0.3f);
    setFilterEnv(0.005f, 0.3f, 0.f, 0.3f, 0.f);
}

float WavetableSynth::rnd() {
    ageSeed_ = ageSeed_ * 1664525u + 1013904223u;
    return (int32_t)(ageSeed_ >> 8) / 8388608.f - 1.f;
}

void WavetableSynth::setVoice(bool mono, float glide, float unison, float age) {
    if (mono != mono_) { allOff(); nStack_ = 0; }
    mono_ = mono;
    glide_ = glide < 0 ? 0 : glide;
    glideK_ = glide_ > 0 ? 1.f - expf(-1.f * BLOCK / (glide_ * voiceRate_)) : 1.f;
    unison_ = unison < 0 ? 0 : unison;
    age_ = age < 0 ? 0 : (age > 1 ? 1 : age);
}

static inline uint32_t incOf(double pitch, float vr) {
    return (uint32_t)(int32_t)lrint(4294967296.0 * 440.0 * pow(2.0, (pitch - 69) / 12.0) / vr);
}

void WavetableSynth::setPitchTarget(Voice& v, int pitch) {
    v.pitch = pitch;
    v.pitchTgt = (float)pitch;
    if (glide_ <= 0) { v.pitchCur = (float)pitch; v.inc = incOf(pitch + octSemi_, voiceRate_); }
}

// MONO: stos trzymanych nut (ostatnia ma priorytet), legato bez ponownego startu obwiedni, unison 6 glosow.
void WavetableSynth::monoOn(int pitch, float vel) {
    for (int i = 0; i < nStack_; i++) if (stack_[i] == pitch) { for (int j = i; j < nStack_ - 1; j++) stack_[j] = stack_[j + 1]; nStack_--; break; }
    if (nStack_ < 16) stack_[nStack_++] = pitch;
    const int n = unison_ > 0 ? VOICES : 1;
    const float from = glide_ > 0 && lastPitch_ >= 0 ? (float)lastPitch_ : (float)pitch;  // wspolny start glide stosu
    bool held = false;
    for (int i = 0; i < n; i++) if (v_[i].stage == ATTACK || v_[i].stage == DECAY || v_[i].stage == SUSTAIN) held = true;
    for (int i = 0; i < n; i++) {
        float det = n > 1 ? unison_ * (2.f * i / (n - 1) - 1.f) : 0.f;  // symetrycznie -unison..+unison
        Voice& v = v_[i];
        if (held && v.stage != OFF && v.stage != RELEASE && v.stage != FADE) { setPitchTarget(v, pitch); v.det = det; continue; }
        v.age = ++age_counter_;
        if (v.stage != OFF && v.env > 1e-3f) { v.stage = FADE; v.pendPitch = pitch; v.pendVel = vel; v.pendDet = det; v.pendFrom = from; }
        else startVoice(i, pitch, vel, det, from);
    }
    lastPitch_ = pitch;
}

void WavetableSynth::monoOff(int pitch) {
    for (int i = 0; i < nStack_; i++) if (stack_[i] == pitch) { for (int j = i; j < nStack_ - 1; j++) stack_[j] = stack_[j + 1]; nStack_--; break; }
    const int n = unison_ > 0 ? VOICES : 1;
    if (nStack_ > 0) {  // wroc legato do ostatniej trzymanej nuty
        int p = stack_[nStack_ - 1];
        for (int i = 0; i < n; i++) if (v_[i].stage != OFF && v_[i].stage != RELEASE) setPitchTarget(v_[i], p);
        lastPitch_ = p;
        return;
    }
    for (int i = 0; i < VOICES; i++) {
        Voice& v = v_[i];
        if (v.stage == FADE) { v.pendPitch = -1; continue; }
        if (v.stage != OFF && v.stage != RELEASE) {
            v.stage = RELEASE;
            v.fstage = RELEASE; v.mstage = RELEASE;
            if (rel_k_ >= 1.f) { v.env = 0; v.stage = OFF; }
        }
    }
}

void WavetableSynth::setFilterEnv(float a, float d, float s, float r, float amount) {
    const float br = voiceRate_ / BLOCK;  // bloki na sekunde
    fA_ = a > 0 ? 1.f / (a * br) : 2.f;
    fD_ = d > 0 ? 1.f - expf(-5.f / (d * br)) : 1.f;
    fS_ = s < 0 ? 0 : (s > 1 ? 1 : s);
    fR_ = r > 0 ? 1.f - expf(-5.f / (r * br)) : 1.f;
    fAmt_ = amount < -1 ? -1 : (amount > 1 ? 1 : amount);
}

void WavetableSynth::setKind(int k) {
    if (k == kind_) return;
    if (kind_ == 23) ginAllOff(gin_);  // GIN: zmiana silnika wycisza bebny
    if (kind_ == 28) velourAllOff(vel_);
    allOff();
    kind_ = k;
    if (k == 1 && tfBase_ < 0) tfBase_ = taffyAllocVoices(VOICES);
}

void WavetableSynth::setExt(int16_t wave2, int semi1, int semi2, float cents1, float cents2, float lvl1, float lvl2,
                            float sub, int fmode, float drive) {
    wave2_ = wave2 < 0 ? 0 : wave2;
    semi_[0] = semi1;
    semi_[1] = semi2;
    cents_[0] = cents1;
    cents_[1] = cents2;
    lvl_[0] = lvl1;
    lvl_[1] = lvl2;
    sub_ = sub;
    fmode_ = fmode;
    drive_ = drive;
}

// SUPERSAW: filtr DAW-POL za oryginalnym silnikiem — SVF z rownaniami trapezowymi (Simper / Cytomic), LP24 = dwa stopnie
// (Butterworth przy RES 0, rezonans w drugim). Czestotliwosc jak w odczycie galki: 30 Hz .. 16 kHz wykladniczo.
// Sterowanie na blok z wygladzeniem (~3 ms), stan liczony zawsze (bez skoku przy otwieraniu z pozycji "otwarty").
void WavetableSynth::ssFilter(int vi, const int16_t* in, float* out, float cut, float res) {
    SsFilt& f = ssf_[vi];
    if (!f.ready) { f = SsFilt{0, 0, 0, 0, cut, res, true}; }
    f.cut += (cut - f.cut) * 0.35f;
    f.res += (res - f.res) * 0.35f;
    const float hz = 30.f * powf(16000.f / 30.f, f.cut < 0 ? 0 : (f.cut > 1 ? 1 : f.cut));
    const float g = tanf((float)M_PI * hz / voiceRate_);
    const float r = f.res < 0 ? 0 : (f.res > 1 ? 1 : f.res);
    const float k2 = ssMode_ == 0 ? 0.765f * (1.f - r) + 0.1f * r : 1.414f * (1.f - r) + 0.1f * r;  // RES max: Q 10
    const float inG = ssMode_ == 2 ? 1.f : 1.f - (ssMode_ == 0 ? 0.4f : 0.55f) * r;  // kompensacja glosnosci rezonansu
    const float k1 = 1.848f;
    const float a1 = 1.f / (1.f + g * (g + k2)), a2 = g * a1, a3 = g * a2;
    const float b1 = 1.f / (1.f + g * (g + k1)), b2 = g * b1, b3 = g * b2;
    float s1 = f.s1, s2 = f.s2, s3 = f.s3, s4 = f.s4;
    for (int i = 0; i < BLOCK; i++) {
        float x = (float)in[i] * inG;
        if (ssMode_ == 0) {  // pierwszy stopien LP (bez rezonansu)
            const float v3 = x - s4, v1 = b1 * s3 + b2 * v3, v2 = s4 + b2 * s3 + b3 * v3;
            s3 = 2.f * v1 - s3; s4 = 2.f * v2 - s4;
            x = v2;
        }
        const float v3 = x - s2, v1 = a1 * s1 + a2 * v3, v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2.f * v1 - s1; s2 = 2.f * v2 - s2;
        float y = ssMode_ <= 1 ? v2 : (ssMode_ == 2 ? v1 * k2 * (1.f + 2.f * r) : x - k2 * v1 - v2);  // BP: waski przy RES — podbicie
        y = y > 60000.f ? 60000.f : (y < -60000.f ? -60000.f : y);
        out[i] = y;
    }
    f.s1 = s1; f.s2 = s2; f.s3 = s3; f.s4 = s4;
}

// Jeden blok glosu w trybie rozszerzonym. vk = galki po modulacji (0 fala OSC 1, 2 CUTOFF, 3 RES).
void WavetableSynth::renderExtVoice(int vi, int16_t* out, int16_t* vk, const float* mod, uint32_t inc, int vel) {
    WtExtState* s = SX(sx_, vi);
    auto clampK = [](float v) { return v < 0 ? 0 : (v > 32767 ? 32767 : (int32_t)v); };
    const int32_t k0 = clampK(vk[0] + mod[DEST_W1] * 32767.f);  // vk[0] zawiera juz WAVE (obie fale)
    const int32_t k0b = clampK(wave2_ + (mod[DEST_WAVE] + mod[DEST_W2]) * 32767.f);
    const int32_t k2 = vk[2], k3 = vk[3];
    // wysokosc: interwal + rozstrojenie (DTUN skaluje rozstrojenie), P1/P2 = +-1 oktawa przy glebokosci 1
    const float dsc = 1.f + mod[DEST_DETUNE];
    const double r1 = pow(2.0, (semi_[0] + cents_[0] * dsc / 100.0) / 12.0 + mod[DEST_P1]);
    const double r2 = pow(2.0, (semi_[1] + cents_[1] * dsc / 100.0) / 12.0 + mod[DEST_P2]);
    const uint32_t inc1 = (uint32_t)(inc * r1), inc2 = (uint32_t)(inc * r2);
    auto lv = [](float v) { return (int32_t)((v < 0 ? 0 : (v > 1 ? 1 : v)) * 32767.f); };
    // kontrolki jak w wavetable_dsp_render (cutoff z tabeli, rezonans, drive, makeup, kompensacja poziomu)
    const int32_t ci = k2 * WT_CUTOFFS;
    const int32_t g0 = wt_cutoff[ci >> 15];
    const int32_t g = g0 + (((wt_cutoff[(ci >> 15) + 1] - g0) * (ci & 0x7FFF)) >> 15);
    const int32_t kk = (k3 * 15974) >> 15;
    const int32_t drive = 4096 + (k3 * 3 >> 3) + (int32_t)(drive_ * 12288.f);  // + do 3x wiecej
    int32_t makeup = (((((3072 + (kk >> 2)) * vel) >> 7) * 4755 >> 12) * 8192) >> 13;
    if (makeup > 32767) makeup = 32767;
    const int32_t level = wt_level_at(k0, k2, k3);
    const int32_t target[10] = {k0 * (WT_WAVES - 1), k0b * (WT_WAVES - 1), lv(lvl_[0] + mod[DEST_LV1]),
                                lv(lvl_[1] + mod[DEST_LV2]), lv(sub_), g, kk, drive, makeup, level};
    int32_t control[10], delta[10];
    for (int j = 0; j < 10; j++) {
        const int32_t prev = s->ready ? s->control[j] : target[j];
        control[j] = prev * BLOCK;
        delta[j] = target[j] - prev;
        s->control[j] = target[j];
    }
    s->ready = 1;
    int32_t c[10];
    for (int i = 0; i < BLOCK; i++) {
        for (int j = 0; j < 10; j++) c[j] = control[j] >> 7;
        out[i] = wtExtSample(s, inc1, inc2, c, fmode_);
        for (int j = 0; j < 10; j++) control[j] += delta[j];
    }
    modKnob_[4].store((int16_t)k0b, std::memory_order_relaxed);
    vk[0] = (int16_t)k0;  // ekran: fala OSC 1 po modulacji W1 / W1+2
}

void WavetableSynth::setModEnv(float a, float d, float s, float r, float amount, int dest) {
    const float br = voiceRate_ / BLOCK;
    mA_ = a > 0 ? 1.f / (a * br) : 2.f;
    mD_ = d > 0 ? 1.f - expf(-5.f / (d * br)) : 1.f;
    mS_ = s < 0 ? 0 : (s > 1 ? 1 : s);
    mR_ = r > 0 ? 1.f - expf(-5.f / (r * br)) : 1.f;
    mAmt_ = amount < -1 ? -1 : (amount > 1 ? 1 : amount);
    mDest_ = dest;
}

void WavetableSynth::setLfo(int i, int shape, float rate, float depth, int dest, bool sync, float beats) {
    if (i < 0 || i > 1) return;
    lfo_[i].sync = sync;
    lfo_[i].beats = beats > 0 ? beats : 1.f;
    lfo_[i].shape = shape;
    lfo_[i].rate = rate;
    lfo_[i].depth = depth < 0 ? 0 : (depth > 1 ? 1 : depth);
    lfo_[i].dest = dest;
}

// Przesuniecie faz LFO o czas (wywolywane dla kazdej porcji wyjscia — takze w ciszy).
// Synchronizowane LFO przy grajacym utworze: faza = pozycja w utworze / dlugosc cyklu (przypieta do bitu).
// Bez transportu (albo bez sync): swobodny bieg, przy sync z czestotliwoscia wynikajaca z tempa.
void WavetableSynth::advanceLfo(int outFrames) {
    for (auto& l : lfo_) {
        double old = l.ph;
        if (l.sync && tPlaying_) {
            double c = tBeat_ / l.beats;
            l.ph = c - floor(c);
        } else {
            double hz = l.sync ? tBpm_ / 60.0 / l.beats : l.rate;
            l.ph += hz * outFrames / outRate_;
            l.ph -= floor(l.ph);
        }
        if (l.ph < old) {  // nowy cykl: nowa wartosc losowa
            l.seed = l.seed * 1664525u + 1013904223u;
            l.rnd = (int32_t)(l.seed >> 8) / 8388608.f - 1.f;
        }
    }
    if (movSync_) {  // ELG: faza ruchu jak synchronizowane LFO (przy grajacym utworze przypieta do bitu)
        if (tPlaying_) { double c = tBeat_ / movBeats_; movPh_ = c - floor(c); }
        else { movPh_ += tBpm_ / 60.0 / movBeats_ * outFrames / outRate_; movPh_ -= floor(movPh_); }
    }
    if (tPlaying_) tBeat_ += (double)outFrames / outRate_ * tBpm_ / 60.0;
}

// Wartosc LFO -1..1 w biezacej fazie.
float WavetableSynth::lfoValue(LfoState& l) {
    float p = (float)l.ph;
    switch (l.shape) {
    case 0: return sinf(6.2831853f * p);
    case 1: return p < 0.5f ? 4.f * p - 1.f : 3.f - 4.f * p;
    case 2: return 1.f - 2.f * p;
    case 3: return p < 0.5f ? 1.f : -1.f;
    default: return l.rnd;
    }
}

void WavetableSynth::setKnobs(const int16_t k[4]) {
    for (int i = 0; i < 4; i++) knobs_[i] = k[i] < 0 ? 0 : k[i];
}

void WavetableSynth::setEnvelope(float a, float d, float s, float r, float amount) {
    envAmt_ = amount < -1 ? -1 : (amount > 1 ? 1 : amount);
    atk_ = a > 0 ? 1.f / (a * voiceRate_) : 2.f;
    dec_k_ = d > 0 ? 1.f - expf(-5.f / (d * voiceRate_)) : 1.f;  // ~ -43 dB po czasie d
    sus_ = s < 0 ? 0 : (s > 1 ? 1 : s);
    rel_k_ = r > 0 ? 1.f - expf(-5.f / (r * voiceRate_)) : 1.f;
}

void WavetableSynth::noteOn(int pitch, float vel) {
    if (kind_ == 23) { ginNoteOn(gin_, pitch, (int)lrintf(vel * 127.f)); return; }  // GIN: C2..B3 = 24 dzwieki zestawu
    if (kind_ == 28) { velourNoteOn(vel_, pitch + octSemi_); return; }  // VELOUR: kazdy klawisz (bez dynamiki, jak oryginal)
    if (mono_) { monoOn(pitch, vel); return; }
    int slot = -1;
    for (int i = 0; i < VOICES; i++)
        if (v_[i].stage != OFF && (v_[i].stage == FADE ? v_[i].pendPitch == pitch : v_[i].pitch == pitch)) { slot = i; break; }
    if (slot < 0) for (int i = 0; i < VOICES; i++) if (v_[i].stage == OFF) { slot = i; break; }
    if (slot < 0) {  // kradziez: najpierw najcichszy w release, potem najstarszy
        float best = 2.f;
        for (int i = 0; i < VOICES; i++) if (v_[i].stage == RELEASE && v_[i].env < best) { best = v_[i].env; slot = i; }
        if (slot < 0) { slot = 0; for (int i = 1; i < VOICES; i++) if (v_[i].age < v_[slot].age) slot = i; }
    }
    Voice& v = v_[slot];
    v.age = ++age_counter_;
    if (v.stage != OFF && v.env > 1e-3f) {  // glos jeszcze brzmi: najpierw wycisz (~2 ms), potem nowa nuta
        v.stage = FADE;
        v.pendPitch = pitch;
        v.pendVel = vel;
        v.pendDet = 0;
        v.pendFrom = -1;
        return;
    }
    startVoice(slot, pitch, vel);
}

void WavetableSynth::startVoice(int slot, int pitch, float vel, float det, float from) {
    Voice& v = v_[slot];
    v.pitch = pitch;
    v.pendPitch = -1;
    v.det = det;
    v.pitchTgt = (float)pitch;  // portamento: start od ostatnio granej wysokosci
    if (from >= 0) v.pitchCur = from;  // MONO/unison: wspolny punkt startu
    else v.pitchCur = glide_ > 0 && lastPitch_ >= 0 ? (float)lastPitch_ : (float)pitch;
    if (from < 0) lastPitch_ = pitch;
    v.vel = (int)lrintf(vel * 127.f);
    if (v.vel < 1) v.vel = 1;
    if (v.vel > 127) v.vel = 127;
    // przyrost fazy jak ENGINE_PITCH: 2^32 * f / czestotliwosc glosu
    v.inc = (uint32_t)(int32_t)lrint(4294967296.0 * 440.0 * pow(2.0, (pitch + octSemi_ - 69) / 12.0) / voiceRate_);
    v.stage = ATTACK;
    v.env = atk_ >= 1.f ? 1.f : 0.f;
    if (v.env >= 1.f) v.stage = DECAY;
    v.fstage = ATTACK;
    v.fenv = 0;
    v.mstage = ATTACK;
    v.menv = 0;
    v.mvolPrev = mAmt_ != 0.f && mDest_ == DEST_VOLUME ? (mAmt_ > 0 ? 1.f - mAmt_ : 1.f) : 1.f;
    v.gate = 0;
    wavetable_dsp_on(ST(st_, slot));  // engine_note_on
    wtExtOn(SX(sx_, slot));
    if (kind_ == 1) taffyNoteOn(tfBase_ + slot);
    else if (kind_ == 3) elgNoteOn(elg_, slot);
    else if (kind_ == 5) mbiraNoteOn(mbira_, slot, knobs_, v.vel);
    else if (kind_ == 6) reactorNoteOn(reactor_, slot, knobs_, v.vel);
    else if (kind_ == 9) sidNoteOn(sid_, slot, knobs_);
    else if (kind_ == 11) vaporaNoteOn(vpr_, slot, knobs_);
    else if (kind_ == 13) mixturNoteOn(mxt_, slot, knobs_);
    else if (kind_ == 14) brassNoteOn(brs_, slot, knobs_);
    else if (kind_ == 15) hollowNoteOn(hls_, slot, knobs_);
    else if (kind_ == 16) bloodNoteOn(bld_, slot, knobs_);
    else if (kind_ == 17) blotNoteOn(blt_, slot, knobs_);
    else if (kind_ == 18) aetherNoteOn(aet_, slot, knobs_);
    else if (kind_ == 19) arcNoteOn(arc_, slot, knobs_);
    else if (kind_ == 20) ddddNoteOn(ddd_, slot, knobs_);
    else if (kind_ == 21) juniNoteOn(jun_, slot, knobs_);
    else if (kind_ == 22) outatimeNoteOn(ota_, slot, knobs_);
    else if (kind_ == 24) parallaxNoteOn(prx_, slot);
    else if (kind_ == 25) loungeNoteOn(lng_, slot);
    else if (kind_ == 26) feltNoteOn(flt_, slot);
    else if (kind_ == 27) grandNoteOn(grd_, slot);
    else if (kind_ == 12) { supersawNoteOn(ssw_, slot, knobs_); ssf_[slot].ready = false; }  // filtr od zera, jak stan oryginalu
    else if (kind_ == 10) {  // MONOLIT: puszczone klawisze najpierw (wybrzmienie CONTOUR), potem nowa nuta
        for (int i = 0; i < VOICES; i++)
            if (i != slot && (v_[i].stage == OFF || v_[i].stage == RELEASE || v_[i].stage == FADE)) monolitNoteOff(mnl_, i);
        monolitNoteOn(mnl_, slot, knobs_);
    }
    else if (kind_ == 8) {  // OXBOW+: jak OXBOW
        for (int i = 0; i < VOICES; i++)
            if (i != slot && (v_[i].stage == OFF || v_[i].stage == RELEASE || v_[i].stage == FADE)) oxbowPlusNoteOff(oxp_, i);
        oxbowPlusNoteOn(oxp_, slot, knobs_, v.vel, &obx_);
    }
    else if (kind_ == 7) {  // OXBOW: najpierw puszczone klawisze (bramka obwiedni filtra; LFO od pierwszego klawisza)
        for (int i = 0; i < VOICES; i++)
            if (i != slot && (v_[i].stage == OFF || v_[i].stage == RELEASE || v_[i].stage == FADE)) oxbowNoteOff(oxbow_, i);
        oxbowNoteOn(oxbow_, slot, knobs_, v.vel);
    }
    else if (kind_ == 4) {  // EVA: najpierw puszczone klawisze (sygnal Quindar przy pierwszym klawiszu)
        for (int i = 0; i < VOICES; i++)
            if (i != slot && (v_[i].stage == OFF || v_[i].stage == RELEASE || v_[i].stage == FADE)) evaNoteOff(eva_, i);
        evaNoteOff(eva_, slot);
        evaNoteOn(eva_, slot, knobs_, v.vel);
    }
    else if (kind_ == 2) {  // PEW: najpierw zwolnij bramki puszczonych klawiszy (pojedyncze wyzwalanie perkusji)
        for (int i = 0; i < VOICES; i++)
            if (i != slot && (v_[i].stage == OFF || v_[i].stage == RELEASE || v_[i].stage == FADE)) pewNoteOff(pew_, i);
        pewNoteOff(pew_, slot);
        pewNoteOn(pew_, slot);
    }
}

void WavetableSynth::noteOff(int pitch) {
    if (kind_ == 23) return;  // GIN: uderzenia wybrzmiewaja same
    if (kind_ == 28) { velourNoteOff(vel_, pitch + octSemi_); return; }
    if (mono_) { monoOff(pitch); return; }
    for (auto& v : v_) {
        if (v.stage == FADE && v.pendPitch == pitch) { v.pendPitch = -1; continue; }  // puszczona przed startem
        if (v.stage != OFF && v.stage != RELEASE && v.stage != FADE && v.pitch == pitch) {
            v.stage = RELEASE;
            v.fstage = RELEASE; v.mstage = RELEASE;
            if (rel_k_ >= 1.f) { v.env = 0; v.stage = OFF; }
        }
    }
}

void WavetableSynth::allOff() {
    if (kind_ == 23) return;  // GIN: stop nie ucina wybrzmienia
    if (kind_ == 28) { velourAllOff(vel_); return; }  // VELOUR: puszczenie klawiszy (SUSTAIN wybrzmiewa)
    for (auto& v : v_) {
        v.pendPitch = -1;
        if (v.stage != OFF) { v.stage = RELEASE; v.fstage = RELEASE; v.mstage = RELEASE; if (rel_k_ >= 1.f) { v.env = 0; v.stage = OFF; } }
    }
}

// Jeden blok glosow (128 probek przy 96 kHz) -> 64 probki wyjscia.
void WavetableSynth::renderBlock() {
    double sum[BLOCK];
    for (int i = 0; i < BLOCK; i++) sum[i] = 0;
    uint32_t pitchBuf[BLOCK];
    int16_t out[BLOCK];
    float fbuf[BLOCK];
    int act = 0, loud = -1;
    float loudEnv = -1;
    // LFO raz na blok (wspolne dla glosow sciezki)
    // cele 0..3 = galki silnika, 4 = PITCH (glebokosc 1 = +-1 oktawa), 5 = VOLUME (tremolo: 1 - glebokosc .. 1)
    float mod[LFO_DESTS] = {};
    float trem = 1.f;
    for (int l = 0; l < 2; l++) {
        // faza przesuwana w render() w czasie rzeczywistym; wygladzenie ~3 ms (2 bloki) zaokragla krawedzie SAW/SQR
        lfo_[l].out += (lfoValue(lfo_[l]) - lfo_[l].out) * 0.4f;
        float val = lfo_[l].out;
        if (lfo_[l].depth <= 0) continue;
        if (lfo_[l].dest == DEST_VOLUME) trem *= 1.f - lfo_[l].depth * 0.5f * (1.f - val);
        else if (lfo_[l].dest < LFO_DESTS) mod[lfo_[l].dest] += val * lfo_[l].depth;
    }

    if (kind_ == 27)  // GRAND: puszczony klawisz — tlumik (od E6 w gore go nie ma)
        for (int vi = 0; vi < VOICES; vi++)
            if (v_[vi].stage == OFF || v_[vi].stage == RELEASE || v_[vi].stage == FADE) grandNoteOff(grd_, vi);
    if (kind_ == 26)  // FELT: puszczony klawisz — tlumik opada na struny
        for (int vi = 0; vi < VOICES; vi++)
            if (v_[vi].stage == OFF || v_[vi].stage == RELEASE || v_[vi].stage == FADE) feltNoteOff(flt_, vi);
    if (kind_ == 22)  // OUTATIME: puszczony klawisz (ekran: samochod zwalnia)
        for (int vi = 0; vi < VOICES; vi++)
            if (v_[vi].stage == OFF || v_[vi].stage == RELEASE || v_[vi].stage == FADE) outatimeNoteOff(ota_, vi);
    if (kind_ == 20)  // DDDD: puszczony klawisz przestaje prowadzic sciane, gdy inne sa trzymane
        for (int vi = 0; vi < VOICES; vi++)
            if (v_[vi].stage == OFF || v_[vi].stage == RELEASE || v_[vi].stage == FADE) ddddNoteOff(ddd_, vi);
    if (kind_ == 19)  // ARC: puszczony klawisz (koniec trzaskow)
        for (int vi = 0; vi < VOICES; vi++)
            if (v_[vi].stage == OFF || v_[vi].stage == RELEASE || v_[vi].stage == FADE) arcNoteOff(arc_, vi);
    if (kind_ == 17)  // BLOT: puszczony klawisz (tylko ekran)
        for (int vi = 0; vi < VOICES; vi++)
            if (v_[vi].stage == OFF || v_[vi].stage == RELEASE || v_[vi].stage == FADE) blotNoteOff(blt_, vi);
    if (kind_ == 16)  // BLOODMOON: puszczony klawisz (wibrato wygasa)
        for (int vi = 0; vi < VOICES; vi++)
            if (v_[vi].stage == OFF || v_[vi].stage == RELEASE || v_[vi].stage == FADE) bloodNoteOff(bld_, vi);
    if (kind_ == 15)  // HOLLOW SAINT: puszczony klawisz (smyczek sie unosi)
        for (int vi = 0; vi < VOICES; vi++)
            if (v_[vi].stage == OFF || v_[vi].stage == RELEASE || v_[vi].stage == FADE) hollowNoteOff(hls_, vi);
    if (kind_ == 14)  // BRASSOPHONE: puszczony klawisz (wibrato wygasa)
        for (int vi = 0; vi < VOICES; vi++)
            if (v_[vi].stage == OFF || v_[vi].stage == RELEASE || v_[vi].stage == FADE) brassNoteOff(brs_, vi);
    if (kind_ == 13)  // MIXTUR: puszczony klawisz (tylko ekran: zegar drabiny staje)
        for (int vi = 0; vi < VOICES; vi++)
            if (v_[vi].stage == OFF || v_[vi].stage == RELEASE || v_[vi].stage == FADE) mixturNoteOff(mxt_, vi);
    if (kind_ == 12)  // SUPERSAW: puszczony klawisz (tylko ekran: lancuch zwalnia)
        for (int vi = 0; vi < VOICES; vi++)
            if (v_[vi].stage == OFF || v_[vi].stage == RELEASE || v_[vi].stage == FADE) supersawNoteOff(ssw_, vi);
    if (kind_ == 11)  // VAPORA: puszczony klawisz (tylko ekran: koniec "trzymania" fali)
        for (int vi = 0; vi < VOICES; vi++)
            if (v_[vi].stage == OFF || v_[vi].stage == RELEASE || v_[vi].stage == FADE) vaporaNoteOff(vpr_, vi);
    if (kind_ == 10)  // MONOLIT: puszczony klawisz = wybrzmienie CONTOUR
        for (int vi = 0; vi < VOICES; vi++)
            if (v_[vi].stage == OFF || v_[vi].stage == RELEASE || v_[vi].stage == FADE) monolitNoteOff(mnl_, vi);
    if (kind_ == 8)  // OXBOW+: puszczony klawisz = wybrzmienie obwiedni filtra
        for (int vi = 0; vi < VOICES; vi++)
            if (v_[vi].stage == OFF || v_[vi].stage == RELEASE || v_[vi].stage == FADE) oxbowPlusNoteOff(oxp_, vi);
    if (kind_ == 7)  // OXBOW: puszczony klawisz = wybrzmienie obwiedni filtra
        for (int vi = 0; vi < VOICES; vi++)
            if (v_[vi].stage == OFF || v_[vi].stage == RELEASE || v_[vi].stage == FADE) oxbowNoteOff(oxbow_, vi);
    if (kind_ == 6)  // REACTOR: puszczony klawisz (bramka glosu jak note_off oryginalu)
        for (int vi = 0; vi < VOICES; vi++)
            if (v_[vi].stage == OFF || v_[vi].stage == RELEASE || v_[vi].stage == FADE) reactorNoteOff(reactor_, vi);
    if (kind_ == 4)  // EVA: puszczony klawisz (koncowy sygnal Quindar gra w wybrzmieniu glosu)
        for (int vi = 0; vi < VOICES; vi++)
            if (v_[vi].stage == OFF || v_[vi].stage == RELEASE || v_[vi].stage == FADE) evaNoteOff(eva_, vi);
    if (kind_ == 2)  // PEW: puszczony klawisz (wybrzmienie, wyciszenie, cisza) zwalnia bramke perkusji
        for (int vi = 0; vi < VOICES; vi++)
            if (v_[vi].stage == OFF || v_[vi].stage == RELEASE || v_[vi].stage == FADE) pewNoteOff(pew_, vi);
    int16_t vk[8];
    int16_t loudK[4] = {knobs_[0], knobs_[1], knobs_[2], knobs_[3]};
    int newest = -1;
    for (int vi = 0; vi < VOICES; vi++) {
        Voice& v = v_[vi];
        if (v.stage == OFF) continue;
        bool useF = false;  // SUPERSAW z filtrem: wyjscie z fbuf
        act++;
        // obwiednia filtra (na blok)
        switch (v.fstage) {
        case ATTACK: v.fenv += fA_; if (v.fenv >= 1.f) { v.fenv = 1.f; v.fstage = DECAY; } break;
        case DECAY: v.fenv += (fS_ - v.fenv) * fD_; break;
        case RELEASE: v.fenv -= v.fenv * fR_; break;
        default: break;
        }
        // obwiednia modulacji (na blok) i modulacje tego glosu = LFO + MOD ENV
        switch (v.mstage) {
        case ATTACK: v.menv += mA_; if (v.menv >= 1.f) { v.menv = 1.f; v.mstage = DECAY; } break;
        case DECAY: v.menv += (mS_ - v.menv) * mD_; break;
        case RELEASE: v.menv -= v.menv * mR_; break;
        default: break;
        }
        float vm[LFO_DESTS];
        for (int i = 0; i < LFO_DESTS; i++) vm[i] = mod[i];
        float mvol = 1.f;
        if (mAmt_ != 0.f) {
            if (mDest_ == DEST_VOLUME) mvol = mAmt_ > 0 ? 1.f - mAmt_ + mAmt_ * v.menv : 1.f + mAmt_ * v.menv;
            else if (mDest_ < LFO_DESTS) vm[mDest_] += mAmt_ * v.menv;
        }
        const float pitchMul = vm[DEST_PITCH] != 0 ? powf(2.f, vm[DEST_PITCH]) : 1.f;  // glebokosc 1 = +-1 oktawa
        // galki glosu = galki sciezki + LFO + obwiednie (przy zerowych modulacjach — bez zmian)
        for (int i = 0; i < 8; i++) vk[i] = knobs_[i];
        for (int i = 0; i < 4; i++) {
            float m = vm[i] + (i == (kind_ == 1 || kind_ == 10 ? 1 : (kind_ == 12 ? -1 : 2)) ? fAmt_ * v.fenv : 0.f);  // SUPERSAW: obwiednia na filtr (nizej)  // jasnosc: DCW / CUTOFF MONOLIT (1), CUTOFF (2)
            if (m != 0.f) {
                int x = knobs_[i] + (int)lrintf(m * 32767.f);
                vk[i] = (int16_t)(x < 0 ? 0 : (x > 32767 ? 32767 : x));
            }
        }
        if (kind_ == 26 || kind_ == 27) {  // FELT, GRAND: modulacja strony 2 — DETUNE, WORN, BODY (LFO i MOD ENV)
            static const int dst[3] = {DEST_W1, DEST_W2, DEST_P1};
            for (int j = 0; j < 3; j++)
                if (vm[dst[j]] != 0.f) {
                    int x = knobs_[5 + j] + (int)lrintf(vm[dst[j]] * 32767.f);
                    vk[5 + j] = (int16_t)(x < 0 ? 0 : (x > 32767 ? 32767 : x));
                }
        }
        if (kind_ == 19) {  // ARC: modulacja strony 2 — RATIO (0 = 8192 = 1:1), COUPLING (0 = wyl.), BRIGHTNESS (0 = 8192)
            static const int dst[3] = {DEST_W1, DEST_W2, DEST_P1};
            for (int j = 0; j < 3; j++)
                if (vm[dst[j]] != 0.f) {
                    const int base = knobs_[5 + j] > 0 ? knobs_[5 + j] : (j == 1 ? 0 : 8192);
                    int x = base + (int)lrintf(vm[dst[j]] * 32767.f);
                    vk[5 + j] = (int16_t)(x < (j == 1 ? 0 : 1) ? (j == 1 ? 0 : 1) : (x > 32767 ? 32767 : x));
                }
        }
        if (kind_ == 13) {  // MIXTUR: modulacja formantow F1 / F2 / RES (slowa 5..7; 0 = 8192 jak w oryginale)
            static const int dst[3] = {DEST_W1, DEST_W2, DEST_P1};
            for (int j = 0; j < 3; j++)
                if (vm[dst[j]] != 0.f) {
                    int x = (knobs_[5 + j] > 0 ? knobs_[5 + j] : 8192) + (int)lrintf(vm[dst[j]] * 32767.f);
                    vk[5 + j] = (int16_t)(x < 1 ? 1 : (x > 32767 ? 32767 : x));
                }
        }
        if (kind_ == 9 && vm[0] != 0.f) {  // SID: modulacja WAVE skokowo o cale fale (+-5 przy glebokosci 1), bez przelatywania pasm
            int mode = (int)sidWaveMode(knobs_[0]) + (int)lrintf(vm[0] * 5.f);
            mode = mode < 0 ? 0 : (mode > 5 ? 5 : mode);
            vk[0] = (int16_t)(mode * 512 + 256);
        }
        // portamento, unison, AGE (przy wylaczonych — dokladnie jak oryginal: v.inc)
        uint32_t base = v.inc;
        if (glide_ > 0 || v.det != 0.f || age_ > 0) {
            v.pitchCur += (v.pitchTgt - v.pitchCur) * glideK_;
            float cents = v.det;
            if (age_ > 0) {
                if (fabsf(v.ageTp - v.ageP) < 0.05f) v.ageTp = rnd();
                if (fabsf(v.ageTc - v.ageC) < 0.05f) v.ageTc = rnd();
                v.ageP += (v.ageTp - v.ageP) * 0.003f;  // ~0,4 s na blok 1,33 ms: powolny dryf
                v.ageC += (v.ageTc - v.ageC) * 0.003f;
                cents += (v.ageP * 9.f + v.statP * 7.f) * age_;
                const int bi = kind_ == 1 || kind_ == 10 ? 1 : 2;  // jasnosc: CUTOFF albo DCW
                int c2 = vk[bi] + (int)lrintf((v.ageC * 0.6f + v.statC * 0.4f) * age_ * 3000.f);
                vk[bi] = (int16_t)(c2 < 0 ? 0 : (c2 > 32767 ? 32767 : c2));
            }
            base = incOf(v.pitchCur + octSemi_ + cents / 100.0, voiceRate_);
        }
        uint32_t inc = pitchMul != 1.f ? (uint32_t)((double)base * pitchMul) : base;
        if (kind_ == 27) {  // GRAND (wlasny model fortepianu koncertowego)
            grandRender(grd_, vi, out, vk, inc, voiceRate_, v.vel);
        } else if (kind_ == 26) {  // FELT (wlasny model fortepianu z filcem)
            feltRender(flt_, vi, out, vk, inc, voiceRate_, v.vel);
        } else if (kind_ == 25) {  // LOUNGE (wlasny model: zab / jezyczek / struna + przetwornik)
            loungeRender(lng_, vi, out, vk, inc, voiceRate_, v.vel);
        } else if (kind_ == 24) {  // PARALLAX (wlasny DSP: fala, unison, filtr 303)
            parallaxRender(prx_, vi, out, vk, inc, voiceRate_);
        } else if (kind_ == 22) {  // OUTATIME (oryginalny DSP z paczki Hundekraft)
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            outatimeRender(ota_, vi, out, vk, pitchBuf, v.vel);
        } else if (kind_ == 21) {  // JUNI (oryginalny DSP op1r): obwiednia glosnosci (AMP ENV) prowadzi tez filtr (SWEEP)
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            juniRender(jun_, vi, out, vk, pitchBuf, v.vel, (int)lrintf(v.env * 32767.f));
        } else if (kind_ == 20) {  // DDDD (oryginalny DSP op1r): obwiednia glosnosci (AMP ENV) otwiera filtr sciany
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            ddddRender(ddd_, vi, out, vk, pitchBuf, v.vel, (int)lrintf(v.env * 32767.f));
        } else if (kind_ == 19) {  // ARC (oryginalny DSP op1r)
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            arcRender(arc_, vi, out, vk, pitchBuf, v.vel);
        } else if (kind_ == 18) {  // AETHER (oryginalny DSP op1r): wirtualny czujnik = strona TILT + modulacja PTLT / VTLT / SHAK
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            aetherRender(aet_, vi, out, vk, pitchBuf, v.vel, (int)lrintf(v.env * 32767.f), tilt_[0] + vm[DEST_W1],
                         tilt_[1] + vm[DEST_W2], tilt_[2] + vm[DEST_P1]);
        } else if (kind_ == 17) {  // BLOT (oryginalny DSP op1r)
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            blotRender(blt_, vi, out, vk, pitchBuf, v.vel);
        } else if (kind_ == 16) {  // BLOODMOON (oryginalny DSP op1r)
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            bloodRender(bld_, vi, out, vk, pitchBuf, v.vel);
        } else if (kind_ == 15) {  // HOLLOW SAINT (oryginalny DSP op1r): obwiednia (AMP ENV), zegar taktu z transportu, RELIC
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            const double half = voiceRate_ * 0.5, bpm = tBpm_ < 30.f ? 30.0 : tBpm_;  // probki "44,1 kHz" glosu
            const double lenS = half * 60.0 / bpm;
            const double fr = tBeat_ - floor(tBeat_);
            const int32_t len = (int32_t)(lenS * 16384.0), next = (int32_t)((1.0 - fr) * lenS * 16384.0);
            hollowRender(hls_, vi, out, vk, pitchBuf, v.vel, (int)lrintf(v.env * 32767.f), relic_, len, next,
                         tPlaying_ ? 0x100000 : 0);
        } else if (kind_ == 14) {  // BRASSOPHONE (oryginalny DSP op1r): obwiednia glosnosci (AMP ENV) prowadzi barwe i podjazd
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            brassRender(brs_, vi, out, vk, pitchBuf, v.vel, (int)lrintf(v.env * 32767.f));
        } else if (kind_ == 13) {  // MIXTUR (oryginalny DSP op1r)
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            mixturRender(mxt_, vi, out, vk, pitchBuf, v.vel);
        } else if (kind_ == 12) {  // SUPERSAW (oryginalny DSP op1r)
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            supersawRender(ssw_, vi, out, vk, pitchBuf, v.vel);
            const float fc = ssCut_ + vm[DEST_W1] + fAmt_ * v.fenv, fr = ssRes_ + vm[DEST_W2];
            ssFilter(vi, out, fbuf, fc, fr);
            useF = !(fc >= 1.f && fr <= 0.f && ssf_[vi].cut >= 0.999f && ssf_[vi].res <= 0.001f);  // otwarty = oryginal bit w bit
        } else if (kind_ == 11) {  // VAPORA (oryginalny DSP op1r)
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            vaporaRender(vpr_, vi, out, vk, pitchBuf, v.vel);
        } else if (kind_ == 10) {  // MONOLIT (oryginalny DSP op1r)
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            monolitRender(mnl_, vi, out, vk, pitchBuf, v.vel);
        } else if (kind_ == 9) {  // SID (oryginalny DSP op1r / cRSID)
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            sidRender(sid_, vi, out, vk, pitchBuf, v.vel);
        } else if (kind_ == 8) {  // OXBOW+ (rozszerzona kopia DSP kennel): modulacje PW1/PW2, P1/P2, LV1/LV2 per glos
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            obx_params q = obx_;
            auto pwm = [](uint32_t pw, float m) {
                double x = (double)pw / 4294967296.0 + m * 0.45;
                x = x < 0.05 ? 0.05 : (x > 0.95 ? 0.95 : x);
                return (uint32_t)(x * 4294967296.0);
            };
            if (vm[DEST_W1] != 0.f) q.pw1 = pwm(q.pw1, vm[DEST_W1]);
            if (vm[DEST_W2] != 0.f) q.pw2 = pwm(q.pw2, vm[DEST_W2]);
            if (vm[DEST_P1] != 0.f) q.cent8_1 += (int32_t)lrintf(vm[DEST_P1] * 12.f * 256.f);  // +-1 oktawa
            if (vm[DEST_P2] != 0.f) q.cent8_2 += (int32_t)lrintf(vm[DEST_P2] * 12.f * 256.f);
            auto lv = [](int32_t l, float m) { int x = (int)lrintf(l * (1.f + m)); return x < 0 ? 0 : (x > 126 ? 126 : x); };
            if (vm[DEST_LV1] != 0.f) q.lvl1 = lv(q.lvl1, vm[DEST_LV1]);
            if (vm[DEST_LV2] != 0.f) q.lvl2 = lv(q.lvl2, vm[DEST_LV2]);
            oxbowPlusRender(oxp_, vi, out, vk, pitchBuf, v.vel, &q);
        } else if (kind_ == 7) {  // OXBOW (oryginalny DSP kennel)
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            oxbowRender(oxbow_, vi, out, vk, pitchBuf, v.vel);
        } else if (kind_ == 6) {  // REACTOR (oryginalny DSP op1r)
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            reactorRender(reactor_, vi, out, vk, pitchBuf, v.vel);
        } else if (kind_ == 5) {  // MBIRA (oryginalny DSP op1r)
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            mbiraRender(mbira_, vi, out, vk, pitchBuf, v.vel);
        } else if (kind_ == 4) {  // EVA (oryginalny DSP op1r)
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            evaRender(eva_, vi, out, vk, pitchBuf, v.vel);
        } else if (kind_ == 3) {  // ELG (oryginalny DSP op1r)
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            elgRender(elg_, vi, out, vk, pitchBuf, v.vel, movSync_ ? (int64_t)(movPh_ * 4294967296.0) % 4294967296LL : -1);
        } else if (kind_ == 2) {  // PEW (oryginalny DSP op1r)
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            pewRender(pew_, vi, out, vk, pitchBuf, v.vel);
        } else if (kind_ == 1) {  // TAFFY (oryginalny DSP op1r)
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            taffyRender(tfBase_ + vi, out, vk, pitchBuf, v.vel);
        } else if (alt_) renderExtVoice(vi, out, vk, vm, inc, v.vel);
        else {
            for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
            wavetable_dsp_render(ST(st_, vi), out, vk, pitchBuf, v.vel);
        }
        for (int i = 0; i < BLOCK; i++) {
            switch (v.stage) {
            case ATTACK: v.env += atk_; if (v.env >= 1.f) { v.env = 1.f; v.stage = DECAY; } break;
            case DECAY:
            case SUSTAIN: v.env += (sus_ - v.env) * dec_k_; break;  // podtrzymanie sledzi galke SUSTAIN na biezaco
            case RELEASE: v.env -= v.env * rel_k_; if (v.env < 1e-4f) { v.env = 0; v.stage = OFF; } break;
            case FADE: v.env -= fade_; if (v.env <= 0.f) { v.env = 0; v.stage = OFF; } break;
            }
            float g = v.env;
            if (envAmt_ < 1.f) {  // AMOUNT < 100%: czesc glosnosci = bramka (rampa 3 ms); ujemny = obwiednia odwrocona
                bool open = v.stage != RELEASE && v.stage != FADE && v.stage != OFF;
                v.gate += open ? (v.gate < 1.f ? gateK_ : 0.f) : (v.gate > 0.f ? -gateK_ : 0.f);
                v.gate = v.gate < 0 ? 0 : (v.gate > 1 ? 1 : v.gate);
                if (envAmt_ >= 0.f) g = (1.f - envAmt_) * v.gate + envAmt_ * v.env;
                else {
                    float inv = v.gate - v.env;  // przy szczycie obwiedni najciszej
                    g = (1.f + envAmt_) * v.gate - envAmt_ * (inv > 0 ? inv : 0.f);
                }
            }
            if (trem != 1.f || tremPrev_ != 1.f) g *= tremPrev_ + (trem - tremPrev_) * (i + 1) * (1.f / BLOCK);
            if (mvol != 1.f || v.mvolPrev != 1.f) g *= v.mvolPrev + (mvol - v.mvolPrev) * (i + 1) * (1.f / BLOCK);
            if (mono_ && unison_ > 0) g *= 0.4082f;  // unison: 6 glosow ~ glosnosc jednego (1/sqrt(6))
            sum[i] += (useF ? (double)fbuf[i] : (double)out[i]) * g;
        }
        v.mvolPrev = mvol;
        if (v.stage == OFF && v.pendPitch >= 0) startVoice(vi, v.pendPitch, v.pendVel, v.pendDet, v.pendFrom);  // po wyciszeniu
        if (v.env > loudEnv) { loudEnv = v.env; loud = vi; }
        if (newest < 0 || v.age > v_[newest].age) {
            newest = vi;
            for (int i = 0; i < 4; i++) loudK[i] = vk[i];
            if (kind_ == 1) tfEnv_.store(taffyVoiceEnv(tfBase_ + vi), std::memory_order_relaxed);
            vLev_.store((int16_t)(v.env * v.vel * (32767.f / 127.f)), std::memory_order_relaxed);
        }
    }
    tremPrev_ = trem;
    for (int i = 0; i < 4; i++) modKnob_[i].store(loudK[i], std::memory_order_relaxed);
    if (loud >= 0) drift_.store(ST(st_, loud)->ph2 - ST(st_, loud)->ph1, std::memory_order_relaxed);
    active_.store(act, std::memory_order_relaxed);
    // decymacja 2:1 (jak dec_block w harnessie: ta sama kolejnosc sumowania -> wynik identyczny)
    for (int i = 0; i < BLOCK; i++) {
        zp_ = zp_ ? zp_ - 1 : TAPS - 1;
        z_[zp_] = z_[zp_ + TAPS] = sum[i];
        if (i & 1) {
            const double* z = z_ + zp_;
            double a = 0;
            for (int n = 0; n < TAPS; n++) a += h_[n] * z[n];
            dec_[i >> 1] = a;
        }
    }
    // MONOLIT jest glosniejszy od reszty (oryginal bez trymu; 2 nuty dochodzily do pelnej skali): -3 dB na wyjsciu
    // sciezki, po decymacji — DSP i test zgodnosci (lastDecimated) bez zmian
    // VAPORA odwrotnie: oryginal zostawia zapas na 6 glosow (~10 dB ciszej od reszty) — +9,5 dB (x3)
    // SUPERSAW tak jak MONOLIT (geste piły z przesterem): -3 dB
    const double outGain = kind_ == 10 || kind_ == 12 || kind_ == 18 || kind_ == 19 || kind_ == 21 ? 0.7079 / 32768.0 :  // AETHER: czysty sinus, glosny
                                 kind_ == 11 ? 3.0 / 32768.0 : kind_ == 17 ? 2.0 / 32768.0 : 1.0 / 32768.0;  // BLOT: +6 dB (oryginal x0,6, filtr 303 szybko sie zamyka)
    for (int i = 0; i < BLOCK / 2; i++) fifo_[i] = (float)(dec_[i] * outGain);
    fifoPos_ = 0;
}

void WavetableSynth::setGin(int kit, const GinSound* s) { ginSetKit(gin_, kit, s); }

// GIN: wlasna sciezka (oryginal 88,2 kHz -> 48 kHz w gin_dsp.c), bez obwiedni i modulacji gospodarza.
void WavetableSynth::renderGin(float* out, int n) {
    float buf[256];
    int voices = 0;
    for (int pos = 0; pos < n; pos += 256) {
        const int m = n - pos < 256 ? n - pos : 256;
        for (int i = 0; i < m; i++) buf[i] = 0;
        voices = ginRender(gin_, buf, m);
        for (int i = 0; i < m; i++) {
            vol_ += (volT_ - vol_) * volK_;
            const float x = buf[i] * 2.f * vol_;  // +6 dB: zestawy oryginalu szczytuja ~0,3
            const float ax = x < 0 ? -x : x;
            if (ax > peak_) peak_ = ax;
            out[(pos + i) * 2] += x;
            out[(pos + i) * 2 + 1] += x;
        }
    }
    active_.store(voices, std::memory_order_relaxed);
}

// VELOUR: wlasna sciezka (oryginal PicoFaceSM natywnie w 48 kHz, stereo z ensemble), bez obwiedni gospodarza.
void WavetableSynth::renderVelour(float* out, int n) {
    float l[256], r[256];
    for (int pos = 0; pos < n; pos += 256) {
        const int m = n - pos < 256 ? n - pos : 256;
        velourRender(vel_, l, r, m);
        for (int i = 0; i < m; i++) {
            vol_ += (volT_ - vol_) * volK_;
            const float a = l[i] * 5.f * vol_, b = r[i] * 5.f * vol_;  // +14 dB: oryginal skaluje wyjscie 0,115 pod limiter Pico
            const float ax = a < 0 ? -a : a, bx = b < 0 ? -b : b;
            if (ax > peak_) peak_ = ax;
            if (bx > peak_) peak_ = bx;
            out[(pos + i) * 2] += a;
            out[(pos + i) * 2 + 1] += b;
        }
    }
    active_.store(velourActiveKeys(vel_), std::memory_order_relaxed);
}

void WavetableSynth::render(float* out, int n) {
    if (kind_ == 23) { renderGin(out, n); return; }
    if (kind_ == 28) { renderVelour(out, n); return; }
    advanceLfo(n);
    bool any = false;
    for (auto& v : v_) if (v.stage != OFF) { any = true; break; }
    if (any) flush_ = 2;  // po ostatnim glosie jeszcze 2 bloki ciszy: ogon filtra decymujacego wybrzmiewa (bez klika)
    if (!any && fifoPos_ >= BLOCK / 2 && flush_ == 0) {
        vol_ = volT_;
        active_.store(0, std::memory_order_relaxed);
        return;
    }
    for (int i = 0; i < n; i++) {
        if (fifoPos_ >= BLOCK / 2) {
            bool live = false;
            for (auto& v : v_) if (v.stage != OFF) { live = true; break; }
            if (!live && flush_ > 0) flush_--;
            renderBlock();
        }
        vol_ += (volT_ - vol_) * volK_;
        float x = fifo_[fifoPos_++] * vol_;
        float ax = x < 0 ? -x : x;
        if (ax > peak_) peak_ = ax;
        out[i * 2] += x;
        out[i * 2 + 1] += x;
    }
}

}  // namespace dp

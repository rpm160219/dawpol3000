#include "synth/wavetable_synth.h"
#include "seq/project.h"
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
    if (glide_ <= 0) { v.pitchCur = (float)pitch; v.inc = incOf(pitch, voiceRate_); }
}

// MONO: stos trzymanych nut (ostatnia ma priorytet), legato bez ponownego startu obwiedni, unison 6 glosow.
void WavetableSynth::monoOn(int pitch, float vel) {
    for (int i = 0; i < nStack_; i++) if (stack_[i] == pitch) { for (int j = i; j < nStack_ - 1; j++) stack_[j] = stack_[j + 1]; nStack_--; break; }
    if (nStack_ < 16) stack_[nStack_++] = pitch;
    const int n = unison_ > 0 ? VOICES : 1;
    bool held = false;
    for (int i = 0; i < n; i++) if (v_[i].stage == ATTACK || v_[i].stage == DECAY || v_[i].stage == SUSTAIN) held = true;
    for (int i = 0; i < n; i++) {
        float det = n > 1 ? unison_ * (2.f * i / (n - 1) - 1.f) : 0.f;  // symetrycznie -unison..+unison
        Voice& v = v_[i];
        if (held && v.stage != OFF && v.stage != RELEASE && v.stage != FADE) { setPitchTarget(v, pitch); v.det = det; continue; }
        v.age = ++age_counter_;
        if (v.stage != OFF && v.env > 1e-3f) { v.stage = FADE; v.pendPitch = pitch; v.pendVel = vel; v.pendDet = det; }
        else startVoice(i, pitch, vel, det);
    }
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
            v.fstage = RELEASE;
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
        return;
    }
    startVoice(slot, pitch, vel);
}

void WavetableSynth::startVoice(int slot, int pitch, float vel, float det) {
    Voice& v = v_[slot];
    v.pitch = pitch;
    v.pendPitch = -1;
    v.det = det;
    v.pitchTgt = (float)pitch;  // portamento: start od ostatnio granej wysokosci
    v.pitchCur = glide_ > 0 && lastPitch_ >= 0 ? (float)lastPitch_ : (float)pitch;
    lastPitch_ = pitch;
    v.vel = (int)lrintf(vel * 127.f);
    if (v.vel < 1) v.vel = 1;
    if (v.vel > 127) v.vel = 127;
    // przyrost fazy jak ENGINE_PITCH: 2^32 * f / czestotliwosc glosu
    v.inc = (uint32_t)(int32_t)lrint(4294967296.0 * 440.0 * pow(2.0, (pitch - 69) / 12.0) / voiceRate_);
    v.stage = ATTACK;
    v.env = atk_ >= 1.f ? 1.f : 0.f;
    if (v.env >= 1.f) v.stage = DECAY;
    v.fstage = ATTACK;
    v.fenv = 0;
    v.gate = 0;
    wavetable_dsp_on(ST(st_, slot));  // engine_note_on
    wtExtOn(SX(sx_, slot));
}

void WavetableSynth::noteOff(int pitch) {
    if (mono_) { monoOff(pitch); return; }
    for (auto& v : v_) {
        if (v.stage == FADE && v.pendPitch == pitch) { v.pendPitch = -1; continue; }  // puszczona przed startem
        if (v.stage != OFF && v.stage != RELEASE && v.stage != FADE && v.pitch == pitch) {
            v.stage = RELEASE;
            v.fstage = RELEASE;
            if (rel_k_ >= 1.f) { v.env = 0; v.stage = OFF; }
        }
    }
}

void WavetableSynth::allOff() {
    for (auto& v : v_) {
        v.pendPitch = -1;
        if (v.stage != OFF) { v.stage = RELEASE; v.fstage = RELEASE; if (rel_k_ >= 1.f) { v.env = 0; v.stage = OFF; } }
    }
}

// Jeden blok glosow (128 probek przy 96 kHz) -> 64 probki wyjscia.
void WavetableSynth::renderBlock() {
    double sum[BLOCK];
    for (int i = 0; i < BLOCK; i++) sum[i] = 0;
    uint32_t pitchBuf[BLOCK];
    int16_t out[BLOCK];
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
    const float pitchMul = mod[DEST_PITCH] != 0 ? powf(2.f, mod[DEST_PITCH]) : 1.f;
    int16_t vk[8];
    int16_t loudK[4] = {knobs_[0], knobs_[1], knobs_[2], knobs_[3]};
    int newest = -1;
    for (int vi = 0; vi < VOICES; vi++) {
        Voice& v = v_[vi];
        if (v.stage == OFF) continue;
        act++;
        // obwiednia filtra (na blok)
        switch (v.fstage) {
        case ATTACK: v.fenv += fA_; if (v.fenv >= 1.f) { v.fenv = 1.f; v.fstage = DECAY; } break;
        case DECAY: v.fenv += (fS_ - v.fenv) * fD_; break;
        case RELEASE: v.fenv -= v.fenv * fR_; break;
        default: break;
        }
        // galki glosu = galki sciezki + LFO + obwiednia filtra (przy zerowych modulacjach — bez zmian)
        for (int i = 0; i < 8; i++) vk[i] = knobs_[i];
        for (int i = 0; i < 4; i++) {
            float m = mod[i] + (i == 2 ? fAmt_ * v.fenv : 0.f);
            if (m != 0.f) {
                int x = knobs_[i] + (int)lrintf(m * 32767.f);
                vk[i] = (int16_t)(x < 0 ? 0 : (x > 32767 ? 32767 : x));
            }
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
                int c2 = vk[2] + (int)lrintf((v.ageC * 0.6f + v.statC * 0.4f) * age_ * 3000.f);
                vk[2] = (int16_t)(c2 < 0 ? 0 : (c2 > 32767 ? 32767 : c2));
            }
            base = incOf(v.pitchCur + cents / 100.0, voiceRate_);
        }
        uint32_t inc = pitchMul != 1.f ? (uint32_t)((double)base * pitchMul) : base;
        if (alt_) renderExtVoice(vi, out, vk, mod, inc, v.vel);
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
            sum[i] += (double)out[i] * g;
        }
        if (v.stage == OFF && v.pendPitch >= 0) startVoice(vi, v.pendPitch, v.pendVel, v.pendDet);  // po wyciszeniu
        if (v.env > loudEnv) { loudEnv = v.env; loud = vi; }
        if (newest < 0 || v.age > v_[newest].age) { newest = vi; for (int i = 0; i < 4; i++) loudK[i] = vk[i]; }
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
    for (int i = 0; i < BLOCK / 2; i++) fifo_[i] = (float)(dec_[i] * (1.0 / 32768.0));
    fifoPos_ = 0;
}

void WavetableSynth::render(float* out, int n) {
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

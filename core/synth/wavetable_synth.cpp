#include "synth/wavetable_synth.h"
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

void WavetableSynth::init(float outRate) {
    outRate_ = outRate;
    voiceRate_ = outRate * 2.f;
    for (int i = 0; i < VOICES; i++) { v_[i] = Voice(); wavetable_dsp_on(ST(st_, i)); }
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
    setEnvelope(0.005f, 0.4f, 0.75f, 0.3f);
}

void WavetableSynth::setKnobs(const int16_t k[4]) {
    for (int i = 0; i < 4; i++) knobs_[i] = k[i] < 0 ? 0 : k[i];
}

void WavetableSynth::setEnvelope(float a, float d, float s, float r) {
    atk_ = a > 0 ? 1.f / (a * voiceRate_) : 2.f;
    dec_k_ = d > 0 ? 1.f - expf(-5.f / (d * voiceRate_)) : 1.f;  // ~ -43 dB po czasie d
    sus_ = s < 0 ? 0 : (s > 1 ? 1 : s);
    rel_k_ = r > 0 ? 1.f - expf(-5.f / (r * voiceRate_)) : 1.f;
}

void WavetableSynth::noteOn(int pitch, float vel) {
    int slot = -1;
    for (int i = 0; i < VOICES; i++) if (v_[i].stage != OFF && v_[i].pitch == pitch) { slot = i; break; }
    if (slot < 0) for (int i = 0; i < VOICES; i++) if (v_[i].stage == OFF) { slot = i; break; }
    if (slot < 0) {  // kradziez: najpierw najcichszy w release, potem najstarszy
        float best = 2.f;
        for (int i = 0; i < VOICES; i++) if (v_[i].stage == RELEASE && v_[i].env < best) { best = v_[i].env; slot = i; }
        if (slot < 0) { slot = 0; for (int i = 1; i < VOICES; i++) if (v_[i].age < v_[slot].age) slot = i; }
    }
    Voice& v = v_[slot];
    v.pitch = pitch;
    v.vel = (int)lrintf(vel * 127.f);
    if (v.vel < 1) v.vel = 1;
    if (v.vel > 127) v.vel = 127;
    // przyrost fazy jak ENGINE_PITCH: 2^32 * f / czestotliwosc glosu
    v.inc = (uint32_t)(int32_t)lrint(4294967296.0 * 440.0 * pow(2.0, (pitch - 69) / 12.0) / voiceRate_);
    v.stage = ATTACK;
    v.env = atk_ >= 1.f ? 1.f : 0.f;
    if (v.env >= 1.f) v.stage = DECAY;
    v.age = ++age_;
    wavetable_dsp_on(ST(st_, slot));  // engine_note_on
}

void WavetableSynth::noteOff(int pitch) {
    for (auto& v : v_)
        if (v.stage != OFF && v.stage != RELEASE && v.pitch == pitch) {
            v.stage = RELEASE;
            if (rel_k_ >= 1.f) { v.env = 0; v.stage = OFF; }
        }
}

void WavetableSynth::allOff() {
    for (auto& v : v_) if (v.stage != OFF) { v.stage = RELEASE; if (rel_k_ >= 1.f) { v.env = 0; v.stage = OFF; } }
}

// Jeden blok glosow (128 probek przy 96 kHz) -> 64 probki wyjscia.
void WavetableSynth::renderBlock() {
    double sum[BLOCK];
    for (int i = 0; i < BLOCK; i++) sum[i] = 0;
    uint32_t pitchBuf[BLOCK];
    int16_t out[BLOCK];
    int act = 0, loud = -1;
    float loudEnv = -1;
    for (int vi = 0; vi < VOICES; vi++) {
        Voice& v = v_[vi];
        if (v.stage == OFF) continue;
        act++;
        for (int i = 0; i < BLOCK; i++) pitchBuf[i] = v.inc;
        wavetable_dsp_render(ST(st_, vi), out, knobs_, pitchBuf, v.vel);
        for (int i = 0; i < BLOCK; i++) {
            switch (v.stage) {
            case ATTACK: v.env += atk_; if (v.env >= 1.f) { v.env = 1.f; v.stage = DECAY; } break;
            case DECAY: v.env += (sus_ - v.env) * dec_k_; if (fabsf(v.env - sus_) < 1e-4f) { v.env = sus_; v.stage = SUSTAIN; } break;
            case SUSTAIN: break;
            case RELEASE: v.env -= v.env * rel_k_; if (v.env < 1e-4f) { v.env = 0; v.stage = OFF; } break;
            }
            sum[i] += (double)out[i] * v.env;
        }
        if (v.env > loudEnv) { loudEnv = v.env; loud = vi; }
    }
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
    bool any = false;
    for (auto& v : v_) if (v.stage != OFF) { any = true; break; }
    if (!any && fifoPos_ >= BLOCK / 2) {
        vol_ = volT_;
        active_.store(0, std::memory_order_relaxed);
        // ogon decymatora wygasa po ciszy — wyzeruj, zeby kolejna nuta startowala czysto
        for (int i = 0; i < 2 * TAPS; i++) z_[i] = 0;
        return;
    }
    for (int i = 0; i < n; i++) {
        if (fifoPos_ >= BLOCK / 2) renderBlock();
        vol_ += (volT_ - vol_) * volK_;
        float x = fifo_[fifoPos_++] * vol_;
        float ax = x < 0 ? -x : x;
        if (ax > peak_) peak_ = ax;
        out[i * 2] += x;
        out[i * 2 + 1] += x;
    }
}

}  // namespace dp

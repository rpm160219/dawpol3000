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
    fade_ = 1.f / (0.002f * voiceRate_);  // ~2 ms
    gateK_ = 1.f / (0.003f * voiceRate_); // ~3 ms
    for (int i = 0; i < 4; i++) modKnob_[i].store(knobs_[i]);
    setEnvelope(0.005f, 0.4f, 0.75f, 0.3f);
    setFilterEnv(0.005f, 0.3f, 0.f, 0.3f, 0.f);
}

void WavetableSynth::setFilterEnv(float a, float d, float s, float r, float amount) {
    const float br = voiceRate_ / BLOCK;  // bloki na sekunde
    fA_ = a > 0 ? 1.f / (a * br) : 2.f;
    fD_ = d > 0 ? 1.f - expf(-5.f / (d * br)) : 1.f;
    fS_ = s < 0 ? 0 : (s > 1 ? 1 : s);
    fR_ = r > 0 ? 1.f - expf(-5.f / (r * br)) : 1.f;
    fAmt_ = amount < -1 ? -1 : (amount > 1 ? 1 : amount);
}

void WavetableSynth::setLfo(int i, int shape, float rate, float depth, int dest) {
    if (i < 0 || i > 1) return;
    lfo_[i].shape = shape;
    lfo_[i].rate = rate;
    lfo_[i].depth = depth < 0 ? 0 : (depth > 1 ? 1 : depth);
    lfo_[i].dest = dest;
}

// Przesuniecie faz LFO o czas (wywolywane dla kazdej porcji wyjscia — takze w ciszy).
void WavetableSynth::advanceLfo(int outFrames) {
    for (auto& l : lfo_) {
        l.ph += l.rate * outFrames / outRate_;
        if (l.ph >= 1.0) {
            l.ph -= floor(l.ph);
            l.seed = l.seed * 1664525u + 1013904223u;  // nowa wartosc losowa co cykl
            l.rnd = (int32_t)(l.seed >> 8) / 8388608.f - 1.f;
        }
    }
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
    envAmt_ = amount < 0 ? 0 : (amount > 1 ? 1 : amount);
    atk_ = a > 0 ? 1.f / (a * voiceRate_) : 2.f;
    dec_k_ = d > 0 ? 1.f - expf(-5.f / (d * voiceRate_)) : 1.f;  // ~ -43 dB po czasie d
    sus_ = s < 0 ? 0 : (s > 1 ? 1 : s);
    rel_k_ = r > 0 ? 1.f - expf(-5.f / (r * voiceRate_)) : 1.f;
}

void WavetableSynth::noteOn(int pitch, float vel) {
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
    v.age = ++age_;
    if (v.stage != OFF && v.env > 1e-3f) {  // glos jeszcze brzmi: najpierw wycisz (~2 ms), potem nowa nuta
        v.stage = FADE;
        v.pendPitch = pitch;
        v.pendVel = vel;
        return;
    }
    startVoice(slot, pitch, vel);
}

void WavetableSynth::startVoice(int slot, int pitch, float vel) {
    Voice& v = v_[slot];
    v.pitch = pitch;
    v.pendPitch = -1;
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
}

void WavetableSynth::noteOff(int pitch) {
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
    float mod[6] = {0, 0, 0, 0, 0, 0};
    float trem = 1.f;
    for (int l = 0; l < 2; l++) {
        float val = lfoValue(lfo_[l]);  // faza przesuwana w render() w czasie rzeczywistym
        if (lfo_[l].depth <= 0) continue;
        if (lfo_[l].dest == 5) trem *= 1.f - lfo_[l].depth * 0.5f * (1.f - val);
        else mod[lfo_[l].dest] += val * lfo_[l].depth;
    }
    const float pitchMul = mod[4] != 0 ? powf(2.f, mod[4]) : 1.f;
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
        uint32_t inc = pitchMul != 1.f ? (uint32_t)((double)v.inc * pitchMul) : v.inc;
        for (int i = 0; i < BLOCK; i++) pitchBuf[i] = inc;
        wavetable_dsp_render(ST(st_, vi), out, vk, pitchBuf, v.vel);
        for (int i = 0; i < BLOCK; i++) {
            switch (v.stage) {
            case ATTACK: v.env += atk_; if (v.env >= 1.f) { v.env = 1.f; v.stage = DECAY; } break;
            case DECAY:
            case SUSTAIN: v.env += (sus_ - v.env) * dec_k_; break;  // podtrzymanie sledzi galke SUSTAIN na biezaco
            case RELEASE: v.env -= v.env * rel_k_; if (v.env < 1e-4f) { v.env = 0; v.stage = OFF; } break;
            case FADE: v.env -= fade_; if (v.env <= 0.f) { v.env = 0; v.stage = OFF; } break;
            }
            float g = v.env;
            if (envAmt_ < 1.f) {  // AMOUNT < 100%: czesc glosnosci = bramka (rampa 3 ms) zamiast obwiedni
                bool open = v.stage != RELEASE && v.stage != FADE && v.stage != OFF;
                v.gate += open ? (v.gate < 1.f ? gateK_ : 0.f) : (v.gate > 0.f ? -gateK_ : 0.f);
                v.gate = v.gate < 0 ? 0 : (v.gate > 1 ? 1 : v.gate);
                g = (1.f - envAmt_) * v.gate + envAmt_ * v.env;
            }
            sum[i] += trem != 1.f ? (double)out[i] * g * trem : (double)out[i] * g;
        }
        if (v.stage == OFF && v.pendPitch >= 0) startVoice(vi, v.pendPitch, v.pendVel);  // po wyciszeniu
        if (v.env > loudEnv) { loudEnv = v.env; loud = vi; }
        if (newest < 0 || v.age > v_[newest].age) { newest = vi; for (int i = 0; i < 4; i++) loudK[i] = vk[i]; }
    }
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

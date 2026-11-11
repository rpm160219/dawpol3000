#include "synth/saw_synth.h"
#include <math.h>

namespace dp {

void SawSynth::init(float sr) {
    sr_ = sr;
    attack_ = 1.f / (0.003f * sr);
    decay_ = 1.f - expf(-1.f / (0.25f * sr));
    release_ = 1.f - expf(-1.f / (0.08f * sr));
    lpK_ = 1.f - expf(-6.2831853f * 4000.f / sr);
    volK_ = 1.f - expf(-1.f / (0.01f * sr));
}

void SawSynth::noteOn(int pitch, float vel) {
    int slot = -1;
    for (int i = 0; i < VOICES; i++) if (v_[i].pitch == pitch && v_[i].stage != OFF) { slot = i; break; }
    if (slot < 0) for (int i = 0; i < VOICES; i++) if (v_[i].stage == OFF) { slot = i; break; }
    if (slot < 0) {  // kradziez najstarszego glosu
        slot = 0;
        for (int i = 1; i < VOICES; i++) if (v_[i].age < v_[slot].age) slot = i;
    }
    Voice& v = v_[slot];
    v.pitch = pitch;
    v.dt = 440.f * powf(2.f, (pitch - 69) / 12.f) / sr_;
    v.vel = vel;
    v.stage = ATTACK;
    v.age = ++age_;
}

void SawSynth::noteOff(int pitch) {
    for (auto& v : v_) if (v.pitch == pitch && v.stage != OFF && v.stage != RELEASE) v.stage = RELEASE;
}

void SawSynth::allOff() {
    for (auto& v : v_) if (v.stage != OFF) v.stage = RELEASE;
}

static inline float polyBlep(float t, float dt) {
    if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
    if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
    return 0.f;
}

void SawSynth::render(float* out, int n) {
    bool any = false;
    for (auto& v : v_) if (v.stage != OFF) { any = true; break; }
    if (!any) { vol_ = volT_; return; }
    // Glosy sumowane do bufora mono, potem dodawane do wyjscia z glosnoscia sciezki.
    float mono[256];
    for (int off = 0; off < n; off += 256) {
        int m = n - off < 256 ? n - off : 256;
        for (int i = 0; i < m; i++) mono[i] = 0.f;
        for (auto& v : v_) {
            if (v.stage == OFF) continue;
            for (int i = 0; i < m; i++) {
                switch (v.stage) {
                case ATTACK: v.env += attack_; if (v.env >= 1.f) { v.env = 1.f; v.stage = DECAY; } break;
                case DECAY: v.env += (sustain_ - v.env) * decay_; break;
                case RELEASE: v.env -= v.env * release_; if (v.env < 1e-4f) { v.env = 0; v.stage = OFF; } break;
                }
                float s = 2.f * v.phase - 1.f - polyBlep(v.phase, v.dt);
                v.phase += v.dt;
                if (v.phase >= 1.f) v.phase -= 1.f;
                v.lp1 += (s - v.lp1) * lpK_;
                v.lp2 += (v.lp1 - v.lp2) * lpK_;
                mono[i] += v.lp2 * v.env * v.vel * 0.4f;
                if (v.stage == OFF) break;
            }
        }
        float* o = out + off * 2;
        for (int i = 0; i < m; i++) {
            vol_ += (volT_ - vol_) * volK_;
            float x = mono[i] * vol_;
            o[i * 2] += x;
            o[i * 2 + 1] += x;
        }
    }
}

}  // namespace dp

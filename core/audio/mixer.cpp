// Mikser DAW-POL: DSP kanalow, szyn AUX i mastera (wlasny kod, zob. mixer.h).
#include "audio/mixer.h"
#include <math.h>
#include <string.h>
#include "fx/op1r_fx.h"

namespace dp {

static inline float db2lin(float db) { return powf(10.f, db * .05f); }
static inline float lin2db(float x) { return 20.f * log10f(x > 1e-9f ? x : 1e-9f); }
static inline float coefOf(float sec, float fs) { return sec <= 0.f ? 1.f : 1.f - expf(-1.f / (sec * fs)); }

// ---------------- EQ ----------------
void Eq3::reset() { for (auto& b : b_) memset(b.z, 0, sizeof b.z); }

void Eq3::set(const MixEq& e, float fs) {
    if (fs == fs_ && !memcmp(&e, &cur_, sizeof e)) return;
    cur_ = e;
    fs_ = fs;
    max_ = e.on && e.max;
    active_ = e.on && !e.flat();
    outG_ = db2lin(e.on ? e.out : 0.f);
    for (int k = 0; k < 3; k++) {
        Bq& q = b_[k];
        float f = e.freq[k];
        if (f < 20.f) f = 20.f;
        if (f > fs * .45f) f = fs * .45f;
        const float A = powf(10.f, e.gain[k] / 40.f), w0 = 6.2831853f * f / fs, cw = cosf(w0);
        const float al = sinf(w0) / (2.f * (e.q[k] > .1f ? e.q[k] : .1f)), sa = 2.f * sqrtf(A) * al;
        float b0, b1, b2, a0, a1, a2;
        if (k == 0) {  // polka niska
            b0 = A * ((A + 1) - (A - 1) * cw + sa); b1 = 2 * A * ((A - 1) - (A + 1) * cw); b2 = A * ((A + 1) - (A - 1) * cw - sa);
            a0 = (A + 1) + (A - 1) * cw + sa; a1 = -2 * ((A - 1) + (A + 1) * cw); a2 = (A + 1) + (A - 1) * cw - sa;
        } else if (k == 2) {  // polka wysoka
            b0 = A * ((A + 1) + (A - 1) * cw + sa); b1 = -2 * A * ((A - 1) + (A + 1) * cw); b2 = A * ((A + 1) + (A - 1) * cw - sa);
            a0 = (A + 1) - (A - 1) * cw + sa; a1 = 2 * ((A - 1) - (A + 1) * cw); a2 = (A + 1) - (A - 1) * cw - sa;
        } else {  // dzwon
            b0 = 1 + al * A; b1 = -2 * cw; b2 = 1 - al * A; a0 = 1 + al / A; a1 = -2 * cw; a2 = 1 - al / A;
        }
        q.b0 = b0 / a0; q.b1 = b1 / a0; q.b2 = b2 / a0; q.a1 = a1 / a0; q.a2 = a2 / a0;
    }
}

void Eq3::process(float* l, float* r, int n) {
    if (!active_) return;
    float* ch[2] = {l, r};
    for (int k = 0; k < 3; k++) {
        if (cur_.gain[k] == 0.f) continue;
        Bq& q = b_[k];
        for (int c = 0; c < 2; c++) {
            float* x = ch[c];
            float z1 = q.z[c][0], z2 = q.z[c][1];
            for (int i = 0; i < n; i++) {  // TDF II
                const float in = x[i], y = q.b0 * in + z1;
                z1 = q.b1 * in - q.a1 * y + z2;
                z2 = q.b2 * in - q.a2 * y;
                x[i] = y;
            }
            q.z[c][0] = z1; q.z[c][1] = z2;
        }
    }
    for (int c = 0; c < 2; c++)
        for (int i = 0; i < n; i++) {
            float y = ch[c][i] * outG_;
            if (max_) y = tanhf(y * 1.25f) * .8f;  // MAX EQ: lekkie nasycenie
            ch[c][i] = y;
        }
}

float eqResponseDb(const MixEq& e, float hz, float fs) {
    if (!e.on) return 0.f;
    Eq3 tmp;
    tmp.set(e, fs);
    return tmp.responseDb(hz) + e.out;
}

float Eq3::responseDb(float hz) const {
    const float w = 6.2831853f * hz / fs_;
    const float c1 = cosf(w), s1 = sinf(w), c2 = cosf(2 * w), s2 = sinf(2 * w);
    float db = 0;
    for (int k = 0; k < 3; k++) {
        if (cur_.gain[k] == 0.f) continue;
        const Bq& q = b_[k];
        const float nr = q.b0 + q.b1 * c1 + q.b2 * c2, ni = -(q.b1 * s1 + q.b2 * s2);
        const float dr = 1 + q.a1 * c1 + q.a2 * c2, di = -(q.a1 * s1 + q.a2 * s2);
        db += 10.f * log10f((nr * nr + ni * ni) / (dr * dr + di * di) + 1e-12f);
    }
    return db;
}

// ---------------- kompresor kanalu ----------------
void Compressor::set(const MixComp& c, float fs) { c_ = c; fs_ = fs; }

void Compressor::process(float* l, float* r, int n) {
    if (!c_.on) { grDb_ += (0.f - grDb_) * .2f; grLin_ = 1.f; return; }
    const float* p = c_.p[c_.type];
    const int SUB = 4;
    float thr, ratio, knee, makeup, atk, rel, mix = 1.f, pre = 1.f;
    bool rmsDet = true;
    switch (c_.type) {
    case COMP_OPTO:  // optyczny: miekki, wolny, powrot dwuetapowy zalezny od tego, jak dlugo sciszal
        thr = -6.f - 34.f * p[0];
        ratio = p[2] >= .5f ? 12.f : 3.f;
        knee = p[2] >= .5f ? 6.f : 12.f;
        makeup = -10.f + 35.f * p[1];
        atk = .010f;
        rel = .06f + 2.5f * charge_;
        mix = p[3];
        break;
    case COMP_VCA:   // VCA: szybki RMS, twarde albo OVEREASY kolano, ratio do nieskonczonosci
        thr = -50.f + 50.f * p[0];
        ratio = p[1] > .97f ? 1000.f : 1.f + 40.f * p[1] * p[1];
        knee = p[3] >= .5f ? 12.f : 0.f;
        makeup = -20.f + 40.f * p[2];
        atk = .004f;
        rel = .09f;
        break;
    default: {       // FET: szczytowy, bardzo szybki atak, wejscie wpycha w staly prog
        rmsDet = false;
        pre = db2lin(-10.f + 40.f * p[0]);
        thr = -20.f;
        static const float R[5] = {4.f, 8.f, 12.f, 20.f, 30.f};
        int ri = (int)(p[4] * 4.999f);
        ri = ri < 0 ? 0 : (ri > 4 ? 4 : ri);
        ratio = R[ri];
        knee = ri == 4 ? 10.f : 3.f;
        makeup = -30.f + 40.f * p[1];
        atk = .0008f * (1.f - p[2]) + .00002f * p[2];
        rel = 1.1f * (1.f - p[3]) + .05f * p[3];
        break;
    }
    }
    const float ka = coefOf(atk, fs_ / SUB), kr = coefOf(rel, fs_ / SUB), kd = coefOf(.010f, fs_);
    const float mk = db2lin(makeup);
    const bool all = c_.type == COMP_FET && (int)(p[4] * 4.999f) >= 4;
    for (int i0 = 0; i0 < n; i0 += SUB) {
        const int m = n - i0 < SUB ? n - i0 : SUB;
        float lv = 0;
        for (int i = i0; i < i0 + m; i++) {
            const float a = fabsf(l[i]) > fabsf(r[i]) ? fabsf(l[i]) : fabsf(r[i]);
            if (rmsDet) { rms_ += (a * a - rms_) * kd; lv = sqrtf(rms_) * 1.414f; }
            else if (a * pre > lv) lv = a * pre;
        }
        const float x = lin2db(lv), over = x - thr;
        float g;  // docelowe sciszenie dB
        if (knee > 0.f && over > -knee * .5f && over < knee * .5f) {
            const float t = over + knee * .5f;
            g = (1.f - 1.f / ratio) * t * t / (2.f * knee);
        } else g = over > 0.f ? (1.f - 1.f / ratio) * over : 0.f;
        if (all) g *= 1.25f;  // ALL: przesterowane "wszystkie przyciski"
        env_ += (g - env_) * (g > env_ ? ka : kr);
        if (c_.type == COMP_OPTO) { charge_ += ((env_ > 3.f ? 1.f : 0.f) - charge_) * (env_ > 3.f ? .00005f : .00002f) * SUB; }
        grDb_ = env_;
        const float gt = db2lin(-env_) * (c_.type == COMP_FET ? pre : 1.f) * mk;
        const float dg = (gt - grLin_) / (float)m;
        for (int i = i0; i < i0 + m; i++) {
            grLin_ += dg;
            float yl = l[i] * grLin_, yr = r[i] * grLin_;
            if (c_.type == COMP_FET) { yl = tanhf(yl * .9f) / .9f; yr = tanhf(yr * .9f) / .9f; }  // zabarwienie FET
            if (mix < 1.f) { yl = l[i] + (yl - l[i]) * mix; yr = r[i] + (yr - r[i]) * mix; }
            l[i] = yl; r[i] = yr;
        }
    }
}

// ---------------- kompresor szyny master ----------------
void BusCompressor::set(const BusComp& b, float fs) { b_ = b; fs_ = fs; }

void BusCompressor::process(float* l, float* r, int n) {
    if (!b_.on) { grDb_ += (0.f - grDb_) * .2f; return; }
    static const float RA[3] = {2.f, 4.f, 10.f}, AT[6] = {.0001f, .0003f, .001f, .003f, .01f, .03f}, RE[4] = {.1f, .3f, .6f, 1.2f};
    const float thr = -30.f + 30.f * b_.thresh, ratio = RA[b_.ratio % 3];
    const float ka = coefOf(AT[b_.attack % 6], fs_);
    const bool autoRel = b_.release >= 4;
    const float kr = coefOf(autoRel ? .1f : RE[b_.release % 4], fs_), ks = coefOf(1.2f, fs_);
    const float mk = db2lin(b_.makeup);
    for (int i = 0; i < n; i++) {
        const float a = fabsf(l[i]) > fabsf(r[i]) ? fabsf(l[i]) : fabsf(r[i]);
        const float over = lin2db(a) - thr;
        float g = 0.f;
        const float knee = 3.f;
        if (over > knee * .5f) g = (1.f - 1.f / ratio) * over;
        else if (over > -knee * .5f) { const float t = over + knee * .5f; g = (1.f - 1.f / ratio) * t * t / (2.f * knee); }
        if (g > env_) env_ += (g - env_) * ka;
        else if (autoRel) {  // AUTO: szybki powrot po krotkich szczytach, wolny po dluzszym sciszaniu
            slow_ += (env_ - slow_) * ks;
            const float target = g > slow_ ? g : slow_;
            env_ += (target - env_) * kr;
        } else env_ += (g - env_) * kr;
        const float gl = db2lin(-env_) * mk;
        l[i] *= gl; r[i] *= gl;
    }
    grDb_ = env_;
}

// ---------------- efekty op1r ----------------
FxHost::~FxHost() { if (inst_) free(inst_); }

void FxHost::setType(int fx) {
    if (fx == type_) return;
    if (inst_) { free(inst_); inst_ = nullptr; }
    type_ = fx;
    if (fx == FX_JUNI_CHORUS) inst_ = fxJuniChorusCreate();
    else if (fx == FX_SISTER) inst_ = fxSisterCreate();
    memset(inL_, 0, sizeof inL_); memset(inR_, 0, sizeof inR_);
    posIn_ = 0; blkN_ = 0;
    midN_ = 96;  // wyprzedzenie kolejki wyjscia (~2 ms)
    memset(midL_, 0, sizeof midL_); memset(midR_, 0, sizeof midR_);
    posOut_ = 1.0;
    memset(tel_, 0, sizeof tel_);
}

static inline float herm(float y0, float y1, float y2, float y3, float t) {
    const float c1 = .5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.f * y2 - .5f * y3, c3 = .5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * t + c2) * t + c1) * t + y1;
}
static inline int16_t toS16(float x) { x *= 32767.f; return (int16_t)(x > 32767.f ? 32767 : (x < -32767.f ? -32767 : x)); }

void FxHost::process(float* l, float* r, int n, const int16_t* w) {
    if (!inst_ || type_ == FX_NONE || bypass_) return;
    const double stepIn = 48000.0 / 44100.0, stepOut = 44100.0 / 48000.0;
    for (int i = 0; i < n; i++) {
        // wejscie: nowa probka 48 kHz, wyjscia 44,1 kHz miedzy inL_[1] a inL_[2]
        inL_[0] = inL_[1]; inL_[1] = inL_[2]; inL_[2] = inL_[3]; inL_[3] = l[i];
        inR_[0] = inR_[1]; inR_[1] = inR_[2]; inR_[2] = inR_[3]; inR_[3] = r[i];
        while (posIn_ < 1.0) {
            const float t = (float)posIn_;
            blkL_[blkN_] = toS16(herm(inL_[0], inL_[1], inL_[2], inL_[3], t));
            blkR_[blkN_] = toS16(herm(inR_[0], inR_[1], inR_[2], inR_[3], t));
            if (++blkN_ == 64) {
                if (type_ == FX_JUNI_CHORUS) { fxJuniChorusProcess(inst_, blkL_, blkR_, w); fxJuniChorusTelemetry(tel_); }
                else { fxSisterProcess(inst_, blkL_, blkR_, w); fxSisterTelemetry(inst_, tel_); }
                if (midN_ + 64 <= 512)
                    for (int k = 0; k < 64; k++) { midL_[midN_ + k] = blkL_[k] * (1.f / 32768.f); midR_[midN_ + k] = blkR_[k] * (1.f / 32768.f); }
                midN_ += midN_ + 64 <= 512 ? 64 : 0;
                blkN_ = 0;
            }
            posIn_ += stepIn;
        }
        posIn_ -= 1.0;
        // wyjscie: probka 48 kHz z kolejki 44,1 kHz
        int k = (int)posOut_;
        if (k + 2 >= midN_) { l[i] = r[i] = 0.f; continue; }  // (nie powinno sie zdarzyc po wyprzedzeniu)
        const float t = (float)(posOut_ - k);
        l[i] = herm(midL_[k - 1], midL_[k], midL_[k + 1], midL_[k + 2], t);
        r[i] = herm(midR_[k - 1], midR_[k], midR_[k + 1], midR_[k + 2], t);
        posOut_ += stepOut;
        if (posOut_ >= 256.0) {  // przesun kolejke
            const int sh = (int)posOut_ - 1;
            memmove(midL_, midL_ + sh, sizeof(float) * (midN_ - sh));
            memmove(midR_, midR_ + sh, sizeof(float) * (midN_ - sh));
            midN_ -= sh;
            posOut_ -= sh;
        }
    }
}

// ---------------- mikser ----------------
void MixerDsp::init(float fs) {
    fs_ = fs;
    for (int i = 0; i < MAX_TRACKS; i++) gain_[i] = gainT_[i] = 0.f;
}

void MixerDsp::apply(const Project& p) {
    nTracks_ = p.nTracks;
    for (int i = 0; i < p.nTracks && i < MAX_TRACKS; i++) {
        const Track& t = p.tracks[i];
        eq_[i].set(t.eq, fs_);
        comp_[i].set(t.comp, fs_);
        for (int k = 0; k < MIX_INSERTS; k++) {
            ins_[i][k].setType(t.ins[k].fx);
            ins_[i][k].setBypass(t.ins[k].bypass);
            memcpy(insW_[i][k], t.ins[k].w, sizeof insW_[i][k]);
        }
        pan_[i] = t.pan;
        for (int a = 0; a < MIX_AUX; a++) send_[i][a] = t.send[a];
    }
    for (int a = 0; a < MIX_AUX; a++) {
        auxFx_[a].setType(p.aux[a].fx.fx);
        auxFx_[a].setBypass(p.aux[a].fx.bypass);
        memcpy(auxW_[a], p.aux[a].fx.w, sizeof auxW_[a]);
        auxEq_[a].set(p.aux[a].eq, fs_);
        auxLevel_[a] = p.aux[a].level;
    }
    bus_.set(p.bus, fs_);
    masterVol_ = p.masterVol;
}

void MixerDsp::process(float* const* tracks, const bool* active, int nTracks, float* out, int n) {
    float ml[256], mr[256];
    memset(ml, 0, sizeof(float) * n);
    memset(mr, 0, sizeof(float) * n);
    bool auxUsed[MIX_AUX] = {};
    for (int a = 0; a < MIX_AUX; a++) { memset(auxBuf_[a][0], 0, sizeof(float) * n); memset(auxBuf_[a][1], 0, sizeof(float) * n); }
    const float gk = 1.f - expf(-1.f / (.01f * fs_));  // wygladzanie tlumika ~10 ms
    const float pd = expf(-(float)n / (.08f * fs_));  // miernik: szczyt z opadaniem ~80 ms (UI czyta rzadziej niz bloki)
    auto hold = [pd](std::atomic<float>& m, float v) { const float o = m.load(std::memory_order_relaxed) * pd; m.store(v > o ? v : o, std::memory_order_relaxed); };
    for (int c = 0; c < nTracks && c < MAX_TRACKS; c++) {
        const float* in = tracks[c];
        float pk = 0;
        for (int i = 0; i < n * 2; i++) { const float a = fabsf(in[i]); if (a > pk) pk = a; }
        bool hasIns = false;
        for (int k = 0; k < MIX_INSERTS; k++) hasIns |= ins_[c][k].active();
        if (pk > 0.f || active[c]) tail_[c] = (int)(fs_ * 12.f);  // ogon insertow (poglos) po ciszy
        else if (tail_[c] > 0) tail_[c] -= n;
        if (pk == 0.f && (!hasIns || tail_[c] <= 0)) {  // cisza: kanal pomijany
            gain_[c] = gainT_[c];
            hold(ch[c].peak, 0.f);
            ch[c].gr.store(comp_[c].gr(), std::memory_order_relaxed);
            continue;
        }
        for (int i = 0; i < n; i++) { l_[i] = in[i * 2]; r_[i] = in[i * 2 + 1]; }
        for (int k = 0; k < MIX_INSERTS; k++) ins_[c][k].process(l_, r_, n, insW_[c][k]);
        eq_[c].process(l_, r_, n);
        comp_[c].process(l_, r_, n);
        const float th = (pan_[c] + 1.f) * .78539816f, gl = cosf(th) * 1.41421356f, gr = sinf(th) * 1.41421356f;
        float opk = 0;
        for (int i = 0; i < n; i++) {
            gain_[c] += (gainT_[c] - gain_[c]) * gk;
            const float yl = l_[i] * gain_[c] * gl, yr = r_[i] * gain_[c] * gr;
            l_[i] = yl; r_[i] = yr;
            ml[i] += yl; mr[i] += yr;
            const float a = fabsf(yl) > fabsf(yr) ? fabsf(yl) : fabsf(yr);
            if (a > opk) opk = a;
        }
        for (int a = 0; a < MIX_AUX; a++) {
            const float s = send_[c][a];
            if (s <= 0.f) continue;
            auxUsed[a] = true;
            for (int i = 0; i < n; i++) { auxBuf_[a][0][i] += l_[i] * s; auxBuf_[a][1][i] += r_[i] * s; }
        }
        hold(ch[c].peak, opk);
        ch[c].gr.store(comp_[c].gr(), std::memory_order_relaxed);
    }
    for (int a = 0; a < MIX_AUX; a++) {
        if (auxUsed[a]) auxTail_[a] = (int)(fs_ * 12.f);
        else if (auxTail_[a] > 0) auxTail_[a] -= n;
        if (!auxUsed[a] && (!auxFx_[a].active() || auxTail_[a] <= 0)) { hold(aux[a].peak, 0.f); continue; }  // brak wysylek: AUX pomijany
        float* al = auxBuf_[a][0];
        float* ar = auxBuf_[a][1];
        auxFx_[a].process(al, ar, n, auxW_[a]);
        auxEq_[a].process(al, ar, n);
        float pk = 0;
        for (int i = 0; i < n; i++) {
            const float yl = al[i] * auxLevel_[a], yr = ar[i] * auxLevel_[a];
            ml[i] += yl; mr[i] += yr;
            const float m = fabsf(yl) > fabsf(yr) ? fabsf(yl) : fabsf(yr);
            if (m > pk) pk = m;
        }
        hold(aux[a].peak, pk);
    }
    bus_.process(ml, mr, n);
    float pk = 0;
    for (int i = 0; i < n; i++) {
        float o[2] = {ml[i] * masterVol_, mr[i] * masterVol_};
        for (int c = 0; c < 2; c++) {  // lagodny ogranicznik: powyzej 0,8 miekko do 1,0
            float x = o[c];
            const float a = fabsf(x);
            if (a > .8f) x = copysignf(.8f + .2f * tanhf((a - .8f) * 5.f), x);
            out[i * 2 + c] = x;
            if (fabsf(x) > pk) pk = fabsf(x);
        }
    }
    hold(master.peak, pk);
    master.gr.store(bus_.gr(), std::memory_order_relaxed);
}

}  // namespace dp

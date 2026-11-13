// VELOUR — opakowanie oryginalu PicoFaceSM (zob. velour_dsp.h).
#include "synth/velour_dsp.h"
#include <string.h>
#include <new>
#include "solina/solina.h"

// slowo -> 0..1: 16384 = dokladnie 0,5 (strojenie), 32767 = 1,0 (rejestr wlaczony jak w oryginale)
float velourWordToFloat(int16_t w) { return w <= 0 ? 0.f : (w >= 32767 ? 1.f : w / 32768.f); }

struct VelourCore {
    Solina s;
    int16_t w[VELOUR_PARAMS];
    bool first = true;
};

VelourCore* velourCreate(float sampleRate) {
    VelourCore* v = new (std::nothrow) VelourCore();
    if (!v) return nullptr;
    v->s.setSampleRate(sampleRate);
    memset(v->w, 0, sizeof v->w);
    return v;
}
void velourDestroy(VelourCore* v) { delete v; }

void velourSetParams(VelourCore* v, const int16_t p[VELOUR_PARAMS]) {
    if (!v) return;
    for (int i = 0; i < VELOUR_PARAMS; i++) {
        if (!v->first && p[i] == v->w[i]) continue;
        v->w[i] = p[i];
        v->s.setParameter(i, velourWordToFloat(p[i]));
    }
    v->first = false;
}
void velourNoteOn(VelourCore* v, int pitch) { if (v) v->s.noteOn(pitch, 100); }
void velourNoteOff(VelourCore* v, int pitch) { if (v) v->s.noteOff(pitch); }
void velourAllOff(VelourCore* v) { if (v) v->s.stopVoices(); }
void velourRender(VelourCore* v, float* l, float* r, int n) { v->s.processFloat(l, r, n); }

int velourActiveKeys(const VelourCore* v) { return v ? v->s.keyboard().activeKeys() + (v->s.keyboard().bassEnv() > 0.f ? 1 : 0) : 0; }
float velourEnv(const VelourCore* v) { return v ? v->s.keyboard().envMax() : 0.f; }
float velourBassEnv(const VelourCore* v) { return v ? v->s.keyboard().bassEnv() : 0.f; }

const char* velourProgramName(int i) { return solinaPrograms[i < 0 ? 0 : i % SOLINA_NPROGRAMS].name; }
void velourProgramWords(int i, int16_t out[VELOUR_PARAMS]) {
    const SolinaProgram& p = solinaPrograms[i < 0 ? 0 : i % SOLINA_NPROGRAMS];
    for (int k = 0; k < VELOUR_PARAMS; k++) { const int x = (int)(p.param[k] * 32768.f + .5f); out[k] = (int16_t)(x > 32767 ? 32767 : x); }
}

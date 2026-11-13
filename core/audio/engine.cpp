#include "audio/engine.h"
#include <math.h>
#include <string.h>

namespace dp {

void AudioEngine::init(float sr) {
    sr_ = sr;
    for (int i = 0; i < MAX_TRACKS; i++) { level_[i] = 0; levelOut_[i].store(0.f); }
    seq_.init(sr);
    for (int i = 0; i < MAX_TRACKS; i++) {
        synth_[i].init(sr);
        wt_[i].init(sr);
        eng_[i] = ENG_WAVETABLE;
        sinks_[i] = &wt_[i];
    }
    seq_.setProject(&prj_.front());
}

void AudioEngine::applyProject() {
    const Project& p = prj_.front();
    seq_.setProject(&p);
    seq_.setTempo(p.bpm);
    for (int i = 0; i < MAX_TRACKS; i++) {
        float v = i < p.nTracks ? p.tracks[i].volume : 0.f;
        synth_[i].setVolume(v);
        wt_[i].setVolume(v);
        if (i >= p.nTracks) continue;
        const Track& t = p.tracks[i];
        uint8_t e = t.engine < ENG_COUNT ? t.engine : (uint8_t)ENG_WAVETABLE;
        if (e != eng_[i]) {  // zmiana instrumentu: wycisz stary
            sinks_[i]->allOff();
            eng_[i] = e;
            sinks_[i] = e == ENG_SAW ? (NoteSink*)&synth_[i] : (NoteSink*)&wt_[i];  // WAVETABLE i TAFFY: ten sam silnik glosow
        }
        wt_[i].setKind(e == ENG_TAFFY ? 1 : e == ENG_PEW ? 2 : e == ENG_ELG ? 3 : e == ENG_EVA ? 4 : e == ENG_MBIRA ? 5 : e == ENG_REACTOR ? 6 : e == ENG_OXBOW ? (t.alt ? 8 : 7) : e == ENG_SID ? 9 : e == ENG_MONOLIT ? 10 : e == ENG_VAPORA ? 11 : e == ENG_SUPERSAW ? 12 : e == ENG_MIXTUR ? 13 : e == ENG_BRASS ? 14 : e == ENG_HOLLOW ? 15 : e == ENG_BLOOD ? 16 : e == ENG_BLOT ? 17 : e == ENG_AETHER ? 18 : e == ENG_ARC ? 19 : e == ENG_DDDD ? 20 : e == ENG_JUNI ? 21 : e == ENG_OUTATIME ? 22 : e == ENG_GIN ? 23 : e == ENG_PARALLAX ? 24 : e == ENG_LOUNGE ? 25 : e == ENG_FELT ? 26 : e == ENG_GRAND ? 27 : 0);
        if (e == ENG_GIN) wt_[i].setGin(t.ginKit < 0 ? 0 : t.ginKit, t.ginKit < 0 ? nullptr : t.gin);
        wt_[i].setTilt(t.tilt[0] / 32767.f, t.tilt[1] / 32767.f, t.tilt[2] / 32767.f);
        wt_[i].setRelic(t.relic);
        wt_[i].setOctave(t.octave);
        wt_[i].setSsFilter((32767 - t.push[0]) / 32767.f, t.push[1] / 32767.f, t.push[2]);  // SUPERSAW: push = zamkniecie filtra, RES, typ
        synth_[i].setOctave(t.octave);
        const bool bare = e == ENG_ELG || e == ENG_BRASS || e == ENG_HOLLOW || e == ENG_BLOOD || e == ENG_DDDD || e == ENG_JUNI;  // HOLLOW SAINT: sam silnik 1:1  // BRASSOPHONE: sam silnik 1:1  // ELG: sam silnik 1:1 — tylko obwiednia glosnosci
        const bool noMod = bare || e == ENG_BLOT || e == ENG_OUTATIME;  // OUTATIME: jak BLOT (glide i tryby gry jak w OP-1)  // BLOT: sam silnik + MODE/GLIDE (slide 303 = portamento gospodarza)
        const bool simple = bare || e == ENG_GRAND || e == ENG_FELT || e == ENG_LOUNGE || e == ENG_PARALLAX || e == ENG_MIXTUR || e == ENG_ARC || e == ENG_EVA || e == ENG_MBIRA || e == ENG_REACTOR || e == ENG_SID || e == ENG_VAPORA || (e == ENG_OXBOW && !t.alt);
        const bool sidFenv = e == ENG_SID || e == ENG_VAPORA || e == ENG_PARALLAX;  // PARALLAX: obwiednia filtra na FILTER FREQ  // VAPORA: obwiednia filtra na BRIGHTNESS (galka 2)
         // SID: obwiednia filtra na CUTOFF (galka 2), bez trybow glosu  // OXBOW+: tryby glosu jak WAVETABLE+
        const bool oxPlus = e == ENG_OXBOW && t.alt;  // ELG, EVA: bez drugiej obwiedni i trybow glosu (EVA: MOD ENV i LFO sa)
        wt_[i].setKnobs(t.knob);
        if (oxPlus) {  // OXBOW+: FILTER i DRIVE w slowach 6, 7 (jak strona 2 oryginalu); reszta w obx_params
            const ObExt& o = t.obx;
            const int16_t pw[3] = {0, (int16_t)(o.ftype * 512 + 256), (int16_t)(o.drive * 32767 / 127)};
            wt_[i].setPush(pw);
            obx_params q = OBX_DEFAULTS;
            q.w1 = o.wave[0]; q.w2 = o.wave[1];
            q.semi1 = o.semi[0]; q.semi2 = o.semi[1];
            q.cent8_1 = o.cents1 * 256 / 100; q.cent8_2 = 0;
            q.lvl1 = o.level[0]; q.lvl2 = o.level[1]; q.lvln = o.noise;
            q.pw1 = ((uint32_t)o.pw[0] << 25) + (1u << 24); q.pw2 = ((uint32_t)o.pw[1] << 25) + (1u << 24);
            q.sync = o.sync; q.xmod = o.xmod; q.envo2 = o.envPitch;
            q.keytrk = o.keytrk; q.velflt = o.velflt; q.vint = o.vint;
            q.fa = oxbowPlusAttackIndex(t.fenv[0]); q.fd = oxbowPlusDecayIndex(t.fenv[1]); q.fr = oxbowPlusDecayIndex(t.fenv[3]);
            q.sus = (int32_t)(t.fenv[2] * (float)(1 << 24));
            wt_[i].setObx(q);
        } else wt_[i].setPush(t.push);
        // minimalny atak 1 ms i wybrzmienie 3 ms — bez klikow na poczatku/koncu nuty
        wt_[i].setEnvelope(t.env[0] < 0.001f ? 0.001f : t.env[0], t.env[1], t.env[2], t.env[3] < 0.003f ? 0.003f : t.env[3],
                           t.envAmt);
        wt_[i].setFilterEnv(t.fenv[0] < 0.001f ? 0.001f : t.fenv[0], t.fenv[1], t.fenv[2], t.fenv[3], e == ENG_PEW || e == ENG_BLOT || e == ENG_AETHER || e == ENG_OUTATIME || (simple && !sidFenv) || oxPlus ? 0.f : t.fenvAmt);  // SUPERSAW: obwiednia na filtr DAW-POL  // MONOLIT: obwiednia filtra na CUTOFF (obok CONTOUR)
        wt_[i].setAlt(t.alt && e == ENG_WAVETABLE);
        wt_[i].setMoveSync(e == ENG_ELG && t.movSync, lfoDivBeats(t.movDiv));
        if (simple) wt_[i].setVoice(false, 0.f, 0.f, 0.f);
        else if (e == ENG_MONOLIT || e == ENG_SUPERSAW || e == ENG_BLOT || e == ENG_AETHER || e == ENG_OUTATIME) wt_[i].setVoice(t.mono, glideSeconds(t, p.bpm), 0.f, 0.f);  // MONOLIT, SUPERSAW: tylko MODE i GLIDE
        else wt_[i].setVoice(t.mono, glideSeconds(t, p.bpm), t.unison, t.age);
        wt_[i].setModEnv(t.menv[0] < 0.001f ? 0.001f : t.menv[0], t.menv[1], t.menv[2], t.menv[3], noMod ? 0.f : t.menvAmt, t.menvDest);
        {
            const WtExt& x = t.ext;
            wt_[i].setExt(x.osc[1].wave, x.osc[0].semi, x.osc[1].semi, x.osc[0].cents, x.osc[1].cents, x.osc[0].level,
                          x.osc[1].level, x.sub, x.fmode, x.drive);
        }
        for (int l = 0; l < 2; l++)
            wt_[i].setLfo(l, t.lfo[l].shape, t.lfo[l].rate, noMod || ((e == ENG_MIXTUR || e == ENG_ARC) && l == 1) ? 0.f : t.lfo[l].depth,  // MIXTUR: jedno LFO
                          t.lfo[l].dest, t.lfo[l].sync,
                          lfoDivBeats(t.lfo[l].div));
    }
    applyMutes();
    master_ = p.masterVol;
}

// Mute/solo dziala w trybie SONG (w piano rollu slychac edytowana sciezke zawsze).
void AudioEngine::applyMutes() {
    const Project& p = prj_.front();
    bool song = seq_.mode() == Sequencer::SONG;
    for (int i = 0; i < p.nTracks && i < MAX_TRACKS; i++) {
        float v = !song || p.audible(i) ? p.tracks[i].volume : 0.f;
        synth_[i].setVolume(v);
        wt_[i].setVolume(v);
    }
}

void AudioEngine::process(float* out, int frames) {
    memset(out, 0, sizeof(float) * 2 * frames);
    if (prj_.consume()) applyProject();
    EngineCmd c;
    while (q_.pop(c)) {
        switch (c.type) {
        case EC_NOTE_ON: if (c.track < MAX_TRACKS) sinks_[c.track]->noteOn(c.pitch, c.value); break;
        case EC_NOTE_OFF: if (c.track < MAX_TRACKS) sinks_[c.track]->noteOff(c.pitch); break;
        case EC_PLAY_SONG: seq_.playSong(sinks_); applyMutes(); break;
        case EC_PLAY_PATTERN: seq_.playPattern(c.pattern, c.track, sinks_); applyMutes(); break;
        case EC_STOP: seq_.stop(sinks_); break;
        }
    }
    int pos = 0;
    while (pos < frames) {
        int n = seq_.samplesToNextTick();
        if (n > frames - pos) n = frames - pos;
        const bool pl = seq_.playing();
        const double bp = seq_.beatPos();
        for (int i = 0; i < MAX_TRACKS; i++) {
            synth_[i].render(out + pos * 2, n);
            wt_[i].setTransport(pl, bp, seq_.tempo());
            wt_[i].render(out + pos * 2, n);
        }
        seq_.advance(n, sinks_);
        pos += n;
    }
    // Master + lagodny ogranicznik (powyzej 0.8 miekko dobija do 1.0, zamiast trzeszczec)
    for (int i = 0; i < frames * 2; i++) {
        float x = out[i] * master_, a = fabsf(x);
        if (a > 0.8f) x = copysignf(0.8f + 0.2f * tanhf((a - 0.8f) * 5.f), x);
        out[i] = x;
    }
    // VU: szczyt bloku, opadanie ~24 dB/s
    float fall = powf(10.f, -24.f / 20.f * frames / sr_);
    for (int i = 0; i < MAX_TRACKS; i++) {
        float pk = synth_[i].takePeak(), pw = wt_[i].takePeak();
        if (pw > pk) pk = pw;
        level_[i] = pk > level_[i] * fall ? pk : level_[i] * fall;
        levelOut_[i].store(level_[i], std::memory_order_relaxed);
    }
    tickOut_.store(seq_.tick(), std::memory_order_relaxed);
    modeOut_.store((int)seq_.mode(), std::memory_order_relaxed);
    playingOut_.store(seq_.playing(), std::memory_order_relaxed);
}

}  // namespace dp

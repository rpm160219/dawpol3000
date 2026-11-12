// WAVETABLE — port 1:1 silnika "wavetable" z op1r (Hundekraft, GPL-3.0-or-later; third_party/op1r_wavetable).
// DSP bez zmian (sciezka C z wavetable_dsp.h, liczby calkowite): 2 oscylatory wavetable z morfingiem 8 ksztaltow,
// rozstrojenie, 4-biegunowy filtr ladder z rezonansem, soft clip, statyczna kompensacja glosnosci.
// Dodatki DAW-POL (poza oryginalem, przez galki silnika): obwiednia filtra na glos, 2 LFO na sciezke,
// wyciszanie ~2 ms przy przejmowaniu brzmiacego glosu (bez klikow). Przy zerowych ustawieniach — bit w bit jak oryginal.
// Tryb rozszerzony (setAlt): wlasny DSP wzorowany na wavetable_sample (te same tabele, arytmetyka, ladder, soft clip,
// kompensacja): 2 niezalezne oscylatory (fala, interwal, rozstrojenie, poziom), sub (sinus -1 okt.), tryby filtra
// LP24 (= oryginal) / LP12 / BP / HP z wyjsc stopni ladderu, dodatkowy drive.
// Jak na OP-1 R246: glos liczony z 2x nadprobkowaniem (u nas 96 kHz), bloki po 128 probek, decymacja 2:1
// (63-tapowy polpasmowy FIR z ich harnessa). ADSR i velocity dawal firmware OP-1 — tu wlasna obwiednia.
// Galki (0..32767): 0 WAVE (niebieski), 1 DETUNE (zielony), 2 CUTOFF (bialy), 3 RESONANCE (czerwony).
#pragma once
#include "synth/gin_dsp.h"
#include <atomic>
#include <stdint.h>
#include "synth/note_sink.h"
#include "../../third_party/kennel_oxbow/plugins/oxbow/oxbow_plus.h"

extern "C" { struct EvaCore; struct MbiraCore; struct ReactorCore; struct OxbowCore; struct OxbowPlusCore; struct SidCore; struct MonolitCore; struct VaporaCore; struct SupersawCore; struct MixturCore; struct BrassCore; struct HollowCore; struct BloodCore; struct BlotCore; struct AetherCore; struct ArcCore; struct DdddCore; struct JuniCore; struct OutatimeCore; struct GinCore; struct ParallaxCore; }
namespace dp {
struct PewCore;
struct ElgCore;


class WavetableSynth : public NoteSink {
public:
    static const int VOICES = 6;  // jak OP-1 (ENGINE_VOICES)
    static const int BLOCK = 128; // probek glosu na blok (WAVETABLE_BLOCK)
    void init(float outRate);     // glos = 2 x outRate
    void setKnobs(const int16_t k[4]);
    // Obwiednia: czasy w sekundach, sustain 0..1. Czas 0 = natychmiast.
    void setEnvelope(float a, float d, float s, float r, float amount = 1.f);
    void setVolume(float v) { volT_ = v; }
    // Glosy: MONO (stos nut, legato), portamento (s), unison (centy, tylko MONO: 6 glosow na jedna nute), AGE 0..1
    void setVoice(bool mono, float glide, float unison, float age);
    // Rodzaj DSP glosu: 0 = WAVETABLE (oryginal/rozszerzony), 1 = TAFFY (CZ, op1r). Reszta (obwiednie, LFO,
    // glosy, decymacja) wspolna. "Jasnosc" (cel obwiedni filtra i AGE): WAVETABLE = galka 2 (CUTOFF), TAFFY = 1 (DCW).
    void setKind(int k);  // 0 WAVETABLE, 1 TAFFY, 2 PEW, 3 ELG, 4 EVA, 5 MBIRA, 6 REACTOR, 7 OXBOW, 8 OXBOW+, 9 SID, 10 MONOLIT
    // SUPERSAW: filtr za oryginalnym silnikiem (DAW-POL). cut 0..1 (1 = otwarty: bez filtra, brzmienie 1:1), res 0..1,
    // mode 0 LP24, 1 LP12, 2 BP, 3 HP. Modulacja: obwiednia filtra i cele CUT (W1) / RES (W2).
    void setSsFilter(float cut, float res, int mode) { ssCut_ = cut; ssRes_ = res; ssMode_ = mode & 3; }
    void setOctave(int o) { octSemi_ = 12 * (o < -3 ? -3 : (o > 3 ? 3 : o)); }  // OCTAVE instrumentu (-3..+3): tylko wysokosc dzwieku, nuty klawiszy bez zmian
    void setPush(const int16_t p[3]) { for (int i = 0; i < 3; i++) knobs_[5 + i] = p[i] < 0 ? 0 : p[i]; }  // PEW: strona 2
    const PewCore* pew() const { return pew_; }
    const ElgCore* elg() const { return elg_; }
    const ::EvaCore* eva() const { return eva_; }
    const ::MbiraCore* mbira() const { return mbira_; }
    const ::ReactorCore* reactor() const { return reactor_; }
    ::OxbowCore* oxbow() const { return oxbow_; }
    const ::OxbowPlusCore* oxbowPlus() const { return oxp_; }
    const ::MonolitCore* monolit() const { return mnl_; }
    const ::VaporaCore* vapora() const { return vpr_; }
    const ::SupersawCore* supersaw() const { return ssw_; }
    const ::MixturCore* mixtur() const { return mxt_; }
    const ::BrassCore* brass() const { return brs_; }
    const ::HollowCore* hollow() const { return hls_; }
    const ::BloodCore* blood() const { return bld_; }
    const ::BlotCore* blot() const { return blt_; }
    const ::AetherCore* aether() const { return aet_; }
    const ::ArcCore* arc() const { return arc_; }
    const ::DdddCore* dddd() const { return ddd_; }
    const ::JuniCore* juni() const { return jun_; }
    const ::OutatimeCore* outatime() const { return ota_; }
    const ::GinCore* gin() const { return gin_; }
    void setGin(int kit, const GinSound* s);  // GIN: zestaw sciezki (przygotowuje zmienione dzwieki)
    void setTilt(float p, float v, float s) { tilt_[0] = p; tilt_[1] = v; tilt_[2] = s; }  // AETHER: wirtualny czujnik
    void setRelic(bool on) { relic_ = on; }  // HOLLOW SAINT: tasma Lo-Fi Relic
    void setObx(const obx_params& q) { obx_ = q; }  // OXBOW+: parametry sciezki
    int kind() const { return kind_; }
    int16_t taffyEnv() const { return tfEnv_.load(std::memory_order_relaxed); }  // obwiednia barwy (ekran TAFFY)
    int16_t voiceLevel() const { return vLev_.load(std::memory_order_relaxed); }  // obwiednia x velocity, Q15
    // Tryb rozszerzony: wl/wyl + parametry (fala OSC 1 = galka 0; CUTOFF/RES = galki 2/3)
    void setAlt(bool on) { alt_ = on; }
    void setExt(int16_t wave2, int semi1, int semi2, float cents1, float cents2, float lvl1, float lvl2, float sub,
                int fmode, float drive);
    // Obwiednia filtra: a, d, r w s; s 0..1; amount -1..1 (ulamek zakresu CUTOFF)
    void setFilterEnv(float a, float d, float s, float r, float amount);
    // Obwiednia modulacji: a, d, r w s; s 0..1; amount -1..1; cel (LfoDest)
    void setModEnv(float a, float d, float s, float r, float amount, int dest);
    // LFO i (0/1): ksztalt (LfoShape), Hz, glebokosc 0..1, cel (LfoDest); sync: cykl = beats cwiercnut
    void setLfo(int i, int shape, float rate, float depth, int dest, bool sync = false, float beats = 1.f);
    // Transport (watek audio, przed kazdym odcinkiem): czy gra, pozycja w cwiercnutach, tempo
    void setMoveSync(bool on, float beats) { movSync_ = on; movBeats_ = beats > 0 ? beats : 1.f; }  // ELG: ruch z tempem
    void setTransport(bool playing, double beatPos, float bpm) { tPlaying_ = playing; tBeat_ = beatPos; tBpm_ = bpm; }
    void noteOn(int pitch, float vel) override;
    void noteOff(int pitch) override;
    void allOff() override;
    void render(float* out, int n);  // DODAJE n ramek stereo
    float takePeak() { float p = peak_; peak_ = 0; return p; }
    // Dla ekranu (watek UI): roznica faz oscylatorow najglosniejszego glosu, liczba aktywnych glosow.
    uint32_t drift() const { return drift_.load(std::memory_order_relaxed); }
    int activeVoices() const { return active_.load(std::memory_order_relaxed); }
    // Galki po modulacji (ostatnio uderzona nuta) — ekran pokazuje ruch LFO/obwiedni filtra
    int16_t modKnob(int i) const { return modKnob_[i].load(std::memory_order_relaxed); }  // 0..3 galki, 4 = fala OSC 2
    // Do testu zgodnosci z oryginalem: ostatni blok po decymacji (przed skalowaniem), BLOCK/2 wartosci.
    const double* lastDecimated() const { return dec_; }

private:
    enum { OFF, ATTACK, DECAY, SUSTAIN, RELEASE, FADE };
    struct Voice {
        int pitch = -1, stage = OFF, vel = 127;
        uint32_t inc = 0, age = 0;
        float env = 0;
        int pendPitch = -1;   // nuta czekajaca na koniec wyciszania (FADE)
        float pendVel = 0, pendDet = 0, pendFrom = -1;
        float pitchCur = 60, pitchTgt = 60;  // wysokosc (poltony) — portamento
        float det = 0;                       // rozstrojenie unison (centy)
        float ageP = 0, ageTp = 0, ageC = 0, ageTc = 0;  // dryf AGE (losowy, powolny)
        float statP = 0, statC = 0;                      // staly "charakter" glosu dla AGE
        int fstage = OFF;     // obwiednia filtra (licz. raz na blok)
        float fenv = 0;
        float gate = 0;       // 0..1, rampa 3 ms (dla AMOUNT obwiedni glosnosci)
        int mstage = OFF;     // obwiednia modulacji (na blok)
        float menv = 0, mvolPrev = 1;  // mvolPrev: interpolacja celu VOL probka po probce
    };
    struct LfoState {
        int shape = 0, dest = 2;
        float rate = 2.f, depth = 0.f, beats = 1.f;
        bool sync = false;
        double ph = 0;
        float rnd = 0, out = 0;  // out = wartosc wygladzona (~3 ms), bez skokow na krawedziach SAW/SQR
        uint32_t seed = 22222;
    };
    bool tPlaying_ = false;
    double tBeat_ = 0;
    float tBpm_ = 120.f;
    float tremPrev_ = 1.f;  // tremolo z poprzedniego bloku (interpolacja probka po probce)
    LfoState lfo_[2];
    float fA_ = 0, fD_ = 0, fS_ = 0, fR_ = 1, fAmt_ = 0;  // obwiednia filtra (przyrost/wspolczynnik na blok)
    float envAmt_ = 1.f, gateK_ = 0;
    float mA_ = 2, mD_ = 1, mS_ = 0, mR_ = 1, mAmt_ = 0;
    int mDest_ = 4;
    float fade_ = 0;                            // przyrost wyciszania na probke
    std::atomic<int16_t> modKnob_[5];
    // tryb rozszerzony
    bool alt_ = false;
    int kind_ = 0, tfBase_ = -1;
    PewCore* pew_ = nullptr;
    ElgCore* elg_ = nullptr;
    ::EvaCore* eva_ = nullptr;
    ::MbiraCore* mbira_ = nullptr;
    ::ReactorCore* reactor_ = nullptr;
    ::OxbowCore* oxbow_ = nullptr;
    ::OxbowPlusCore* oxp_ = nullptr;
    ::SidCore* sid_ = nullptr;
    ::ParallaxCore* prx_ = nullptr;  // PARALLAX: glosy tej sciezki (C)
    ::GinCore* gin_ = nullptr;  // GIN: zestaw perkusyjny tej sciezki (C)
    void renderGin(float* out, int n);
    ::OutatimeCore* ota_ = nullptr;  // OUTATIME: glosy tej sciezki (C)
    ::JuniCore* jun_ = nullptr;  // JUNI: stan silnika tej sciezki (C)
    ::DdddCore* ddd_ = nullptr;  // DDDD: cala sciana tej sciezki (C)
    ::ArcCore* arc_ = nullptr;  // ARC: glosy tej sciezki (C)
    ::AetherCore* aet_ = nullptr;  // AETHER: glosy tej sciezki (C)
    float tilt_[3] = {0, 0, 0};
    ::BlotCore* blt_ = nullptr;  // BLOT: glosy tej sciezki (C)
    ::BloodCore* bld_ = nullptr;  // BLOODMOON: glosy tej sciezki (C)
    ::HollowCore* hls_ = nullptr;  // HOLLOW SAINT: caly stan silnika tej sciezki (C)
    bool relic_ = false;
    ::BrassCore* brs_ = nullptr;  // BRASSOPHONE: glosy tej sciezki (C)
    ::MixturCore* mxt_ = nullptr;  // MIXTUR: glosy tej sciezki (C)
    ::SupersawCore* ssw_ = nullptr;  // SUPERSAW: glosy tej sciezki (C)
    ::VaporaCore* vpr_ = nullptr;  // VAPORA: glosy tej sciezki (C)
    ::MonolitCore* mnl_ = nullptr;  // MONOLIT: glosy tej sciezki (C)  // SID: glosy tej sciezki (C)  // OXBOW+: glosy tej sciezki (C)
    obx_params obx_ = OBX_DEFAULTS;  // OXBOW: glosy tej sciezki (C)  // REACTOR: glosy tej sciezki (C)  // MBIRA: glosy tej sciezki (C)  // EVA: glosy tej sciezki (C)
    bool movSync_ = false;
    float movBeats_ = 4.f;
    double movPh_ = 0;  // faza ruchu ELG przy synchronizacji (0..1)  // ELG: glosy tej sciezki  // PEW: organy tej sciezki (tworzone w init, poza watkiem audio)
    std::atomic<int16_t> tfEnv_{0}, vLev_{0};
    int16_t wave2_ = 16384;
    int semi_[2] = {0, 0}, fmode_ = 0;
    float cents_[2] = {-4.f, 4.f}, lvl_[2] = {1.f, 1.f}, sub_ = 0.f, drive_ = 0.f;
    alignas(8) unsigned char sx_[VOICES][128];  // stan glosu trybu rozszerzonego
    void renderExtVoice(int vi, int16_t* out, int16_t* vk, const float* mod, uint32_t inc, int vel);
    void startVoice(int slot, int pitch, float vel, float det = 0.f, float from = -1.f);  // from = start glide
    bool mono_ = false;
    float glide_ = 0, glideK_ = 1, unison_ = 0, age_ = 0;
    int lastPitch_ = -1;
    int stack_[16];
    int nStack_ = 0;
    uint32_t ageSeed_ = 12345;
    float rnd();  // -1..1
    void monoOn(int pitch, float vel);
    void monoOff(int pitch);
    void setPitchTarget(Voice& v, int pitch);
    float lfoValue(LfoState& l);
    void advanceLfo(int outFrames);  // LFO chodzi w czasie rzeczywistym, tez gdy nic nie gra
    float outRate_ = 48000.f, voiceRate_ = 96000.f;
    Voice v_[VOICES];
    alignas(8) unsigned char st_[VOICES][96];  // WavetableState z wavetable_dsp.h (rozmiar sprawdzany w .cpp)
    int octSemi_ = 0;
    float ssCut_ = 1.f, ssRes_ = 0.f;
    int ssMode_ = 0;
    struct SsFilt { float s1, s2, s3, s4, cut, res; bool ready; } ssf_[6] = {};
    void ssFilter(int vi, const int16_t* in, float* out, float cut, float res);
    int16_t knobs_[8] = {16384, 8192, 20000, 8192, 0, 0, 0, 0};
    float atk_ = 0, dec_k_ = 0, sus_ = 1, rel_k_ = 0;  // przyrosty/wspolczynniki na probke glosu

    int flush_ = 0;                                    // bloki ciszy do wyplukania decymatora
    uint32_t age_counter_ = 0;  // kolejnosc uderzen (kradziez najstarszego glosu)
    // decymator 2:1 (FIR jak w harnessie op1r, w double — wynik zgodny bit w bit z ich renderem)
    static const int TAPS = 63;
    double h_[TAPS], z_[2 * TAPS];  // bufor kolowy zapisany podwojnie: z_[zp_ + n] = n-ta najnowsza probka
    int zp_ = 0;
    double dec_[BLOCK / 2];
    float fifo_[BLOCK / 2];
    int fifoPos_ = BLOCK / 2;
    float vol_ = 0.8f, volT_ = 0.8f, volK_ = 0, peak_ = 0;
    std::atomic<uint32_t> drift_{0};
    std::atomic<int> active_{0};
    void renderBlock();
};

// Morf fali przy galce WAVE k0 (0..32767) i fazie ph — dla ekranu (wavetable_dsp_shape).
int32_t wavetableShape(int32_t k0, uint32_t ph);

}  // namespace dp

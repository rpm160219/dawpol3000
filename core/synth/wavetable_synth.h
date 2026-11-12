// WAVETABLE — port 1:1 silnika "wavetable" z op1r (Hundekraft, GPL-3.0-or-later; third_party/op1r_wavetable).
// DSP bez zmian (sciezka C z wavetable_dsp.h, liczby calkowite): 2 oscylatory wavetable z morfingiem 8 ksztaltow,
// rozstrojenie, 4-biegunowy filtr ladder z rezonansem, soft clip, statyczna kompensacja glosnosci.
// Dodatki DAW-POL (poza oryginalem, przez galki silnika): obwiednia filtra na glos, 2 LFO na sciezke,
// wyciszanie ~2 ms przy przejmowaniu brzmiacego glosu (bez klikow). Przy zerowych ustawieniach — bit w bit jak oryginal.
// Jak na OP-1 R246: glos liczony z 2x nadprobkowaniem (u nas 96 kHz), bloki po 128 probek, decymacja 2:1
// (63-tapowy polpasmowy FIR z ich harnessa). ADSR i velocity dawal firmware OP-1 — tu wlasna obwiednia.
// Galki (0..32767): 0 WAVE (niebieski), 1 DETUNE (zielony), 2 CUTOFF (bialy), 3 RESONANCE (czerwony).
#pragma once
#include <atomic>
#include <stdint.h>
#include "synth/note_sink.h"

namespace dp {


class WavetableSynth : public NoteSink {
public:
    static const int VOICES = 6;  // jak OP-1 (ENGINE_VOICES)
    static const int BLOCK = 128; // probek glosu na blok (WAVETABLE_BLOCK)
    void init(float outRate);     // glos = 2 x outRate
    void setKnobs(const int16_t k[4]);
    // Obwiednia: czasy w sekundach, sustain 0..1. Czas 0 = natychmiast.
    void setEnvelope(float a, float d, float s, float r);
    void setVolume(float v) { volT_ = v; }
    // Obwiednia filtra: a, d w s; s 0..1; amount -1..1 (ulamek zakresu CUTOFF)
    void setFilterEnv(float a, float d, float s, float amount);
    // LFO i (0/1): ksztalt (LfoShape), Hz, glebokosc 0..1, cel (LfoDest)
    void setLfo(int i, int shape, float rate, float depth, int dest);
    void noteOn(int pitch, float vel) override;
    void noteOff(int pitch) override;
    void allOff() override;
    void render(float* out, int n);  // DODAJE n ramek stereo
    float takePeak() { float p = peak_; peak_ = 0; return p; }
    // Dla ekranu (watek UI): roznica faz oscylatorow najglosniejszego glosu, liczba aktywnych glosow.
    uint32_t drift() const { return drift_.load(std::memory_order_relaxed); }
    int activeVoices() const { return active_.load(std::memory_order_relaxed); }
    // Galki po modulacji (najglosniejszy glos) — ekran pokazuje ruch LFO/obwiedni filtra
    int16_t modKnob(int i) const { return modKnob_[i].load(std::memory_order_relaxed); }
    // Do testu zgodnosci z oryginalem: ostatni blok po decymacji (przed skalowaniem), BLOCK/2 wartosci.
    const double* lastDecimated() const { return dec_; }

private:
    enum { OFF, ATTACK, DECAY, SUSTAIN, RELEASE, FADE };
    struct Voice {
        int pitch = -1, stage = OFF, vel = 127;
        uint32_t inc = 0, age = 0;
        float env = 0;
        int pendPitch = -1;   // nuta czekajaca na koniec wyciszania (FADE)
        float pendVel = 0;
        int fstage = OFF;     // obwiednia filtra (licz. raz na blok)
        float fenv = 0;
    };
    struct LfoState { int shape = 0, dest = 2; float rate = 2.f, depth = 0.f; double ph = 0; float rnd = 0; uint32_t seed = 22222; };
    LfoState lfo_[2];
    float fA_ = 0, fD_ = 0, fS_ = 0, fAmt_ = 0;  // obwiednia filtra (przyrost/wspolczynnik na blok)
    float fade_ = 0;                            // przyrost wyciszania na probke
    std::atomic<int16_t> modKnob_[4];
    void startVoice(int slot, int pitch, float vel);
    float lfoValue(LfoState& l);
    float outRate_ = 48000.f, voiceRate_ = 96000.f;
    Voice v_[VOICES];
    alignas(8) unsigned char st_[VOICES][96];  // WavetableState z wavetable_dsp.h (rozmiar sprawdzany w .cpp)
    int16_t knobs_[8] = {16384, 8192, 20000, 8192, 0, 0, 0, 0};
    float atk_ = 0, dec_k_ = 0, sus_ = 1, rel_k_ = 0;  // przyrosty/wspolczynniki na probke glosu
    float relT_ = 0.3f;                                // czas wybrzmienia (s) — tez dla obwiedni filtra
    int flush_ = 0;                                    // bloki ciszy do wyplukania decymatora
    uint32_t age_ = 0;
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

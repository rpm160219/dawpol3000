// WAVETABLE — port 1:1 silnika "wavetable" z op1r (Hundekraft, GPL-3.0-or-later; third_party/op1r_wavetable).
// DSP bez zmian (sciezka C z wavetable_dsp.h, liczby calkowite): 2 oscylatory wavetable z morfingiem 8 ksztaltow,
// rozstrojenie, 4-biegunowy filtr ladder z rezonansem, soft clip, statyczna kompensacja glosnosci.
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
    void noteOn(int pitch, float vel) override;
    void noteOff(int pitch) override;
    void allOff() override;
    void render(float* out, int n);  // DODAJE n ramek stereo
    float takePeak() { float p = peak_; peak_ = 0; return p; }
    // Dla ekranu (watek UI): roznica faz oscylatorow najglosniejszego glosu, liczba aktywnych glosow.
    uint32_t drift() const { return drift_.load(std::memory_order_relaxed); }
    int activeVoices() const { return active_.load(std::memory_order_relaxed); }
    // Do testu zgodnosci z oryginalem: ostatni blok po decymacji (przed skalowaniem), BLOCK/2 wartosci.
    const double* lastDecimated() const { return dec_; }

private:
    enum { OFF, ATTACK, DECAY, SUSTAIN, RELEASE };
    struct Voice {
        int pitch = -1, stage = OFF, vel = 127;
        uint32_t inc = 0, age = 0;
        float env = 0;
    };
    float outRate_ = 48000.f, voiceRate_ = 96000.f;
    Voice v_[VOICES];
    alignas(8) unsigned char st_[VOICES][96];  // WavetableState z wavetable_dsp.h (rozmiar sprawdzany w .cpp)
    int16_t knobs_[8] = {16384, 8192, 20000, 8192, 0, 0, 0, 0};
    float atk_ = 0, dec_k_ = 0, sus_ = 1, rel_k_ = 0;  // przyrosty/wspolczynniki na probke glosu
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

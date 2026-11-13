// Mikser DAW-POL (watek audio): kazdy kanal = inserty (4) -> EQ 3-pasmowy -> kompresor (OPTO / VCA / FET) -> tlumik +
// panorama -> szyna master, wysylki A..F po tlumiku na 6 szyn AUX (efekt -> EQ -> poziom -> master), master: kompresor
// szyny (w stylu SSL G) -> glosnosc -> lagodny ogranicznik. Wszystko float, bez alokacji w process (efekty tworzone
// przy zmianie typu w apply — wywolywane z watku audio przy publikacji projektu, alokacja jednorazowa).
#pragma once
#include <atomic>
#include <stdint.h>
#include "seq/project.h"

namespace dp {

float eqResponseDb(const MixEq& e, float hz, float fs);  // odpowiedz EQ (dB) — podglad i okno EQ (watek UI)

class Eq3 {
public:
    void set(const MixEq& e, float fs);
    void process(float* l, float* r, int n);
    void reset();
    float responseDb(float hz) const;  // bez stopnia wyjscia
private:
    struct Bq { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; float z[2][2] = {{0, 0}, {0, 0}}; };
    Bq b_[3];
    MixEq cur_;
    bool active_ = false, max_ = false;
    float outG_ = 1.f;
    float fs_ = 0;
};

class Compressor {  // kanal: modele OPTO / VCA / FET
public:
    void set(const MixComp& c, float fs);
    void process(float* l, float* r, int n);
    float gr() const { return grDb_; }  // obecne sciszenie (dB, >= 0)
private:
    MixComp c_;
    float fs_ = 48000.f;
    float env_ = 0, grLin_ = 1.f, grDb_ = 0, charge_ = 0, rms_ = 0;
};

class BusCompressor {
public:
    void set(const BusComp& b, float fs);
    void process(float* l, float* r, int n);
    float gr() const { return grDb_; }
private:
    BusComp b_;
    float fs_ = 48000.f;
    float env_ = 0, grDb_ = 0, slow_ = 0;
};

class FxHost {  // efekt op1r: 48 kHz <-> 44,1 kHz, bloki 64 probek int16 stereo
public:
    ~FxHost();
    void setType(int fx);
    int type() const { return type_; }
    void process(float* l, float* r, int n, const int16_t* w);
    // telemetria ekranu efektu (kopia po kazdym bloku; czytana w watku UI — wylacznie do rysowania)
    const uint8_t* telemetry() const { return tel_; }
private:
    alignas(32) uint8_t tel_[64] = {};
    int type_ = FX_NONE;
    void* inst_ = nullptr;
    // wejscie 48 kHz -> 44,1 kHz
    float inL_[4] = {}, inR_[4] = {};  // ostatnie 4 probki (interpolacja Hermite'a)
    double posIn_ = 0;                 // pozycja wyjscia 44,1 kHz w probkach wejscia (wzgledem inL_[1])
    int16_t blkL_[64], blkR_[64];
    int blkN_ = 0;
    // wyjscie 44,1 kHz -> 48 kHz (kolejka)
    float midL_[512], midR_[512];
    int midN_ = 0;
    double posOut_ = 1.0;
};

struct ChannelMeter { std::atomic<float> peak{0.f}, gr{0.f}; };

class MixerDsp {
public:
    void init(float fs);
    void apply(const Project& p);  // parametry z projektu (watek audio, po publikacji)
    void setGain(int ch, float g) { if (ch >= 0 && ch < MAX_TRACKS) gainT_[ch] = g; }
    // tracks: bufory przeplatane L R (n ramek) kazdej sciezki; out: przeplatane L R (nadpisywane)
    void process(float* const* tracks, const bool* active, int nTracks, float* out, int n);
    ChannelMeter ch[MAX_TRACKS], aux[MIX_AUX], master;
    const FxHost& insert(int c, int k) const { return ins_[c][k]; }
    const FxHost& auxFx(int a) const { return auxFx_[a]; }
private:
    float fs_ = 48000.f;
    int nTracks_ = 0;
    Eq3 eq_[MAX_TRACKS], auxEq_[MIX_AUX];
    Compressor comp_[MAX_TRACKS];
    BusCompressor bus_;
    FxHost ins_[MAX_TRACKS][MIX_INSERTS], auxFx_[MIX_AUX];
    int16_t insW_[MAX_TRACKS][MIX_INSERTS][8], auxW_[MIX_AUX][8];
    float pan_[MAX_TRACKS] = {}, send_[MAX_TRACKS][MIX_AUX] = {}, auxLevel_[MIX_AUX] = {};
    float gainT_[MAX_TRACKS] = {}, gain_[MAX_TRACKS] = {};
    float masterVol_ = .8f;
    int tail_[MAX_TRACKS] = {};
    int auxTail_[MIX_AUX] = {};  // ramki ogona efektu AUX po ostatniej wysylce
    float auxBuf_[MIX_AUX][2][256];
    float l_[256], r_[256];
};

}  // namespace dp

// Ekran MIXER: 5 kanalow na strone (sciezki, potem powroty AUX A..F) + staly master. Kanal: VU (luk ze wskazowka),
// podglad EQ (krzywa), podglad kompresora (model + sciszanie), 6 wysylek AUX (galeczka + miernik sygnalu wysylki),
// 4 inserty, panorama, glosnosc (duza liczba w kolorze sciezki), nazwa. A na podgladzie / insercie otwiera okno: EQ (3 pasma: wzmocnienie, czestotliwosc, Q; MAX
// EQ, CLEAN, wyjscie), kompresor (OPTO / VCA / FET), efekt (ekran efektu op1r + galki; X = lista wyboru), kompresor szyny master (BUS).
#pragma once
#include "ui/view.h"
#include "ui/encoder.h"

namespace dp {

class MixerView : public View {
public:
    void update(const Buttons& b, const InputState& in, float dt) override;
    void render(Canvas& c) override;
    bool modal() const { return win_ != W_NONE; }  // okno otwarte: L2/R2 nie przelaczaja zakladek
    void setCursor(int strip, int row) { strip_ = strip; row_ = row; }  // (headless)
    void openWindow(int w, int slot = 0);                            // (headless)
    enum Win { W_NONE, W_EQ, W_COMP, W_FX, W_BUS };
    enum Row { R_EQ, R_COMP, R_SEND0, R_INS0 = R_SEND0 + 6, R_PAN = R_INS0 + 4, R_VOL, R_COUNT };
private:
    int strip_ = 0, row_ = R_VOL;  // strip_: 0.. kanaly + AUX, -1 = master
    int win_ = W_NONE, slot_ = 0, eqLayer_ = 0, fxSel_ = 0, fxPage_ = 0, sendSel_ = 0;
    bool fxList_ = false, fxReset_ = true;
    bool compAdv_ = false;                // kompresory: panel zaawansowany (L3); domyslnie animowana scena
    float press_ = 0, pressV_ = 0, wob_ = 0, lvl_ = 0;  // animacja prasy FET  // okno efektu: lista wyboru / edycja (ekran efektu)
    float vu_[64], vuM_ = -40.f;
    bool vuInit_ = false;
    RelEncoder stepEnc_[4];
    bool rowValid(int strip, int row) const;
    void moveRow(int d);
    void updateMain(const Buttons& b, const InputState& in, float dt);
    void updateEq(const Buttons& b, const InputState& in, float dt);
    void updateComp(const Buttons& b, const InputState& in, float dt);
    void updateBus(const Buttons& b, const InputState& in, float dt);
    void updateFx(const Buttons& b, const InputState& in, float dt);
    void renderStrip(Canvas& c, int strip, float x, float w, float dt);
    void renderMaster(Canvas& c, float x, float w);
    void renderEq(Canvas& c);
    void renderComp(Canvas& c);
    void renderBus(Canvas& c);
    void renderFx(Canvas& c);
    void renderFxList(Canvas& c);
    void renderFetPress(Canvas& c, const float* q, float gr, float level, bool on, Color col);
    void renderVcaDam(Canvas& c, const float* q, float gr, float level, bool on, Color col);
    void renderOptoLight(Canvas& c, const float* q, float gr, float level, bool on, Color col);
    float dt_ = .016f;
    int lastPage_ = 0;
};

}  // namespace dp

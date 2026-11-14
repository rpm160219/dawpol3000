#pragma once
#include "gfx/canvas.h"
#include "synth/prophet_synth.h"
namespace dp {
class ProphetScreen {
  public:
    void draw(Canvas &, const int16_t *, const ProphetView &, float dt);

  private:
    float time_ = 0, eye_ = 1, flash_ = 0, lastAmp_ = 0, shimmerClock_ = 0;
    uint32_t rng_ = 0x501fead1u;
    float shimmer_[6] = {1, 1, 1, 1, 1, 1}, target_[6] = {1, 1, 1, 1, 1, 1};
    float random();
};
void drawProphetWheel(Canvas &, const int16_t *, float x, float y, float w, float h);
void drawProphetIllustration(Canvas &, float x, float y, float w, float h);
} // namespace dp

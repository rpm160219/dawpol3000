#pragma once
#include "gfx/canvas.h"
#include "synth/prophet_synth.h"
namespace dp {
class ProphetScreen {
  public:
    void draw(Canvas &, const int16_t *, const ProphetView &, float dt);

  private:
    float time_ = 0;
};
void drawProphetIllustration(Canvas &, float x, float y, float w, float h);
} // namespace dp

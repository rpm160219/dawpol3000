#pragma once
#include "gfx/canvas.h"
#include "synth/polivoks_synth.h"
namespace dp {
class PolivoksScreen {
public:
    void draw(Canvas& c,const int16_t* params,const PolivoksView& view,float dt);
private:
    float jaw_ = 0;
};
void drawPolivoksIllustration(Canvas& c,float x,float y,float w,float h);
}

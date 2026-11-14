#pragma once
#include "gfx/canvas.h"
#include "synth/polivoks_synth.h"
namespace dp {
class PolivoksScreen {
public:
    ~PolivoksScreen() { delete cache_; }
    void draw(Canvas& c,const int16_t* params,const PolivoksView& view,float dt);
private:
    Canvas* cache_ = nullptr;
    float time_ = 0, jaw_ = 0;
};
void drawPolivoksIllustration(Canvas& c,float x,float y,float w,float h);
}

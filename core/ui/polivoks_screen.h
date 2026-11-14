#pragma once
#include "gfx/canvas.h"
#include "synth/polivoks_synth.h"
namespace dp {
class PolivoksScreen {
public:
    void draw(Canvas& c,const int16_t* params,const PolivoksView& view,float dt);
private:
    float jaw_ = 0, time_ = 0, sparkClock_ = 0, boltFlash_ = 0;
    uint32_t rng_ = 0x713a49e5u;
    bool spaceReady_ = false;
    int flashSide_ = 0;
    struct Planet { float x=0,y=0,r=0; } planets_[6];
    float sparks_[2][14] = {};
    float random();
    void drawSpace(Canvas& c);
    void animate(Canvas& c,const int16_t* p,const PolivoksView& view,float dt);
};
void drawPolivoksWave(Canvas& c,int wave,float x,float y,float w,float h,Color color);
void drawPolivoksIllustration(Canvas& c,float x,float y,float w,float h);
}

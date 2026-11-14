// Original reference contours, with a local jaw deformation and eye tint.
// Geometry is vector data; only the native Canvas raster is cached in the UI.
#include "ui/polivoks_screen.h"
#include "gfx/font.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <chrono>
namespace dp {
namespace {
#include "ui/polivoks_art.inc"
constexpr float sourceScale=.25f, sourceX=20, sourceY=5;
struct ReferenceArt {
    Canvas canvas;
    ReferenceArt() {
        canvas.clear(pal::bg);
        float xy[2*kBearMaxPoints];
        for(const BearContour& contour:kBearContours) {
            for(unsigned i=0;i<contour.count;++i) {
                xy[2*i]=sourceX+sourceScale*kBearPoints[contour.start+i][0];
                xy[2*i+1]=sourceY+sourceScale*kBearPoints[contour.start+i][1];
            }
            canvas.fillPolygon(xy,contour.count,contour.color);
        }
    }
};
const Canvas& referenceArt() { static ReferenceArt art; return art.canvas; }
float clamp(float v,float a=0,float b=1) { return std::clamp(v,a,b); }
// Inverse of a continuous, monotonic deformation of the mouth/chin. The
// ushanka, eyes, nose, upper fangs and shoulders stay fixed; the neck joins
// the moving chin continuously.
float jawSourceY(float x,float y,float opening) {
    float sx=(x-sourceX)/sourceScale;
    float weight=clamp((sx-960)/100)*clamp((1440-sx)/100);
    const float delta=35*(1-opening)*weight;
    const float top=237, gum=277, chin=300, bottom=327.5f;
    if(y<top || y>=bottom || delta<=0) return y;
    if(y<gum-delta) return top+(y-top)*(gum-top)/(gum-top-delta);
    if(y<chin-delta) return y+delta;
    return chin+(y-(chin-delta))*(bottom-chin)/(bottom-chin+delta);
}
void sample(const Canvas& art,float x,float y,uint8_t* dst) {
    x=clamp(x,0,Canvas::W-1.001f); y=clamp(y,0,Canvas::H-1.001f);
    int ix=(int)x,iy=(int)y; float fx=x-ix,fy=y-iy;
    const uint8_t* p=art.pixels()+4*(iy*Canvas::W+ix);
    for(int channel=0;channel<3;++channel) {
        float a=p[channel]+fx*(p[channel+4]-p[channel]);
        float b=p[channel+4*Canvas::W]+fx*(p[channel+4*Canvas::W+4]-p[channel+4*Canvas::W]);
        dst[channel]=(uint8_t)clamp(a+fy*(b-a)+.5f,0,255);
    }
    dst[3]=255;
}
void eyes(Canvas& c,float resonance) {
    // Only the red optical circuits/glow of the supplied drawing are tinted.
    // Fur, metal, white sparks and the red hat/star remain their original color.
    for(int y=157;y<224;++y) for(int x=214;x<426;++x) {
        uint8_t* p=c.pixels()+4*(y*Canvas::W+x);
        if(p[0]>p[1]*1.4f && p[0]>p[2]*1.25f) {
            float green=std::fmin(p[1],p[0]*.23f),blue=std::fmin(p[2],p[0]*.20f);
            p[1]=(uint8_t)std::lround(p[0]+resonance*(green-p[0]));
            p[2]=(uint8_t)std::lround(p[0]+resonance*(blue-p[0]));
        }
    }
}
Color levelColor(Color color,float level) {
    return rgb((uint8_t)(pal::white.r+level*(color.r-pal::white.r)),
               (uint8_t)(pal::white.g+level*(color.g-pal::white.g)),
               (uint8_t)(pal::white.b+level*(color.b-pal::white.b)));
}
void coilLevels(Canvas& c,const int16_t* p) {
    for(int osc=0;osc<2;++osc) {
        float level=p[osc?PVX_MIX_OSC2:PVX_MIX_OSC1]/32767.f;
        int x0=osc?516:55,x1=osc?585:128,y0=osc?134:85;
        for(int y=y0;y<284;++y) for(int x=x0;x<x1;++x) {
            uint8_t* pixel=c.pixels()+4*(y*Canvas::W+x);
            bool ink=osc ? pixel[1]>pixel[0]+4 && pixel[1]>pixel[2]*1.05f : pixel[2]>pixel[0]+4 && pixel[2]>pixel[1]*1.05f;
            if(!ink) continue;
            int v=std::max({pixel[0],pixel[1],pixel[2]});
            for(int channel=0;channel<3;++channel)
                pixel[channel]=(uint8_t)std::lround(v+level*(pixel[channel]-v));
        }
    }
}
void oscillatorCaptions(Canvas& c,const int16_t* p) {
    const char* feet[]={"32'","16'","8'","4'","2'"};
    for(int osc=0;osc<2;++osc) {
        Color color=levelColor(osc?pal::green:pal::blue,p[osc?PVX_MIX_OSC2:PVX_MIX_OSC1]/32767.f);
        int octave=p[osc?PVX_OSC2_OCT:PVX_OSC1_OCT]*5/32768;
        int wave=p[osc?PVX_OSC2_WAVE:PVX_OSC1_WAVE]*5/32768;
        float x=osc?590:48, cy=215;
        drawText(c,x,190,feet[octave],12,1.3f,color,ALIGN_CENTER);
        drawPolivoksWave(c,wave,x-12,cy-9,24,18,color);
        drawText(c,osc?549:91,287,osc?"OSC 2":"OSC 1",10,1.2f,color,ALIGN_CENTER);
    }
}
void scope(Canvas& c,const PolivoksView& view,float opening,bool bp) {
    float ox=318,oy=245.5f-7*(1-opening),ex=500;
    float half=(bp?23:46)*(.2f+.8f*opening);
    float fan[]={ox,oy,ex,oy-half,ex,oy+half};
    c.fillPolygon(fan,3,rgb(5,40,30,64));
    c.polyline(fan,3,1.25f,pal::green,true);
    for(int lane=-2;lane<=2;++lane) for(int j=1;j<28;++j) {
        float t=j/28.f;
        c.fillCircle(ox+(ex-ox)*t,oy+lane*half*t*.34f,.55f,rgb(30,185,105));
    }
    float peak=.025f;
    for(float value:view.scope) peak=std::fmax(peak,std::fabs(value));
    float wave[256],echo[256];
    for(int i=0;i<128;++i) {
        float t=i/127.f;
        wave[2*i]=echo[2*i]=ox+(ex-ox)*t;
        wave[2*i+1]=oy-half*.9f*t*view.scope[i]/peak;
        echo[2*i+1]=oy-half*.64f*t*view.scope[(i+7)%128]/peak;
    }
    c.polyline(echo,128,1.1f,pal::blue);
    c.polyline(wave,128,1.3f,pal::green);
}
} // namespace
void drawPolivoksWave(Canvas& c,int wave,float x,float y,float w,float h,Color color) {
    float points[32]; int n=0;
    auto point=[&](float phase,float value) { points[2*n]=x+w*phase/2;points[2*n+1]=y+h*(1-value)/2;++n; };
    if(wave==PVX_TRI) {
        for(int i=0;i<=4;++i) point(i*.5f,i%2?1:-1);
    } else if(wave==PVX_SAW) {
        point(0,-1);
        for(int i=0;i<2;++i) {point(i+1,1);point(i+1,-1);}
    } else {
        float duty=wave==PVX_SQUARE?.5f:wave==PVX_PULSE25?.25f:.125f;
        point(0,1);
        for(int i=0;i<2;++i) {point(i+duty,1);point(i+duty,-1);point(i+1,-1);point(i+1,1);}
    }
    c.polyline(points,n,1.3f,color);
}
float PolivoksScreen::random() {
    rng_^=rng_<<13;rng_^=rng_>>17;rng_^=rng_<<5;
    return (rng_>>8)*(1.f/16777216.f);
}
void PolivoksScreen::drawSpace(Canvas& c) {
    if(!spaceReady_) {
        rng_^=(uint32_t)std::chrono::steady_clock::now().time_since_epoch().count();
        if(!rng_) rng_=0x713a49e5u;
        // Separate free-sky cells guarantee clearance from header, stars,
        // hat, coils, lightning and one another, including Saturn's rings.
        for(int i=0;i<6;++i) {
            Planet& p=planets_[i];
            p.x=(i<3?136:426)+(i%3)*34+random()*8;
            p.y=58+random()*28;p.r=2.3f+random()*2.5f;
        }
        spaceReady_=true;
    }
    const Color colors[]={rgb(30,47,72),rgb(30,67,43),rgb(71,60,43),rgb(58,38,66),rgb(74,30,35),rgb(55,58,62)};
    for(int i=0;i<6;++i) {
        const Planet& p=planets_[i];Color color=colors[i];
        c.fillCircle(p.x,p.y,p.r,rgb(color.r/5,color.g/5,color.b/5));
        c.circle(p.x,p.y,p.r,.8f,color);
        c.arc(p.x-p.r*.25f,p.y,p.r*.73f,-1.1f,1.1f,.6f,rgb(color.r*.65f,color.g*.65f,color.b*.65f));
        if(i%3==0) {
            float ring[50];
            for(int j=0;j<25;++j) {float a=j*6.2831853f/24;ring[2*j]=p.x+2*p.r*std::cos(a);ring[2*j+1]=p.y+.55f*p.r*std::sin(a)-.25f*(ring[2*j]-p.x);}
            c.polyline(ring,25,.65f,color);
        }
    }
}
void PolivoksScreen::animate(Canvas& c,const int16_t* p,const PolivoksView& view,float dt) {
    dt=clamp(dt,0,.1f);time_+=dt;
    float depth=p[PVX_LFO_TO_FILT]/32767.f;
    if(depth>0) {
        float pulse=clamp(.5f+.5f*view.state.lfo);
        float brightness=1-depth*(.65f-.65f*pulse);
        const int boxes[][4]={{293,49,349,103},{234,84,250,99},{260,69,284,93},{358,69,382,93},{391,84,407,99}};
        for(const auto& box:boxes) for(int y=box[1];y<box[3];++y) for(int x=box[0];x<box[2];++x) {
            uint8_t* pixel=c.pixels()+4*(y*Canvas::W+x);
            for(int channel=0;channel<3;++channel) pixel[channel]=(uint8_t)(pixel[channel]*brightness);
        }
    }
    bool sounding=view.state.held_keys>0 || view.state.amp_env>.02f || view.peak>.005f;
    boltFlash_*=std::exp(-dt*22);
    if(sounding && random()<dt*5) {boltFlash_=1;flashSide_=random()>.5f;}
    sparkClock_-=dt;
    if(sparkClock_<=0) {
        sparkClock_=.08f+random()*.14f;
        for(int side=0;side<2;++side) {
            float start=145+random()*42,base=133+random()*9;
            for(int j=0;j<7;++j) {
                float x=start+j*5;
                sparks_[side][2*j]=side?640-x:x;
                sparks_[side][2*j+1]=base+j*2+(random()-.5f)*13;
            }
        }
    }
    const int boxes[][4]={{108,119,244,177},{396,119,536,177}};
    for(int side=0;side<2;++side) {
        const auto& box=boxes[side];
        float redGain=1-(sounding && side==flashSide_?.27f*boltFlash_:0);
        float sparkGain=.35f+.65f*clamp(sparkClock_/.2f);
        for(int y=box[1];y<box[3];++y) for(int x=box[0];x<box[2];++x) {
            uint8_t* pixel=c.pixels()+4*(y*Canvas::W+x);
            bool red=pixel[0]>pixel[1]*1.5f && pixel[0]>pixel[2]*1.3f;
            bool bright=pixel[0]>150 && pixel[1]>pixel[0]*.5f && pixel[2]>pixel[0]*.45f;
            float gain=red?redGain:bright?sparkGain:1;
            for(int channel=0;channel<3;++channel) pixel[channel]=(uint8_t)(pixel[channel]*gain);
        }
        float intensity=(sounding?1.f:.4f)*(.3f+.7f*clamp(sparkClock_/.18f));
        c.polyline(sparks_[side],7,.8f,rgb(240,216,208,(uint8_t)(180*intensity)));
    }
}
void PolivoksScreen::draw(Canvas& c,const int16_t* p,const PolivoksView& view,float dt) {
    const Canvas& art=referenceArt();
    c.copyRows(art,44,334);
    drawSpace(c);
    animate(c,p,view,dt);
    float target=p[PVX_FILT_CUTOFF]/32767.f;
    jaw_+=(target-jaw_)*(1-std::exp(-18*clamp(dt,0,.1f)));
    if(std::fabs(target-jaw_)<.0005f) jaw_=target;
    // Transform the existing reference contours' cached pixels; no second,
    // approximate jaw is drawn on top of the bear.
    if(jaw_<1) for(int y=237;y<328;++y) for(int x=260;x<380;++x) {
        float sy=jawSourceY(x+.5f,y+.5f,jaw_)-.5f;
        sample(art,(float)x,sy,c.pixels()+4*(y*Canvas::W+x));
    }
    eyes(c,p[PVX_FILT_RES]/32767.f);
    coilLevels(c,p);
    oscillatorCaptions(c,p);
    c.line(18,303,244,303,1,rgb(92,75,118));
    c.line(396,303,622,303,1,rgb(92,75,118));
    scope(c,view,jaw_,p[PVX_FILT_MODE]>=16384);
}
void drawPolivoksIllustration(Canvas& c,float x,float y,float w,float h) {
    const Canvas& art=referenceArt();
    const float scale=std::fmin(w/600,h/284),width=600*scale,height=284*scale;
    float left=x+(w-width)/2,top=y+(h-height)/2;
    for(int py=(int)std::ceil(top);py<top+height && py<Canvas::H;++py)
        for(int px=(int)std::ceil(left);px<left+width && px<Canvas::W;++px) {
            if(px<0 || py<0) continue;
            sample(art,20+(px-left+.5f)/scale,50+(py-top+.5f)/scale,
                   c.pixels()+4*(py*Canvas::W+px));
        }
}
} // namespace dp

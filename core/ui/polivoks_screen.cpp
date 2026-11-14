// Original reference contours, with a local jaw deformation and eye tint.
// Geometry is vector data; only the native Canvas raster is cached in the UI.
#include "ui/polivoks_screen.h"
#include "gfx/font.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
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
void oscillatorCaptions(Canvas& c,const int16_t* p) {
    const char* feet[]={"32'","16'","8'","4'","2'"};
    for(int osc=0;osc<2;++osc) {
        Color color=osc?pal::green:pal::blue;
        int octave=p[osc?PVX_OSC2_OCT:PVX_OSC1_OCT]*5/32768;
        int wave=p[osc?PVX_OSC2_WAVE:PVX_OSC1_WAVE]*5/32768;
        float x=osc?590:48, cy=215;
        drawText(c,x,190,feet[octave],12,1.3f,color,ALIGN_CENTER);
        float line[50];
        for(int i=0;i<25;++i) {
            float t=i/24.f;
            float v=wave==0?1-4*std::fabs(t-.5f):wave==1?std::sin(6.2831853f*t):(t<(wave==2?.5f:wave==3?.25f:.125f)?1.f:-1.f);
            line[2*i]=x-12+i; line[2*i+1]=cy-9*v;
        }
        c.polyline(line,25,1.3f,color);
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
void PolivoksScreen::draw(Canvas& c,const int16_t* p,const PolivoksView& view,float dt) {
    const Canvas& art=referenceArt();
    c.copyRows(art,44,334);
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

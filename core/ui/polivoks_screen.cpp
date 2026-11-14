// Vector reconstruction of the supplied cyber-bear concept, at 640x480.
// Only static line art is cached. Moving parts use audio-thread snapshots.
#include "ui/polivoks_screen.h"
#include "gfx/font.h"
#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <cstdio>
namespace dp {
namespace {
constexpr float pi=3.14159265359f;
const Color fur=rgb(168,145,110), bone=rgb(220,212,194), metal=rgb(97,100,104), dark=rgb(53,48,44);
struct Pen {
    Canvas& c; float x,y,s;
    void path(std::initializer_list<float> xy,Color col,float width=1.3f,bool closed=false,float mirror=1) {
        float points[256]; int i=0;
        for(float v:xy) { points[i]=(i&1 ? y+s*v : x+s*v*mirror); ++i; }
        c.polyline(points,i/2,width*s,col,closed);
    }
    void circle(float a,float b,float r,Color col,float width=1) { c.circle(x+s*a,y+s*b,s*r,s*width,col); }
};
void star(Canvas& c,float x,float y,float radius,Color color,bool glow=false) {
    float points[20];
    for(int i=0;i<10;++i) { float a=-pi/2+i*pi/5,r=i&1?radius*.43f:radius; points[2*i]=x+r*std::cos(a); points[2*i+1]=y+r*std::sin(a); }
    if(glow) c.polyline(points,10,5,rgb(color.r,color.g,color.b,24),true);
    c.polyline(points,10,glow?1.5f:1,color,true);
}
void bear(Pen& p) {
    // Ushanka: jagged outer fur, inner brim and hanging ear flaps.
    p.path({-72,-48,-78,-57,-80,-78,-76,-92,-69,-100,-54,-105,-59,-101,-39,-108,-18,-109,-22,-106,0,-108,22,-106,18,-109,39,-108,59,-101,54,-105,69,-100,76,-92,80,-78,78,-57,72,-48},fur,1.6f,true);
    p.path({-65,-52,-69,-65,-69,-83,-61,-92,-55,-94,-58,-89,-44,-96,-45,-91,-29,-98,-32,-93,-13,-98,0,-98,13,-98,32,-93,29,-98,45,-91,44,-96,58,-89,55,-94,61,-92,69,-83,69,-65,65,-52,45,-57,24,-60,0,-61,-24,-60,-45,-57},bone,1.4f,true);
    for(int side:{-1,1}) {
        p.path({67,-51,77,-46,81,-30,81,-5,86,13,79,34,68,45,61,31,64,10,63,-10},fur,1.4f,false,(float)side);
        p.path({72,-39,75,-26,73,-12,78,4,72,20,69,30},bone,1,false,(float)side);
        p.path({58,-46,65,-31,65,-12,73,5,66,13,77,24,70,31,73,42,62,45,60,59,51,61,49,73,38,76,32,88,20,91},fur,1.6f,false,(float)side);
        p.path({56,-24,61,-13,57,0,65,11,59,25,63,39,53,43,49,57,42,61,39,72,31,75},bone,1.1f,false,(float)side);
        // Brow ridge, cheek armor and embedded wiring.
        p.path({18,-26,27,-34,42,-39,58,-37,51,-31,37,-30,24,-19},fur,1.5f,false,(float)side);
        p.path({20,-21,32,-25,45,-28,58,-24},bone,1.5f,false,(float)side);
        p.path({32,4,44,11,61,10,55,19,39,19,32,12},metal,1.7f,false,(float)side);
        p.path({34,25,48,25,59,18,61,31,52,38,40,37,34,31},metal,1.4f,false,(float)side);
        p.path({38,42,48,46,48,59,40,66,33,64},fur,1.2f,false,(float)side);
        p.path({42,43,42,52,38,56,38,61},metal,1,false,(float)side);
        p.circle(side*48,30,1.1f,fur); p.circle(side*54,28,1.1f,fur);
        // Fur swept outwards from the muzzle.
        for(int i=0;i<8;++i) {
            float x=24+i*3.1f,y=12+i*3.6f;
            p.path({x,y,x+4,y+7,x+3,y+11},i&1?fur:dark,1,false,(float)side);
        }
        // Shoulder and ribbed military collar.
        p.path({52,56,73,48,88,55,101,72,121,81,139,106,149,120,127,119,117,112,91,112,79,124,62,132,40,135,25,123},fur,1.5f,false,(float)side);
        p.path({64,69,82,61,94,78,87,83,104,100,96,108,77,113,67,126,42,130},metal,1.4f,false,(float)side);
        p.path({86,80,77,87,85,101,71,110,59,124},fur,1.1f,false,(float)side);
        for(int i=0;i<6;++i) p.path({92.f+i*3,85.f+i*3,112.f+i*3,93.f+i*3},metal,.9f,false,(float)side);
        p.path({137,99,149,113,151,120,131,127,104,127,79,139,48,145},rgb(110,91,139),1.2f,false,(float)side);
    }
    // Forehead center plate.
    p.path({-14,-57,-10,-47,10,-47,14,-57},metal,1.5f);
    p.path({-8,-43,0,-46,8,-43,6,-32,-6,-32},fur,1,true);
    p.circle(0,-40,1.5f,metal); p.circle(0,-34,1,metal);
    // Broad bear nose and nostrils.
    p.path({-24,-9,-19,-17,-9,-21,9,-21,19,-17,24,-9,24,4,17,13,0,17,-17,13,-24,4},metal,1.8f,true);
    p.path({-20,-8,-15,-14,0,-16,15,-14,20,-8,18,3,10,10,0,12,-10,10,-18,3},bone,1.2f,true);
    p.path({-18,-3,-15,-8,-10,-9,-6,-5,-6,1,-11,4,-17,1},metal,1.6f,true);
    p.path({18,-3,15,-8,10,-9,6,-5,6,1,11,4,17,1},metal,1.6f,true);
    p.path({-23,11,-22,21,-33,30,-30,42,-19,48,0,52,19,48,30,42,33,30,22,21,23,11},fur,1.4f);
    p.path({-22,25,-14,20,0,23,14,20,22,25},bone,1.3f);
    p.path({-18,24,-8,27,0,25,8,27,18,24},dark,1.2f);
    // Top gum and long fangs.
    p.path({-29,34,-22,29,-12,30,0,32,12,30,22,29,29,34},bone,1.5f);
    p.path({-27,34,-25,55,-20,38,-21,31},bone,1.6f,true);
    p.path({27,34,25,55,20,38,21,31},bone,1.6f,true);
    for(int i=-2;i<=2;++i) { float x=i*7.f; p.path({x-2,34,x,41,x+2,34},bone,1.1f); }
    star(p.c,p.x,p.y-89*p.s,5*p.s,pal::red);
    drawText(p.c,p.x,p.y-82*p.s,"CCCP",19*p.s,1.25f*p.s,pal::red,ALIGN_CENTER);
}
void coil(Canvas& c,float x,float y,float scale,Color color,float phase,int reg,int wave) {
    float points[600];
    const int turns=10+reg;
    for(int i=0;i<300;++i) {
        float t=i/299.f,a=t*turns*2*pi+phase;
        points[2*i]=x+scale*17*std::sin(a);
        points[2*i+1]=y+scale*(128*t+3*std::cos(a));
    }
    c.polyline(points,300,3.7f*scale,rgb(color.r,color.g,color.b,18));
    c.polyline(points,300,1.5f*scale,color);
    c.line(x,y-8*scale,x,y-29*scale,1.4f*scale,color);
    c.line(x,y+128*scale,x,y+139*scale,1.4f*scale,color);
    if(scale<.7f) return;
    const char* feet[]={"32'","16'","8'","4'","2'"};
    drawText(c,x<320?x-27:x+27,y+50,feet[reg],11,1.2f,color,x<320?ALIGN_RIGHT:ALIGN_LEFT);
    float waveLine[50];
    for(int i=0;i<25;++i) {
        float t=i/24.f;
        float v=wave==0?1-4*std::fabs(t-.5f):wave==1?2*t-1:(t<(wave==2?.5f:wave==3?.25f:.125f)?1.f:-1.f);
        waveLine[2*i]=(x<320?x-50:x+30)+i*1.f;
        waveLine[2*i+1]=y+82-6*v;
    }
    c.polyline(waveLine,25,1.2f,color);
}
int pick(int x,int n) { return x*n/32768; }
} // namespace
void PolivoksScreen::draw(Canvas& c,const int16_t* p,const PolivoksView& view,float dt) {
    time_+=std::fmin(dt,.1f);
    if(!cache_) {
        cache_=new Canvas(); cache_->clear(pal::bg);
        Pen pen{*cache_,320,192,1}; bear(pen);
        cache_->line(18,333,244,333,1,rgb(92,75,118));
        cache_->line(396,333,622,333,1,rgb(92,75,118));
    }
    c.copyRows(*cache_,44,334);
    const float res=p[PVX_FILT_RES]/32767.f;
    const float cut=p[PVX_FILT_CUTOFF]/32767.f;
    const float env=view.state.filter_env;
    const float amount=(p[PVX_FILT_ENV_AMT]-16384)/16384.f;
    const float mouth=std::clamp(cut+.6f*amount*env,0.f,1.f);
    jaw_+=(mouth-jaw_)*std::fmin(1.f,dt*12);
    // Star constellation: gate and repeating envelope give it a pulse.
    bool repeat=p[PVX_FENV_REPEAT]>=16384 || p[PVX_AENV_REPEAT]>=16384;
    float pulse=repeat ? .35f+.65f*std::fmax(env,view.state.amp_env) : .55f+.45f*std::fmin(1.f,view.peak*3);
    star(c,320,70,20,rgb((uint8_t)(140+115*pulse),45,50),true);
    c.arc(322,70,4.8f,-.3f,3.1f,1.2f,pal::red); c.line(317,66,324,75,1.4f,pal::red); c.line(318,67,322,64,1.5f,pal::red);
    for(int side:{-1,1}) {
        star(c,320+side*46,80,7,view.state.held_keys ? fur : metal);
        star(c,320+side*77,92,4.5f,view.state.held_keys>1?pal::red:dark);
        const float eyeX=320+side*39,eyeY=171;
        float radius=3.5f+res*2;
        c.fillCircle(eyeX,eyeY,11,rgb(110,13,20,95));
        c.circle(eyeX,eyeY,9.2f,1.2f,fur);
        c.fillCircle(eyeX,eyeY,radius+2,rgb(255,45,46,45));
        c.fillCircle(eyeX,eyeY,radius,pal::red);
        c.fillCircle(eyeX-side,eyeY-1,2.1f,rgb(255,151,130));
        for(int j=0;j<3;++j) {
            float radius2=16+j*9;
            c.arc(eyeX,eyeY,radius2,side>0?-.8f:2.3f,side>0?.8f:4.0f,1,rgb((uint8_t)(60+150*res),30,35));
        }
    }
    int reg1=pick(p[PVX_OSC1_OCT],5),reg2=pick(p[PVX_OSC2_OCT],5);
    coil(c,78,151,1,pal::blue,view.state.phase[0]*.15f,reg1,pick(p[PVX_OSC1_WAVE],5));
    coil(c,564,151,1,pal::green,view.state.phase[1]*.15f,reg2,pick(p[PVX_OSC2_WAVE],5));
    drawText(c,78,304,"OSC 1",9,1.2f,pal::blue,ALIGN_CENTER);
    drawText(c,564,304,"OSC 2",9,1.2f,pal::green,ALIGN_CENTER);
    for(int side:{-1,1}) {
        float drive=std::max(0.f,(p[side<0?PVX_MIX_OSC1:PVX_MIX_OSC2]/32767.f-.5f)*2);
        float depth=p[side<0?PVX_XMOD_DEPTH:PVX_MIX_RING]/32767.f;
        float alpha=std::fmin(1.f,.22f+drive*.5f+depth*.7f);
        float bolt[22];
        for(int i=0;i<11;++i) {
            bolt[2*i]=320+side*(218-i*12.f);
            bolt[2*i+1]=145+(i%2?9.f:-5.f)+(i==0||i==10?0:3*std::sin(time_*17+i*2));
        }
        c.polyline(bolt,11,5,rgb(255,40,50,(uint8_t)(alpha*28)));
        c.polyline(bolt,11,1.2f,rgb((uint8_t)(120+alpha*135),40,45));
        if(depth>.12f || res>.8f) {
            float x=320+side*151;
            c.line(x,145,x+side*7,129,1,pal::white);
            c.line(x+side*7,129,x+side*17,132,1,pal::red);
            c.line(x+side*17,132,x+side*22,122,1,pal::red);
        }
    }
    // The lower jaw follows effective filter opening, not a generic host ADSR.
    float bottom=252+28*jaw_;
    c.fillRoundRect(300,236,40,bottom-236,15,pal::bg);
    c.arc(320,bottom-3,25,.15f,pi-.15f,1.8f,fur);
    c.arc(320,bottom-5,19,.2f,pi-.2f,1.2f,metal);
    for(int i=-2;i<=2;++i) {
        float x=320+i*6.f;
        float gum=bottom+12-std::fabs((float)i)*1.5f;
        float tooth[]={x-2,gum,x,gum-7,x+2,gum};
        c.polyline(tooth,3,1.1f,bone);
    }
    // True output scope, projected from the mouth into the beam.
    float ox=322,oy=244+8*jaw_,ex=508,half=p[PVX_FILT_MODE]>=16384?15:29;
    float fan[]={ox,oy,ex,oy-half,ex,oy+half};
    c.fillPolygon(fan,3,rgb(5,34,25,90));
    c.polyline(fan,3,1,pal::green,true);
    for(int lane=-1;lane<=1;++lane) for(int j=1;j<20;++j) {
        float t=j/20.f;
        c.fillCircle(ox+(ex-ox)*t,oy+lane*half*t*.55f,.55f,rgb(25,116,72));
    }
    float peak=.025f;
    for(float value:view.scope) peak=std::fmax(peak,std::fabs(value));
    float wave[256],echo[256];
    for(int i=0;i<128;++i) {
        float t=i/127.f;
        wave[2*i]=echo[2*i]=ox+(ex-ox)*t;
        wave[2*i+1]=oy-half*.84f*t*view.scope[i]/peak;
        echo[2*i+1]=oy-half*.58f*t*view.scope[(i+7)%128]/peak;
    }
    c.polyline(echo,128,1,pal::blue); c.polyline(wave,128,1.3f,pal::green);
}
void drawPolivoksIllustration(Canvas& c,float x,float y,float w,float h) {
    float scale=std::fmin(w/330,h/245);
    Pen p{c,x+w*.5f,y+h*.44f,scale}; bear(p);
    coil(c,x+w*.13f,y+h*.28f,.48f,pal::blue,0,1,1);
    coil(c,x+w*.87f,y+h*.28f,.48f,pal::green,0,2,2);
}
} // namespace dp

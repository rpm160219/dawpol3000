#include "synth/polivoks_synth.h"
#include <cmath>
#include <cstring>

namespace dp {
void PolivoksSynth::init(float rate) {
    rate_=rate;
    if (!core_) core_=polivoksCreate();
    polivoksDefaultParams(params_);
    for (int& key:map_) key=-1;
}
void PolivoksSynth::configure(const int16_t* params, int octave) {
    std::memcpy(params_,params,sizeof(params_));
    octave_=octave < -3 ? -3 : octave > 3 ? 3 : octave;
}
void PolivoksSynth::noteOn(int pitch,float velocity) {
    if (!core_ || pitch<0 || pitch>127) return;
    if (velocity<=0) { noteOff(pitch); return; }
    if (map_[pitch]>=0) noteOff(pitch);
    int note=pitch+12*octave_;
    if (note<0 || note>127) return;
    map_[pitch]=note;
    ++references_[note];
    polivoksKeyDown(core_,note,(int)std::lround(127*std::fmin(velocity,1.f)),params_);
    live_=true;
}
void PolivoksSynth::noteOff(int pitch) {
    if (pitch<0 || pitch>127 || map_[pitch]<0) return;
    const int note=map_[pitch]; map_[pitch]=-1;
    if (--references_[note]==0) polivoksKeyUp(core_,note,params_);
}
void PolivoksSynth::allOff() {
    for (int note=0;note<128;++note) {
        if (references_[note]) polivoksKeyUp(core_,note,params_);
        references_[note]=0; map_[note]=-1;
    }
}
void PolivoksSynth::reset() {
    allOff(); polivoksPanic(core_); live_=false;
    active_.store(0,std::memory_order_relaxed);
    std::memset(scope_,0,sizeof(scope_));
    view_.back()=PolivoksView(); view_.publish();
}
void PolivoksSynth::render(float* stereo,int frames) {
    if (!core_) return;
    float peak=0;
    for (int pos=0;pos<frames;) {
        const int n=frames-pos<POLIVOKS_BLOCK ? frames-pos : POLIVOKS_BLOCK;
        float mono[POLIVOKS_BLOCK];
        polivoksBeginFrames(core_,params_,rate_,n);
        polivoksRenderFrames(core_,0,mono,nullptr,params_,0,rate_,n);
        for (int i=0;i<n;++i) {
            /* A reset mutes only the retired instance; no extra envelope/gain
             * is added to normal playing. Oscillators/LFO still free-run. */
            float x=live_ || params_[PVX_VCA_HOLD]>=16384 ? mono[i] : 0;
            stereo[2*(pos+i)]+=x; stereo[2*(pos+i)+1]+=x;
            peak=std::fmax(peak,std::fabs(x));
            if (++scopeClock_==4) { scopeClock_=0; scope_[scopePos_]=x; scopePos_=(scopePos_+1)%128; }
        }
        pos+=n;
    }
    PolivoksView& v=view_.back();
    polivoksGetState(core_,0,&v.state); v.peak=peak;
    for (unsigned i=0;i<128;++i) v.scope[i]=scope_[(scopePos_+i)%128];
    active_.store(v.state.held_keys || v.state.amp_env>0.00001f || params_[PVX_VCA_HOLD]>=16384,std::memory_order_relaxed);
    view_.publish();
}
} // namespace dp

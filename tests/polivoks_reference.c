/* Build against the approved pre-integration DSP for a golden float stream. */
#include "polivoks_dsp.h"
#include "polivoks_presets.h"
#include <stdio.h>
int main(int argc,char** argv) {
    if(argc!=2) return 1;
    FILE* f=fopen(argv[1],"wb"); if(!f) return 2;
    for(int preset=0;preset<polivoksPresetCount();++preset) {
        int16_t params[POLIVOKS_NUM_PARAMS]; polivoksLoadPreset(preset,params);
        struct PolivoksCore* p=polivoksCreate(); if(!p) return 3;
        for(int frame=0;frame<76800;frame+=128) {
            if(frame==0) polivoksKeyDown(p,45,110,params);
            if(frame==5120) polivoksKeyDown(p,57,110,params);
            if(frame==10240) polivoksKeyUp(p,57,params);
            if(frame==15360) polivoksKeyUp(p,45,params);
            if(frame==25600) polivoksKeyDown(p,48,110,params);
            if(frame==51200) polivoksKeyUp(p,48,params);
            float out[128]; polivoksRenderFloat(p,0,out,NULL,params,0,48000);
            if(fwrite(out,sizeof(float),128,f)!=128) return 4;
        }
        polivoksDestroy(p);
    }
    return fclose(f)!=0;
}

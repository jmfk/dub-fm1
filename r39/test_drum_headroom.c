#include <assert.h>
#include <stdio.h>
#include "engine.h"
static int32_t pcm[512];static int16_t wave[256];
int main(void){d_init();assert(d_sample_limit==4&&d_osc_limit==8&&!dp[3].p[P_DRIVE]);ds_pattern_at(0)->drums=1;for(int i=0;i<256;i++)wave[i]=30000;for(int p=0;p<11;p++){d_sample_views[p].pcm=wave;d_sample_views[p].frames=256;d_sample_views[p].loop=1;}d_drum_cache_init();int notes[]={36,38,42,45};for(int i=0;i<4;i++)d_drum_on(notes[i],127,0);d_render(pcm,256);assert(d_drum_bus[8]>29000&&d_drum_bus[8]<=30000);for(int i=0;i<11;i++)if(d_drum_voice[i].active)assert(d_drum_raw[i]>29000&&d_drum_raw[i]<=30000);assert(!d_clips&&!d_lim.preclip_frames);d_drum_on(56,127,0);assert(d_drum_voice[d_drum_pad(36)].active);assert(d_sample_active_count()==4);puts("PASS4x30000 PCM normalized before drive, clean drum boot DRIVE0, unchanged raw levels, kick protection and4/8 default caps");}

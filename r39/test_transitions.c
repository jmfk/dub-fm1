#include <assert.h>
#include <stdio.h>
#include "engine.h"
static int32_t pcm[512];static int16_t constant[256];
int main(void){d_init();ds_pattern_at(0)->drums=1;for(int i=0;i<256;i++)constant[i]=12000;for(int p=0;p<2;p++){d_sample_views[p].pcm=constant;d_sample_views[p].frames=256;d_sample_views[p].loop=1;}
 d_drum_cache_init();d_sample_limit_set(1);d_drum_on(36,127,0);d_render(pcm,64);int32_t old=d_drum_raw[0];assert(old>11000);d_drum_on(38,127,0);assert(!d_drum_voice[0].active&&d_pcm_tail[0]==old);assert(d_sample_active_count()==1);d_render(pcm,1);assert(d_drum_raw[0]==old&&d_drum_raw[1]==0);for(int i=1;i<32;i++){d_render(pcm,1);assert(da(d_drum_raw[0]+d_drum_raw[1]-old)<4);}assert(!d_pcm_tail[0]);
 d_render(pcm,32);old=d_drum_raw[1];d_drum_on(38,127,0);d_render(pcm,1);assert(d_drum_raw[1]==old);for(int i=1;i<32;i++){d_render(pcm,1);assert(da(d_drum_raw[1]-old)<4);}assert(d_sample_active_count()==1);
 d_init();d_osc_limit_set(1);d_note_on(0,48,100,0);d_render(pcm,256);int selected=-1;for(int i=0;i<8;i++)if(d_budget_osc_active(0,i))selected=i;assert(selected>=0&&d_osc_last[0][selected]);int32_t last=d_osc_last[0][selected];d_note_on(1,55,100,0);assert(d_osc_tail[0]==last&&d_osc_tail_left[0]==32);d_render(pcm,32);assert(!d_osc_tail[0]&&d_osc_active_count()==1);
 d_note_on(1,58,100,0);d_render(pcm,256);d_full_panic();for(int p=0;p<4;p++)assert(!d_osc_tail[p]);for(int i=0;i<11;i++)assert(!d_pcm_tail[i]);d_render(pcm,256);for(int i=0;i<512;i++)assert(!pcm[i]);
 puts("PASS PCM steal/retrigger32sample complementary fades, synth steal bounded tail, strict source caps and panic silence");}

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "panel.h"
static int32_t out[512];
int main(void){d_init();panel_init();assert(d_cpu[0]==4&&d_cpu[1]&&d_cpu[2]&&!d_cpu[3]);
 // Disabled reverb tank and its positions stay untouched under a nonzero send.
 int32_t send[3]={0,0,16000},l,r;for(int i=0;i<1000;i++)d_fx_tick(0,0,send,&l,&r);assert(d_rpos[0]==0&&d_rv_buf[0][0]==0);
 d_cpu_set(3,1);for(int i=0;i<1000;i++)d_fx_tick(0,0,send,&l,&r);assert(d_rpos[0]!=0);d_cpu_set(3,0);assert(!d_rpos[0]);
 d_cpu_set(2,0);for(int i=0;i<2000;i++){send[0]=16000;d_fx_tick(1000,-1000,send,&l,&r);if(i>15)assert(l==1000&&r==-1000);}assert(d_rate_down[0][0][0]==0&&d_rpos[0]==0);
 // OFF skips filter state; minimal OSC counts respect each active voice group.
 d_cpu_set(0,0);dp[0].s1=123;d_render(out,256);assert(dp[0].s1==123&&d_active==0);d_cpu_set(0,1);for(int i=0;i<4;i++)d_note_on(0,48+i*4,100,0);d_render(out,256);assert(d_active==4);
 d_route_set(&dp[0],0,0,50);d_route_set(&dp[0],0,1,127);d_cpu_set(1,0);d_render(out,256);for(int i=0;i<30;i++)assert(!dp[0].fx_mod[i]);assert(d_route_target(&dp[0],0)==50);assert(d_amp_envelopes(&dp[0],0)==32768);
 d_cpu_set(0,0);dp[0].fx_mod[20]=127;d_prepare_fx();assert(d_fxdv[V_DECAY]==dv[V_DECAY]);dp[0].fx_mod[20]=0;d_cpu_set(0,1);
 // Physical CPU menu, original settings retained.
 panel_nav(4,0);panel_button(5,1,1);panel_button(8,1,2);panel_button(8,0,3);panel_button(5,0,4);assert(panel.group==12);panel_turn(5,1);d_render(out,32);assert(d_cpu[3]);panel_turn(5,-1);d_render(out,32);assert(!d_cpu[3]);
 uint8_t set[]={0xf0,0x7d,0x44,0x55,0x42,1,2,3,0,0xf7},reply[14];assert(d_runtime_request(set,10)==2);for(int n=0;n<10;n++)assert(d_runtime_request(set,n)<0);set[7]=4;assert(d_runtime_request(set,10)<0);assert(d_runtime_reply(reply,2)==14);assert(reply[6]==0x42&&reply[11]==0);if(1){FILE *f=fopen(".build-env/dub-r39-runtime-fixture.bin","wb");assert(f);fwrite(reply,1,14,f);fclose(f);}
 puts("PASS runtime defaults/reverb tank bypass, converter bypass/dry latency, synth OFF, per-voice OSC limit, matrix bypass/fixed ENV, preserved routes, physical controls, strict MIDI codec");}

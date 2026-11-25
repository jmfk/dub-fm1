#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "engine.h"
#include "panel.h"
static int16_t saved[8][RV_N];static int32_t pcm[512];
static void infinite(void){d_init();dv[V_DECAY]=100;d_prepare_fx();}
static int32_t stimulus(int j){return (j*7919)%12000-6000;}
static int64_t tank_energy(void){int64_t v=0;for(int j=0;j<8;j++)for(int k=0;k<RV_N;k++)v+=da(d_rv_buf[j][k]);return v;}
static void settle(int seconds){int32_t out[2];for(int i=0;i<22050*seconds;i++)d_reverb_tick(0,out);}
int main(void){
 int32_t out[2];infinite();int64_t energy=0;for(int j=0;j<44100;j++){d_reverb_tick(j<11025?stimulus(j):0,out);energy+=abs(out[0])+abs(out[1]);}assert(energy>10000&&tank_energy()>10000&&d_rv_frozen);
 settle(3);memcpy(saved,d_rv_buf,sizeof saved);int64_t before=tank_energy();for(int j=0;j<22050;j++)d_reverb_tick(j<2000?stimulus(j+47)*2:0,out);settle(3);assert(memcmp(saved,d_rv_buf,sizeof saved)&&tank_energy()>10000);printf("FOREVER accepts sound from empty tank and additional sound: stored magnitude before=%lld after=%lld\n",(long long)before,(long long)tank_energy());
 /* Settled silence does not rewrite or attenuate the overdubbed tail. */
 memcpy(saved,d_rv_buf,sizeof saved);uint32_t phase=d_rvlfo,taps[8];memcpy(taps,d_hold_tq,sizeof taps);int64_t first=0,last=0;
 for(int j=0;j<22050*120;j++){d_reverb_tick(0,out);assert(abs(out[0])<=65534&&abs(out[1])<=65534);if(j<22050)first+=abs(out[0])+abs(out[1]);if(j>=22050*119)last+=abs(out[0])+abs(out[1]);}
 assert(d_rv_frozen&&!memcmp(saved,d_rv_buf,sizeof saved)&&d_rvlfo==phase&&!memcmp(taps,d_hold_tq,sizeof taps));assert(first>1000&&last>first/10&&last<first*10);printf("120s settled-silence windows: first=%lld last=%lld; tank exactly unchanged\n",(long long)first,(long long)last);
 /* Known prefix sample verifies addition, plus soft knee on overloaded writes. */
 for(int high=0;high<2;high++){infinite();dv[V_DIFFUSION]=0;d_prepare_fx();d_diff[1][0]=4000;for(int j=0;j<8;j++)d_rv_buf[j][0]=high?30000:1000;d_reverb_tick(0,out);for(int j=0;j<8;j++)assert(d_rv_buf[j][0]==(high?26182:2000));}
 /* Two minutes of maximum alternating/noise input: bounded, no unchecked growth. */
 infinite();for(int j=0;j<22050*120;j++){int32_t input=j&1?32767:-32767;if(j%7==0)input=stimulus(j%10000)*5;d_reverb_tick(input,out);assert(abs(out[0])<=65534&&abs(out[1])<=65534);}assert(tank_energy()>1000);settle(4);memcpy(saved,d_rv_buf,sizeof saved);settle(2);assert(!memcmp(saved,d_rv_buf,sizeof saved));
 dv[V_DECAY]=0;d_prepare_fx();settle(3);energy=0;for(int j=0;j<22050;j++){d_reverb_tick(0,out);energy+=abs(out[0])+abs(out[1]);}assert(!d_rv_frozen&&energy<=22050*8);
 /* Global panic clears, while retaining FOREVER and accepting fresh reverb. */
 infinite();for(int j=0;j<5000;j++)d_reverb_tick(stimulus(j),out);d_send(DC_PANIC,0,0,0,0);d_render(pcm,256);for(int i=0;i<512;i++)assert(!pcm[i]);assert(!tank_energy()&&dv[V_DECAY]==100);d_note_on(0,60,100,0);for(int j=0;j<100;j++)d_render(pcm,256);assert(d_active&&tank_energy()>1000);
 panel_init();panel_nav(2,2);dv[V_DECAY]=88;panel_turn(2,12);d_render(pcm,256);dub_control c[4];panel_controls(c);char text[24];pvalue(c[0],text);assert(!strcmp(c[0].name,"TIME")&&!strcmp(text,"FOREVER"));panel_turn(2,-1);d_render(pcm,256);panel_controls(c);pvalue(c[0],text);assert(dv[V_DECAY]==99&&!strcmp(text,"60.5S"));
 for(int i=1;i<100;i++)assert(d_reverb_time_ms[i]>=d_reverb_time_ms[i-1]);
 dv[V_DECAY]=100;preset_patch patch;preset_capture(&patch,"FOREVER INPUT");assert(preset_valid(&patch));dv[V_DECAY]=12;preset_apply(&patch);d_prepare_fx();for(int j=0;j<5000;j++)d_reverb_tick(stimulus(j),out);assert(dv[V_DECAY]==100&&d_rv_frozen&&tank_energy()>1000);
 d_init();dp[0].p[P_LFO_SHAPE]=1;dp[0].lfo[0]=0x80000000u;d_route_set(&dp[0],0,0,41);d_route_set(&dp[0],0,1,127);d_prepare_part(&dp[0]);d_prepare_fx();assert(d_fxdv[V_DECAY]==100&&dv[V_DECAY]==88);d_reverb_tick(10000,out);assert(d_rv_frozen);dp[0].lfo[0]=0;d_prepare_part(&dp[0]);d_prepare_fx();d_reverb_tick(0,out);assert(!d_rv_frozen&&dv[V_DECAY]==88);
 puts("PASS FOREVER input from empty/filled tank, exact overdub and soft-knee arithmetic,120s continuous full-scale input bounds,120s exact settled-tail sustain, finite release, panic/fresh wet input, physical TIME, saved settings/input and matrix modulation");
}

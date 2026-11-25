#include <assert.h>
#include <stdio.h>
#include "engine.h"
static uint16_t mask_of(int32_t *h){uint16_t m=0;for(int i=0;i<16;i++)if(h[i])m|=1u<<i;return m;}
static void validate(void){for(int b=0;b<3;b++){assert(mask_of(d_down[b])==d_down_nz[b]);for(int ch=0;ch<2;ch++)assert(mask_of(d_up[b][ch])==d_up_nz[b][ch]);}for(int b=0;b<2;b++)for(int j=0;j<2;j++){assert(mask_of(d_rate_down[b][j])==d_rate_down_nz[b][j]);for(int ch=0;ch<2;ch++)assert(mask_of(d_rate_up[b][j][ch])==d_rate_up_nz[b][j][ch]);}}
int main(void){d_init();d_cpu_set(3,1);int32_t h[16]={0};uint16_t mask=0;unsigned seed=7;
 for(int i=0;i<4000;i++){seed=seed*1664525u+1013904223u;unsigned pos=seed&15;int32_t x=(seed&32)?0:(int32_t)((seed>>8)&65535)-32768;d_fir_store(h,&mask,pos,x);assert(mask==mask_of(h));assert(d_delay_masked(h,mask,pos,1)==d_delay_fir(h,pos,1));assert(d_delay_masked(h,mask,pos,2)==d_delay_fir(h,pos,2));}
 int32_t send[3]={0},l,r;for(int i=0;i<1000;i++)d_fx_tick(0,0,send,&l,&r);validate();assert(!d_down_nz[0]&&!d_up_nz[2][1]&&!d_rate_up_nz[1][1][1]);assert(d_rate_frame==500&&d_dpos==125&&d_rpos[0]==500&&d_fxlfo==500u*68174u);
 for(int i=0;i<8000;i++){send[0]=i==0?25000:0;send[1]=i==250?-18000:0;send[2]=i==1000?12000:0;d_fx_tick(0,0,send,&l,&r);if(!(i&31))validate();}d_fx_init();validate();
 // Default LFO may ramp a zero-state filter; exact cached zero remains valid.
 int32_t pcm[512];d_init();d_cpu_set(2,0);for(int j=0;j<100;j++)d_render(pcm,256);for(int p=0;p<4;p++)assert(d_idle[p].sleep&&dp[p].s1==0&&dp[p].s2==0);
 puts("PASS21 FIR ring masks/signed reference dots, silence clocks advance, impulses and history clear; LFO zero-filter coefficient ramps cache exactly");}

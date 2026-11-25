/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "engine.h"
static int32_t pcm[512];
static void blocks(int n){while(n--)d_render(pcm,256);}
static void midi(int st,int n,int v,int src){assert(d_send(DC_MIDI,st,n,v,src));blocks(1);}
static long long energy(int n){long long e=0;while(n--){blocks(1);for(int j=0;j<512;j++)e+=da(pcm[j]);}return e;}
int main(void){
 d_init();midi(0x90,60,100,0);assert(dp[0].filter_triggers==1);assert(d_active==2);assert(energy(20)>100000);
 midi(0x90,64,100,0);midi(0x90,67,100,0);midi(0x90,70,100,0);assert(dp[0].filter_triggers==1);
 int count[4]={0};for(int i=0;i<8;i++)count[(int)dp[0].osc[i].slot]++;for(int j=0;j<4;j++)assert(count[j]==2);
 midi(0xb0,80,127,0);assert(dp[0].filter_triggers==2);
 midi(0x80,60,0,0);midi(0x80,64,0,0);midi(0x80,67,0,0);midi(0x90,70,0,0);assert(dp[0].filter_env.stage==4);
 blocks(800);assert(d_active==0);midi(0x90,62,100,0);assert(dp[0].filter_triggers==3);
 midi(0xb0,64,127,0);midi(0x80,62,0,0);assert(dp[0].post_env.stage!=4);midi(0xb0,64,0,0);assert(dp[0].post_env.stage==4);
 d_init();for(int ch=0;ch<4;ch++){midi(0x90|ch,48+ch*3,100,0);assert(dp[ch].filter_triggers==1);}assert(d_active==32);
 midi(0x94,80,100,0);assert(d_active==32); /* channel5 ignored */
 d_init();midi(0x90,60,100,0);midi(0x90,60,100,1);midi(0x80,60,0,0);assert(d_gates(&dp[0])==1);midi(0x80,60,0,1);assert(d_gates(&dp[0])==0);
 d_init();for(int j=0;j<5;j++)midi(0x90,60+j,100,0);assert(d_active==8);assert(dp[0].filter_triggers==1);
 midi(0x80,60,0,0);assert(d_gates(&dp[0])==4);for(int j=1;j<5;j++)midi(0x80,60+j,0,0);assert(!d_gates(&dp[0]));
 d_init();midi(0x90,60,100,0);midi(0x90,60,100,0);midi(0x80,60,0,0);assert(d_gates(&dp[0]));midi(0x80,60,0,0);assert(!d_gates(&dp[0]));
 d_init();midi(0x90,60,100,0);d_send(DC_PARAM,0,P_CHANNEL,2,0);blocks(1);assert(!d_gates(&dp[0]));midi(0x80,60,0,0);assert(!d_gates(&dp[0]));
 d_init();for(int i=0;i<129;i++)d_send(DC_MIDI,0x90,60,100,0);blocks(1);assert(d_overflows==1);assert(!d_active);
 for(int wave=0;wave<4;wave++)for(int mode=0;mode<4;mode++){
  d_init();for(int p=0;p<4;p++){dp[p].p[P_MODE]=mode;dp[p].p[P_CUT]=20+wave*30;dp[p].p[P_RES]=120;for(int i=0;i<8;i++)dp[p].cfg[i].wave=wave;
   for(int j=0;j<4;j++)d_note_on(p,24+p*12+j*7,120,0);}
  assert(energy(24)>1000);for(int p=0;p<4;p++)d_panic_part(&dp[p]);blocks(3);
 }
 d_init();dg[G_DLY_A_RET]=dg[G_DLY_B_RET]=0;dg[G_REV_RET]=0;midi(0x90,60,100,0);midi(0xb0,123,0,0);blocks(1000);assert(!d_active);
 printf("PASS MIDI channels, 32 oscillators, balanced allocation, legato/retrigger, sustain, source ownership, stealing, duplicate notes, routing changes, panic, wave/filter matrix\n");
 d_init();for(int j=0;j<2;j++){dd[j][D_FEEDBACK]=94;dd[j][D_MOD]=127;dd[j][D_TIME_MS]=700;}dv[V_DECAY]=98;dv[V_MOD]=127;
 int32_t s[3]={0,0,0},l,r;long long tail=0;
 d_prepare_fx();for(int i=0;i<44100*4;i++){s[0]=s[1]=s[2]=i==0?12000:0;d_fx_tick(0,0,s,&l,&r);assert(da(l)<100000&&da(r)<100000);if(i>22050)tail+=da(l)+da(r);}
 assert(tail>10000);printf("PASS bounded shared delays/FDN, maximum feedback/modulation and nonzero tails\n");return 0;
}

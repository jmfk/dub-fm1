/* Focused actual modulation behavior, not just parameter plumbing. */
#include <assert.h>
#include <stdio.h>
#include "engine.h"
static int32_t out[512];
static void quiet_lfos(d_part *t){for(int i=0;i<3;i++)t->p[d_lfo_ids[i][1]]=0;}
static void source(d_part *t,int i,int target,int phase,int amount){t->p[d_lfo_ids[i][1]]=amount;t->p[d_lfo_ids[i][2]]=1;t->p[d_lfo_ids[i][3]]=target;t->lfo[i]=(uint32_t)phase;}
int main(void){
 d_init();d_part *t=&dp[0];assert(t->p[P_LFO_AMT]==12&&t->p[P_LFO2_AMT]==0&&t->p[P_LFO3_AMT]==0);
 uint32_t phase[3];for(int i=0;i<3;i++)phase[i]=t->lfo[i];d_render(out,256);
 for(int i=0;i<3;i++)assert(t->lfo[i]==phase[i]+d_lfo_inc[t->p[d_lfo_ids[i][0]]]*256u);
 assert(t->lfo[0]!=t->lfo[1]&&t->lfo[1]!=t->lfo[2]);
 for(int i=0;i<3;i++){
  d_init();t=&dp[0];quiet_lfos(t);source(t,i,DL_FILTER,0x80000000u,127);d_prepare_part(t);assert(t->modulation[DL_FILTER]>100);
  assert(dp[1].p[d_lfo_ids[i][1]]==(i?0:12));
  d_init();t=&dp[0];quiet_lfos(t);d_note_on(0,60,100,0);source(t,i,DL_PITCH,0x80000000u,127);d_prepare_part(t);uint32_t higher=t->osc[0].inc;
  source(t,i,DL_PITCH,0,127);d_prepare_part(t);assert(higher>t->osc[0].inc&&t->modulation[DL_PITCH]<0);
  d_init();t=&dp[0];quiet_lfos(t);source(t,i,DL_AMP,0x80000000u,127);d_prepare_part(t);int high=t->gainstep[0];assert(t->modulation[DL_AMP]>32000);
  source(t,i,DL_AMP,0,127);d_prepare_part(t);assert(t->gainstep[0]<high/100&&t->modulation[DL_AMP]<10);
  d_init();t=&dp[0];quiet_lfos(t);source(t,i,DL_PAN,0x80000000u,127);d_prepare_part(t);assert(!t->gainstep[0]&&t->gainstep[1]>0);
  source(t,i,DL_PAN,0,127);d_prepare_part(t);assert(t->gainstep[0]>0&&!t->gainstep[1]);
 }
 d_init();t=&dp[0];quiet_lfos(t);source(t,0,DL_FILTER,0x80000000u,64);source(t,1,DL_FILTER,0,64);d_prepare_part(t);assert(da(t->modulation[DL_FILTER])<=1);
 for(int target=0;target<DL_COUNT;target++){
  d_init();for(int p=0;p<4;p++){for(int i=0;i<3;i++)source(&dp[p],i,target,0x80000000u,127);for(int k=0;k<4;k++)d_note_on(p,48+p*3+k*4,110,0);}
  for(int b=0;b<400;b++){d_render(out,256);for(int n=0;n<512;n++)assert(out[n]>=-32767&&out[n]<=32767);}
 }
 d_init();assert(d_send(DC_PARAM,0,P_LFO2_TARGET,999,0));assert(d_send(DC_PARAM,0,P_LFO3_SHAPE,-4,0));d_render(out,256);assert(dp[0].p[P_LFO2_TARGET]==50&&dp[0].p[P_LFO3_SHAPE]==0);
 d_midi(0xb0,1,90,0);assert(dp[0].p[P_LFO_AMT]==90&&dp[0].p[P_LFO2_AMT]==0);d_midi(0xb0,121,0,0);assert(dp[0].p[P_LFO_AMT]==12);
 puts("PASS independent LFO phases/rates, per-source and per-part isolation, real filter/pitch/amp/pan modulation, summed cancellation, maximum-depth four-part render, clamps and CC1 compatibility");return 0;
}

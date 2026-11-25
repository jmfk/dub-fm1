#include <assert.h>
#include <stdio.h>
#include "engine.h"
static int32_t out[512];
static int value(int i){return i<20?d_fxdd[i/10][i%10]:i<26?d_fxdv[i-20]:d_fxg[d_fx_global_ids[i-26]];}
static int maxval(int i){return i<20?(i%10==D_SYNC?8:i%10==D_TIME_MS?d_delay_max_ms(i/10):d_max[i%10]):i<26?v_max[i-20]:g_max[d_fx_global_ids[i-26]];}
int main(void){
 for(int i=0;i<30;i++){
  d_init();int base=value(i);int16_t beforeD[2][D_COUNT],beforeV[V_COUNT],beforeG[G_COUNT];memcpy(beforeD,dd,sizeof dd);memcpy(beforeV,dv,sizeof dv);memcpy(beforeG,dg,sizeof dg);
  for(int t=0;t<4;t++){dp[t].p[P_LFO_SHAPE]=1;dp[t].lfo[0]=0x80000000u;d_route_set(&dp[t],0,0,21+i);d_route_set(&dp[t],0,1,127);d_prepare_part(&dp[t]);}d_prepare_fx();assert(value(i)==maxval(i)&&value(i)>=base);
  for(int t=0;t<4;t++){dp[t].lfo[0]=0;d_prepare_part(&dp[t]);}d_prepare_fx();int lo=i<20&&i%10==D_TIME_MS?1:0;assert(value(i)==lo);
  assert(!memcmp(beforeD,dd,sizeof dd)&&!memcmp(beforeV,dv,sizeof dv)&&!memcmp(beforeG,dg,sizeof dg));
 }
 d_init();dp[0].filter_env.v=D_Q;d_route_set(&dp[0],11,0,24);d_route_set(&dp[0],11,1,-127);d_prepare_part(&dp[0]);d_prepare_fx();assert(d_fxdd[0][D_FEEDBACK]==0&&dd[0][D_FEEDBACK]==d_default[D_FEEDBACK]);
 d_route_set(&dp[0],11,0,20);d_prepare_part(&dp[0]);d_prepare_fx();assert(d_fxdd[0][D_FEEDBACK]==d_default[D_FEEDBACK]);
 d_init();d_note_on(0,60,100,0);int64_t energy=0;for(int b=0;b<8;b++){d_render(out,256);for(int j=0;j<512;j++)energy+=da(out[j]);}assert(energy>1000);
 d_init();d_route_set(&dp[0],0,0,50);d_route_set(&dp[0],0,1,127);dp[0].p[P_LFO_SHAPE]=1;dp[0].lfo[0]=0;d_note_on(0,60,100,0);d_render(out,256);assert(d_fxg[G_MASTER]==0&&dg[G_MASTER]==96);for(int j=0;j<512;j++)assert(!out[j]);
 puts("PASS all30 FX destinations: four-track sum, bounds/discrete rounding, signed ENV, OFF restore, immutable base params and actual audio master modulation");
}

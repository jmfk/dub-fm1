#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "panel.h"
static int32_t out[512];
int main(void){
 for(int index=0;index<4;index++){
  d_init();d_mod_index=index;d_clock_set(0,D_CLK_INT);dg[G_TEMPO]=120;
  for(int p=0;p<4;p++){dp[p].p[P_LFO2_RATE]=D_LFO_SYNC|D_LFO_INIT|(5<<7)|48;}
  unsigned left=0;uint32_t phases[4][3]={{0}};
  for(int sample=0;sample<4096;sample++){
   if(sample%61==7)d_send(DC_MOD_RATE,0,0,(sample/61+index)%4,0);
   int selected=sample%61==7?(sample/61+index)%4:d_mod_index;
   if(!left){unsigned span=d_mod_intervals[selected];left=span;for(int p=0;p<4;p++)for(int l=0;l<3;l++)phases[p][l]+=d_lfo_increment(dp[p].p[d_lfo_ids[l][0]])*span;}
   d_render(out,1);left--;assert(d_mod_left==left);
   for(int p=0;p<4;p++)for(int l=0;l<3;l++)assert(dp[p].lfo[l]==phases[p][l]);
  }
 }
 for(int i=0;i<4;i++){
  d_init();d_mod_index=i;ds_pattern_at(0)->drums=1;unsigned span=d_mod_intervals[i];
  dp[0].p[P_LFO_AMT]=0;dp[0].p[P_ENV_AMT]=64;dp[0].env_amount[8]=0;dp[0].p[P_CUT]=30;
  d_pad[0].level=63;int32_t c0=dp[0].c[0],g0=dp[0].gain[0],pad0=d_pad_gain[0];
  d_render(out,1);int32_t cs=dp[0].cstep[0],gs=dp[0].gainstep[0],ps=d_pad_step[0];
  d_render(out,span-1);assert(dp[0].c[0]==c0+cs*(int)span);assert(dp[0].gain[0]==g0+gs*(int)span);assert(d_pad_gain[0]==pad0+ps*(int)span);assert(d_mod_left==0);
  // Finish old span before applying new rate; no early phase advance/ramp restart.
  d_render(out,3);unsigned old=d_mod_span,left=d_mod_left;d_send(DC_MOD_RATE,0,0,(i+1)%4,0);d_render(out,1);assert(d_mod_span==old&&d_mod_left==left-1);
  if(d_mod_left)d_render(out,d_mod_left);d_render(out,1);assert(d_mod_span==d_mod_intervals[(i+1)%4]);
 }
 d_init();panel_init();panel_nav(12,0);const char *labels[12];panel_labels(labels);assert(!strcmp(labels[6],"ISOLATE")&&!strcmp(labels[7],"MODRATE"));panel_button(7,1,1);panel_button(7,0,2);assert(panel.page[12]==1);dub_control c[4];panel_controls(c);assert(c[0].op==DC_MOD_RATE&&c[1].op<0);
 panel_turn(2,3);d_render(out,32);assert(d_mod_index==3);panel_turn(3,-1);d_render(out,32);assert(d_mod_index==3);panel_turn(2,-9);d_render(out,32);assert(!d_mod_index);
 d_send(DC_MOD_RATE|256,0,0,1,0);d_send(DC_MOD_RATE|256,0,0,1,0);d_render(out,32);assert(d_mod_index==2);d_init();assert(!d_mod_index&&d_mod_span==32&&!d_mod_left);
 puts("PASS32/16/8/4 sample phases/free+beat LFOs, midspan transitions, filter/track/pad ramps, physical pages/K1/unused knobs, queued relative values and boot default32");
}

/* Differential integration fixture: R32 frame renderer vs R39 track blocks.
 * Write PCM + every USB tap + scheduler/clock boundaries, including frame wrap. */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
static FILE *capture;
static void tap(const int16_t *p){assert(fwrite(p,2,17,capture)==17);}
#define D_CAPTURE_HOOK(frame) tap(frame)
#include "engine.h"
static int32_t out[512];
int main(int argc,char **argv){assert(argc==2);capture=fopen(argv[1],"wb");assert(capture);
 for(int scenario=0;scenario<4;scenario++){
  d_init();d_capture_active=1;d_clock_set(1,1);
  for(int p=0;p<4;p++){dp[p].voice_mode=p%3;dp[p].p[P_LEVEL]=40;dp[p].p[P_LFO_AMT]=30;dp[p].p[P_LFO_TARGET]=p;}
  for(int p=0;p<3;p++){
   ds_pattern *q=ds_pattern_at(p);q->drums=p&1;q->repeats=1;q->next=p==2?DS_STOP:p+1;
   for(int t=0;t<4;t++){
    ds_track *tr=&q->track[t];tr->length=1+(p+t)%3;tr->grid=(t&1)?3:4;
    for(int k=0;k<4;k++){ds_step *step=&tr->step[k];step->note[0]=48+t*5+k;step->note[1]=55+t*3+k;step->length=k==0?10:k==1?75:102;step->velocity=45+k*17;}
    if(q->drums)for(int k=0;k<4;k++)q->hits[k]=(uint16_t)(1u<<(k+t));
   }
  }
  if(scenario==1)d_clock_set(0,D_CLK_EXT);
  if(scenario==2)d_frame=0xffffe000u;
  if(scenario==3)d_clock_set(0,D_CLK_INT);
  dg[G_TEMPO]=239;d_seq_control(DS_PLAY,0,0,0);
  for(int block=0;block<2100;block++){
   if(scenario!=3&&block<300&&block%7==0)d_clock_receive(0);
   if(block==450||block==900||block==1200)d_mod_index=(uint8_t)((block/150)%4);
   if(block==600){d_osc_limit_set(0);d_sample_limit_set(0);}
   if(block==650){d_osc_limit_set(33);d_sample_limit_set(12);}
   if(block==700)d_cpu_set(2,0);if(block==750)d_cpu_set(2,1);
   if(block==800)d_cpu_set(3,1);if(block==1000)dv[V_DECAY]=100;
   if(block==1100)for(int j=0;j<2;j++){dd[j][D_SYNC]=8;dd[j][D_TIME_MS]=7000;}
   if(block==1300){d_seq_control(DS_PATTERN,0,0,0);d_seq_control(DS_PLAY,0,0,0);}
   if(block==1500)d_clock_set(0,D_CLK_INT);
   if(block==1600)dg[G_TEMPO]=30;
   if(block==1700){d_sample_limit_set(0);d_osc_limit_set(4);}
   if(block==1800){d_sample_limit_set(4);d_osc_limit_set(4);}
   unsigned n=block%5==0?1:block%5==1?31:block%5==2?127:block%5==3?253:256;
   d_render(out,n);assert(fwrite(out,4,n*2,capture)==n*2);
   uint32_t state[]={d_frame,ds.tick,ds.phase,ds.pattern,ds.elapsed,ds.repeat,ds.sub,ds.playing,ds.external,ds.rng,d_mod_left,d_clock_phase};
   fwrite(state,sizeof state,1,capture);
   while(d_clock_take()){uint32_t stamp=d_clock_stamp;fwrite(&stamp,4,1,capture);}
  }
 }
 fclose(capture);return 0;
}

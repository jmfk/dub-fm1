/* Compare cache ON/OFF binaries, sample-for-sample over runtime changes. */
#include <stdio.h>
#include "engine.h"
static int32_t out[512];
int main(int argc,char **argv){if(argc!=2)return 1;FILE *f=fopen(argv[1],"wb");if(!f)return 1;d_init();d_cpu_set(3,1);
 for(int block=0;block<3000;block++){
  if(block==20||block==500||block==1800)for(int p=0;p<4;p++)d_note_on(p,48+p*3,100,0);
  if(block==60||block==550||block==1850)for(int p=0;p<4;p++)d_note_off(p,48+p*3,0);
  if(block==250){d_route_set(&dp[0],12,0,20);/* final AMP unrouted: preserve tiny filter DC exactly */}
  if(block==600){dp[1].p[P_CUT]=90;dp[2].p[P_MODE]=2;dp[0].p[P_LEVEL]=10;}
  if(block==700)d_cpu_set(1,0);
  if(block==900)d_cpu_set(1,1);
  if(block==1100){dp[0].p[P_LFO_AMT]=80;dp[0].p[P_LFO_TARGET]=0;}
  if(block==1400)dp[0].p[P_LFO_AMT]=0;
  if(block==1900)d_retrigger(&dp[0]);
  if(block==2100){ds_pattern_at(0)->drums=1;d_note_on(3,36,110,0);}
  unsigned n=block%3==0?256:block%3==1?127:19;d_render(out,n);fwrite(out,sizeof(int32_t),2*n,f);
 }
 fclose(f);return 0;}

#include <stdio.h>
#include "engine.h"
static int32_t out[512];
int main(int argc,char **argv){if(argc!=2)return 1;FILE *f=fopen(argv[1],"wb");if(!f)return 1;d_init();d_cpu_set(3,1);
 for(int block=0;block<2400;block++){
  if(block%300==0){for(int p=0;p<4;p++){dp[p].voice_mode=(block/300+p)%3;dp[p].p[P_MODE]=(block/300+p)%4;for(int n=0;n<4;n++)d_note_on(p,40+p*5+n*3,70+n*15,0);}}
  if(block%300==190)for(int p=0;p<4;p++)for(int n=0;n<4;n++)d_note_off(p,40+p*5+n*3,0);
  if(block==350){d_route_set(&dp[0],11,0,2);d_route_set(&dp[0],11,1,-90);d_route_set(&dp[0],12,0,2);d_route_set(&dp[0],12,1,100);}
  if(block==450){d_route_set(&dp[0],12,0,12);d_route_set(&dp[0],12,1,40);d_route_set(&dp[0],11,0,2);d_route_set(&dp[0],11,1,30);}
  if(block==550){dp[1].p[P_LFO_TARGET]=12;dp[1].p[P_LFO_AMT]=100;}
  if(block==650)d_cpu_set(1,0);
  if(block==850)d_cpu_set(1,1);
  if(block==1100)d_cpu_set(0,1);
  if(block==1300)d_cpu_set(0,4);
  if(block==1500){for(int p=0;p<4;p++){dp[p].p[P_CUT]=110;dp[p].p[P_RES]=110;dp[p].p[P_DRIVE]=127;}}
  if(block==1800)d_retrigger(&dp[0]);
  unsigned n=block%3==0?256:block%3==1?127:19;d_render(out,n);fwrite(out,sizeof(int32_t),2*n,f);
 }
 fclose(f);return 0;}

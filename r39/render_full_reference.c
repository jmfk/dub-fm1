#include <assert.h>
#include <stdio.h>
#include "engine.h"
static int16_t captured[17];
/* Main PCM plus raw stems/buses: exact comparison checks routing as well. */
int main(int argc,char **argv){if(argc!=2)return 1;FILE *file=fopen(argv[1],"wb");assert(file);int32_t out[512];d_init();d_cpu_set(3,1);ds_pattern_at(0)->drums=1;
 dd[0][D_FEEDBACK]=65;dd[0][D_TIME_MS]=8000;dd[1][D_TIME_MS]=1000;dv[V_DECAY]=100;
 for(int p=0;p<3;p++){dp[p].voice_mode=p;dp[p].p[P_LEVEL]=45;dp[p].p[P_DRIVE]=p*50;for(int n=0;n<4;n++)d_note_on(p,48+n*5,65,0);}
 for(int block=0;block<2700;block++){
  if(block%22==0)for(int i=0;i<11;i++)d_drum_on(d_drum_keys[i],40+i*7,i%2?0:4);
  if(block%22==15)for(int i=0;i<11;i++)d_drum_off(d_drum_keys[i],4);
  if(block==300){for(int i=0;i<11;i++){d_pad[i].level=20+i*10;d_pad[i].pan=i*12;for(int j=0;j<3;j++)d_pad[i].send[j]=(i+j)%3?1+i*10:0;}d_mix_dirty=1;}
  if(block==500){d_pad[1].mute=1;d_pad[3].mute=1;d_mix_dirty=1;}
  if(block==510)d_cpu_set(3,0);if(block==610)d_cpu_set(2,0);if(block==710)d_cpu_set(3,1);if(block==810)d_cpu_set(2,1);
  if(block==700)dp[3].p[P_DRIVE]=127;
  if(block==900)for(int p=0;p<3;p++)for(int n=0;n<4;n++)d_note_off(p,48+n*5,0);
  if(block==1100)for(int p=0;p<3;p++)for(int n=0;n<4;n++)d_note_on(p,52+n*5,80,0);
  if(block==1300)d_cpu_set(2,0);if(block==1400)d_cpu_set(2,1);if(block==1700)d_cpu_set(1,0);if(block==1800)d_cpu_set(1,1);
  if(block==1900)dv[V_DECAY]=80;if(block==2000)dv[V_DECAY]=100;
  unsigned count=block%3==0?256:block%3==1?253:127;d_render(out,count);fwrite(out,sizeof(int32_t),count*2,file);fwrite(d_drum_raw,sizeof d_drum_raw,1,file);fwrite(d_drum_bus,sizeof d_drum_bus,1,file);
 }
 fclose(file);return 0;}

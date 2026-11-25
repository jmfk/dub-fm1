#include <stdio.h>
#include "engine.h"
int main(int argc,char **argv){if(argc!=2)return 1;FILE *f=fopen(argv[1],"wb");if(!f)return 1;d_init();d_cpu_set(3,1);dd[0][D_TIME_MS]=8000;dd[1][D_TIME_MS]=402;d_fx_init();
 for(unsigned i=0;i<14u*44100;i++){
  if(!(i&31))d_prepare_fx();
  if(i==4u*44100)dv[V_DECAY]=100;
  if(i==10u*44100)d_cpu_set(2,0);
  if(i==11u*44100)d_cpu_set(2,1);
  int32_t in[3]={i==1000||i==8u*44100+1000?24000:0,i==1300?-17000:0,i==2000||i==5u*44100?16000:0},out[2];d_fx_tick(i==900?2000:0,i==900?-2000:0,in,&out[0],&out[1]);fwrite(out,sizeof(int32_t),2,f);
 }
 fclose(f);return 0;}

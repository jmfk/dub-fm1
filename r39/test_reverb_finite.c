#include <stdio.h>
#include "engine.h"
int main(void){uint32_t h=2166136261u;for(int t=0;t<=99;t++){d_init();dv[V_DECAY]=t;d_prepare_fx();for(int j=0;j<7000;j++){int32_t out[2];d_reverb_tick(j<1500?((j*7919)%12000-6000):0,out);h=(h^(uint32_t)out[0])*16777619u;h=(h^(uint32_t)out[1])*16777619u;}}printf("Finite0..99 audio hash: %u\n",h);}

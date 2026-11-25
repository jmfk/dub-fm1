#include <assert.h>
#include <stdio.h>
#include "engine.h"
int main(void){d_init();int32_t edge[]={-65535,-32768,-1,0,1,32767,65535};int32_t gain[]={0,1,127,16384,32767,32768};for(unsigned i=0;i<sizeof edge/sizeof *edge;i++)for(unsigned j=0;j<sizeof gain/sizeof *gain;j++)assert(d_q15_small(edge[i],gain[j])==dm(edge[i],gain[j]));
 unsigned seed=7;for(int i=0;i<100000;i++){seed=seed*1664525u+1013904223u;int32_t x=(int32_t)(seed%131071)-65535;seed=seed*1664525u+1013904223u;int32_t g=seed%32769;assert(d_q15_small(x,g)==dm(x,g));}
 for(int k=0;k<30;k++){d_part *t=&dp[k%4];for(int e=8;e<10;e++){d_route_env(t,e)->v=(e+1)*D_Q/10;t->env_target[e]=(k+e)%5==0?20:e<8?4+(k+e)%8:2;t->env_amount[e]=k%3==0?127:k%3==1?-90:40;}d_cpu[1]=k&1;d_prepare_lfos(t);for(int i=0;i<9;i++)assert(d_amp_fast(t,i)==d_amp_envelopes(t,i));}
 // A sustained/no-work envelope tick is an exact no-op.
 d_env e={123456,0,123456,0,3},before=e;uint8_t cfg[4]={1,1,60,1};d_env_tick(&e,cfg);assert(!memcmp(&e,&before,sizeof e));
 puts("PASS bounded32-bit Q15 extremes/random signed reference, shared canonical/custom/inverted/multi-ENV fast paths and sustained-envelope no-op");}

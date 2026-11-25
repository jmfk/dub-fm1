#include <assert.h>
#include <stdio.h>
#include "engine.h"
int main(void){d_init();uint32_t seed=7;int32_t h[16];
 for(int t=0;t<5000;t++){for(int i=0;i<16;i++){seed=seed*1664525u+1013904223u;h[i]=(i&1)?0:(int32_t)(seed&2097151)-1048576;}for(int p=0;p<16;p++)assert(d_fir_interp_sum(h,p)==d_fir_sum(h,p));}
 int32_t out[512];for(int mode=0;mode<4;mode++){d_init();d_cpu_set(2,mode&1);d_cpu_set(3,(mode>>1)&1);d_note_on(0,48,80,0);for(int i=0;i<10;i++)d_render(out,256);
  if(!(mode&1))for(int b=0;b<2;b++){assert(!d_down_nz[b]);for(int ch=0;ch<2;ch++)assert(!d_up_nz[b][ch]);}
  if(!(mode&2)){assert(!d_down_nz[2]);for(int ch=0;ch<2;ch++)assert(!d_up_nz[2][ch]);}
 }
 puts("PASS sparse interpolation equals wide FIR for both phases; disabled FX histories remain clear in all4 switch combinations");}

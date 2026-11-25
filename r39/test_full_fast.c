#include <assert.h>
#include <stdio.h>
#include "engine.h"
int main(void){d_init();assert(dd[0][D_FEEDBACK]==40&&dd[1][D_FEEDBACK]==45);uint32_t seed=7;int32_t h[16];
 for(int t=0;t<20000;t++){
  for(int i=0;i<16;i++){seed=seed*1664525u+1013904223u;h[i]=(int32_t)(seed&2097151)-1048576;}
  for(int p=0;p<16;p++){int64_t ref=0;for(int k=0;k<15;k++)ref+=(int64_t)h[(p+16-k)&15]*d_fir[k];assert(ref==d_fir_sum(h,p));}
 }
 for(int i=0;i<11;i++){uint32_t n=d_drum_offsets[i+1]-d_drum_offsets[i];for(uint32_t p=0;p<n*2;p++){
  uint32_t at=p>>1;int32_t ref=d_drum_pcm[d_drum_offsets[i]+at];if((p&1)&&at+1<n)ref=(ref+d_drum_pcm[d_drum_offsets[i]+at+1])/2;assert(d_drum_read(i,p,n)==ref);
 }assert(d_drum_read(i,0,n)==d_drum_pcm[d_drum_offsets[i]]);}
 for(int x=-32768;x<=32767;x+=19)for(int g=0;g<=32768;g+=256)assert(d_pad_scale(x,g)==dm(x,g));
 for(int x=-100000;x<=100000;x++){int a=dc(x,-32767,32767);assert(d_soft(x)==(a-dm(dm(a,a),a)/3)*3/2);}
 puts("PASS symmetric FIR bounded random/reference dots, every PCM position/interpolation/cache boundary/retrigger, bounded drum/unity/soft-clip arithmetic");}

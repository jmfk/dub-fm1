#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "engine.h"
static void advance(void){if(!(d_rate_frame&3u)&&++d_dpos==DD_N)d_dpos=0;d_rate_frame++;}
static void setup(int ms){d_init();for(int b=0;b<2;b++){dd[b][D_TIME_MS]=ms;dd[b][D_MODE]=0;dd[b][D_FEEDBACK]=0;dd[b][D_ROUTE]=2;dd[b][D_WIDTH]=127;}d_prepare_fx();d_fx_init();}
int main(void){
 int prev=0,maxerr=0;for(int x=0;x<=32767;x++){int y=d_delay_decode(d_delay_encode(x)),neg=d_delay_decode(d_delay_encode(-x));assert(y>=prev&&neg==-y);prev=y;if(abs(y-x)>maxerr)maxerr=abs(y-x);}assert(d_delay_encode(0)==0&&d_delay_decode(0)==0&&maxerr<650);
 /* Both actual decimator / delay / interpolator chains; ring wraps twice. */
 setup(8000);dg[G_TEMPO]=30;for(int b=0;b<2;b++)dd[b][D_SYNC]=8;d_prepare_fx();
 int peak[2]={0,0},when[2]={0,0};int32_t z[2];
 for(int i=0;i<400000;i++){
  for(int b=0;b<2;b++){
   d_delay_convert(b,i==b*400?24000:0,z);
   if(abs(z[0])>peak[b]){peak[b]=abs(z[0]);when[b]=i;}
   assert(z[1]==0);if(i<176390)assert(z[0]==0);
  }advance();
 }
 for(int b=0;b<2;b++){assert(peak[b]>1000);assert(abs(when[b]-(176400+b*400+42))<=4);}assert(d_dpos<DD_N);
 printf("8s echo peaks (22050Hz frames): A=%d B=%d, expected nominal176400 + converter42, B input offset400; peak%d/%d, companding max error%d\n",when[0],when[1],peak[0],peak[1],maxerr);
 /* At8000ms the optional right stereo offset must also fit without clamping. */
 assert(8000*7056/5+(8000*7056/5)/11<(DD_N-2)*256);
 /* Input anti-alias rejection at frequencies above the delay Nyquist. */
 double energies[2]={0,0};
 for(int test=0;test<2;test++){setup(100);double hz=test?4500:1000;
  for(int i=0;i<12000;i++){int32_t in=(int32_t)(12000*sin(2*3.141592653589793*hz*i/22050));d_delay_convert(0,in,z);if(i>6000)energies[test]+=(double)z[0]*z[0];advance();}
 }
 assert(energies[0]>1e9&&energies[1]<energies[0]*0.01);
 /* Ping-pong uses stereo cross-feedback at the same8s repeat interval. */
 setup(8000);dd[0][D_FEEDBACK]=50;d_prepare_fx();int peaks[2]={0,0};
 for(int i=0;i<360000;i++){d_delay_convert(0,i==0?24000:0,z);if(abs(z[0])>peaks[0])peaks[0]=abs(z[0]);if(abs(z[1])>peaks[1]){peaks[1]=abs(z[1]);assert(i>352750);}advance();}assert(peaks[1]>500&&peaks[1]<peaks[0]);
 /* Actual processor time modulation extremes and panic clears new histories. */
 setup(8000);dd[0][D_MODE]=1;dd[0][D_MOD]=127;dd[0][D_FEEDBACK]=94;dd[0][D_ROUTE]=1;d_prepare_fx();for(int i=0;i<200000;i++){d_delay_convert(0,i<100?30000:0,z);d_fxlfo+=68174u;advance();assert(abs(z[0])<100000&&abs(z[1])<100000);}d_fx_init();for(unsigned i=0;i<sizeof d_rate_down/sizeof(int32_t);i++)assert(!((int32_t*)d_rate_down)[i]);for(unsigned i=0;i<sizeof d_rate_up/sizeof(int32_t);i++)assert(!((int32_t*)d_rate_up)[i]);
 puts("PASS8s actual stereo delay chains at30BPM, independent buses, ring wraps, stereo offset headroom, logarithmic codec bounds/silence, anti-alias rejection, ping-pong second echo16s, modulation/feedback limits and panic converter reset");
}

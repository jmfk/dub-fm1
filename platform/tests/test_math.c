#define A_MATH_CHECK 1
#include "engine.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 for(int32_t x=-18000000;x<=18000000;x++)assert(a_div127(x)==x/127);
 for(uint32_t i=0;i<1000000;i++){
  int32_t x=(int32_t)arand();assert(a_div3(x)==x/3);
  int32_t a=(int32_t)(arand()%131071u)-65535,b=arand()%32769u;
  assert(a_pcm_gain(a,b)==am(a,b));
 }
 for(uint32_t a=0;a<=25400;a++){
  assert((uint32_t)(((uint64_t)a*1431656u)>>32)==a/3000u);
  assert((uint32_t)(((uint64_t)a*2863312u)>>32)==a/1500u);
 }
 for(uint32_t a=0;a<=a_inc[34];a++)assert((uint32_t)(((uint64_t)a*458129845u)>>40)==a/2400u);
 puts("PASS: exhaustive bounded /127, drum decay and kick division; million-case /3 and PCM multiply equivalence");
}

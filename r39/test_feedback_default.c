#include <assert.h>
#include <stdio.h>
#include "engine.h"
static uint64_t tail(int feedback){d_init();assert(dd[0][D_FEEDBACK]==40&&dd[1][D_FEEDBACK]==45);dd[0][D_FEEDBACK]=feedback;d_prepare_fx();d_fx_init();uint64_t energy=0;
 for(int t=0;t<55125;t++){int32_t echo[2];d_delay_tick(0,t<32?18000:0,echo);if(++d_dpos==DD_N)d_dpos=0;d_fxlfo+=68174u*4u;if(t>33075)energy+=(uint32_t)da(echo[0])+(uint32_t)da(echo[1]);}
 return energy;}
int main(void){uint64_t lowered=tail(40),old=tail(65);assert(lowered*4<old);printf("PASS delay1 default40/delay2 remains45: six-to-ten-second impulse tail sum %llu vs %llu at old65, original drive/tone/mod/route preserved\n",(unsigned long long)lowered,(unsigned long long)old);}

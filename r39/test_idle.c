#include <assert.h>
#include <stdio.h>
#include "engine.h"
static int32_t out[512];
int main(void){d_init();d_cpu_set(1,0);d_cpu_set(2,0);d_render(out,256);for(int p=0;p<4;p++)assert(d_idle[p].sleep&&d_idle[p].sample==0);
 d_note_on(0,48,100,0);assert(!d_idle[0].sleep);d_render(out,256);assert(!d_idle[0].sleep);d_note_off(0,48,0);
 for(int i=0;i<180;i++)d_render(out,256);assert(!dp[0].post_env.stage&&d_idle[0].sleep);int a=dp[0].s1,b=dp[0].s2;d_render(out,256);assert(dp[0].s1==a&&dp[0].s2==b);
 // Gain smoothing still advances; a live coefficient/LFO change wakes the path.
 dp[0].p[P_LEVEL]=20;d_render(out,256);int gain=dp[0].gain[0];dp[0].p[P_LEVEL]=120;d_render(out,256);assert(dp[0].gain[0]>gain);
 d_retrigger(&dp[0]);assert(!d_idle[0].sleep);d_render(out,32);assert(!d_idle[0].sleep);
 d_panic_part(&dp[0]);d_render(out,256);assert(d_idle[0].sleep);d_send(DC_NOTE,0,60,100,0);d_render(out,7);assert(!d_idle[0].sleep&&dp[0].slot[0].gate);
 puts("PASS exact zero/fixed-point idle caching, release tails, immediate note/retrigger wake, live gain smoothing and non32 render blocks");}

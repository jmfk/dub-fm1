#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "panel.h"
static int32_t pcm[512];static int ticks;static uint32_t previous,min_gap,max_gap;
static void collect(void){while(d_clock_take()){uint32_t at=d_clock_stamp;if(ticks){uint32_t gap=at-previous;if(gap<min_gap)min_gap=gap;if(gap>max_gap)max_gap=gap;}previous=at;ticks++;}}
static void advance(uint32_t n,int drain){while(n){uint32_t k=n>256?256:n;d_render(pcm,k);if(drain)collect();n-=k;}}
static void reset_count(void){ticks=0;previous=0;min_gap=0xffffffffu;max_gap=0;}
int main(void){
 d_init();panel_init();assert(d_clock_mode==D_CLK_AUTO&&!d_clock_output);advance(44100,1);assert(!ticks);
 dg[G_TEMPO]=120;d_clock_set(0,D_CLK_INT);d_clock_set(1,1);reset_count();advance(88200,1);assert(ticks==96&&min_gap==918&&max_gap==919);
 d_midi(0xf8,0,0,0);advance(919,0);d_midi(0xf8,0,0,0);assert(!d_clock_external()&&dg[G_TEMPO]==120);
 d_clock_set(1,0);assert(!d_clock_take());d_clock_set(0,D_CLK_EXT);d_clock_set(1,1);reset_count();advance(44100,1);assert(!ticks&&!d_clock_live());
 dg[G_TEMPO]=143;d_midi(0xf8,0,0,0);advance(919,1);d_midi(0xf8,0,0,1);assert(d_clock_source==0&&d_clock_count==1);d_midi(0xf8,0,0,0);collect();assert(ticks==1&&d_clock_tempo()==120&&dg[G_TEMPO]==143);
 for(int i=0;i<12;i++){advance(919,1);d_midi(0xf8,0,0,0);collect();}assert(ticks==13&&d_clock_external());reset_count();advance(44100,1);assert(!ticks&&!d_clock_live()&&d_clock_tempo()==120);
 d_clock_set(0,D_CLK_AUTO);reset_count();assert(d_clock_tempo()==143);d_midi(0xf8,0,0,1);advance(919,0);d_midi(0xf8,0,0,1);collect();assert(d_clock_external()&&d_clock_source==1&&d_clock_tempo()==120);reset_count();advance(22050,1);assert(!ticks&&!d_clock_external()&&d_clock_tempo()==143);advance(44100,1);assert(ticks==57);
 d_clock_set(0,D_CLK_INT);d_clock_set(1,0);advance(1000,0);d_clock_set(1,1);assert(!d_clock_take());advance(44100*3u,0);assert(d_clock_drops&& !d_clock_take());reset_count();advance(1000,1);assert(ticks>0);
 d_send(DC_PANIC,0,0,0,0);reset_count();advance(1000,1);assert(d_clock_output&&ticks>0);
 preset_patch patch;preset_defaults(&patch);preset_apply(&patch);assert(d_clock_mode==D_CLK_INT&&d_clock_output);
 panel_nav(4,0);dub_control c[4];panel_controls(c);assert(c[2].op==DC_CLOCK&&c[2].index==0&&c[3].index==1);panel_turn(4,2);advance(256,1);assert(d_clock_mode==D_CLK_AUTO);panel_turn(5,-1);advance(256,1);assert(!d_clock_output);
 // Queue timing remains correct across frame-counter wrap.
 d_clock_set(0,D_CLK_EXT);d_frame=0xfffffc00u;d_midi(0xf8,0,0,0);advance(919,0);d_midi(0xf8,0,0,0);advance(919,0);d_midi(0xf8,0,0,0);assert(d_clock_external()&&d_clock_tempo()==120);
 puts("PASS 24PPQN exact count/sample spacing, OFF/INT/EXT/AUTO, source locking, no base-tempo overwrite, external relay/no doubled clocks, timeout/fallback, stale/full queue, panic/preset independence, physical knobs and frame wrap");
}

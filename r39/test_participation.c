#include <assert.h>
#include <stdio.h>
#include "engine.h"
static int32_t pcm[512];
int main(void){d_init();d_bench_command(9,1);d_bench_command(7,1);d_bench_command(0,1);d_bench_command(4,4);d_bench_command(5,4);d_bench_command(8,1);for(int t=0;t<3;t++)d_note_on(t,48+t*7,90,0);int notes[]={36,38,42,45};for(int i=0;i<4;i++)d_note_on(3,notes[i],90,0);for(int f=0;f<8;f++)d_render(pcm,256);d_bench_command(-1,0);for(int p=0;p<3;p++){assert(d_rt_snapshot[57+p]==1);assert(d_sound_frames[p]>256);}assert(d_sound_frames[3]>256&&d_common_frames>256&&d_sample_active_count()<=4);printf("PASS simultaneous tracks1/2/3/drums: OSC1/1/1, contribution frames%u/%u/%u/%u, all-four%u\n",d_sound_frames[0],d_sound_frames[1],d_sound_frames[2],d_sound_frames[3],d_common_frames);d_bench_command(9,0);assert(!d_common_frames&&!d_bench_active);}

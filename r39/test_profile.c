#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
static uint32_t timer,auto_tick;static uint32_t ticks(void){uint32_t now=timer;timer+=auto_tick;return now;}
#define D_PROFILE_TICKS() ticks()
#include "engine.h"
static uint8_t sent[D_PROF_REPLY];static unsigned sent_n,send_calls;static int wire_busy;
static void dub_profile_service(void);
static int ota_wire_send(const uint8_t *p,uint32_t n){send_calls++;dub_profile_service();if(wire_busy)return 1;assert(n==D_PROF_REPLY);memcpy(sent,p,n);sent_n=n;return 0;}
#include "profile_bridge.h"
static void block(unsigned n,uint32_t frame,uint32_t amount){d_prof_begin(n,frame);timer+=amount;d_prof_event(DPE_CACHE_FILL);d_prof_switch(DPF_DRUM);timer+=10;d_prof_current.cache_ticks=5;d_prof_end();}
int main(int argc,char **argv){
 d_init();assert(!d_prof_enabled&&!d_prof_collecting);block(32,0,100);assert(!d_prof.blocks);
 d_prof_pending=1;d_prof_begin(32,100);assert(d_prof_enabled&&d_prof.start_frame==100);timer+=10;d_prof_event(DPE_COMMAND);d_prof_switch(DPF_CONTROL);timer+=20;d_prof_switch(DPF_SYNTH);timer+=30;d_prof_switch(DPF_DRUM);timer+=40;d_prof_event(DPE_CACHE_FILL);d_prof_switch(DPF_FX);timer+=50;d_prof_switch(DPF_OUTPUT);timer+=60;d_prof_end();
 assert(d_prof.blocks==1&&d_prof.frames==32&&!d_prof.late&&d_prof.total[0]==10&&d_prof.total[5]==60);assert(d_prof.top[0].ticks==210&&d_prof.events[DPE_CACHE_FILL]==1);
 block(32,132,30000);assert(d_prof.late==1&&d_prof.late_total[0]==30000);block(64,164,40000);assert(d_prof.late==2);assert(d_prof.top[0].frames==32);block(32,228,60000);block(32,260,50000);assert(d_prof.top[0].ticks==60010&&d_prof.top[1].ticks==50010&&d_prof.top[2].ticks==30010&&d_prof.top[3].ticks==40010);
 d_prof.total[0]=0xffffffffu;block(32,292,10);assert(d_prof.total[0]==0x100000009ull);
 timer=0xfffffff0u;block(32,324,30);assert(d_prof.total[0]==0x100000027ull);
 d_prof_stats copy;uint32_t enabled;d_prof_sequence|=1;assert(!d_prof_copy(&copy,&enabled));d_prof_sequence++;assert(d_prof_copy(&copy,&enabled)&&enabled);
 uint8_t get[]={240,125,68,85,66,1,6,77,247},set[]={240,125,68,85,66,1,7,78,0,247};
 for(unsigned n=0;n<9;n++)assert(d_profile_request(get,n)<0);assert(d_profile_request(get,sizeof get)==6);assert(d_profile_request(set,sizeof set)==7);set[8]=2;assert(d_profile_request(set,sizeof set)<0);set[8]=0;
 assert(dub_profile_receive(get,sizeof get));wire_busy=1;dub_profile_service();assert(!sent_n&&send_calls==1&&dub_prof_action==6);wire_busy=0;dub_profile_service();assert(sent_n==D_PROF_REPLY&&send_calls==2&&dub_prof_action<0);assert(sent[7]==77&&sent[8]==116);
 assert(dub_profile_receive(set,sizeof set));dub_profile_service();assert(d_prof_pending==0);d_prof_begin(32,400);assert(!d_prof_enabled);dub_profile_service();assert(dub_prof_action<0&&!sent[19]);uint32_t gen=d_prof.generation,count=d_prof.blocks;block(32,432,100000);assert(d_prof.generation==gen&&d_prof.blocks==count);
 /* Duplicate enable/freeze requests must not reset a new window after a lost ACK. */
 set[7]=79;set[8]=1;assert(dub_profile_receive(set,10));dub_profile_service();d_prof_begin(32,500);timer+=1;d_prof_end();dub_profile_service();gen=d_prof.generation;assert(gen&&d_prof_enabled&&d_prof.blocks==1);assert(dub_profile_receive(set,10));dub_profile_service();assert(d_prof_pending<0&&d_prof.generation==gen&&d_prof.blocks==1);
 d_bench_active=1;d_bench_saved[0]=4;d_bench_end();assert(!d_prof_enabled);
 d_prof_enabled=1;d_prof_window_reset(100);auto_tick=10;d_prof_begin(32,100);d_prof_switch(DPF_DRUM);d_drum_cache_fill(0,0,d_drum_length(0));d_prof_end();assert(d_prof.cache_total==10&&d_prof.top[0].cache_ticks==10&&d_prof.top[0].stage[DPF_DRUM]>=10);auto_tick=0;
 d_prof_enabled=1;d_prof_window_reset(123);block(256,123,200000);block(256,379,10000);assert(d_prof_copy(&copy,&enabled));assert(d_prof_encode(sent,6,77,&copy,enabled)==D_PROF_REPLY);
 if(argc>1){FILE *f=fopen(argv[1],"wb");assert(f);assert(fwrite(sent,1,sizeof sent,f)==sizeof sent);fclose(f);}
 puts("PASS stage sums, late-block totals/top4 normalized ranking and event snapshots, timer/64-bit wrap, seqlock, zero/OFF, bounded codec, audio-owned freeze/enable ACK, duplicate request idempotence, TX retry/reentrant guard and lease disable");
}

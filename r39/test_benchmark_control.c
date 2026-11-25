#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "panel.h"
static int32_t out[512];
static void set(int f,int v){assert(d_send(DC_RUNTIME,f,0,v,0));d_render(out,32);assert(!d_rt_snapshot[0]);}
int main(int argc,char **argv){
 d_init();panel_init();uint8_t wire[256],get[]={0xf0,0x7d,0x44,0x55,0x42,1,4,77,0xf7},put[]={0xf0,0x7d,0x44,0x55,0x42,1,5,78,4,0,0xf7};
 assert(d_runtime_request(get,sizeof get)==4&&d_runtime_request(put,sizeof put)==5);put[9]=13;assert(d_runtime_request(put,sizeof put)<0);put[8]=5;put[9]=33;assert(d_runtime_request(put,sizeof put)==5);put[8]=12;assert(d_runtime_request(put,sizeof put)<0);
 ds_pattern before=*ds_pattern_at(0);uint32_t hash=d_bench_sound_hash();set(9,1);assert(d_bench_active&&d_clock_mode==D_CLK_INT&&!d_clock_output);set(9,1); /* retry must not replace original restore values */
 set(4,0);set(5,0);set(7,1);d_note_on(0,60,100,0);d_note_on(3,36,100,0);d_render(out,32);assert(!d_osc_active_count()&&!d_sample_active_count());
 set(4,2);set(5,4);set(6,2);set(1,0);set(2,0);set(3,0);d_note_on(0,60,100,0);d_note_on(3,36,100,0);set(-1,0);assert(d_rt_snapshot[5]==2&&d_rt_snapshot[6]==4&&d_rt_snapshot[7]==2&&d_rt_snapshot[8]&&d_rt_snapshot[16]==2&&d_rt_snapshot[17]==1&&d_bench_sound_hash()==hash);
 assert(!d_runtime_lease_due(15000,0)&&d_runtime_lease_due(15001,0));assert(d_runtime_lease_due(10000,0xffff0000u));
 unsigned n=d_runtime_extended_reply(wire,4,77);assert(n==137&&wire[7]==77&&wire[8]==126&&wire[n-1]==0xf7);uint8_t sum=0;for(unsigned i=1;i<n-2;i++){assert(wire[i]<128);sum^=wire[i];}assert(sum==wire[n-2]);
 if(argc>1){FILE *f=fopen(argv[1],"wb");assert(f);assert(fwrite(wire,1,n,f)==n);fclose(f);}
 set(12,1);set(13,3);set(14,1);set(15,90);set(16,1);set(17,4);set(18,1);set(19,127);assert(d_rt_snapshot[49]==1&&d_rt_snapshot[50]==3&&d_rt_snapshot[56]==127);
 set(8,1);assert(d_bench_active&&!d_osc_active_count()&&!d_sample_active_count());set(9,0);assert(!d_bench_active&&d_sample_limit==4&&d_osc_limit==8&&d_mod_index==0&&d_cpu[1]&&d_cpu[2]&&!d_cpu[3]&&d_clock_mode==D_CLK_AUTO);assert(!memcmp(&before,ds_pattern_at(0),sizeof before));for(int b=0;b<2;b++)assert(!d_delay_cfg[b][0]&&d_delay_cfg[b][1]==2&&!d_delay_cfg[b][2]&&!d_delay_cfg[b][3]);
 set(9,1);set(4,0);set(5,0);d_send(DC_PANIC,0,0,0,0);d_render(out,32);assert(!d_bench_active&&d_sample_limit==4&&d_osc_limit==8);
 ds.playing=1;d_send(DC_RUNTIME,9,0,1,0);d_render(out,32);assert(d_rt_snapshot[0]==1&&!d_bench_active);ds.playing=0;
 assert(d_runtime_reply(wire,3)==14);puts("PASS additive request IDs/validation, applied audio snapshots/caps/counts/hash, session retry/restore/REC, pattern preservation, busy refusal, wrap-safe lease and unchanged legacy reply");
}

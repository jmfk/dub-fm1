/* SPDX-License-Identifier: GPL-3.0-only; track-major blocks with exact event boundaries.
 * Buffers are fixed-size and audio-owned. No heap, I/O or extra audio latency. */
#pragma once
#define D_BLOCK_FRAMES 32u
static int32_t d_block_bus[D_BLOCK_FRAMES][5] D_POOL;
static int16_t d_block_stem[D_BLOCK_FRAMES][15] D_POOL;

/* Called AFTER d_seq_sample for the first frame. Stop before any later frame
 * can emit a tick, switch clock source, or lose external-clock liveness. */
static unsigned d_seq_block_span(unsigned n){
 if(!ds.playing)return n;
 int ext=d_clock_mode==D_CLK_EXT||d_clock_external();
 if(ext){
  if(!d_clock_live())return n;
  uint32_t elapsed=d_frame-d_clock_last,timeout=D_FS/2-elapsed;
  if(timeout<n)n=timeout;
  if(ds.sub<3){uint32_t at=(d_clock_period*(ds.sub+1u)+3u)/4u;
   if(at>elapsed&&at-elapsed<n)n=at-elapsed;
  }
 }else{
  uint32_t inc=(uint32_t)dg[G_TEMPO]*96u;
  uint32_t at=(D_FS*60u-ds.phase+inc-1u)/inc;
  if(at<n)n=at;
 }
 return n?n:1;
}
static __attribute__((always_inline)) inline void d_block_mix(d_part *t,int32_t x,int32_t bus[5]){
 if(!x)return;int32_t l=dm(x,t->gain[0]>>8),r=dm(x,t->gain[1]>>8);bus[0]+=l;bus[1]+=r;if(d_bench_active&&(l||r))d_sound_mask|=1u<<(unsigned)(t-dp);
 for(int j=0;j<3;j++)if(!D_SPARSE_FAST||(j<2?d_cpu[2]:d_cpu[3]))bus[2+j]+=dm(x,t->gain[j+2]>>8);
}
/* The inner audio loop stays in SRAM for the entire track block. */
static D_SAMPLE_RAM void d_synth_block(d_part *t,unsigned n,int32_t bus[][5],int16_t stem[][15]){
 unsigned part=(unsigned)(t-dp),limit=d_cpu_oscs[d_cpu[0]];
 if(!d_cpu[0]||!d_osc_limit)return;
 if(D_IDLE_CACHE&&d_idle[part].sleep&&!d_idle[part].sample){
  for(int i=0;i<4;i++)t->c[i]+=t->cstep[i]*(int32_t)n;
  for(int i=0;i<5;i++)t->gain[i]+=t->gainstep[i]*(int32_t)n;return;
 }
 for(unsigned f=0;f<n;f++){
  int32_t x;
  if(D_IDLE_CACHE&&d_idle[part].sleep){
   for(int i=0;i<4;i++)t->c[i]+=t->cstep[i];for(int i=0;i<5;i++)t->gain[i]+=t->gainstep[i];x=d_idle[part].sample;
  }else{
   int32_t old_s1=t->s1,old_s2=t->s2;unsigned release=t->post_env.stage==4?d_release[part].tail:0;
   if(t->filter_env.left)d_env_tick(&t->filter_env,t->fe);if(t->post_env.left)d_env_tick(&t->post_env,t->pe);
   x=d_synth_core(t,limit,release,d_amp_fast(t,8));
   if(D_IDLE_CACHE&&d_idle[part].eligible&&old_s1==t->s1&&old_s2==t->s2){d_idle[part].sample=x;d_idle[part].sleep=1;}
  }
  if((uint32_t)da(x)>t->peak)t->peak=(uint32_t)da(x);
  if(d_capture_active&&(d_usb_groups&2))stem[f][part]=(int16_t)dc(x/2,-32767,32767);
  d_block_mix(t,x,bus[f]);
 }
}
static D_SAMPLE_RAM void d_fx_block(const int32_t bus[][5],int32_t *out,unsigned n){
 for(unsigned f=0;f<n;f++)d_fx_core(bus[f][0],bus[f][1],bus[f]+2,out+2*f,out+2*f+1);
}
#if defined(D_PLATFORM_MEMORY) && !defined(DINGHY_HOST)
static void d_synth_block_call(d_part *t,unsigned n){void (*volatile entry)(d_part *,unsigned,int32_t (*)[5],int16_t (*)[15])=d_synth_block;entry(t,n,d_block_bus,d_block_stem);}
static void d_fx_block_call(int32_t *out,unsigned n){void (*volatile entry)(const int32_t (*)[5],int32_t *,unsigned)=d_fx_block;entry(d_block_bus,out,n);}
#else
#define d_synth_block_call(t,n) d_synth_block(t,n,d_block_bus,d_block_stem)
#define d_fx_block_call(out,n) d_fx_block(d_block_bus,out,n)
#endif
#ifndef D_DRUM_BLOCK_FAST
#define D_DRUM_BLOCK_FAST 1
#endif
#if D_DRUM_BLOCK_FAST
#include "drum_block.h"
#include "drum_parallel.h"
#else
static void d_drum_block(d_part *t,unsigned n){
 /* Disabled/finished PCM contributes no buses. Advance smoothing once rather
  * than scanning eleven pads, clearing twenty buses, and mixing zeros per frame. */
 if(!d_sample_limit||!d_sample_active_count()){
  memset(d_drum_raw,0,sizeof d_drum_raw);memset(d_drum_bus,0,sizeof d_drum_bus);d_drum_source_mono=0;
  if(d_sample_limit)for(int i=0;i<11;i++)d_pad_gain[i]+=d_pad_step[i]*(int32_t)n;
  for(int i=0;i<5;i++)t->gain[i]+=t->gainstep[i]*(int32_t)n;
  return;
 }
 for(unsigned f=0;f<n;f++){
  int32_t x=d_drive(d_drum_tick(),t->p[P_DRIVE]);for(int i=0;i<5;i++)t->gain[i]+=t->gainstep[i];
  if((uint32_t)da(x)>t->peak)t->peak=(uint32_t)da(x);
  if(d_capture_active){if(d_usb_groups&2)d_block_stem[f][3]=(int16_t)dc(d_drum_bus[8]/2,-32767,32767);
   if(d_usb_groups&4)for(int j=0;j<11;j++)d_block_stem[f][4+j]=(int16_t)dc(d_drum_raw[j],-32768,32767);
  }
  int32_t bus[8];for(int j=0;j<8;j++){
   if(D_SPARSE_FAST&&j>=2&&!((j-2)%3<2?d_cpu[2]:d_cpu[3]))continue;
   if(!D_FULL_FAST)bus[j]=d_drive(d_drum_bus[j],t->p[P_DRIVE]);
   else if(!d_drum_bus[j])bus[j]=0;
   else if(d_drum_bus[j]==d_drum_source_mono)bus[j]=x;
   else if(j&&d_drum_bus[j]==d_drum_bus[0])bus[j]=bus[0];
   else bus[j]=d_drive(d_drum_bus[j],t->p[P_DRIVE]);
  }
  d_block_bus[f][0]+=dm(bus[0],t->gain[0]>>8);d_block_bus[f][1]+=dm(bus[1],t->gain[1]>>8);
  for(int j=0;j<3;j++)if(!D_SPARSE_FAST||(j<2?d_cpu[2]:d_cpu[3]))d_block_bus[f][2+j]+=dm(bus[2+j],t->gain[2+j]>>8)+dm(bus[5+j],(t->gain[0]+t->gain[1])>>8);
 }
}

#endif
#ifndef D_BLOCK_RENDER
#define D_BLOCK_RENDER 1
#endif
static void d_render(int32_t *out,uint32_t n){
 if(!D_BLOCK_RENDER){d_render_reference(out,n);return;}
 d_prof_begin(n,d_frame);d_commands();d_seq_service();
 for(unsigned done=0;done<n;){
  d_prof_switch(DPF_CONTROL);d_seq_sample();
  if(!d_mod_left){d_prof_event(DPE_CONTROL);d_mod_span=d_mod_intervals[d_mod_index];d_mod_left=d_mod_span;
   for(int p=0;p<4;p++){dp[p].peak=0;if(d_cpu[0]||(p==3&&d_seq_is_drum()))d_prepare_part(&dp[p]);}
   d_prepare_fx();d_mix_prepare();
  }
  unsigned count=n-done;if(count>d_mod_left)count=d_mod_left;if(count>D_BLOCK_FRAMES)count=D_BLOCK_FRAMES;
  count=d_seq_block_span(count);
  int internal=ds.playing&&d_clock_mode!=D_CLK_EXT&&!d_clock_external();
  d_sound_mask=0;memset(d_block_bus,0,count*sizeof d_block_bus[0]);if(d_capture_active)memset(d_block_stem,0,count*sizeof d_block_stem[0]);
#if D_DRUM_BLOCK_FAST
  int running=0;unsigned drum_count=0;int drums=d_seq_is_drum();
  if(drums){d_prof_switch(DPF_DRUM);drum_count=d_drum_launch(count,&running);}
  for(unsigned p=0;p<(drums?3u:4u);p++){d_prof_switch(DPF_SYNTH);d_synth_block_call(&dp[p],count);}
  if(drums){d_prof_switch(DPF_DRUM);d_drum_join(count,drum_count,running);}
#else
  for(unsigned p=0;p<4;p++){if(p==3&&d_seq_is_drum()){d_prof_switch(DPF_DRUM);d_drum_block(&dp[p],count);}else{d_prof_switch(DPF_SYNTH);d_synth_block_call(&dp[p],count);}}
#endif
  if(d_bench_active){for(unsigned p=0;p<4;p++)if(d_sound_mask&(1u<<p))d_sound_frames[p]+=count;if(d_sound_mask==15)d_common_frames+=count;}
  d_prof_switch(DPF_FX);int32_t *pcm=out+done*2;d_fx_block_call(pcm,count);d_prof_switch(DPF_OUTPUT);
  for(unsigned f=0;f<count;f++){if(pcm[2*f])pcm[2*f]=d_master_scale(pcm[2*f],d_fxg[G_MASTER]);if(pcm[2*f+1])pcm[2*f+1]=d_master_scale(pcm[2*f+1],d_fxg[G_MASTER]);}
  d_limiter_block(pcm,count);
  for(unsigned f=0;f<count;f++){
   if(da(pcm[2*f])>32767||da(pcm[2*f+1])>32767)d_clips++;
   pcm[2*f]=dc(pcm[2*f],-32767,32767);pcm[2*f+1]=dc(pcm[2*f+1],-32767,32767);
   if(d_capture_active){int16_t stem[17];stem[0]=(d_usb_groups&1)?dc(pcm[2*f]*d_capture_main_gain/4096,-32767,32767):0;stem[1]=(d_usb_groups&1)?dc(pcm[2*f+1]*d_capture_main_gain/4096,-32767,32767):0;
    for(int j=0;j<15;j++)stem[j+2]=(d_usb_groups&(j<4?2:4))?d_block_stem[f][j]:0;D_CAPTURE_HOOK(stem);
   }
   d_clock_step();d_frame++;
  }
  if(internal)ds.phase+=(count-1u)*(uint32_t)dg[G_TEMPO]*96u;
  d_mod_left-=count;done+=count;
 }
 d_active=0;for(int p=0;p<4;p++)for(int i=0;i<8;i++)d_active+=!!d_osc_running(&dp[p],i);d_prof_end();
}

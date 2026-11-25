/* SPDX-License-Identifier: GPL-3.0-only; private drum bus, join before FX.
 * Only mix gains are mutable worker inputs; PCM/source phases are advanced on core0 once.
 */
#pragma once
#ifndef D_CORE_SCRATCH
#define D_CORE_SCRATCH
#endif
#ifndef D_DEVICE_CORES
static uint32_t d_core_state,d_core_jobs,d_core_wait_ticks,d_core_job_ticks,d_core_peak_ticks,d_core_timeouts;
static int d_core_run(void (*fn)(uint32_t),uint32_t arg){(void)fn;(void)arg;return 0;}
static int d_core_wait(void){return 0;}
static uint32_t d_core_ticks(void){return 0;}
#endif
static struct {int32_t bus[32][5],gain[16];uint32_t ticks;uint8_t pads[11];} d_drum_job D_CORE_SCRATCH;
static void d_drum_job_run(uint32_t arg){unsigned n=arg&255u,count=arg>>8;uint32_t start=d_core_ticks();d_drum_mix_call(&dp[3],n,d_drum_job.pads,count,d_drum_job.bus);uint32_t elapsed=d_core_ticks()-start;d_drum_job.ticks=elapsed;}
static unsigned d_drum_launch(unsigned n,int *running){
 unsigned count=d_drum_prepare(&dp[3],n,d_drum_job.pads);*running=0;if(!count)return 0;
 memset(d_drum_job.bus,0,n*sizeof d_drum_job.bus[0]);for(unsigned i=0;i<11;i++)d_drum_job.gain[i]=d_pad_gain[i];for(unsigned i=0;i<5;i++)d_drum_job.gain[11+i]=dp[3].gain[i];
 *running=d_core_run(d_drum_job_run,n|(count<<8));return count;
}
static void d_drum_join(unsigned n,unsigned count,int running){
 if(!count)return;
 int good=running&&!d_core_wait();
 if(good){d_core_job_ticks+=d_drum_job.ticks;if(d_drum_job.ticks>d_core_peak_ticks)d_core_peak_ticks=d_drum_job.ticks;}
 else{
  /* A failed wait has held core1. Roll back any partial smoothing, not PCM positions. */
  for(unsigned i=0;i<11;i++)d_pad_gain[i]=d_drum_job.gain[i];for(unsigned i=0;i<5;i++)dp[3].gain[i]=d_drum_job.gain[11+i];d_drum_sound=0;
  memset(d_drum_job.bus,0,n*sizeof d_drum_job.bus[0]);d_drum_job_run(n|(count<<8));
 }
 for(unsigned f=0;f<n;f++)for(unsigned j=0;j<5;j++)d_block_bus[f][j]+=d_drum_job.bus[f][j];d_sound_mask|=d_drum_sound;
}

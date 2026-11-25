/* SPDX-License-Identifier: GPL-3.0-only; opt-in audio wall-time profiling.
 * Audio owns state. Main only requests enable/freeze and copies by seqlock.
 * No calls are introduced inside the SRAM kernels. */
#pragma once
#include <stdint.h>
#ifndef D_PROFILE_TICKS
#define D_PROFILE_TICKS() 0u
#endif
#define D_PROF_STAGES 6u
#define D_PROF_EVENTS 6u
#define D_PROF_TOP 4u
#define D_PROF_FIELDS 116u
#define D_PROF_REPLY (11u+D_PROF_FIELDS*5u)
enum {DPF_COMMAND,DPF_CONTROL,DPF_SYNTH,DPF_DRUM,DPF_FX,DPF_OUTPUT};
enum {DPE_COMMAND,DPE_NOTE_ON,DPE_NOTE_OFF,DPE_DRUM_ON,DPE_CACHE_FILL,DPE_CONTROL};
typedef struct {uint32_t ticks,frames,budget,frame,stage[6],event[6],cache_ticks;} d_prof_block;
typedef struct {uint32_t generation,blocks,frames,late,late_frames,start_frame;uint64_t total[6],late_total[6],cache_total,cache_late;uint32_t maximum[6],events[6];d_prof_block top[4];} d_prof_stats;
static volatile d_prof_stats d_prof;
static d_prof_block d_prof_current;
static volatile uint32_t d_prof_sequence,d_prof_revision;
static volatile int d_prof_pending=-1;
static volatile uint8_t d_prof_enabled;
static uint8_t d_prof_collecting,d_prof_stage;
static uint32_t d_prof_start,d_prof_last;
static void d_prof_window_reset(uint32_t frame){
 d_prof_sequence++;__asm__ volatile("":::"memory");uint32_t gen=d_prof.generation+1;d_prof_stats zero={0};d_prof=zero;d_prof.generation=gen;d_prof.start_frame=frame;
 __asm__ volatile("":::"memory");d_prof_sequence++;
}
static void d_prof_disable(void){d_prof_enabled=d_prof_collecting=0;d_prof_pending=-1;d_prof_revision++;}
static void d_prof_init(void){d_prof_disable();d_prof_window_reset(0);}
static void d_prof_apply(uint32_t frame){int mode=d_prof_pending;if(mode<0)return;d_prof_pending=-1;
 if(mode)d_prof_window_reset(frame);d_prof_enabled=(uint8_t)mode;d_prof_revision++;
}
static __attribute__((always_inline)) inline void d_prof_event(unsigned event){if(d_prof_collecting)d_prof_current.event[event]++;}
static void d_prof_begin(uint32_t n,uint32_t frame){d_prof_apply(frame);d_prof_collecting=d_prof_enabled&&n;if(!d_prof_collecting)return;
 d_prof_current=(d_prof_block){0};d_prof_current.frames=n;d_prof_current.budget=n*80000u/147u;d_prof_current.frame=frame;
 d_prof_stage=DPF_COMMAND;d_prof_start=d_prof_last=D_PROFILE_TICKS();
}
static __attribute__((always_inline)) inline void d_prof_switch(unsigned stage){if(!d_prof_collecting)return;
 uint32_t now=D_PROFILE_TICKS();d_prof_current.stage[d_prof_stage]+=now-d_prof_last;d_prof_last=now;d_prof_stage=(uint8_t)stage;
}
static __attribute__((always_inline)) inline uint32_t d_prof_cache_start(void){return d_prof_collecting?D_PROFILE_TICKS():0;}
static __attribute__((always_inline)) inline void d_prof_cache_end(uint32_t start){if(d_prof_collecting)d_prof_current.cache_ticks+=D_PROFILE_TICKS()-start;}
static void d_prof_end(void){if(!d_prof_collecting)return;uint32_t now=D_PROFILE_TICKS();
 d_prof_current.stage[d_prof_stage]+=now-d_prof_last;d_prof_current.ticks=now-d_prof_start;d_prof_collecting=0;
 d_prof_sequence++;__asm__ volatile("":::"memory");d_prof.blocks++;d_prof.frames+=d_prof_current.frames;
 d_prof.cache_total+=d_prof_current.cache_ticks;int late=d_prof_current.ticks>d_prof_current.budget;if(late)d_prof.cache_late+=d_prof_current.cache_ticks;if(late){d_prof.late++;d_prof.late_frames+=d_prof_current.frames;}
 for(unsigned i=0;i<6;i++){uint32_t ticks=d_prof_current.stage[i];d_prof.total[i]+=ticks;if(late)d_prof.late_total[i]+=ticks;if(ticks>d_prof.maximum[i])d_prof.maximum[i]=ticks;d_prof.events[i]+=d_prof_current.event[i];}
 /* Only the top four block records move; no per-sample tracing or allocation. */
 for(unsigned k=0;k<4;k++)if(!d_prof.top[k].budget||(uint64_t)d_prof_current.ticks*d_prof.top[k].budget>(uint64_t)d_prof.top[k].ticks*d_prof_current.budget){
  for(unsigned j=3;j>k;j--)d_prof.top[j]=d_prof.top[j-1];d_prof.top[k]=d_prof_current;break;
 }
 __asm__ volatile("":::"memory");d_prof_sequence++;
}
static int d_prof_copy(d_prof_stats *out,uint32_t *enabled){for(unsigned attempt=0;attempt<8;attempt++){
 uint32_t seq=d_prof_sequence;if(seq&1u)continue;__asm__ volatile("":::"memory");*out=d_prof;*enabled=d_prof_enabled;__asm__ volatile("":::"memory");if(seq==d_prof_sequence)return 1;
 }return 0;
}
static int d_profile_request(const uint8_t *p,uint32_t n){static const uint8_t prefix[6]={240,125,68,85,66,1};
 if((n!=9&&n!=10)||p[n-1]!=247||p[7]>127)return -1;for(unsigned i=0;i<6;i++)if(p[i]!=prefix[i])return -1;
 if(n==9&&p[6]==6)return 6;if(n==10&&p[6]==7&&p[8]<=1)return 7;return -1;
}
static unsigned d_prof_encode(uint8_t *out,unsigned command,unsigned id,const d_prof_stats *s,unsigned enabled){
 uint32_t v[D_PROF_FIELDS]={39,24000000u,enabled,s->generation,s->blocks,s->frames,s->late,s->start_frame};unsigned at=8;
 for(unsigned i=0;i<6;i++){v[at++]=(uint32_t)s->total[i];v[at++]=(uint32_t)(s->total[i]>>32);}
 for(unsigned i=0;i<6;i++){v[at++]=(uint32_t)s->late_total[i];v[at++]=(uint32_t)(s->late_total[i]>>32);}
 for(unsigned i=0;i<6;i++)v[at++]=s->maximum[i];for(unsigned i=0;i<6;i++)v[at++]=s->events[i];
 for(unsigned k=0;k<4;k++){const d_prof_block *b=&s->top[k];v[at++]=b->ticks;v[at++]=b->frames;v[at++]=b->budget;v[at++]=b->frame;for(unsigned i=0;i<6;i++)v[at++]=b->stage[i];for(unsigned i=0;i<6;i++)v[at++]=b->event[i];v[at++]=b->cache_ticks;}
 v[at++]=(uint32_t)s->cache_total;v[at++]=(uint32_t)(s->cache_total>>32);v[at++]=(uint32_t)s->cache_late;v[at++]=(uint32_t)(s->cache_late>>32);
 if(at!=D_PROF_FIELDS)return 0;out[0]=240;out[1]=125;out[2]=68;out[3]=85;out[4]=66;out[5]=1;out[6]=(uint8_t)(command|64);out[7]=(uint8_t)id;out[8]=D_PROF_FIELDS;at=9;
 for(unsigned i=0;i<D_PROF_FIELDS;i++)for(unsigned j=0;j<5;j++)out[at++]=(uint8_t)((v[i]>>(7*j))&127u);
 uint8_t sum=0;for(unsigned i=1;i<at;i++)sum^=out[i];out[at++]=sum;out[at++]=247;return at;
}

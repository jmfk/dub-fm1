/* SPDX-License-Identifier: GPL-3.0-only; bounded11-part PCM engine, image-resident samples. */
#pragma once
#include "drum_samples.h"
#include "drum_mix.h"
static struct {uint32_t position,age;int32_t gain;uint8_t active,release,source;} d_drum_voice[11];
static int32_t d_pcm_tail[11] D_POOL;
static uint8_t d_pcm_attack[11] D_POOL,d_pcm_tail_left[11] D_POOL;
static void d_pcm_retire(int i){if(d_drum_voice[i].active){d_pcm_tail[i]=d_drum_raw[i];d_pcm_tail_left[i]=32;}d_drum_raw[i]=0;d_drum_voice[i].active=0;}
static int d_pcm_tails(void){for(int i=0;i<11;i++)if(d_pcm_tail[i])return 1;return 0;}
static uint8_t d_drum_held[11][D_SOURCES];
static int d_drum_pad(int note){for(int i=0;i<11;i++)if(d_drum_keys[i]==note)return i;return -1;}
static void d_drum_clear(void){memset(d_pcm_tail,0,sizeof d_pcm_tail);memset(d_pcm_attack,0,sizeof d_pcm_attack);memset(d_pcm_tail_left,0,sizeof d_pcm_tail_left);memset(d_drum_raw,0,sizeof d_drum_raw);memset(d_drum_voice,0,sizeof d_drum_voice);memset(d_drum_held,0,sizeof d_drum_held);}
/* Independent PCM voice ceiling. One active pad is one sampling voice. */
static unsigned d_sample_active_count(void){unsigned n=0;for(int i=0;i<11;i++)n+=!!d_drum_voice[i].active;return n;}
static void d_sample_trim(unsigned target,int keep){
 while(d_sample_active_count()>target){
  int pick=-1,pick_held=3;uint32_t oldest=0;
  for(int i=0;i<11;i++){
   if(i==keep||!d_drum_voice[i].active)continue;int held=d_drum_voice[i].release?0:d_drum_keys[i]==36?2:1;uint32_t age=d_age-d_drum_voice[i].age;
   if(held<pick_held||(held==pick_held&&age>oldest)){pick=i;pick_held=held;oldest=age;}
  }
  if(pick<0)break;d_pcm_retire(pick);
 }
}
static void d_sample_reserve(int pad){unsigned limit=d_sample_limit;if(limit<=11)d_sample_trim(limit-(d_drum_voice[pad].active?0u:1u),pad);}
static void d_sample_limit_set(int value){d_sample_limit=(uint8_t)dc(value,0,12);if(!d_sample_limit){d_drum_clear();memset(d_drum_bus,0,sizeof d_drum_bus);memset(d_drum_raw,0,sizeof d_drum_raw);}else if(d_sample_limit<=11)d_sample_trim(d_sample_limit,-1);}
static void d_drum_on(int note,int velocity,int source){int i=d_drum_pad(note);if(i<0||source<0||source>=D_SOURCES||!d_sample_limit)return;if(source==4)d_drum_held[i][source]=1;else if(d_drum_held[i][source]<255)d_drum_held[i][source]++;d_sample_reserve(i);d_pcm_retire(i);d_pcm_attack[i]=32;d_prof_event(DPE_DRUM_ON);d_drum_voice[i].age=++d_age;d_drum_voice[i].position=0;d_drum_voice[i].gain=velocity*258;d_drum_voice[i].active=1;d_drum_voice[i].release=0;d_drum_voice[i].source=(uint8_t)source;}
static void d_drum_off(int note,int source){int i=d_drum_pad(note);if(i<0||source<0||source>=D_SOURCES)return;if(d_drum_held[i][source])d_drum_held[i][source]--;if(!d_drum_held[i][source]&&(source==4||d_sample_views[i].loop)&&d_drum_voice[i].source==source)d_drum_voice[i].release=1;}

/* Immutable flash PCM: bounded contiguous64-sample cache plus interpolation
 * lookahead per pad. Preload before audio starts; retriggers keep valid cache. */
static int16_t d_drum_cache[11][65] D_POOL;
static uint32_t d_drum_cache_tag[11];
static int32_t d_drum_source_mono;
static const int16_t *d_drum_source(int i){return d_sample_views[i].pcm?d_sample_views[i].pcm:d_drum_pcm+d_drum_offsets[i];}
static const char *d_drum_name(int i){return d_sample_views[i].pcm?(const char *)(d_sample_payload+16+i*40):d_drum_names[i];}
static uint32_t d_drum_length(int i){return d_sample_views[i].pcm?d_sample_views[i].frames:d_drum_offsets[i+1]-d_drum_offsets[i];}
static void d_drum_cache_fill(int pad,uint32_t block,uint32_t n){
 d_prof_event(DPE_CACHE_FILL);uint32_t profile_start=d_prof_cache_start();
 uint32_t count=n-block;if(count>65)count=65;
 memcpy(d_drum_cache[pad],d_drum_source(pad)+block,count*sizeof(int16_t));
 d_drum_cache_tag[pad]=block;d_prof_cache_end(profile_start);
}
static void d_drum_cache_init(void){for(int i=0;i<11;i++)d_drum_cache_fill(i,0,d_drum_length(i));}
static int32_t d_drum_read(int i,uint32_t p,uint32_t n){
 uint32_t at=p>>1;int32_t x;
 if(D_FULL_FAST){uint32_t block=at&~63u;if(d_drum_cache_tag[i]!=block)d_drum_cache_fill(i,block,n);uint32_t pos=at-block;x=d_drum_cache[i][pos];if((p&1)&&at+1<n)x=(x+d_drum_cache[i][pos+1])/2;}
 else{x=d_drum_source(i)[at];if((p&1)&&at+1<n)x=(x+d_drum_source(i)[at+1])/2;}
 if((p&1)&&at+1==n&&d_sample_views[i].loop)x=(x+d_drum_source(i)[0])/2;
 return x;
}
static __attribute__((always_inline)) inline int32_t d_pad_scale(int32_t x,int32_t gain){return D_FULL_FAST?(gain==32768?x:d_q15_small(x,gain)):dm(x,gain);}
static int32_t d_drum_tick(void){
 int32_t sum=0;memset(d_drum_bus,0,sizeof d_drum_bus);memset(d_drum_raw,0,sizeof d_drum_raw);
 if(!d_sample_limit){d_drum_source_mono=0;return 0;}
 for(int i=0;i<11;i++)d_pad_gain[i]+=d_pad_step[i];
 for(int i=0;i<11;i++)if(d_drum_voice[i].active){
  uint32_t p=d_drum_voice[i].position,at=p>>1,n=d_drum_length(i);
  if(at>=n){if(d_sample_views[i].loop){p%=n*2u;d_drum_voice[i].position=p;at=p>>1;}else{d_drum_voice[i].active=0;continue;}}
  int32_t x=d_drum_read(i,p,n);x=d_pad_scale(x,d_drum_voice[i].gain);d_drum_raw[i]=x;sum+=x;
  x=d_pad_scale(x,d_pad_gain[i]);d_drum_bus[8]+=x;d_pad_mix *m=&d_pad[i];
  d_drum_bus[0]+=d_pad_scale(x,d_pad_coeff[i][0]);d_drum_bus[1]+=d_pad_scale(x,d_pad_coeff[i][1]);
  for(int j=0;j<3;j++){if(!m->send[j])d_drum_bus[2+j]+=x;else d_drum_bus[5+j]+=d_pad_scale(x,d_pad_coeff[i][2+j]);}
  d_drum_voice[i].position++;if(d_drum_voice[i].release){d_drum_voice[i].gain-=256;if(d_drum_voice[i].gain<=0)d_drum_voice[i].active=0;}
 }
 for(int j=0;j<9;j++)d_drum_bus[j]/=2;d_drum_source_mono=sum/2;return sum/2;
}

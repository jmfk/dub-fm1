/* SPDX-License-Identifier: GPL-3.0-only; portable fixed-point DUB R36 engine.
 * Audio owns state. Main publishes bounded commands; no heap or flash access. */
#pragma once
#include <stdint.h>
#ifndef D_PLATFORM_MEMORY
#include <string.h>
#endif
#include "tables.h"
#include "params.h"
#include "audio_capture.h"
#include "runtime_control.h"
#include "profile.h"
#ifndef D_POOL
#if defined(__APPLE__)
#define D_POOL __attribute__((section("__DATA,__dubpool")))
#else
#define D_POOL __attribute__((section(".pool")))
#endif
#endif
#define D_FS 44100u
#define D_PARTS 4
#define D_OSCS 8
#define D_SOURCES 5
#define D_Q 8388608
static __attribute__((always_inline)) inline int32_t dc(int32_t x,int32_t lo,int32_t hi){return x<lo?lo:x>hi?hi:x;}
static __attribute__((always_inline)) inline int32_t dm(int32_t x,int32_t y){return (int32_t)(((int64_t)x*y)>>15);}
static __attribute__((always_inline)) inline int32_t dq(int32_t x,int32_t y){return (int32_t)(((int64_t)x*y)>>24);}
static __attribute__((always_inline)) inline int32_t da(int32_t x){return x<0?-x:x;}
#ifndef D_SPARSE_FAST
#define D_SPARSE_FAST 1
#endif
#ifndef D_FULL_FAST
#define D_FULL_FAST 1
#endif
/* Only bounded waveform/amplitude operands, not aggregate drum/FX buses. */
static __attribute__((always_inline)) inline int32_t d_q15_small(int32_t x,int32_t y){return (x*y)>>15;}
static __attribute__((always_inline)) inline int32_t d_soft(int32_t x){x=dc(x,-32767,32767);return (x-(D_FULL_FAST?d_q15_small(d_q15_small(x,x),x):dm(dm(x,x),x))/3)*3/2;}
static __attribute__((always_inline)) inline int32_t d_drive(int32_t x,int amount){return amount?d_soft(x*(32+amount)/32):x;}
/* Keep scaling separate from final clipping: pi32v2 otherwise folds the
 * clipping threshold through /127 into a literal in its forbidden ROM range.
 * This preserves exact integer PCM and keeps the platform ROM guard enabled. */
static __attribute__((noinline)) int32_t d_master_scale(int32_t x,int32_t gain){return x*gain/127;}

/* Shared chronology and allocator declaration precede the drum implementation. */
static uint32_t d_age;
#include "sample_bank.h"
#include "drums.h"

typedef struct {int32_t v,step,target;uint32_t left;uint8_t stage;} d_env;
typedef struct {uint8_t wave,level,env[4];int8_t tune,cents;} d_osc_config;
typedef struct {uint32_t phase,inc;int32_t gain;int8_t slot;uint8_t band;} d_osc;
typedef struct {uint32_t age;uint8_t active,gate,note,source,velocity;} d_slot;
enum {DL_FILTER,DL_PITCH,DL_AMP,DL_PAN,DL_COUNT};
static const uint8_t d_lfo_ids[3][4]={{P_LFO_RATE,P_LFO_AMT,P_LFO_SHAPE,P_LFO_TARGET},{P_LFO2_RATE,P_LFO2_AMT,P_LFO2_SHAPE,P_LFO2_TARGET},{P_LFO3_RATE,P_LFO3_AMT,P_LFO3_SHAPE,P_LFO3_TARGET}};
typedef struct {
 int16_t p[P_COUNT];uint8_t env_target[10];int16_t env_amount[10];d_osc_config cfg[8];uint8_t fe[4],pe[4];
 uint8_t voice_mode; /*0 mono,1 duo,2 four-note paraphonic */
 d_osc osc[8];d_slot slot[4];d_env filter_env,post_env;
 uint8_t held[D_SOURCES][128],sustained[D_SOURCES][128],pedal[D_SOURCES];
 int32_t bend,expression,s1,s2,c[4],cstep[4],gain[5],gainstep[5];
 uint32_t lfo[3],filter_triggers,peak;
 int32_t modulation[DL_COUNT],osc_pitch[8],osc_lfo_amp[8],fx_mod[30];uint16_t env_mask[9];int8_t env_single[9];
} d_part;
#define D_TARGETS 51
static const char *const d_target_names[D_TARGETS]={"FILTER","PITCH","AMP","PAN","O1 AMP","O2 AMP","O3 AMP","O4 AMP","O5 AMP","O6 AMP","O7 AMP","O8 AMP","O1 PITCH","O2 PITCH","O3 PITCH","O4 PITCH","O5 PITCH","O6 PITCH","O7 PITCH","O8 PITCH","OFF","DA MODE","DA MS","DA SYNC","DA FBACK","DA TONE","DA DRIVE","DA MOD","DA ROUTE","DA WIDTH","DA >REV","DB MODE","DB MS","DB SYNC","DB FBACK","DB TONE","DB DRIVE","DB MOD","DB ROUTE","DB WIDTH","DB >REV","RV TIME","RV DAMP","RV DIFF","RV MOD","RV LOWCUT","RV PREDELAY","RET A","RET B","RET REV","MASTER"};
static const char *const d_fx_full_names[30]={"DELAY A MODE","DELAY A TIME MS","DELAY A SYNC","DELAY A FBACK","DELAY A TONE","DELAY A DRIVE","DELAY A MOD","DELAY A ROUTE","DELAY A WIDTH","DELAY A >REV","DELAY B MODE","DELAY B TIME MS","DELAY B SYNC","DELAY B FBACK","DELAY B TONE","DELAY B DRIVE","DELAY B MOD","DELAY B ROUTE","DELAY B WIDTH","DELAY B >REV","REVERB TIME","REVERB DAMP","REVERB DIFF","REVERB MOD","REVERB LOWCUT","REVERB PREDELAY","DELAY A RETURN","DELAY B RETURN","REVERB RETURN","MASTER"};
static const char *d_target_full_name(int target){return target>=21?d_fx_full_names[target-21]:d_target_names[target];}
static const char *const d_source_names[13]={"LFO1","LFO2","LFO3","O1 ENV","O2 ENV","O3 ENV","O4 ENV","O5 ENV","O6 ENV","O7 ENV","O8 ENV","F ENV","A ENV"};
static void d_route_defaults(d_part *t){for(int i=0;i<10;i++){t->env_target[i]=i<8?20:i==8?0:2;t->env_amount[i]=i<8?0:i==8?dc((t->p[P_ENV_AMT]-64)*2,-127,127):127;}}
static int d_route_target(volatile d_part *t,int src){if(src>=3&&src<11)return 20;int target=src<3?t->p[d_lfo_ids[src][3]]:t->env_target[src-3];return target>=4&&target<12?20:target;}
static int d_route_amount(volatile d_part *t,int src){if(src>=3&&src<11)return 0;return src<3?t->p[d_lfo_ids[src][1]]:t->env_amount[src-3];}
/* Legacy source IDs remain fail-closed; no per-OSC ENV state exists. */
static d_env d_no_osc_env;
static __attribute__((always_inline)) inline d_env *d_route_env(d_part *t,int i){return i<8?&d_no_osc_env:i==8?&t->filter_env:&t->post_env;}
static void d_route_set(d_part *t,int src,int field,int value){if(src<0||src>=13||(src>=3&&src<11))return;if(!field&&value>=4&&value<12)value=20;if(src<3)t->p[d_lfo_ids[src][field?1:3]]=dc(value,0,field?127:D_TARGETS-1);else if(field)t->env_amount[src-3]=dc(value,-127,127);else t->env_target[src-3]=dc(value,0,D_TARGETS-1);}
static d_part dp[4];
/* Tiny click suppression only, not per-OSC ADSR or additional voices. */
static int32_t d_osc_last[4][8] D_POOL,d_osc_tail[4] D_POOL;
static uint8_t d_osc_attack[4][8] D_POOL,d_osc_tail_left[4] D_POOL;
static uint32_t d_sound_frames[4] D_POOL,d_common_frames;
static uint8_t d_sound_mask;
static void d_osc_retire(int p,int i){d_osc_tail[p]+=d_osc_last[p][i];if(d_osc_tail[p])d_osc_tail_left[p]=32;d_osc_last[p][i]=0;d_osc_attack[p][i]=0;dp[p].osc[i].slot=-1;}
#include "limiter.h"

static struct {uint32_t frame;uint8_t pending,tail;} d_release[4];
static const uint8_t d_mod_sources[5]={0,1,2,11,12};
static int d_source_valid(int src){return src>=0&&src<13&&(src<3||src>=11);}
static int d_source_index(int src){for(int i=0;i<5;i++)if(d_mod_sources[i]==src)return i;return 0;}
static int d_target_valid(int t){return t>=0&&t<D_TARGETS&&!(t>=4&&t<12);}
static int d_target_step(int target,int delta){if(!d_target_valid(target))target=20;while(delta){int step=delta>0?1:-1,next=target+step;if(next<0||next>=D_TARGETS)break;if(next>=4&&next<12)next=step>0?12:3;target=next;delta-=step;}return target;}

#ifndef D_IDLE_CACHE
#define D_IDLE_CACHE 1
#endif
static struct {int32_t sample;uint8_t eligible,sleep;} d_idle[4];
static void d_idle_wake(d_part *t){d_idle[t-dp].sleep=0;d_idle[t-dp].eligible=0;}
static void d_idle_prepare(d_part *t){unsigned p=(unsigned)(t-dp);d_idle[p].sleep=0;d_idle[p].eligible=0;
 if(!D_IDLE_CACHE||d_osc_tail[p])return;
 for(int i=0;i<4;i++)if(t->slot[i].gate)return;
 if(t->s1||t->s2)for(int i=0;i<4;i++)if(t->cstep[i])return;
 for(int i=8;i<10;i++){d_env *e=d_route_env(t,i);if(e->stage||e->left)return;}
 d_idle[p].eligible=1;if(!t->s1&&!t->s2){d_idle[p].sample=0;d_idle[p].sleep=1;}
}

static int16_t dg[G_COUNT],dd[2][D_COUNT],dv[V_COUNT];
static int16_t d_wave_ram[4][8][257] D_POOL;
static int32_t d_fc_ram[128][16][4] D_POOL;
static volatile uint32_t d_frame,d_clips;
static volatile uint32_t d_seen,d_rejected,d_active;
#include "seq_state.h"
#include "clock.h"
static int16_t d_fxdd[2][D_COUNT],d_fxdv[V_COUNT],d_fxg[G_COUNT];
static const uint8_t d_fx_global_ids[4]={G_DLY_A_RET,G_DLY_B_RET,G_REV_RET,G_MASTER};
/* Sync0 legacy MS,1..8 active beat;9..16 MS remembering beat1..8. */
#define D_DELAY_MAX_MS 32000
static __attribute__((always_inline)) inline unsigned d_delay_shift(int b){return d_delay_cfg[b][0]?d_delay_cfg[b][1]:(d_delay_cfg[b][1]>2?2:d_delay_cfg[b][1]);}
static __attribute__((always_inline)) inline unsigned d_delay_cells(int b){(void)b;return 24576u;}
static __attribute__((always_inline)) inline int d_delay_max_ms(int b){return 1000<<d_delay_shift(b);}
static __attribute__((always_inline)) inline int32_t d_delay_ms_q8(int b,int ms){return dc(ms,1,d_delay_max_ms(b))*(56448>>d_delay_shift(b))/10;}
static void d_delay_config_set(int b,int field,int value);
static const char *const d_delay_sync_names[9]={"MS","1/16","1/8","1/8D","1/4","1/4D","1/2","1/2D","1BAR"};
static int d_delay_sync_active(int code){return code>=1&&code<=8?code:0;}
static int d_delay_sync_choice(int code){return code>=9?code-8:code?code:4;}
static int d_delay_time_mode(int code,int sync){int choice=d_delay_sync_choice(code);return sync?choice:d_delay_sync_active(code)?choice+8:code;}
static __attribute__((always_inline)) inline int d_delay_sync_ms(int sync,int bpm){static const uint16_t numerator[9]={0,1,1,3,1,3,2,3,4},denominator[9]={1,4,2,4,1,2,1,1,1};return sync>=1&&sync<=8?60000*numerator[sync]/(bpm*denominator[sync]):0;}
static int d_fx_scaled(int base,int mod,int lo,int hi){if(!mod)return dc(base,lo,hi);int value=mod*(hi-lo);value=(value+(value<0?-63:63))/127;return dc(base+value,lo,hi);}
static uint32_t d_fx_mod_mask[4];
static void d_prepare_fx(void){int mod[30]={0};uint32_t mask=0;if(d_cpu[1])for(int t=0;t<4;t++)mask|=d_fx_mod_mask[t];
 if(!mask){memcpy(d_fxdd,dd,sizeof dd);for(int b=0;b<2;b++)d_fxdd[b][D_SYNC]=d_delay_sync_active(dd[b][D_SYNC]);memcpy(d_fxdv,dv,sizeof dv);memcpy(d_fxg,dg,sizeof dg);d_fxg[G_TEMPO]=d_clock_tempo();return;}
 if(d_cpu[1])for(int t=0;t<4;t++)if(d_fx_mod_mask[t])for(int i=0;i<30;i++)if(d_fx_mod_mask[t]&(1u<<i))mod[i]+=dp[t].fx_mod[i];
 for(int bus=0;bus<2;bus++)for(int i=0;i<D_COUNT;i++)d_fxdd[bus][i]=d_fx_scaled(i==D_SYNC?d_delay_sync_active(dd[bus][i]):i==D_TIME_MS?dc(dd[bus][i],1,d_delay_max_ms(bus)):dd[bus][i],mod[bus*10+i],i==D_TIME_MS?1:0,i==D_SYNC?8:i==D_TIME_MS?d_delay_max_ms(bus):d_max[i]);
 for(int i=0;i<V_COUNT;i++)d_fxdv[i]=d_fx_scaled(dv[i],mod[20+i],0,v_max[i]);memcpy(d_fxg,dg,sizeof dg);d_fxg[G_TEMPO]=d_clock_tempo();
 for(int i=0;i<4;i++){int id=d_fx_global_ids[i];d_fxg[id]=d_fx_scaled(dg[id],mod[26+i],0,g_max[id]);}}

/* Fixed SPSC queue. Overflow fails silent rather than leaving hanging notes. */
enum {DC_MIDI,DC_NOTE,DC_PARAM,DC_GLOBAL,DC_DELAY,DC_REVERB,DC_OSC,DC_ENV,DC_PANIC,DC_RETRIG,DC_PATCH,DC_ROUTE,DC_CLOCK,DC_VOICE,DC_SEQ,DC_SEQ_CTL,DC_USB_MODE,DC_DRUM_MIX,DC_DRUM_SAVE,DC_USB_GROUP,DC_USB_RELEASE,DC_CPU,DC_MOD_RATE,DC_OSC_LIMIT,DC_SAMPLE_LIMIT,DC_RUNTIME,DC_DELAY_CFG};
typedef struct {int32_t op,a,b,c,source;} d_command;
static d_command d_queue[128];
static volatile uint32_t d_qw,d_qr,d_overflows,d_emergency,d_panic_requested;
static int d_send(int op,int a,int b,int c,int source){
 if((op&255)==DC_PANIC){d_panic_requested=1;return 1;}
 uint32_t w=d_qw;if(w-d_qr>=128u){d_overflows++;d_emergency=1;return 0;}
 d_queue[w&127u]=(d_command){op,a,b,c,source};
 __asm__ volatile("":::"memory");d_qw=w+1;return 1;
}
static __attribute__((always_inline)) inline void d_env_ramp(d_env *e,int stage,int32_t target,int ms){
 e->stage=(uint8_t)stage;e->target=target;e->left=(uint32_t)ms*441u/10u;if(!e->left)e->left=1;
 e->step=(target-e->v)/(int32_t)e->left;
}
static void d_env_on(d_env *e,const uint8_t p[4]){d_env_ramp(e,1,D_Q,d_times[p[0]]);}
static void d_env_off(d_env *e,const uint8_t p[4]){if(e->stage&&e->stage!=4)d_env_ramp(e,4,0,d_times[p[3]]);}
static __attribute__((always_inline)) inline int32_t d_env_tick(d_env *e,const uint8_t p[4]){
 if(e->left){e->v+=e->step;if(!--e->left){e->v=e->target;
 if(e->stage==1)d_env_ramp(e,2,(int32_t)p[2]*D_Q/127,d_times[p[1]]);
 else if(e->stage==2)e->stage=3;else if(e->stage==4)e->stage=0;}}
 return e->v>>8;
}
static uint32_t d_gates(d_part *t){uint32_t n=0;for(int s=0;s<D_SOURCES;s++)for(int k=0;k<128;k++)n+=!!t->held[s][k]||t->sustained[s][k];return n;}
static unsigned d_slot_gates(const d_part *t){unsigned n=0;for(int j=0;j<4;j++)n+=!!t->slot[j].gate;return n;}
static void d_retrigger(d_part *t){d_idle_wake(t);d_env_on(&t->filter_env,t->fe);d_env_on(&t->post_env,t->pe);t->filter_triggers++;}
static void d_panic_part(d_part *t){d_idle_wake(t);memset(&d_release[t-dp],0,sizeof d_release[0]);
 if(t==&dp[3])d_drum_clear();
 memset(d_osc_last[t-dp],0,sizeof d_osc_last[0]);memset(d_osc_attack[t-dp],0,sizeof d_osc_attack[0]);d_osc_tail[t-dp]=0;d_osc_tail_left[t-dp]=0;
 memset(t->held,0,sizeof t->held);memset(t->sustained,0,sizeof t->sustained);memset(t->pedal,0,sizeof t->pedal);
 for(int i=0;i<8;i++)t->osc[i].slot=-1;
 memset(t->slot,0,sizeof t->slot);memset(&t->filter_env,0,sizeof(d_env));memset(&t->post_env,0,sizeof(d_env));t->s1=t->s2=0;
}
static void d_release_slots(d_part *t){
 unsigned released=0;unsigned p=(unsigned)(t-dp);if(d_release[p].frame!=d_frame){d_release[p].frame=d_frame;d_release[p].pending=0;}
 for(int j=0;j<4;j++){d_slot *slot=&t->slot[j];if(!slot->gate)continue;if(t->held[slot->source][slot->note]||t->sustained[slot->source][slot->note])continue;d_release[p].pending|=1u<<j;slot->gate=0;released++;}
 if(released&&d_slot_gates(t)){for(int i=0;i<8;i++)if(t->osc[i].slot>=0&&(d_release[p].pending&(1u<<t->osc[i].slot)))d_osc_retire((int)p,i);}
 if(released&&!d_slot_gates(t)){d_release[p].tail=d_release[p].pending;d_env_off(&t->filter_env,t->fe);d_env_off(&t->post_env,t->pe);}
}
static int d_voice_count(const volatile d_part *t){return 1<<t->voice_mode;}
/* Global oscillator budget. Allocation/limit changes do bounded scans;
 * the audio sample kernel only sees the resulting oscillator assignments. */
static int d_budget_slot_active(int part,int slot){
 const d_part *t=&dp[part];if(part==3&&d_seq_is_drum())return 0;
 return t->slot[slot].active&&(t->slot[slot].gate||(t->post_env.stage==4&&(d_release[part].tail&(1u<<slot))));
}
static int d_budget_osc_active(int part,int i){
 const d_part *t=&dp[part];int mode=d_cpu[0];if(!mode||((unsigned)i&((8u>>t->voice_mode)-1u))>=d_cpu_oscs[mode])return 0;
 int slot=t->osc[i].slot;return slot>=0&&d_budget_slot_active(part,slot);
}
static unsigned d_osc_active_count(void){unsigned n=0;for(int p=0;p<4;p++)for(int i=0;i<8;i++)n+=d_budget_osc_active(p,i);return n;}
static void d_osc_steal(int part,int osc){
 d_part *t=&dp[part];int slot=t->osc[osc].slot;d_osc_retire(part,osc);
 for(int i=0;i<8;i++)if(t->osc[i].slot==slot&&d_budget_osc_active(part,i))return;
 /* Last running oscillator in this note: retire the slot, retaining the held
  * key count so an eventual old note-off cannot release a newer repetition. */
 t->slot[slot].active=t->slot[slot].gate=0;
 for(int i=0;i<8;i++)if(t->osc[i].slot==slot)t->osc[i].slot=-1;
 d_release[part].pending&=(uint8_t)~(1u<<slot);d_release[part].tail&=(uint8_t)~(1u<<slot);
 d_idle_wake(t);if(!d_slot_gates(t)){d_env_off(&t->filter_env,t->fe);d_env_off(&t->post_env,t->pe);}
}
static void d_osc_trim(unsigned target,int keep_part,int keep_slot){
 while(d_osc_active_count()>target){
  int pick_part=-1,pick_osc=-1,pick_held=2;uint32_t oldest=0;
  for(int p=0;p<4;p++)for(int i=0;i<8;i++){
   if(!d_budget_osc_active(p,i))continue;int slot=dp[p].osc[i].slot;if(p==keep_part&&slot==keep_slot)continue;
   int held=!!dp[p].slot[slot].gate;uint32_t age=d_age-dp[p].slot[slot].age;
   if(held<pick_held||(held==pick_held&&(age>oldest||(age==oldest&&i>pick_osc)))){pick_part=p;pick_osc=i;pick_held=held;oldest=age;}
  }
  if(pick_part<0)break;d_osc_steal(pick_part,pick_osc);
 }
}
static unsigned d_osc_reserve(int part,int slot,unsigned width){
 unsigned limit=d_osc_limit;if(!limit)return 0;
 unsigned desired=width,per_voice=d_cpu_oscs[d_cpu[0]];if(desired>per_voice)desired=per_voice;if(limit<=32&&desired>limit)desired=limit;
 /* Replace this physical group's old assignments before reserving its new
  * oscillator count; retriggers cannot consume an extra group of capacity. */
 for(int i=0;i<8;i++)if(dp[part].osc[i].slot==slot)d_osc_retire(part,i);
 if(limit<=32)d_osc_trim(limit-desired,part,slot);return desired;
}
static void d_osc_limit_set(int value){d_osc_limit=(uint8_t)dc(value,0,33);if(!d_osc_limit){for(int p=0;p<4;p++)if(p!=3||!d_seq_is_drum())d_panic_part(&dp[p]);}else if(d_osc_limit<=32)d_osc_trim(d_osc_limit,-1,-1);}
static void d_note_on(int part,int note,int vel,int source){
 if(part<0||part>=4||note<0||note>127||source<0||source>=D_SOURCES||vel<=0||vel>127)return;
 if(part==3&&d_seq_is_drum()){d_drum_on(note,vel,source);return;}
 if(!d_osc_limit)return;
 d_prof_event(DPE_NOTE_ON);d_part *t=&dp[part];d_idle_wake(t);uint32_t before;
 if(t->held[source][note]<255)t->held[source][note]++;
 t->sustained[source][note]=0;int pick=-1,voices=d_voice_count(t);
 for(int j=0;j<voices;j++)if(t->slot[j].active&&t->slot[j].source==source&&t->slot[j].note==note){pick=j;break;}
 if(pick<0)for(int j=0;j<voices;j++)if(!t->slot[j].active||!t->slot[j].gate){pick=j;break;}
 if(pick<0){pick=0;for(int j=1;j<voices;j++)if(t->slot[j].age<t->slot[pick].age)pick=j;}
 int width=8/voices;unsigned count=d_osc_reserve(part,pick,(unsigned)width);before=d_slot_gates(t);
 t->slot[pick]=(d_slot){++d_age,1,1,(uint8_t)note,(uint8_t)source,(uint8_t)vel};
 for(int i=pick*width;i<(pick+1)*width;i++){t->osc[i].slot=(i-pick*width<(int)count)?(int8_t)pick:-1;d_osc_attack[part][i]=32;}
 d_release[part].tail=0;
 if(!before)d_retrigger(t);
}
static void d_note_off(int part,int note,int source){
 d_prof_event(DPE_NOTE_OFF);
 if(part<0||part>=4||note<0||note>127||source<0||source>=D_SOURCES)return;
 if(part==3&&d_seq_is_drum()){d_drum_off(note,source);return;}
 d_part *t=&dp[part];if(!t->held[source][note])return;
 if(!--t->held[source][note])t->sustained[source][note]=t->pedal[source];
 d_release_slots(t);
}
static void d_midi(int st,int n,int v,int source){
 if(source<0||source>=D_SOURCES)return;d_seen++;
 if(st==0xfa||st==0xfb||st==0xfc){d_seq_midi_transport(st,source);return;}
 if(st==0xf8){d_clock_receive(source);return;}
 if(st==0xff){d_seq_stop();for(int p=0;p<4;p++)d_panic_part(&dp[p]);return;}
 if(st<0x80||st>=0xf0||n<0||n>127||v<0||v>127){d_rejected++;return;}
 int ch=(st&15)+1,type=st&240;
 for(int p=0;p<4;p++){d_part *t=&dp[p];if(t->p[P_CHANNEL]!=ch)continue;
 if(type==0x90&&v)d_note_on(p,n,v,source);
 else if(type==0x80||(type==0x90&&!v))d_note_off(p,n,source);
 else if(type==0xe0)t->bend=n+128*v-8192;
 else if(type==0xb0){switch(n){
 case 1:t->p[P_LFO_AMT]=v;break;case 7:t->p[P_LEVEL]=v;break;case 10:t->p[P_PAN]=v;break;
 case 11:t->expression=v*32767/127;break;
 case 64:t->pedal[source]=v>=64;if(v<64){memset(t->sustained[source],0,128);d_release_slots(t);}break;
 case 71:t->p[P_RES]=v>120?120:v;break;case 74:t->p[P_CUT]=v;break;
 case 80:if(v>=64)d_retrigger(t);break;
 case 91:t->p[P_REVERB]=v;break;case 93:t->p[P_DLY_B]=v;break;case 94:t->p[P_DLY_A]=v;break;
 case 120:d_panic_part(t);break;
 case 121:t->bend=0;t->expression=32767;t->p[P_LFO_AMT]=p_default[P_LFO_AMT];t->pedal[source]=0;memset(t->sustained[source],0,128);d_release_slots(t);break;
 case 123:if(p==3&&d_seq_is_drum())for(int i=0;i<11;i++){d_drum_held[i][source]=0;if(d_drum_voice[i].source==source&&d_sample_views[i].loop)d_drum_voice[i].release=1;}memset(t->held[source],0,128);if(t->pedal[source])for(int j=0;j<4;j++)if(t->slot[j].gate&&t->slot[j].source==source)t->sustained[source][t->slot[j].note]=1;d_release_slots(t);break;
 default:break;}}
 }
}
/* Relative encoder changes are resolved in queue order, not against a stale
 * main-loop snapshot; rapid detents cannot overwrite one another. */
static int d_current(d_command x){
 int a=x.a,b=x.b;
 switch(x.op&255){
 case DC_PARAM:if(a>=0&&a<4&&b>=0&&b<P_COUNT)return dp[a].p[b];break;
 case DC_GLOBAL:if(b>=0&&b<G_COUNT)return dg[b];break;
 case DC_DELAY_CFG:return a>=0&&a<2&&b>=0&&b<4?(b==1?(int)d_delay_shift(a):d_delay_cfg[a][b]):0;
 case DC_DELAY:if(a>=0&&a<2&&b>=0&&b<D_COUNT)return b==D_TIME_MS?dc(dd[a][b],1,d_delay_max_ms(a)):dd[a][b];break;
 case DC_REVERB:if(b>=0&&b<V_COUNT)return dv[b];break;
 case DC_VOICE:if(a>=0&&a<4)return dp[a].voice_mode;break;
 case DC_OSC_LIMIT:return d_osc_limit;
 case DC_SAMPLE_LIMIT:return d_sample_limit;
 case DC_MOD_RATE:return d_mod_index;
 case DC_CPU:return a>=0&&a<4?d_cpu[a]:0;
 case DC_USB_GROUP:return !!(d_usb_groups&(1u<<b));
 case DC_USB_MODE:return d_usb_mode;
 case DC_DRUM_MIX:if(a>=0&&a<11&&b>=0&&b<6)return ((uint8_t *)&d_pad[a])[b];break;
 case DC_CLOCK:return b?d_clock_output:d_clock_mode;
 case DC_ROUTE:if(a>=0&&a<4&&b>=0&&b<13)return x.source?d_route_amount(&dp[a],b):d_route_target(&dp[a],b);break;
 case DC_OSC:if(a>=0&&a<4&&b>=0&&b<8){d_osc_config *o=&dp[a].cfg[b];return x.source==0?o->wave:x.source==1?o->tune:x.source==2?o->cents:o->level;}break;
 case DC_ENV:if(a>=0&&a<4&&b>=0&&b<4&&x.source>=8&&x.source<=9){uint8_t *e=x.source<8?dp[a].cfg[x.source].env:x.source==8?dp[a].fe:dp[a].pe;return e[b];}break;
 }
 return 0;
}
static void d_patch_command(int action,int value);
static void d_full_panic(void);
static void d_bench_command(int field,int value);
static void d_bench_end(void);
static void d_usb_release(void){for(int p=0;p<4;p++){d_part *t=&dp[p];memset(t->held[0],0,128);memset(t->sustained[0],0,128);t->pedal[0]=0;d_release_slots(t);}for(int i=0;i<11;i++){d_drum_held[i][0]=0;if(!d_drum_voice[i].source)d_drum_voice[i].release=1;}if(d_clock_source==0){d_clock_source=-1;d_clock_count=d_clock_period=0;d_clock_generation++;}}
static void d_cpu_set(int field,int value);
static void d_commands(void){for(int p=0;p<4;p++)d_idle_wake(&dp[p]);
 if(d_panic_requested||d_emergency){d_panic_requested=d_emergency=0;d_qr=d_qw;d_bench_end();d_full_panic();return;}
 uint32_t end=d_qw;__asm__ volatile("":::"memory");
 while(d_qr!=end){d_command x=d_queue[d_qr&127u];d_qr++;d_prof_event(DPE_COMMAND);int a=x.a,b=x.b,c=x.c;if(x.op&256)c+=d_current(x);
 switch(x.op&255){
 case DC_RUNTIME:d_bench_command(a,c);break;
 case DC_DELAY_CFG:if((x.op&256)&&a>=0&&a<2&&b==1&&!d_delay_cfg[a][0])c=dc(c,0,2);d_delay_config_set(a,b,c);break;
 case DC_OSC_LIMIT:d_osc_limit_set(c);break;
 case DC_SAMPLE_LIMIT:d_sample_limit_set(c);break;
 case DC_MOD_RATE:d_mod_index=(uint8_t)dc(c,0,3);break;
 case DC_CPU:d_cpu_set(a,c);break;
 case DC_USB_RELEASE:d_usb_release();break;
 case DC_USB_GROUP:if(b>=0&&b<3){if(c)d_usb_groups|=1u<<b;else d_usb_groups&=~(1u<<b);}break;
 case DC_USB_MODE:if(d_usb_mode!=!!c){d_usb_release();d_usb_mode=!!c;}break;
 case DC_DRUM_MIX:if(a>=0&&a<11&&b>=0&&b<6&&!d_mix_io){uint8_t *v=(uint8_t *)&d_pad[a];v[b]=dc(c,0,b>=2&&b<=4?128:b==5?1:127);d_mix_result=0;d_mix_dirty=1;}break;
 case DC_DRUM_SAVE:d_mix_stage();break;
 case DC_SEQ:d_seq_edit(a,b,c,x.source);break;
 case DC_SEQ_CTL:d_seq_control(a,b,c,x.source);break;
 case DC_MIDI:d_midi(a,b,c,x.source);break;
 case DC_NOTE:if(c)d_note_on(a,b,c,x.source);else d_note_off(a,b,x.source);break;
 case DC_PARAM:if(a>=0&&a<4&&b>=0&&b<P_COUNT){if(b==P_CHANNEL)d_panic_part(&dp[a]);dp[a].p[b]=(int16_t)dc(c,b==P_CHANNEL?1:0,p_max[b]);if(b==P_ENV_AMT)dp[a].env_amount[8]=dc((dp[a].p[b]-64)*2,-127,127);}break;
 case DC_GLOBAL:if(b>=0&&b<G_COUNT)dg[b]=(int16_t)dc(c,b==G_TEMPO?30:0,g_max[b]);break;
 case DC_DELAY:if(a>=0&&a<2&&b>=0&&b<D_COUNT)dd[a][b]=(int16_t)dc(c,b==D_TIME_MS?1:0,b==D_TIME_MS?d_delay_max_ms(a):d_max[b]);break;
 case DC_REVERB:if(b>=0&&b<V_COUNT)dv[b]=(int16_t)dc(c,0,v_max[b]);break;
 case DC_OSC:if(a>=0&&a<4&&b>=0&&b<8){d_osc_config *o=&dp[a].cfg[b];switch(x.source){case 0:o->wave=dc(c,0,3);break;case 1:o->tune=dc(c,-24,24);break;case 2:o->cents=dc(c,-50,50);break;case 3:o->level=dc(c,0,127);break;}}break;
 case DC_ENV:if(a>=0&&a<4&&b>=0&&b<4&&x.source>=8&&x.source<=9){uint8_t *e=x.source<8?dp[a].cfg[x.source].env:x.source==8?dp[a].fe:dp[a].pe;e[b]=(uint8_t)dc(c,0,127);}break;
 case DC_PATCH:d_patch_command(a,b);break;
 case DC_VOICE:if(a>=0&&a<4&&dp[a].voice_mode!=dc(c,0,2)){d_panic_part(&dp[a]);dp[a].voice_mode=(uint8_t)dc(c,0,2);}break;
 case DC_CLOCK:d_clock_set(b,c);break;
 case DC_ROUTE:if(a>=0&&a<4)d_route_set(&dp[a],b,x.source,c);break;
 case DC_RETRIG:if(a>=0&&a<4)d_retrigger(&dp[a]);break;
 case DC_PANIC:d_full_panic();break;
 }
 }
}
/* Low-rate independent modulators. Phase represents the next32-frame boundary.
 * Zero-depth sources still free-run, but skip waveform/target calculations. */
static int32_t d_lfo_wave(uint32_t phase,int shape){
 if(shape==0)return d_wave_ram[3][7][phase>>24];
 int32_t q=(int32_t)(phase>>16);
 if(shape==1)return q<32768?q*2-32768:98303-q*2;
 return q-32768;
}
/* Rate word: low7 bits free-rate index, bits7..10 beat division,
 * bit11 SYNC, bit12 division initialized. Old0..127 values stay free Hz. */
#define D_LFO_SYNC 2048
#define D_LFO_INIT 4096
static const char *const d_lfo_sync_names[16]={"2 BARS","1 BAR","1/2","1/4D","1/2T","1/4","1/8D","1/4T","1/8","1/16D","1/8T","1/16","1/16T","1/32","1/32T","1/64"};
static int d_lfo_rate_mode(int code,int sync){if(sync){if(!(code&D_LFO_INIT))code=(code&127)|(5<<7)|D_LFO_INIT;return code|D_LFO_SYNC;}return code&~D_LFO_SYNC;}
static uint32_t d_lfo_increment(int code){return code&D_LFO_SYNC?(d_lfo_sync_base[(code>>7)&15]*(uint32_t)d_clock_tempo()+128u)>>8:d_lfo_inc[code&127];}
static void d_prepare_lfos(d_part *t){
 t->modulation[DL_FILTER]=t->modulation[DL_PITCH]=t->modulation[DL_PAN]=0;t->modulation[DL_AMP]=32767;
 unsigned part=(unsigned)(t-dp);if(d_fx_mod_mask[part])memset(t->fx_mod,0,sizeof t->fx_mod);d_fx_mod_mask[part]=0;
 for(int i=0;i<8;i++)t->osc_pitch[i]=0;t->env_mask[8]=0;
 if(!d_cpu[1]){t->modulation[DL_FILTER]=dm(t->filter_env.v>>8,(t->p[P_ENV_AMT]-64)*2);for(int i=0;i<9;i++)t->env_single[i]=-1;return;}
 for(int source=0;source<5;source++){int src=d_mod_sources[source];
  int target=d_route_target(t,src),amount=d_route_amount(t,src);int32_t wave;
  if(src<3){uint32_t phase=t->lfo[src];t->lfo[src]+=d_lfo_increment(t->p[d_lfo_ids[src][0]])*d_mod_span;if(!amount||target==20)continue;wave=d_lfo_wave(phase,t->p[d_lfo_ids[src][2]]);}
  else wave=d_route_env(t,src-3)->v>>8;
  if(!amount||target==20)continue;
  if(target>=21){t->fx_mod[target-21]+=dm(wave,amount);d_fx_mod_mask[part]|=1u<<(target-21);continue;}
  if(src>=3&&target==2){t->env_mask[target==2?8:target-4]|=1u<<(src-3);continue;}
  if(target==0)t->modulation[DL_FILTER]+=src<3?dm(wave,amount*128)/128:dm(wave,amount);
  else if(target==3)t->modulation[DL_PAN]+=dm(wave,amount);
  else if(target==1)t->modulation[DL_PITCH]+=dm(wave,amount*200)/127;
  else if(target>=12&&target<20)t->osc_pitch[target-12]+=dm(wave,amount*200)/127;
  else if(src<3&&target==2){int depth=amount*32767/127;int gain=dc(32767-depth/2+dm(wave,depth)/2,0,32767);t->modulation[DL_AMP]=dm(t->modulation[DL_AMP],gain);}
 }
 for(int dest=8;dest<9;dest++){uint16_t m=t->env_mask[dest];t->env_single[dest]=m?-2:-1;if(m&&!(m&(m-1))){int src=0;while(!(m&1)){src++;m>>=1;}t->env_single[dest]=src;}}
 t->modulation[DL_PITCH]=dc(t->modulation[DL_PITCH],-600,600);
}
static __attribute__((always_inline)) inline int32_t d_amp_envelopes(d_part *t,int dest){if(dest<8)return 32768;if(!d_cpu[1])return d_route_env(t,dest==8?9:dest)->v>>8;int single=t->env_single[dest];if(single==-1)return 32768;if(single>=0){int value=d_route_env(t,single)->v>>8,amount=t->env_amount[single];if(amount<0)value=32768-value;int depth=da(amount);return depth==127?value:32768-depth*32768/127+dm(value,depth*32768/127);}
 uint16_t mask=t->env_mask[dest];int32_t gain=32768;for(int i=0;mask;i++,mask>>=1)if(mask&1){int value=d_route_env(t,i)->v>>8,amount=t->env_amount[i];if(amount<0)value=32768-value;int depth=da(amount);int factor=depth==127?value:32768-depth*32768/127+dm(value,depth*32768/127);gain=dm(gain,factor);}return gain;}
static int d_osc_running(d_part *t,int i){return d_budget_osc_active((int)(t-dp),i);}

/* Cached immutable OSC inputs, separate from phase and routed modulation. */
static struct {uint32_t inc;uint16_t pitch,gain;} d_osc_cache[4][8] D_POOL;
static __attribute__((always_inline)) inline int32_t d_control_step(int32_t delta){
 unsigned shift=d_mod_span==32?5:d_mod_span==16?4:d_mod_span==8?3:2;
 return delta<0?-((-delta)>>shift):delta>>shift;
}
static struct {int32_t key[5],target[5];} d_gain_cache[4] D_POOL;
static void d_prepare_part(d_part *t){
 d_prepare_lfos(t);
 int32_t bend=t->bend;uint32_t ratio=65536+(bend>=0?bend*8025/8192:bend*7150/8192);
 for(int i=0;i<8;i++){d_osc *o=&t->osc[i];if((d_cpu[0]<4&&(i&((8u>>t->voice_mode)-1u))>=d_cpu_oscs[d_cpu[0]])||o->slot<0)continue;d_slot *s=&t->slot[(int)o->slot];d_osc_config *c=&t->cfg[i];
  unsigned part=(unsigned)(t-dp);int product=c->level*s->velocity;
  if(d_osc_cache[part][i].gain!=product){d_osc_cache[part][i].gain=(uint16_t)product;o->gain=product*32767/(127*127);}
  int note=dc(s->note+c->tune,0,127);unsigned key=1u+(unsigned)note+((unsigned)(c->cents+50)<<7);
  if(d_osc_cache[part][i].pitch!=key){d_osc_cache[part][i].pitch=(uint16_t)key;d_osc_cache[part][i].inc=(uint32_t)(((uint64_t)d_inc[note]*d_cents[c->cents+50])>>16);}
  uint32_t inc=d_osc_cache[part][i].inc;
  uint32_t next=(uint32_t)(((uint64_t)inc*ratio)>>16);int cents=dc(t->modulation[DL_PITCH]+t->osc_pitch[i],-600,600);if(cents)next=(uint32_t)(((uint64_t)next*d_pitch_lfo_ratio[cents+600])>>16);
  if(next!=o->inc){o->inc=next;o->band=0;uint32_t h=128;while(o->band<7&&(uint64_t)o->inc*h>0x78000000u){o->band++;h>>=1;}}
 }
 int cut=t->p[P_CUT]+t->modulation[DL_FILTER];
 if(t->p[P_KEYTRACK]){int note=60;uint32_t age=0;for(int j=0;j<4;j++)if(t->slot[j].gate&&t->slot[j].age>age){age=t->slot[j].age;note=t->slot[j].note;}cut+=(note-60)*t->p[P_KEYTRACK]/127;}
 const int32_t *c=d_fc_ram[dc(cut,0,127)][dc(t->p[P_RES]>>3,0,15)];
 for(int i=0;i<4;i++)t->cstep[i]=d_control_step(c[i]-t->c[i]);
 int32_t level=t->p[P_MUTE]?0:t->p[P_LEVEL]*128*t->expression/32767; /* 4-part headroom */
 if(t->modulation[DL_AMP]!=32767)level=dm(level,t->modulation[DL_AMP]);
 int pan=dc(t->p[P_PAN]+t->modulation[DL_PAN],0,127);
 unsigned part=(unsigned)(t-dp);int32_t key[5]={level,pan,t->p[P_DLY_A],t->p[P_DLY_B],t->p[P_REVERB]};
 int changed=0;for(int i=0;i<5;i++)if(key[i]!=d_gain_cache[part].key[i])changed=1;
 int32_t *target=d_gain_cache[part].target;
 if(changed){for(int i=0;i<5;i++)d_gain_cache[part].key[i]=key[i];target[0]=level*(127-pan)/127;target[1]=level*pan/127;for(int i=2;i<5;i++)target[i]=level*key[i]/127;}
 for(int i=0;i<5;i++)t->gainstep[i]=d_control_step(target[i]*256-t->gain[i]);
 d_idle_prepare(t);
}
#ifndef D_ACTIVE_FAST
#define D_ACTIVE_FAST 1
#endif
static __attribute__((always_inline)) inline int32_t d_amp_fast(d_part *t,int dest){
 int own=dest==8?9:dest;
 if(dest<8)return 32768;
 if(!d_cpu[1]||(t->env_single[dest]==own&&t->env_amount[own]==127))return d_route_env(t,own)->v>>8;
 if(t->env_single[dest]==-1)return 32768;
 return d_amp_envelopes(t,dest);
}
#if defined(D_PLATFORM_MEMORY) && !defined(DINGHY_HOST)
#define D_SAMPLE_RAM __attribute__((section(".ram_text"),noinline))
#else
#define D_SAMPLE_RAM __attribute__((noinline))
#endif
/* No calls or flash-dependent helpers: checked in the native disassembly. */
static __attribute__((always_inline)) inline int32_t d_synth_core(d_part *t,unsigned limit,unsigned release,int32_t post){
 unsigned part=(unsigned)(t-dp);int32_t sum=d_osc_tail[part];if(d_osc_tail_left[part]){d_osc_tail[part]-=sum/d_osc_tail_left[part];if(!--d_osc_tail_left[part])d_osc_tail[part]=0;}
 unsigned within=(8u>>t->voice_mode)-1u;
 for(int i=0;i<8;i++){
  if((i&within)>=limit)continue;d_osc *o=&t->osc[i];int slot=o->slot;if(slot<0||(!t->slot[slot].gate&&!(release&(1u<<slot))))continue;const int16_t *w=d_wave_ram[t->cfg[i].wave][o->band];unsigned at=o->phase>>24;int32_t fraction=(o->phase>>9)&32767u;
  int32_t x=w[at]+d_q15_small(w[at+1]-w[at],fraction);o->phase+=o->inc;int32_t y=d_q15_small(x,o->gain);if(d_osc_attack[part][i]){y=y*(32-d_osc_attack[part][i])/32;d_osc_attack[part][i]--;}d_osc_last[part][i]=(post||t->post_env.stage)?y:0;sum+=y;
 }
 sum=d_drive(sum/8,t->p[P_DRIVE]);for(int i=0;i<4;i++)t->c[i]+=t->cstep[i];
 int32_t v3=sum-t->s2,v1=dq(t->c[0],t->s1)+dq(t->c[1],v3),v2=t->s2+dq(t->c[1],t->s1)+dq(t->c[2],v3);
 t->s1=dc(2*v1-t->s1,-262143,262143);t->s2=dc(2*v2-t->s2,-262143,262143);
 int32_t hp=sum-dq(t->c[3],v1)-v2,x=t->p[P_MODE]==0?v2:t->p[P_MODE]==1?dq(t->c[3],v1):t->p[P_MODE]==2?hp:hp+v2;
 x=d_q15_small(dc(x,-65535,65535),post);for(int i=0;i<5;i++)t->gain[i]+=t->gainstep[i];return x;
}
static D_SAMPLE_RAM int32_t d_synth_sample(d_part *t,unsigned limit,unsigned release,int32_t post){return d_synth_core(t,limit,release,post);}
#if defined(D_PLATFORM_MEMORY) && !defined(DINGHY_HOST)
static __attribute__((always_inline)) inline int32_t d_synth_call(d_part *t,unsigned limit,unsigned release,int32_t post){
 /* XIP-to-SRAM exceeds this ISA's direct-call range: same volatile-pointer
    far-call pattern as the platform flash HAL. */
 int32_t (*volatile entry)(d_part *,unsigned,unsigned,int32_t)=d_synth_sample;return entry(t,limit,release,post);
}
#else
#define d_synth_call d_synth_sample
#endif
#if D_ACTIVE_FAST
static int32_t d_part_tick(d_part *t){
 if(t==&dp[3]&&d_seq_is_drum()){int32_t x=d_drive(d_drum_tick(),t->p[P_DRIVE]);for(int i=0;i<5;i++)t->gain[i]+=t->gainstep[i];if((uint32_t)da(x)>t->peak)t->peak=(uint32_t)da(x);return x;}
 int mode=d_cpu[0];if(!mode||!d_osc_limit)return 0;
 unsigned idle=(unsigned)(t-dp);if(D_IDLE_CACHE&&d_idle[idle].sleep){for(int i=0;i<4;i++)t->c[i]+=t->cstep[i];for(int i=0;i<5;i++)t->gain[i]+=t->gainstep[i];return d_idle[idle].sample;}
 int32_t old_s1=t->s1,old_s2=t->s2;unsigned release=t->post_env.stage==4?d_release[idle].tail:0;
 if(t->filter_env.left)d_env_tick(&t->filter_env,t->fe);if(t->post_env.left)d_env_tick(&t->post_env,t->pe);
 int32_t x=d_synth_call(t,d_cpu_oscs[mode],release,d_amp_fast(t,8));
 if(D_IDLE_CACHE&&d_idle[idle].eligible&&old_s1==t->s1&&old_s2==t->s2){d_idle[idle].sample=x;d_idle[idle].sleep=1;}
 if((uint32_t)da(x)>t->peak)t->peak=(uint32_t)da(x);return x;
}
#else
static int32_t d_part_tick(d_part *t){
 if(t==&dp[3]&&d_seq_is_drum()){int32_t x=d_drive(d_drum_tick(),t->p[P_DRIVE]);for(int i=0;i<5;i++)t->gain[i]+=t->gainstep[i];if((uint32_t)da(x)>t->peak)t->peak=(uint32_t)da(x);return x;}
 if(!d_cpu[0]||!d_osc_limit)return 0;
 unsigned idle=(unsigned)(t-dp);if(D_IDLE_CACHE&&d_idle[idle].sleep){for(int i=0;i<4;i++)t->c[i]+=t->cstep[i];for(int i=0;i<5;i++)t->gain[i]+=t->gainstep[i];return d_idle[idle].sample;}
 int32_t old_s1=t->s1,old_s2=t->s2;
 uint8_t running[8];for(int i=0;i<8;i++){running[i]=d_osc_running(t,i);}
 int32_t sum=0;for(int i=0;i<8;i++){d_osc *o=&t->osc[i];if(!running[i])continue;
  d_osc_config *c=&t->cfg[i];const int16_t *w=d_wave_ram[c->wave][o->band];uint32_t at=o->phase>>24,f=(o->phase>>9)&32767u;
  int32_t x=w[at]+dm(w[at+1]-w[at],f);o->phase+=o->inc;
  x=dm(x,o->gain);sum+=x;
 }
 sum=d_drive(sum/8,t->p[P_DRIVE]);
 for(int i=0;i<4;i++)t->c[i]+=t->cstep[i];
 int32_t v3=sum-t->s2,v1=dq(t->c[0],t->s1)+dq(t->c[1],v3),v2=t->s2+dq(t->c[1],t->s1)+dq(t->c[2],v3);
 t->s1=dc(2*v1-t->s1,-262143,262143);t->s2=dc(2*v2-t->s2,-262143,262143);
 int32_t hp=sum-dq(t->c[3],v1)-v2,x=t->p[P_MODE]==0?v2:t->p[P_MODE]==1?dq(t->c[3],v1):t->p[P_MODE]==2?hp:hp+v2;
 d_env_tick(&t->filter_env,t->fe);d_env_tick(&t->post_env,t->pe);x=dm(dc(x,-65535,65535),d_amp_envelopes(t,8));
 for(int i=0;i<5;i++)t->gain[i]+=t->gainstep[i];
 if(D_IDLE_CACHE&&d_idle[idle].eligible&&old_s1==t->s1&&old_s2==t->s2){d_idle[idle].sample=x;d_idle[idle].sleep=1;}
 if((uint32_t)da(x)>t->peak)t->peak=(uint32_t)da(x);return x;
}
#endif
#include "effects.h"
#include "sequencer.h"
static void d_cpu_set(int field,int value){if(field<0||field>3)return;value=dc(value,0,field?1:4);if(d_cpu[field]!=value){d_cpu[field]=value;if(field==0){for(int p=0;p<4;p++)if(p!=3||!d_seq_is_drum()){d_panic_part(&dp[p]);if(!value)memset(dp[p].fx_mod,0,sizeof dp[p].fx_mod);}}if(field>=2){uint32_t limits=d_fx_limits;d_fx_init();d_fx_limits=limits;}}d_cpu_revision++;}

static void d_init(void){
 d_prof_init();d_limiter_reset();memset(d_osc_last,0,sizeof d_osc_last);memset(d_osc_tail,0,sizeof d_osc_tail);memset(d_osc_attack,0,sizeof d_osc_attack);memset(d_osc_tail_left,0,sizeof d_osc_tail_left);memset(d_sound_frames,0,sizeof d_sound_frames);d_common_frames=0;
 memset(&d_no_osc_env,0,sizeof d_no_osc_env);memset(d_release,0,sizeof d_release);memset(d_idle,0,sizeof d_idle);memset(dp,0,sizeof dp);memcpy(d_wave_ram,d_waves,sizeof d_wave_ram);memcpy(d_fc_ram,d_fc,sizeof d_fc_ram);
 memcpy(dg,g_default,sizeof dg);memcpy(dv,v_default,sizeof dv);for(int j=0;j<2;j++)memcpy(dd[j],d_default,sizeof dd[j]);
 dd[1][D_MODE]=0;dd[1][D_TIME_MS]=536;dd[1][D_FEEDBACK]=45;dd[1][D_TO_REV]=15;
 for(int p=0;p<4;p++){d_part *t=&dp[p];t->voice_mode=2;memcpy(t->p,p_default,sizeof t->p);t->p[P_CHANNEL]=p+1;if(p==3)t->p[P_DRIVE]=0;d_route_defaults(t);
  for(int i=0;i<8;i++){t->cfg[i]=(d_osc_config){0,100,{1,34,90,32},0,(int8_t)((i&1)?4:-4)};t->osc[i].slot=-1;}
  uint8_t fe[4]={1,45,0,30},pe[4]={1,25,127,42};memcpy(t->fe,fe,4);memcpy(t->pe,pe,4);t->expression=32767;
  memcpy(t->c,d_fc_ram[t->p[P_CUT]][t->p[P_RES]>>3],sizeof t->c);
 }
 memset(d_gain_cache,0,sizeof d_gain_cache);memset(d_osc_cache,0,sizeof d_osc_cache);memset(d_fx_mod_mask,0,sizeof d_fx_mod_mask);
 d_qw=d_qr=d_overflows=d_emergency=d_panic_requested=d_frame=d_age=d_clips=d_seen=d_rejected=d_active=0;
 d_cpu[0]=4;d_cpu[1]=d_cpu[2]=1;d_cpu[3]=0;d_cpu_revision=d_rt_revision=0;d_bench_active=0;d_mod_index=0;d_osc_limit=8;d_sample_limit=4;d_mod_span=32;d_mod_left=0;for(int b=0;b<2;b++){d_delay_cfg[b][0]=d_delay_cfg[b][2]=d_delay_cfg[b][3]=0;d_delay_cfg[b][1]=2;}d_usb_mode=d_capture_active=0;d_usb_groups=7;d_sample_catalog_reset();d_mix_init();d_drum_cache_init();d_clock_init();d_seq_init();d_prepare_fx();d_fx_init();
}
static void d_render_reference(int32_t *out,uint32_t n){
 d_commands();d_seq_service();for(uint32_t f=0;f<n;f++){
  d_seq_sample();if(!d_mod_left){d_mod_span=d_mod_intervals[d_mod_index];d_mod_left=d_mod_span;for(int p=0;p<4;p++){dp[p].peak=0;if(d_cpu[0]||(p==3&&d_seq_is_drum()))d_prepare_part(&dp[p]);}d_prepare_fx();d_mix_prepare();}
  int32_t l=0,r=0,send[3]={0,0,0};int16_t stem[17];
  for(int p=0;p<4;p++){d_part *t=&dp[p];int32_t x=d_part_tick(t);if(d_capture_active)stem[2+p]=(d_usb_groups&2)?dc((p==3&&d_seq_is_drum()?d_drum_bus[8]:x)/2,-32767,32767):0;if(p==3&&d_seq_is_drum()){int32_t bus[8];for(int j=0;j<8;j++){
 if(D_SPARSE_FAST&&j>=2&&!((j-2)%3<2?d_cpu[2]:d_cpu[3]))continue;
 if(!D_FULL_FAST)bus[j]=d_drive(d_drum_bus[j],t->p[P_DRIVE]);
 else if(!d_drum_bus[j])bus[j]=0;
 else if(d_drum_bus[j]==d_drum_source_mono)bus[j]=x;
 else if(j&&d_drum_bus[j]==d_drum_bus[0])bus[j]=bus[0];
 else bus[j]=d_drive(d_drum_bus[j],t->p[P_DRIVE]);
}l+=dm(bus[0],t->gain[0]>>8);r+=dm(bus[1],t->gain[1]>>8);for(int j=0;j<3;j++)if(!D_SPARSE_FAST||(j<2?d_cpu[2]:d_cpu[3]))send[j]+=dm(bus[2+j],t->gain[2+j]>>8)+dm(bus[5+j],(t->gain[0]+t->gain[1])>>8);}else if(x){l+=dm(x,t->gain[0]>>8);r+=dm(x,t->gain[1]>>8);for(int j=0;j<3;j++)if(!D_SPARSE_FAST||(j<2?d_cpu[2]:d_cpu[3]))send[j]+=dm(x,t->gain[j+2]>>8);}}
  d_fx_tick(l,r,send,&l,&r);if(l)l=d_master_scale(l,d_fxg[G_MASTER]);if(r)r=d_master_scale(r,d_fxg[G_MASTER]);
  if(da(l)>32767||da(r)>32767)d_clips++;out[2*f]=dc(l,-32767,32767);out[2*f+1]=dc(r,-32767,32767);if(d_capture_active){stem[0]=(d_usb_groups&1)?dc(out[2*f]*d_capture_main_gain/4096,-32767,32767):0;stem[1]=(d_usb_groups&1)?dc(out[2*f+1]*d_capture_main_gain/4096,-32767,32767):0;for(int j=0;j<11;j++)stem[6+j]=(d_usb_groups&4)&&d_seq_is_drum()?d_drum_raw[j]:0;D_CAPTURE_HOOK(stem);}d_clock_step();d_frame++;d_mod_left--;
 }
 d_active=0;for(int p=0;p<4;p++)for(int i=0;i<8;i++)d_active+=!!d_osc_running(&dp[p],i);
}

#include "block_render.h"

#include "presets.h"

/* Global panic is serviced at the next audio block, outside the SPSC queue.
 * Drop queued note-ons/preview commands so they cannot restart old sound. */
static void d_full_panic(void){
 ds.fill=ds.trig=0;d_seq_stop();d_limiter_reset();memset(d_sound_frames,0,sizeof d_sound_frames);d_common_frames=0;
 int restored=preset_preview;if(restored)preset_apply(&preset_backup);
 preset_preview=preset_listen=preset_busy=0;
 for(int p=0;p<4;p++){d_panic_part(&dp[p]);dp[p].bend=0;dp[p].expression=32767;dp[p].peak=0;}
 d_active=0;if(!restored)d_fx_init();
}

#include "benchmark_control.h"

/* Shared by native app and device-free transfer tests. */
static int d_sample_audio_gate(int32_t *out,uint32_t n,int streaming){
 if(d_sample_pause==1){d_commands();if(d_bench_active||streaming||ds.playing||d_osc_active_count()||d_sample_active_count()||preset_preview||preset_busy||preset_io>=0||ds.saving>=0||d_mix_io)d_sample_pause=3;else{d_full_panic();d_qr=d_qw;d_sample_pause=2;}}
 if(d_sample_pause==2){d_qr=d_qw;for(uint32_t i=0;i<n*2u;i++)out[i]=0;return 1;}
 return 0;
}

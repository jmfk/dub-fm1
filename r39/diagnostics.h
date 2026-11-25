/* SPDX-License-Identifier: GPL-3.0-only; portable bounded performance accounting. */
#pragma once
#include <stdint.h>
#define D_PERF_FIELDS 35u
#define D_PERF_REPLY_SIZE (8u+D_PERF_FIELDS*5u+2u)
typedef struct {uint32_t blocks,frames;uint64_t ticks,budget;uint32_t peak_ticks,peak_budget,late;} d_perf_scope;
typedef struct {uint32_t generation;d_perf_scope app,isr;} d_perf_snapshot;
static struct {volatile uint32_t sequence,reset_pending,generation;volatile d_perf_scope app,isr;} d_perf;
static void d_perf_record(int isr,uint32_t ticks,uint32_t frames,uint32_t budget){if(!frames||!budget)return;d_perf.sequence++;__asm__ volatile("":::"memory");volatile d_perf_scope *s=isr?&d_perf.isr:&d_perf.app;s->blocks++;s->frames+=frames;s->ticks+=ticks;s->budget+=budget;if(!s->peak_budget||(uint64_t)ticks*s->peak_budget>(uint64_t)s->peak_ticks*budget){s->peak_ticks=ticks;s->peak_budget=budget;}if(ticks>budget)s->late++;__asm__ volatile("":::"memory");d_perf.sequence++;}
static int d_perf_apply_reset(void){if(!d_perf.reset_pending)return 0;d_perf.sequence++;__asm__ volatile("":::"memory");d_perf_scope zero={0};d_perf.app=zero;d_perf.isr=zero;d_perf.generation++;d_perf.reset_pending=0;__asm__ volatile("":::"memory");d_perf.sequence++;return 1;}
static int d_perf_copy(d_perf_snapshot *out){for(int i=0;i<8;i++){uint32_t before=d_perf.sequence;if(before&1u)continue;__asm__ volatile("":::"memory");out->generation=d_perf.generation;out->app=d_perf.app;out->isr=d_perf.isr;__asm__ volatile("":::"memory");if(before==d_perf.sequence)return 1;}return 0;}
static int d_perf_request(const uint8_t *p,uint32_t n){static const uint8_t prefix[6]={0xf0,0x7d,0x44,0x55,0x42,1};if(n!=8||p[7]!=0xf7||p[6]>1)return -1;for(int i=0;i<6;i++)if(p[i]!=prefix[i])return -1;return p[6];}
static void d_perf_scope_fields(uint32_t *v,const d_perf_scope *s){v[0]=s->blocks;v[1]=s->frames;v[2]=(uint32_t)s->ticks;v[3]=(uint32_t)(s->ticks>>32);v[4]=(uint32_t)s->budget;v[5]=(uint32_t)(s->budget>>32);v[6]=s->peak_ticks;v[7]=s->peak_budget;v[8]=s->late;}
static uint32_t d_perf_encode(uint8_t *out,int command,const uint32_t fields[D_PERF_FIELDS]){out[0]=0xf0;out[1]=0x7d;out[2]=0x44;out[3]=0x55;out[4]=0x42;out[5]=1;out[6]=(uint8_t)(0x40|command);out[7]=D_PERF_FIELDS;uint32_t at=8;for(unsigned i=0;i<D_PERF_FIELDS;i++){uint32_t n=fields[i];for(int j=0;j<5;j++){out[at++]=(uint8_t)(n&127u);n>>=7;}}uint8_t checksum=0;for(unsigned i=1;i<at;i++)checksum^=out[i];out[at++]=checksum;out[at++]=0xf7;return at;}

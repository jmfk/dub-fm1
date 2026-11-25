/* Global, session-only CPU isolation. Defaults: reverb bypassed. */
#pragma once
#include <stdint.h>
static volatile uint8_t d_cpu[4]={4,1,1,0};
static volatile uint32_t d_cpu_revision;
/* Session setting; span/remaining are owned by audio. Apply new index at next
 * control boundary so existing ramps finish and LFO phase stays continuous. */
static volatile uint8_t d_mod_index,d_osc_limit,d_sample_limit;
/* engine0 filtered/1 lo-fi; half-rate divisor exponent; linear bit reduction0/4/8/12; feedback degradation. */
static volatile uint8_t d_delay_cfg[2][4]={{0,2,0,0},{0,2,0,0}};
static uint8_t d_mod_span=32,d_mod_left;
static const uint8_t d_mod_intervals[4]={32,16,8,4};
static const uint8_t d_cpu_oscs[5]={0,1,2,4,8};
#define D_RT_FIELDS 126u
static uint8_t d_rt_snapshot[D_RT_FIELDS];
static volatile uint32_t d_rt_revision;
static volatile uint8_t d_bench_active;
static int d_runtime_field_valid(unsigned f,unsigned v){return f==0?v<=4:f<=3?v<=1:f==4?v<=12:f==5?v<=33:f==6?v<=3:f==7?v<=1:f==8?v==1:f==9?v<=1:f==10?v<=2:f==11?v<=1:f>=12&&f<=19?((f-12)%4==1?v<=5:(f-12)%4==2?v<=3:(f-12)%4==3?v<=127:v<=1):0;}
static int d_runtime_lease_due(uint32_t now,uint32_t last){return d_bench_active&&(uint32_t)(now-last)>15000u;}
static int d_runtime_request(const uint8_t *p,uint32_t n){
 static const uint8_t prefix[6]={0xf0,0x7d,0x44,0x55,0x42,1};
 if(n!=8&&n!=9&&n!=10&&n!=11)return -1;for(int i=0;i<6;i++)if(p[i]!=prefix[i])return -1;
 if(n==9&&p[6]==4&&p[7]<128&&p[8]==0xf7)return 4;
 if(n==11&&p[6]==5&&p[7]<128&&d_runtime_field_valid(p[8],p[9])&&p[10]==0xf7)return 5;
 if(n==8&&p[6]==3&&p[7]==0xf7)return 3;
 if(n==10&&p[6]==2&&p[7]<4&&p[8]<=(p[7]?1:4)&&p[9]==0xf7)return 2;
 return -1;
}
static unsigned d_runtime_reply(uint8_t *out,int command){uint32_t before=d_cpu_revision;out[0]=0xf0;out[1]=0x7d;out[2]=0x44;out[3]=0x55;out[4]=0x42;out[5]=1;out[6]=0x40|command;out[7]=4;for(int i=0;i<4;i++)out[8+i]=d_cpu[i];uint8_t sum=0;for(int i=1;i<12;i++)sum^=out[i];out[12]=sum;out[13]=0xf7;__asm__ volatile("":::"memory");return before==d_cpu_revision?14:0;}

static unsigned d_runtime_extended_reply(uint8_t *out,int command,uint8_t id){
 out[0]=0xf0;out[1]=0x7d;out[2]=0x44;out[3]=0x55;out[4]=0x42;out[5]=1;out[6]=(uint8_t)(0x40|command);out[7]=id;out[8]=D_RT_FIELDS;
 for(unsigned i=0;i<D_RT_FIELDS;i++)out[9+i]=d_rt_snapshot[i];uint8_t sum=0;for(unsigned i=1;i<9+D_RT_FIELDS;i++)sum^=out[i];out[9+D_RT_FIELDS]=sum;out[10+D_RT_FIELDS]=0xf7;return 11+D_RT_FIELDS;
}

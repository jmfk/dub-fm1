/* SPDX-License-Identifier: GPL-3.0-only; bounded RAM logging, never flash. */
#pragma once
#include <stdint.h>
#include "run_identity.h"
#define AD_MAGIC 0x344c4144u
#define AD_VERSION 1u
#define AD_CONFIG 0x05010001u
#define AD_POINTS 64u
#define AD_WORDS 96u
#define AD_POINT_WORDS 16u
#define AD_SNAPSHOT_WORDS (AD_WORDS+AD_POINTS*AD_POINT_WORDS)
enum {AD_MAGIC_I,AD_SCHEMA,AD_BUILD,AD_CONFIG_I,AD_GENERATION,AD_SIZE,AD_READY,AD_FAULT,AD_CASE,AD_RUN,AD_STATE,AD_MS,AD_BLOCKS,AD_CPU_LAST,AD_CPU_PEAK,AD_CPU_MEAN,AD_LATE,AD_CONSEC,AD_SYNTH_LAST,AD_SYNTH_PEAK,AD_FX_LAST,AD_FX_PEAK,AD_PLAY,AD_TICK,AD_EVENT,AD_ACTIVE,AD_PEAK,AD_CLAMP,AD_QUEUE_DROP,AD_KEYS,AD_MIDI_ON,AD_MIDI_OFF,AD_MIDI_CC,AD_MIDI_BEND,AD_MIDI_OUT,AD_MASTER,AD_USB_UP,AD_USB_CONFIG,AD_USB_RX_ALT,AD_USB_TX_ALT,AD_USB_RATE,AD_RX_PACKETS,AD_RX_FRAMES,AD_RX_BAD,AD_RX_STATUS,AD_RX_LENGTH,AD_RX_UNDER,AD_RX_OVER,AD_RX_PEAK_L,AD_RX_PEAK_R,AD_TX_PACKETS,AD_TX_FRAMES,AD_TX_UNDER,AD_TX_OVER,AD_TX_MISSED,AD_TX_STALL,AD_PRE_L,AD_PRE_R,AD_CAPTURE_L,AD_CAPTURE_R,AD_DAC_L,AD_DAC_R,AD_LOG_COUNT,AD_LOG_TOTAL,AD_ECHO_COUNT,AD_RECOVERIES,AD_MAIN_PEAK,AD_RENDERS,AD_UAC_LOW=68,AD_UAC_HIGH,AD_UAC_FILL,AD_SHARED_AWAKE,AD_NOTES_BASE=72,AD_OFFS_BASE=80,AD_FX_SKIP_TRACKS=88,AD_FX_SKIP_WET,AD_FX_WET_FRAMES,AD_COMP_UPDATES,AD_USB_ADJ_UP,AD_USB_ADJ_DOWN,AD_RING_PRIME,AD_RING_CAPACITY};
static volatile uint32_t ad_epoch,ad_wanted_case,ad_wanted_run,ad_request,ad_control_ready;
static uint32_t ad_live[AD_WORDS],ad_log[AD_POINTS][AD_POINT_WORDS],ad_snapshot[AD_SNAPSHOT_WORDS];
static uint32_t ad_log_w,ad_log_count,ad_log_total,ad_log_at,ad_generation,ad_frozen_size,ad_frozen_crc;
static uint32_t ad_start_ms,ad_note_stage,ad_note_ons[8],ad_note_offs[8],ad_midi_on,ad_midi_off,ad_midi_cc,ad_midi_bend,ad_midi_out,ad_key_edges,ad_recoveries,ad_main_peak;
static volatile uint32_t ad_pause,ad_pause_ack,ad_ready;
static uint32_t ad_case=0xffffffffu,ad_run,ad_state,ad_echo_note,ad_echo_ch,ad_echo_until,ad_echo_count;
static uint32_t ad_pre_l,ad_pre_r,ad_cap_l,ad_cap_r,ad_dac_l,ad_dac_r,ad_synth_last,ad_synth_peak,ad_fx_last,ad_fx_peak,ad_render_blocks;
static uint32_t ad_cpu_sum,ad_cpu_n;
static uint32_t ad_crc(const void *data,uint32_t n){const uint8_t *p=data;uint32_t c=~0u;while(n--){c^=*p++;for(uint32_t i=0;i<8;i++)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));}return ~c;}
static uint32_t ad_pack(const uint8_t *p,uint32_t n,uint8_t *out){uint32_t acc=0,bits=0,w=0;while(n--){acc|=(uint32_t)*p++<<bits;bits+=8;while(bits>=7){out[w++]=acc&127u;acc>>=7;bits-=7;}}if(bits)out[w++]=acc&127u;return w;}
static uint32_t ad_arg(const uint8_t *p){return p[0]|(uint32_t)p[1]<<7|(uint32_t)p[2]<<14|(uint32_t)p[3]<<21|(uint32_t)p[4]<<28;}
static void ad_begin_write(void){ad_epoch++;__asm__ volatile("":::"memory");}
static void ad_end_write(void){__asm__ volatile("":::"memory");ad_epoch++;}
static void ad_note_seen(uint32_t st,uint32_t n,uint32_t v){uint32_t ch=st&15u,ty=st&0xf0u;(void)n;if(ty==0x90&&v){ad_midi_on++;if(ch<8)ad_note_ons[ch]++;}else if(ty==0x80||(ty==0x90&&!v)){ad_midi_off++;if(ch<8)ad_note_offs[ch]++;}else if(ty==0xb0)ad_midi_cc++;else if(ty==0xe0)ad_midi_bend++;}
static int ad_freeze(void){for(uint32_t attempt=0;attempt<3;attempt++){uint32_t seq=ad_epoch;if(seq&1u)continue;__asm__ volatile("":::"memory");
 for(uint32_t i=0;i<AD_WORDS;i++){ad_snapshot[i]=ad_live[i];}
 uint32_t count=ad_log_count,start=ad_log_w-count;
 for(uint32_t i=0;i<count;i++)for(uint32_t k=0;k<AD_POINT_WORDS;k++)ad_snapshot[AD_WORDS+i*AD_POINT_WORDS+k]=ad_log[(start+i)%AD_POINTS][k];
 __asm__ volatile("":::"memory");if(ad_epoch!=seq)continue;ad_generation++;ad_frozen_size=(AD_WORDS+count*AD_POINT_WORDS)*4u;ad_snapshot[AD_GENERATION]=ad_generation;ad_snapshot[AD_SIZE]=ad_frozen_size;ad_frozen_crc=ad_crc(ad_snapshot,ad_frozen_size);return 1;}return 0;}
static int ad_wire_send(const uint8_t *,uint32_t);
/* F0 7D 41 4C 01 cmd seq argA[5x7] argB[5x7] F7. No firmware commands. */
static int ad_sysex(const uint8_t *p,uint32_t n){
 if(n<4||p[0]!=0x7d||p[1]!=0x41||p[2]!=0x4c||p[3]!=1)return 0;
 uint8_t out[256];uint32_t cmd=n>4?p[4]:0,seq=n>5?p[5]:0,a=0,b=0,w=8,meta[8];
 out[0]=0xf0;out[1]=0x7d;out[2]=0x41;out[3]=0x4c;out[4]=1;out[5]=cmd|0x40u;out[6]=seq;out[7]=0;
 if(n!=16||p[10]>15||p[15]>15||cmd<1||cmd>6)out[7]=2;
 else{a=ad_arg(p+6);b=ad_arg(p+11);
 if(cmd==1){meta[0]=AD_MAGIC;meta[1]=AD_BUILD_TAG;meta[2]=AD_CONFIG;meta[3]=AD_SNAPSHOT_WORDS*4;meta[4]=ad_ready;meta[5]=ad_case;meta[6]=ad_run;meta[7]=ad_state;w+=ad_pack((uint8_t*)meta,sizeof meta,out+w);}
 if(cmd==2){if(!ad_freeze())out[7]=1;else{meta[0]=AD_MAGIC;meta[1]=ad_generation;meta[2]=ad_frozen_size;meta[3]=ad_frozen_crc;meta[4]=AD_BUILD_TAG;meta[5]=AD_CONFIG;meta[6]=ad_run;meta[7]=ad_case;w+=ad_pack((uint8_t*)meta,sizeof meta,out+w);}}
 if(cmd==3){if(!ad_frozen_size||b>192||a>ad_frozen_size||b>ad_frozen_size-a)out[7]=2;else w+=ad_pack((uint8_t*)ad_snapshot+a,b,out+w);}
 if(cmd==4){if(!b||a>35||(!((a<=8)||(a>=16&&a<=23)||a>=32)))out[7]=2;else if(ad_case==a&&ad_run==b&&(ad_state==1||ad_state==2)){}else if(ad_request)out[7]=1;else{ad_wanted_case=a;ad_wanted_run=b;__asm__ volatile("":::"memory");ad_request=1;}}
 if(cmd==5){if(b&&ad_run!=b)out[7]=3;else ad_request=2;}
 if(cmd==6){if(a>7||b>127||ad_echo_until)out[7]=2;else{ad_echo_ch=a;ad_echo_note=b;ad_echo_until=1;}}
 }
 out[w++]=0xf7;ad_wire_send(out,w);return 1;
}

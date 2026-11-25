/* SPDX-License-Identifier: GPL-3.0-only; main/control and ISR telemetry. */
#pragma once
static uint32_t ad_fx_started,ad_fx_ticks,ad_consecutive;
static volatile uint32_t ad_recover;
static int ad_wire_send(const uint8_t *p,uint32_t n){
#ifndef DINGHY_HOST
 return ota_wire_send(p,n);
#else
 (void)p;(void)n;return 1;
#endif
}
static void ad_publish(void){
 ad_begin_write();
 uint32_t *v=ad_live;
 v[AD_MAGIC_I]=AD_MAGIC;v[AD_SCHEMA]=AD_VERSION;v[AD_BUILD]=AD_BUILD_TAG;v[AD_CONFIG_I]=AD_CONFIG;
 v[AD_READY]=ad_ready;v[AD_FAULT]=panel_fault;v[AD_CASE]=ad_case;v[AD_RUN]=ad_run;v[AD_STATE]=ad_state;
 v[AD_MS]=fm1_ms;v[AD_BLOCKS]=ad_cpu_n;v[AD_CPU_LAST]=cpu_last;v[AD_CPU_PEAK]=cpu_peak;v[AD_CPU_MEAN]=ad_cpu_sum;
 v[AD_LATE]=cpu_late;v[AD_CONSEC]=ad_consecutive;v[AD_SYNTH_LAST]=ad_synth_last;v[AD_SYNTH_PEAK]=ad_synth_peak;v[AD_FX_LAST]=ad_fx_last;v[AD_FX_PEAK]=ad_fx_peak;
 v[AD_PLAY]=a_song_play;v[AD_TICK]=a_song_tick;v[AD_EVENT]=a_song_pos;v[AD_ACTIVE]=a_all_active_peak;v[AD_PEAK]=a_peak;v[AD_CLAMP]=a_clamps;v[AD_QUEUE_DROP]=a_queue_drops;
 v[AD_KEYS]=ad_key_edges;v[AD_MIDI_ON]=ad_midi_on;v[AD_MIDI_OFF]=ad_midi_off;v[AD_MIDI_CC]=ad_midi_cc;v[AD_MIDI_BEND]=ad_midi_bend;v[AD_MIDI_OUT]=ad_midi_out;
#ifndef DINGHY_HOST
 v[AD_MASTER]=(uint32_t)master_q12;v[AD_USB_UP]=usb.up;v[AD_USB_CONFIG]=usb.config;v[AD_USB_RX_ALT]=up.alt;v[AD_USB_TX_ALT]=uac.alt;v[AD_USB_RATE]=uac.r48?48000:44100;
 v[AD_RX_PACKETS]=up.packets;v[AD_RX_FRAMES]=up.frames;v[AD_RX_BAD]=up.malformed;v[AD_RX_STATUS]=up.status_errors;v[AD_RX_LENGTH]=up.length_errors;v[AD_RX_UNDER]=up.underruns;v[AD_RX_OVER]=up.overflows;v[AD_RX_PEAK_L]=up.peak_l;v[AD_RX_PEAK_R]=up.peak_r;
 v[AD_UAC_LOW]=uac.fill_lo==0xffffffffu?0:uac.fill_lo;v[AD_UAC_HIGH]=uac.fill_hi;v[AD_UAC_FILL]=ua_w-ua_r;
 v[AD_USB_ADJ_UP]=uac.adj_up;v[AD_USB_ADJ_DOWN]=uac.adj_down;
 v[AD_RING_PRIME]=uac.r48?UA_PRIME48:UA_PRIME;v[AD_RING_CAPACITY]=UA_N;
 v[AD_TX_PACKETS]=uac_total_pkts;v[AD_TX_FRAMES]=uac_total_frames;v[AD_TX_UNDER]=uac.underruns;v[AD_TX_OVER]=uac.overruns;v[AD_TX_MISSED]=uac.missed;v[AD_TX_STALL]=uac.stalls;
#endif
 v[AD_SHARED_AWAKE]=a_shared_awake;v[AD_FX_SKIP_TRACKS]=a_fx_skipped_tracks;v[AD_FX_SKIP_WET]=a_fx_skipped_wet;v[AD_FX_WET_FRAMES]=a_fx_wet_frames;v[AD_COMP_UPDATES]=a_comp_updates;
 v[AD_PRE_L]=ad_pre_l;v[AD_PRE_R]=ad_pre_r;v[AD_CAPTURE_L]=ad_cap_l;v[AD_CAPTURE_R]=ad_cap_r;v[AD_DAC_L]=ad_dac_l;v[AD_DAC_R]=ad_dac_r;v[AD_ECHO_COUNT]=ad_echo_count;v[AD_RECOVERIES]=ad_recoveries;v[AD_MAIN_PEAK]=ad_main_peak;v[AD_RENDERS]=ad_render_blocks;
 for(uint32_t c=0;c<8;c++){v[AD_NOTES_BASE+c]=ad_note_ons[c];v[AD_OFFS_BASE+c]=ad_note_offs[c];}
 if((int32_t)(fm1_ms-ad_log_at)>=0){
  ad_log_at=fm1_ms+250;uint32_t *p=ad_log[ad_log_w++%AD_POINTS];
  const uint8_t fields[16]={AD_MS,AD_CPU_LAST,AD_CPU_PEAK,AD_SYNTH_LAST,AD_FX_LAST,AD_ACTIVE,AD_PLAY,AD_TICK,AD_EVENT,AD_PRE_L,AD_CAPTURE_L,AD_DAC_L,AD_LATE,AD_QUEUE_DROP,AD_FAULT,AD_STATE};
  for(uint32_t i=0;i<16;i++)p[i]=v[fields[i]];
  if(ad_log_count<AD_POINTS){ad_log_count++;}
  ad_log_total++;
 }
 v[AD_LOG_COUNT]=ad_log_count;v[AD_LOG_TOTAL]=ad_log_total;
 ad_end_write();
}
static void ad_clear(void){
 cpu_last=cpu_peak=cpu_late=panel_fault=ad_consecutive=0;
 ad_cpu_n=ad_cpu_sum=ad_synth_last=ad_synth_peak=ad_fx_last=ad_fx_peak=0;
 ad_pre_l=ad_pre_r=ad_cap_l=ad_cap_r=ad_dac_l=ad_dac_r=ad_render_blocks=0;
 ad_midi_on=ad_midi_off=ad_midi_cc=ad_midi_bend=ad_midi_out=ad_echo_count=0;
 az(ad_note_ons,sizeof ad_note_ons);az(ad_note_offs,sizeof ad_note_offs);
 ad_log_count=ad_log_w=ad_log_total=0;ad_log_at=fm1_ms;
}
/* Pause acknowledged at an audio boundary before expensive main-loop reset. */
static void ad_control(void){
 if(ad_request){ad_pause=1;if(ad_pause_ack){
  ad_ready=0;
  if(ad_request==3){a_transport(0);a_panic(-1,1);a_fx_init();a_qr=a_qw;panel_fault=cpu_late=ad_consecutive=0;}
  else{a_init();a_fx_enabled=1;ad_clear();}
  if(ad_request==1){ad_case=ad_wanted_case;ad_run=ad_wanted_run;ad_state=1;ad_start_ms=fm1_ms;ad_note_stage=0;if(ad_case==32)a_fx_enabled=0;if(ad_case>=32&&ad_case<=34)a_transport(1);}
  else {ad_case=0xffffffffu;ad_state=0;}
  ad_request=0;ad_pause_ack=0;ad_pause=0;ad_ready=1;
 }}
#ifndef DINGHY_HOST
 if(ad_echo_until==1){midi_send(0x90|ad_echo_ch,ad_echo_note,100);ad_midi_out++;ad_echo_until=fm1_ms+200;}
 else if(ad_echo_until&&(int32_t)(fm1_ms-ad_echo_until)>=0){midi_send(0x80|ad_echo_ch,ad_echo_note,0);ad_midi_out++;ad_echo_count++;ad_echo_until=0;}
#endif
}
static void ad_schedule(void){
 if(ad_recover){uint32_t play=ad_recover==2;ad_recover=0;a_qr=a_qw;a_transport(0);a_panic(-1,1);panel_fault=cpu_late=ad_consecutive=0;ad_recoveries++;if(play)a_transport(1);}
 if(ad_state!=1)return;
 uint32_t elapsed=fm1_ms-ad_start_ms;
 if(ad_case>=1&&ad_case<=8){uint32_t c=ad_case-1;
  if(elapsed>=300&&ad_note_stage==0){a_note_on(c,48+c*2,100,3);ad_note_stage=1;}
  if(elapsed>=900&&ad_note_stage==1){a_note_on(c,55+c*2,90,3);ad_note_stage=2;}
  if(elapsed>=2200&&ad_note_stage==2){a_note_off(c,48+c*2,3);a_note_off(c,55+c*2,3);ad_note_stage=3;}
 }
 if(elapsed>=4000){a_transport(0);for(uint32_t c=0;c<8;c++)for(uint32_t j=0;j<2;j++)ap[c].voice[j].gate=0;ad_state=2;}
}

/* SPDX-License-Identifier: GPL-3.0-only; DUB R39, MIDI instrument and four-track sequencer. */
#include <stdint.h>
#include "sample_config.h"
static int d_sample_sysex(const uint8_t *,uint32_t);
#define APP_SYSEX d_sample_sysex
#define DUB_DIAGNOSTICS 1
#define DUB_MULTITRACK_USB 1
#define DUB_RUNTIME_CONTROL 1
#include "runtime_control.h"
#ifndef DINGHY_HOST
#define D_CAPTURE_HOOK(frame) uac_stems_tap(frame)
#endif
#include "audio_capture.h"
#include "diagnostics.h"
#ifndef DINGHY_HOST
static inline uint32_t fm1_ticks(void);
#define D_PROFILE_TICKS() fm1_ticks()
#endif
#include "profile.h"
static void app_idle(void);
static void dub_diag_receive(const uint8_t *p,uint32_t n);
static void dub_diag_audio_isr(uint32_t elapsed,uint32_t frames);
#define APP_NAME "DUB R39"
#define STORE_MAGIC 0x31505544u /* DUP1: stable DUB patch-store format, independent of release */
#define FELUCCA_UAC 1
#define CV_MAX (240u*48u)
#define LCD_IDLE app_idle
#include "platform.c"
#define D_PLATFORM_MEMORY 1 /* platform.c already supplies memcpy/memset */
#include "engine.h"
#include "panel.h"
#include "sample_transfer.h"
static volatile uint32_t dub_ready;
static uint32_t dub_keys,dub_draw_at;
static const uint8_t dub_physical[12]={BTN_FX,BTN_SEL,BTN_ENV,BTN_LFO,BTN_EDIT,BTN_GLO,BTN_HOME,BTN_SAVE,BTN_ARP,BTN_SEQ,BTN_PLAY,BTN_REC};
static void app_audio(int32_t *out,uint32_t n){
 if(!dub_ready)return;
 if(d_sample_audio_gate(out,n,d_sample_audio_streaming()))return;
#ifndef DINGHY_HOST
 if(d_perf_apply_reset()){panel_cpu_peak=panel_late=0;if(d_prof_enabled)d_prof_window_reset(d_frame);}
 uint32_t start=fm1_ticks();
#endif
 #ifndef DINGHY_HOST
 d_capture_active=uac_stems_mode&&uac.feed;d_capture_main_gain=master_q12;
 #endif
 d_render(out,n);
#ifndef DINGHY_HOST
 uint32_t elapsed=fm1_ticks()-start,budget=n*80000u/147u;
 d_perf_record(0,elapsed,n,budget);
 panel_cpu=elapsed*100u/budget;if(panel_cpu>panel_cpu_peak)panel_cpu_peak=panel_cpu;if(elapsed>budget)panel_late++;
#endif
}
static volatile int dub_diag_pending=-1,dub_diag_reset_waiting;
#include "runtime_bridge.h"
#include "profile_bridge.h"
static void dub_diag_receive(const uint8_t *p,uint32_t n){if(dub_profile_receive(p,n)||dub_runtime_receive(p,n))return;int command=d_perf_request(p,n);if(command>=0&&dub_diag_pending<0&&!dub_diag_reset_waiting)dub_diag_pending=command;}
static void dub_diag_audio_isr(uint32_t elapsed,uint32_t frames){if(dub_ready)d_perf_record(1,elapsed,frames,frames*80000u/147u);}
static void dub_diag_service(void){
#ifndef DINGHY_HOST
 int command=dub_diag_pending;
 if(dub_diag_reset_waiting){if(d_perf.reset_pending)return;command=1;}
 if(command<0)return;
 if(command==1&&!dub_diag_reset_waiting){dub_diag_pending=-1;dub_diag_reset_waiting=1;d_perf.reset_pending=1;return;}
 d_perf_snapshot snap;if(!d_perf_copy(&snap))return;
 uint32_t fields[D_PERF_FIELDS]={39,FM1_TICKS_PER_US*1000000u,44100,fm1_ms,snap.generation};
 d_perf_scope_fields(fields+5,&snap.app);d_perf_scope_fields(fields+14,&snap.isr);
 fields[23]=d_clips;fields[24]=d_overflows;fields[25]=d_fx_limits;fields[26]=d_clock_drops;
 fields[27]=uac.underruns;fields[28]=uac.overruns;fields[29]=uac.missed;fields[30]=usb.max_gap;fields[31]=usb.resets;fields[32]=uac.alt;fields[33]=uac.flowing;fields[34]=uac_stems_mode?22050:uac.r48?48000:44100;
 uint8_t reply[D_PERF_REPLY_SIZE];uint32_t length=d_perf_encode(reply,command,fields);
 dub_diag_pending=-1;dub_diag_reset_waiting=0;(void)ota_wire_send(reply,length);
#endif
}
static void dub_midi(void){uint8_t st,a,b;int source;while((source=midi_get(&st,&a,&b)))if(!d_sample_pause)d_send(DC_MIDI,st,a,b,source==1?0:1);while(d_clock_take())midi_send(0xf8,0,0);dub_diag_service();dub_runtime_service();dub_profile_service();d_sample_service();}
static void app_idle(void){if(dub_ready)dub_midi();}
static void dub_inputs(void){
 #ifndef DINGHY_HOST
 if(d_usb_mode!=uac_stems_mode){uint32_t flags=irq_save();usb_detach();uac_stream(0);uac.feed=uac.go=0;usb.config=0;uac_stems_mode=d_usb_mode;mi_r=mi_w;irq_restore(flags);d_send(DC_USB_RELEASE,0,0,0,0);fm1_delay_ms(250);usb_start();usb.detached=0;}
 #endif
 if(d_mix_io){d_mix_result=settings_save(&d_mix_work,sizeof d_mix_work)==0?1:-1;d_mix_io=0;}
 uint32_t released,pressed=fm1_input_edges(&released),keys=fm1_in.notes;
 for(int i=0;i<12;i++){if(pressed&(1u<<dub_physical[i]))panel_button(i,1,fm1_ms);if(released&(1u<<dub_physical[i]))panel_button(i,0,fm1_ms);}
 if(pressed&(1u<<BTN_OCT_DOWN))panel_control(19,-1,0,0);if(pressed&(1u<<BTN_OCT_UP))panel_control(19,1,0,0);
 panel_turn(0,fm1_enc_take(ENC_SELECT));panel_turn(1,fm1_enc_take(ENC_ALGORITHM));for(int k=0;k<4;k++)panel_turn(k+2,fm1_enc_take(ENC_KNOB1+k));panel_turn(6,fm1_enc_take(ENC_PRESETS));
 for(int k=0;k<27;k++){if((keys&~dub_keys)&(1u<<k))panel_key(k,1);if((dub_keys&~keys)&(1u<<k))panel_key(k,0);}
 dub_keys=keys;dub_midi();
 if(ds_io>=0&&!ds_claim){ds_claim=1;ds_ack=slot_save((uint32_t)ds_io,&ds_work,sizeof ds_work)==0?1:-1;}
 if(preset_io>=0)preset_saved(blob_save((uint32_t)preset_io,&preset_work,sizeof preset_work)==0);for(int i=0;i<12;i++)fm1_led_key(dub_physical[i],panel_active(i));
 for(int k=0;k<27;k++)fm1_led_key(KEY_LED(k),panel.key_held[k]);
}
static void dub_rect_hw(int x,int y,int w,int h,uint32_t c){cv_rect(x,y,w,h,RGB(c>>16,(c>>8)&255,c&255));}
static void dub_draw(void){for(int y=0;y<240;y+=48){cv_begin(240,48,C_BLACK);cv_oy=-y;
#ifdef DINGHY_HOST
 panel_render(dub_rect_hw,0);
#else
 panel_render(dub_rect_hw,1);
#endif
 cv_blit(0,y);}cv_oy=0;}
static void app_main(void){d_init();panel_init();if(!settings_load(&d_mix_work,sizeof d_mix_work))d_mix_import();for(int i=0;i<8;i++){int loaded=!blob_load(i,&preset_work,sizeof preset_work);if(!loaded&&!blob_load(i,&preset_legacy_work,sizeof preset_legacy_work)){preset_migrate(&preset_work,&preset_legacy_work);loaded=1;}if(loaded&&preset_valid(&preset_work)&&!preset_taken(preset_work.name,-1))memcpy(&preset_bank[i],&preset_work,sizeof preset_work);}if(preset_taken(preset_name,-1))preset_new_name(preset_name);for(int bank=0;bank<8;bank++)if(!slot_load((uint32_t)bank,&ds_work,sizeof ds_work))ds_import(bank);d_sample_store_init();dub_ready=1;palette_set(4);lcd_fill(0,0,240,240,C_BLACK);for(;;){platform_poll();dub_inputs();if((int32_t)(fm1_ms-dub_draw_at)>=0){dub_draw_at=fm1_ms+100;dub_draw();}}}

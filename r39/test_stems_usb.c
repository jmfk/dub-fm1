/* Actual USB source with memory-backed register HAL; no device APIs. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "audio_capture.h"
#define RING_PUBLISH() __asm__ volatile("":::"memory")
#define FELUCCA_CDC 0
#define FELUCCA_UAC 1
#define FELUCCA_OTA 1
#define HALF_FRAMES 128u
#define DUB_MULTITRACK_USB 1
static uint32_t now;
static uint32_t ota_now_ms(void){return now;}
static void ota_idle(void){now++;}
#include "../../.build-env/dub-r39-usb-under-test.h"
static void prime(void){ua_w=ua_r=0;uac.go=0;uac.alt=1;usb.config=1;uac.flowing=1;uac_render_start();assert(uac.go&&uac.feed&&ua_w==48);ua_r=ua_w;}
int main(void){const uint8_t *d;uint16_t len;assert(get_desc(0x200,&d,&len)&&len==178);uac_stems_mode=1;assert(get_desc(0x200,&d,&len)&&len==175);unsigned pos=0,channels=0,rate=0,maxp=0;while(pos<len){unsigned size=d[pos];assert(size>=2&&pos+size<=len);if(d[pos+1]==0x24&&size==11&&d[pos+2]==2){channels=d[pos+4];rate=d[pos+8]+256*d[pos+9]+65536*d[pos+10];}if(d[pos+1]==5&&d[pos+2]==0x84)maxp=d[pos+4]+256*d[pos+5];pos+=size;}assert(channels==17&&rate==22050&&maxp==816&&sizeof ep4tx>=maxp&&maxp<=1023);
 uac_rate_set(48000);assert(!uac.r48);prime();int16_t frame[17];for(int ch=0;ch<17;ch++)frame[ch]=(ch+1)*600;for(int i=0;i<64;i++)uac_stems_tap(frame);for(int ch=0;ch<17;ch++)assert(ua_storage.stems[(ua_w-1)&127][ch]==frame[ch]);
 uint32_t count=uac_packet(ep4tx);assert(count==22);for(unsigned i=8;i<count;i++)for(int ch=0;ch<17;ch++)assert(((int16_t *)ep4tx)[i*17+ch]==frame[ch]);
 // Exactly22050 frames/sec without fill correction; 21..24 frames with drift.
 uac.go=0;uac.acc=0;unsigned total=0;for(int i=0;i<1000;i++)total+=uac_packet(ep4tx);assert(total==22050);uac.go=1;uac.fill_min=100;assert(uac_packet(ep4tx)<=24);uac.fill_min=1;assert(uac_packet(ep4tx)>=21);
 // All channels filtered alike; Nyquist input is rejected after history settles.
 prime();for(int i=0;i<64;i++){for(int ch=0;ch<17;ch++)frame[ch]=(i&1)?16000:-16000;uac_stems_tap(frame);}for(int ch=0;ch<17;ch++)assert(abs(ua_storage.stems[(ua_w-1)&127][ch])<100);
 d_usb_groups=2;for(int i=0;i<32;i++){for(int ch=0;ch<17;ch++)frame[ch]=4000;uac_stems_tap(frame);}for(int ch=0;ch<17;ch++)assert(ua_storage.stems[(ua_w-1)&127][ch]==(ch>=2&&ch<6?4000:0));
 d_usb_groups=0;prime();for(int i=0;i<64;i++)uac_stems_tap(frame);assert(ua_w-ua_r==32);uac.fill_min=48;uac_stems_packet((int16_t *)ep4tx);for(int i=0;i<22*17;i++)assert(!((int16_t *)ep4tx)[i]);d_usb_groups=7;
 unsigned w=ua_w;uac.feed=0;uac_stems_tap(frame);assert(w==ua_w);uac.feed=1;uac_stems_mode=0;uac_stems_tap(frame);assert(w==ua_w);uac_stems_mode=1;
 prime();uac.fill_min=48;hw_ep[4][17]=0;usb.up=1;uac_service();assert(sent_bytes[4]%34==0&&sent_bytes[4]<=816);
 puts("PASS actual USB17ch descriptor/22050 rate, interleaving, FIR rejection, group silence, idle gating, ring bounds and <=816byte packets; no device access");}

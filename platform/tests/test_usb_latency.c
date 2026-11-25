/* Actual USB capture/ISR under nested 1ms consumption during expensive renders. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RING_PUBLISH() __asm__ volatile("":::"memory")
#define FELUCCA_CDC 0
#define FELUCCA_UAC 1
#define FELUCCA_OTA 0
#define HALF_FRAMES 256u
#define HALF_WORDS 512u
#define FM1_USB_HAL "usb_mock.h"
#define FM1_USB_PLAYBACK 1
#ifdef REFERENCE_R4
#include "usb_r4_reference.c"
#else
#include "usb.c"
#endif
#define UAC_ONLY(x) x
#define FM1_AUDIO_HALF 0x80u
static int32_t abuf[1024],master_q12=4096;
static uint8_t in_audio;
static uint8_t fm1_audio_pending(void){return FM1_AUDIO_HALF;}
static void fm1_audio_ack_aux(uint8_t p){(void)p;}
static uint32_t fm1_audio_free_half(void){return 0;}
static void fm1_audio_ack_half(void){}
static double now_us,next_usb,render_us;
static void consume_until(double target){
 while(next_usb<=target){
  now_us=next_usb;hw_ep[4][17]=0;sent_bytes[4]=0;uac_service();
  if(now_us>500000&&uac.go){
   const uint8_t *p=(const uint8_t*)tx_dma[4];
   for(uint32_t i=0;i<sent_bytes[4]/4;i++){
    int16_t l=(int16_t)(p[4*i]|(uint16_t)p[4*i+1]<<8),r=(int16_t)(p[4*i+2]|(uint16_t)p[4*i+3]<<8);
    assert(abs(l-1000)<=2 && abs(r+2000)<=2);
   }
  }
  next_usb+=1000;
 }
 now_us=target;
}
static void app_audio(int32_t *o,uint32_t n){
 consume_until(now_us+render_us*n/256.0);
 for(uint32_t i=0;i<n;i++){o[2*i]=1000;o[2*i+1]=-2000;}
}
#ifdef REFERENCE_R4
#include "render_irq_r4.h"
#else
#include "render_irq.h"
#endif
static void run(uint32_t rate,double load,int ppm){
 memset(&uac,0,sizeof uac);ua_w=ua_r=0;usb.up=usb.config=1;
 uac_rate_set(rate);uac_stream(1);now_us=next_usb=0;render_us=load;
 double period=256000000.0/(44100.0*(1.0+ppm/1000000.0));uint32_t baseline=0;
 for(uint32_t i=0;i<(uint32_t)(5000000.0/period);i++){
  double start=i*period+(i%17==3?500.0:0.0);
  if(start<now_us)start=now_us;
  consume_until(start);fm1_alnk0_irq();assert(!in_audio);
  if(i==50)baseline=uac.underruns;
 }
 uint32_t errors=uac.underruns-baseline;
#ifdef REFERENCE_R4
 assert(errors>50);
 printf("PASS: reproduced R4 bulk-publication underruns, %uHz %.0fus render, %u gaps\n",rate,load,errors);
#else
 assert(!errors&&!uac.overruns&&!uac.missed);
 printf("PASS: progressive capture %uHz %.0fus/256 frames, drift %+dppm, 500us jitter, no steady-state under/overflow\n",rate,load,ppm);
#endif
}
int main(void){
#ifdef REFERENCE_R4
 run(44100,4000,0);run(48000,4000,0);
#else
 const int drift[]={0,400,-400,1000,-1000};const double load[]={400,2000,4000,5000};
 for(uint32_t r=44100;r<=48000;r+=3900)for(uint32_t d=0;d<5;d++)for(uint32_t l=0;l<4;l++)run(r,load[l],drift[d]);
#endif
 return 0;
}

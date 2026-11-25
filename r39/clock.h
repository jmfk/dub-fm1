/* SPDX-License-Identifier: GPL-3.0-only; sample-clock scheduling, main-loop MIDI output. */
#pragma once
enum {D_CLK_INT,D_CLK_EXT,D_CLK_AUTO};
static volatile int d_clock_mode=D_CLK_AUTO,d_clock_output,d_clock_source=-1,d_external_tempo=112;
static volatile uint32_t d_clock_last,d_clock_period,d_clock_count,d_clock_generation;
static uint32_t d_clock_phase;
static struct {uint32_t frame,generation;} d_clock_queue[64];
static volatile uint32_t d_clock_w,d_clock_r;
static uint32_t d_clock_stamp,d_clock_drops;
static int d_clock_live(void){return d_clock_count>=2&&d_clock_period&&(uint32_t)(d_frame-d_clock_last)<D_FS/2;}
static int d_clock_external(void){return d_clock_mode!=D_CLK_INT&&d_clock_live();}
static int d_clock_tempo(void){return d_clock_mode==D_CLK_EXT||d_clock_external()?d_external_tempo:dg[G_TEMPO];}
static void d_clock_enqueue(void){uint32_t w=d_clock_w;if(w-d_clock_r>=64){d_clock_drops++;return;}d_clock_queue[w&63].frame=d_frame;d_clock_queue[w&63].generation=d_clock_generation;__asm__ volatile("":::"memory");d_clock_w=w+1;}
static void d_clock_set(int field,int value){if(field<0||field>1)return;int next=dc(value,0,field?1:2);if(next==(field?d_clock_output:d_clock_mode))return;if(field)d_clock_output=next;else{d_clock_mode=next;d_clock_count=d_clock_period=0;d_clock_source=-1;d_external_tempo=dg[G_TEMPO];}d_clock_phase=0;d_clock_generation++;}
static void d_clock_receive(int source){
 if(d_clock_mode==D_CLK_INT||source<0||source>1)return;
 if(d_clock_source<0||(uint32_t)(d_frame-d_clock_last)>=D_FS/2){d_clock_source=source;d_clock_count=d_clock_period=0;}
 if(source!=d_clock_source)return;
 int was_live=d_clock_live();uint32_t dt=d_frame-d_clock_last;
 if(d_clock_count&&dt>200u&&dt<10000u){d_clock_period=d_clock_period?(d_clock_period*7+dt)/8:dt;if(d_clock_count<64)d_clock_count++;d_external_tempo=dc((D_FS*60u+d_clock_period*12u)/(d_clock_period*24u),30,240);}
 else {d_clock_count=1;d_clock_period=0;}
 d_clock_last=d_frame;
 if(d_clock_live()){d_seq_clock_pulse();if(!was_live){d_clock_generation++;d_clock_phase=0;}if(d_clock_output)d_clock_enqueue();}
}
static void d_clock_step(void){if(!d_clock_output||d_clock_mode==D_CLK_EXT||d_clock_external())return;d_clock_phase+=(uint32_t)dg[G_TEMPO]*24u;if(d_clock_phase>=D_FS*60u){d_clock_phase-=D_FS*60u;d_clock_enqueue();}}
/* Consumer only. Skip stale/generation-invalid ticks rather than burst after a stall. */
static int d_clock_take(void){while(d_clock_r!=d_clock_w){uint32_t r=d_clock_r;__asm__ volatile("":::"memory");uint32_t frame=d_clock_queue[r&63].frame,generation=d_clock_queue[r&63].generation;d_clock_r=r+1;if(!d_clock_output||generation!=d_clock_generation||(uint32_t)(d_frame-frame)>D_FS/10)continue;d_clock_stamp=frame;return 1;}return 0;}
static void d_clock_init(void){d_clock_mode=D_CLK_AUTO;d_clock_output=0;d_clock_source=-1;d_external_tempo=dg[G_TEMPO];d_clock_last=d_clock_period=d_clock_count=d_clock_phase=d_clock_generation=d_clock_w=d_clock_r=d_clock_stamp=d_clock_drops=0;}

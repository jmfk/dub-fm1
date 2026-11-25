#define A_MATH_CHECK 1
#include "engine.h"
#include <assert.h>
#include <stdio.h>
static uint32_t step(void){int32_t b[64]={0};a_render(b,32);uint32_t peak=0;for(int i=0;i<64;i++){uint32_t x=aa(b[i]);if(x>peak)peak=x;assert(x<=30000);}return peak;}
int main(void){
 a_init();for(int i=0;i<100;i++)assert(step()==0);
 assert(a_fx_skipped_tracks==900 && a_fx_wet_frames==0 && a_comp_updates==0);
 puts("PASS: silent parts/shared FX and idle compressors skip DSP");
 a_init();ag[G_BUSCOMP]=ag[G_DRUMGLUE]=0;ag[G_DELAY]=127;ag[G_DTIME]=127;ag[G_FDBK]=0;
 ap[0].p[P_DLY]=127;ap[0].p[P_REV]=ap[0].p[P_CHO]=0;a_note_on(0,48,100,1);
 for(int i=0;i<8;i++)step();a_panic(0,1);
 uint32_t late=0;for(int i=0;i<2500;i++){uint32_t peak=step();if(i>700&&peak>late)late=peak;}
 assert(late>20 && !a_shared_awake);
 puts("PASS: longest delay's late first echo survives silence; FX sleeps only after tail settles");
 a_init();ag[G_BUSCOMP]=ag[G_DRUMGLUE]=0;ap[0].p[P_INSERT]=6;ap[0].p[P_IWET]=127;
 ap[0].p[P_CHO]=ap[0].p[P_DLY]=ap[0].p[P_REV]=0;a_note_on(0,48,100,1);
 for(int i=0;i<8;i++)step();a_panic(0,1);
 late=0;for(int i=0;i<400;i++){uint32_t peak=step();if(i>8&&peak>late)late=peak;}
 assert(late>20 && !a_tail_left[0]);
 ap[0].p[P_IWET]=0;step();assert(a_insert_mode[0]==0);
 puts("PASS: per-part chorus tail survives note release; zero-wet insert bypass is explicit");
 int32_t env=0,g=32767;
 for(int i=0;i<100;i++)a_comp(30000,100,&env,&g,20,0,40,32);
 assert(g>4095&&g<20000);int32_t low=g;
 for(int i=0;i<1000;i++)a_comp(0,100,&env,&g,20,0,40,32);
 assert(g>low && g>=32760);
 puts("PASS: lower-rate compressor reacts and recovers with bounded, interpolated gain");
 return 0;
}

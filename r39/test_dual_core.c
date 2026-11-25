#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#define D_DEVICE_CORES 1
static uint32_t d_core_state,d_core_jobs,d_core_wait_ticks,d_core_job_ticks,d_core_peak_ticks,d_core_timeouts;
static int mode;static pthread_t thread;static void (*job_fn)(uint32_t);static uint32_t job_arg;
static void *worker(void *unused){(void)unused;sched_yield();unsigned n=job_arg&255u;job_fn(mode==2&&n>1?(job_arg&~255u)|(n/2):job_arg);return 0;}
static int d_core_run(void (*fn)(uint32_t),uint32_t arg){if(!mode||d_core_state!=3)return 0;job_fn=fn;job_arg=arg;assert(!pthread_create(&thread,0,worker,0));return 1;}
static int d_core_wait(void){assert(!pthread_join(thread,0));if(mode==2){d_core_timeouts++;d_core_state=2;return -1;}d_core_jobs++;return 0;}
static uint32_t d_core_ticks(void){return 0;}
static int16_t *capture_ptr;static unsigned capture_count;
static void tap(const int16_t *p){memcpy(capture_ptr+17*capture_count,p,34);capture_count++;}
#define D_CAPTURE_HOOK(p) tap(p)
#include "engine.h"
#define FRAMES 12288
static int32_t pcm[512],audio[3][FRAMES*2];static int16_t stems[3][FRAMES*17];
static void scene(int m){mode=m;d_init();d_core_state=3;d_core_jobs=d_core_timeouts=0;capture_ptr=stems[m];capture_count=0;d_capture_active=1;ds_pattern_at(0)->drums=1;
 for(int t=0;t<3;t++)d_note_on(t,48+t*5,100,0);
 for(unsigned at=0;at<FRAMES;){if(!(at%512)){int notes[]={36,38,42,45,56};for(int k=0;k<5;k++)d_drum_on(notes[k],90+k*5,0);d_pad[2].pan=(at/512)%2?0:127;d_pad[1].send[0]=65;d_pad[1].level=(at/512)%3?110:40;d_mix_dirty=1;}
  unsigned n=at%512==0?1:at%512==1?7:32;if(n>512-at%512)n=512-at%512;if(n>FRAMES-at)n=FRAMES-at;d_render(pcm,n);memcpy(audio[m]+2*at,pcm,n*8);at+=n;}
 assert(capture_count==FRAMES);if(m==1)assert(d_core_jobs>300&&!d_core_timeouts);if(m==2)assert(d_core_timeouts==1&&d_core_state==2);d_full_panic();}
int main(void){for(int m=0;m<3;m++)scene(m);assert(!memcmp(audio[0],audio[1],sizeof audio[0]));assert(!memcmp(audio[0],audio[2],sizeof audio[0]));assert(!memcmp(stems[0],stems[1],sizeof stems[0]));assert(!memcmp(stems[0],stems[2],sizeof stems[0]));puts("PASS12288frames serial/pthread/partial-worker-timeout PCM and17taps bit-identical; gain rollback, single sample advance,1/7/32frame blocks, steals/pan/sends and fallback");}

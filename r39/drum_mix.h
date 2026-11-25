/* Global monitor mixer. Sends: 0 inherits track; 1..128 encodes 0..127. */
#pragma once
typedef struct {uint8_t level,pan,send[3],mute;} d_pad_mix;
static d_pad_mix d_pad[11];
static uint8_t d_mix_dirty;
static uint16_t d_pad_coeff[11][5];
static int32_t d_pad_gain[11],d_pad_step[11];
static int32_t d_drum_raw[11],d_drum_bus[9];
static struct {uint32_t tag;d_pad_mix pad[11];uint16_t sum;} d_mix_work;
static volatile int d_mix_io,d_mix_result;
static uint16_t d_mix_sum(void){uint16_t x=0x6217;const uint8_t *p=(const uint8_t *)d_mix_work.pad;for(unsigned i=0;i<sizeof d_mix_work.pad;i++)x=(uint16_t)(x*33+p[i]);return x;}
static int d_mix_import(void){if(d_mix_work.tag!=0x31584d44u||d_mix_work.sum!=d_mix_sum())return 0;for(int i=0;i<11;i++){d_pad_mix *m=&d_mix_work.pad[i];if(m->level>127||m->pan>127||m->mute>1)return 0;for(int j=0;j<3;j++)if(m->send[j]>128)return 0;}memcpy(d_pad,d_mix_work.pad,sizeof d_pad);d_mix_dirty=1;return 1;}
static void d_mix_stage(void){if(d_mix_io)return;d_mix_work.tag=0x31584d44u;memcpy(d_mix_work.pad,d_pad,sizeof d_pad);d_mix_work.sum=d_mix_sum();d_mix_result=0;d_mix_io=1;}
static void d_mix_init(void){memset(d_pad,0,sizeof d_pad);for(int i=0;i<11;i++){d_pad[i].level=127;d_pad[i].pan=64;d_pad_gain[i]=32768;d_pad_step[i]=0;}d_mix_io=d_mix_result=0;d_mix_dirty=1;}
static int32_t d_pad_ramp(int32_t delta){unsigned shift=d_mod_span==32?5:d_mod_span==16?4:d_mod_span==8?3:2;return delta<0?-((-delta)>>shift):delta>>shift;}
static void d_mix_prepare(void){for(int i=0;i<11;i++){int target=d_pad[i].mute?0:d_pad[i].level*32768/127;d_pad_step[i]=d_pad_ramp(target-d_pad_gain[i]);if(!d_mix_dirty)continue;d_pad_mix *m=&d_pad[i];d_pad_coeff[i][0]=m->pan<=64?32768:(127-m->pan)*32768/63;d_pad_coeff[i][1]=m->pan>=64?32768:m->pan*32768/64;for(int j=0;j<3;j++)d_pad_coeff[i][2+j]=m->send[j]?(m->send[j]-1)*32768/127:0;}d_mix_dirty=0;}

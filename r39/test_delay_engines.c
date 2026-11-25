#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "panel.h"
static int32_t out[512];
static void cfg(int b,int engine,int rate,int bits,int degrade){int v[4]={engine,rate,bits,degrade};for(int f=0;f<4;f++)d_delay_config_set(b,f,v[f]);}
static void quiet(void){d_init();d_cpu_set(3,0);dg[G_DLY_A_RET]=127;dg[G_DLY_B_RET]=dg[G_REV_RET]=0;for(int b=0;b<2;b++){dd[b][D_MODE]=dd[b][D_MOD]=dd[b][D_DRIVE]=dd[b][D_FEEDBACK]=0;dd[b][D_ROUTE]=0;}d_prepare_fx();d_fx_init();}
static uint32_t tail(int degrade){quiet();cfg(0,1,3,0,degrade);dd[0][D_TIME_MS]=60;dd[0][D_FEEDBACK]=70;d_prepare_fx();d_time[0]=d_delay_target(0);uint32_t hash=2166136261u;int32_t send[3]={0},l,r;long long energy=0;
 for(unsigned f=0;f<44100;f++){send[0]=f<1500?d_wave_ram[3][7][(f*7)&255]/4:0;d_fx_tick(0,0,send,&l,&r);hash=(hash^(uint32_t)l)*16777619u;if(f>3000)energy+=da(l);assert(da(l)<65536&&da(r)<65536);}assert(energy>1000);return hash;}
int main(void){quiet();assert(sizeof d_delay_buf==196608&&sizeof d_delay_buf[0][0][0]==2&&DD_N==24576);assert(d_delay_max_ms(0)==4000);
 for(int b=0;b<2;b++)assert(d_delay_shift(b)==2&&d_delay_cells(b)==24576);
 cfg(0,0,0,0,0);assert(d_delay_max_ms(0)==1000);cfg(0,0,1,0,0);assert(d_delay_max_ms(0)==2000);cfg(0,1,3,0,0);assert(d_delay_max_ms(0)==8000);cfg(0,1,4,0,0);assert(d_delay_max_ms(0)==16000);cfg(0,1,5,0,0);assert(d_delay_max_ms(0)==32000);
 // Direct PCM16 storage is exact; reduced bit effect never changes cell count.
 quiet();d_delay_buf[1][1][12]=77;d_delay_valid[1]=5;d_rv_buf[0][0]=1234;cfg(0,1,5,0,0);assert(d_delay_buf[1][1][12]==77&&d_delay_valid[1]==5&&d_rv_buf[0][0]==1234);d_delay_buf[0][0][0]=12345;assert(!d_delay_read(0,0,256));int32_t echo[2];d_delay_step(0,12345,echo);assert(d_delay_buf[0][0][0]==12345);d_delay_step(0,-23456,echo);assert(d_delay_buf[0][0][1]==-23456);
 for(int bits=0;bits<4;bits++){cfg(0,1,5,bits,0);unsigned cap=d_delay_cells(0);d_dpos[0]=d_delay_valid[0]=0;d_delay_step(0,1234,echo);assert(cap==24576&&d_delay_max_ms(0)==32000);assert(d_delay_buf[0][0][0]==(bits==0?1234:bits==1?1232:bits==2?1024:0));}
 d_dpos[0]=DD_N-1;d_delay_step(0,2000,echo);assert(!d_dpos[0]);
 for(int e=0;e<2;e++)for(int rate=0;rate<(e?6:3);rate++)for(int bits=0;bits<4;bits++){
  quiet();cfg(0,e,rate,bits,0);dd[0][D_TIME_MS]=60;d_prepare_fx();d_time[0]=d_delay_target(0);int32_t send[3]={0},l,r;int first=-1;long long energy=0;
  for(unsigned f=0;f<9000;f++){send[0]=f>=128&&f<160?12000:0;d_fx_tick(0,0,send,&l,&r);if(l||r){if(first<0)first=(int)f;energy+=da(l)+da(r);}}
  assert(energy>100&&first>2500&&first<3000);
 }
 quiet();cfg(0,1,5,0,0);dd[0][D_TIME_MS]=32000;d_prepare_fx();d_time[0]=d_delay_target(0);int32_t send[3]={0},l,r;int first=-1;long long energy=0;
 for(unsigned f=0;f<44100u*33u;f++){send[0]=f<64?16000:0;d_fx_tick(0,0,send,&l,&r);if(l||r){if(first<0)first=(int)f;energy+=da(l)+da(r);}}
 assert(first>44100*32-100&&first<44100*32+100&&energy>100);assert(tail(0)!=tail(127));
 quiet();dd[0][D_TIME_MS]=8000;cfg(0,0,0,0,0);d_prepare_fx();assert(dd[0][D_TIME_MS]==8000&&d_delay_target(0)==d_delay_ms_q8(0,1000));d_send(DC_DELAY|256,0,D_TIME_MS,-5,0);d_render(out,32);assert(dd[0][D_TIME_MS]==995);
 cfg(0,1,5,3,99);cfg(1,1,3,0,33);d_bench_command(9,1);for(int f=12;f<20;f++)d_bench_command(f,0);d_bench_command(9,0);assert(d_delay_cfg[0][0]==1&&d_delay_cfg[0][1]==5&&d_delay_cfg[0][2]==3&&d_delay_cfg[0][3]==99&&d_delay_cfg[1][1]==3&&d_delay_cfg[1][3]==33);
 panel_init();panel_nav(2,0);panel_button(6,1,1);panel_button(6,0,2);assert(panel.group==2&&panel.delay_page[0]==1);dub_control c[4];panel_controls(c);assert(c[0].op==DC_DELAY_CFG&&c[3].op==DC_DELAY_CFG);panel_turn(2,-1);d_render(out,32);panel_controls(c);assert(c[2].op==DC_DELAY_CFG&&c[3].op<0);panel_turn(0,1);assert(panel.track==1);panel_button(7,1,3);panel_button(7,0,4);assert(panel.page[2]==1&&!panel.delay_page[1]);panel_button(7,1,5);panel_button(7,0,6);assert(panel.delay_page[1]);panel_controls(c);assert(c[0].source==1);
 puts("PASS direct uncompressed PCM16 exact writes, fixed192KiB/24576-cell capacity independent of bit effect,36 rate/bit responses, real32s echo, optional feedback degradation, preserved times/clamp edit, independent reset and physical/MIDI settings");
}

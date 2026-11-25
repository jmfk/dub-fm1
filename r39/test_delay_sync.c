#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "panel.h"
static int32_t pcm[512];static void tick(void){d_render(pcm,256);}
int main(void){d_init();panel_init();dg[G_TEMPO]=120;d_clock_set(0,D_CLK_INT);
 for(int bus=0;bus<2;bus++){
  panel_nav(2,bus);int ms=dd[bus][D_TIME_MS];dub_control c[4];panel_controls(c);assert(c[0].op==DC_DELAY&&c[0].index==D_TIME_MS&&c[2].op==-7);
  panel_turn(4,1);tick();assert(dd[bus][D_SYNC]==4&&dd[bus][D_TIME_MS]==ms&&d_fxdd[bus][D_SYNC]==4);panel_controls(c);assert(c[0].op==-8&&c[0].value==4&&c[2].value==1);assert(d_delay_target(bus)==500*7056/5);
  panel_turn(2,-2);tick();assert(dd[bus][D_SYNC]==2&&dd[bus][D_TIME_MS]==ms&&d_delay_target(bus)==250*7056/5);
  panel_turn(4,-1);tick();assert(dd[bus][D_SYNC]==10&&!d_fxdd[bus][D_SYNC]);panel_turn(2,1);tick();assert(dd[bus][D_TIME_MS]==ms+5);panel_turn(4,1);tick();assert(dd[bus][D_SYNC]==2&&dd[bus][D_TIME_MS]==ms+5);
  panel_turn(2,6);tick();assert(dd[bus][D_SYNC]==8&&d_delay_sync_ms(8,120)==2000&&d_delay_target(bus)==2000*7056/5);panel_controls(c);char text[24];pvalue(c[0],text);assert(text[0]=='1'&&text[4]==0);
  panel_turn(4,-1);tick();assert(dd[bus][D_SYNC]==16&&!d_fxdd[bus][D_SYNC]);panel.alt=1;panel_turn(5,-4);tick();panel.alt=0;assert(dd[bus][D_SYNC]==4);
 }
 dd[0][D_TIME_MS]=8000;dd[1][D_TIME_MS]=8000;preset_patch p;preset_capture(&p,"DELAY UNITS");assert(preset_valid(&p));dd[0][D_SYNC]=0;preset_apply(&p);assert(dd[0][D_SYNC]==4&&dd[0][D_TIME_MS]==8000&&dd[1][D_TIME_MS]==8000);p.part[0].p[P_LFO_RATE]=0; // unrelated rate remains valid
 dd[0][D_SYNC]=14;preset_capture(&p,"MS REMEMBER");dd[0][D_SYNC]=0;preset_apply(&p);assert(dd[0][D_SYNC]==14&&d_delay_sync_choice(dd[0][D_SYNC])==6);d_prepare_fx();assert(!d_fxdd[0][D_SYNC]);
 dp[0].p[P_LFO_SHAPE]=1;dp[0].lfo[0]=0x80000000u;d_route_set(&dp[0],0,0,23);d_route_set(&dp[0],0,1,127);d_prepare_part(&dp[0]);d_prepare_fx();assert(d_fxdd[0][D_SYNC]==8&&dd[0][D_SYNC]==14);
 for(int code=0;code<=16;code++)for(int bpm=30;bpm<=240;bpm+=7){dd[0][D_SYNC]=code;dg[G_TEMPO]=bpm;d_route_set(&dp[0],0,0,20);d_prepare_part(&dp[0]);d_prepare_fx();assert(d_fxdd[0][D_SYNC]>=0&&d_fxdd[0][D_SYNC]<=8);assert(d_delay_target(0)>=1411&&d_delay_target(0)<=d_delay_ms_q8(0,d_delay_max_ms(0)));}
 puts("PASS both delay TIME modes/physical controls, MS and beat value recall, actual clock-derived targets and selected PCM capacity, ALT division control, preset roundtrip and logical FX sync bounds for every storage code/BPM");
}

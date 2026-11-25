#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "panel.h"
static int32_t out[512];
static void step(unsigned n){d_render(out,n);}
static unsigned sound_hash(int legacy){d_init();d_cpu_set(2,0);for(int i=0;i<8;i++)for(int e=0;e<4;e++)dp[0].cfg[i].env[e]=legacy?127:0;if(legacy)for(int i=0;i<8;i++){dp[0].env_target[i]=2;dp[0].env_amount[i]=-127;}d_note_on(0,48,100,0);unsigned h=2166136261u;for(int b=0;b<20;b++){step(256);for(int i=0;i<512;i++)h=(h^(unsigned)out[i])*16777619u;}return h;}
int main(void){assert(sizeof(d_osc)<=16);assert(sound_hash(0)==sound_hash(1));d_init();panel_init();d_cpu_set(2,0);d_note_on(0,48,100,0);d_note_on(0,55,100,0);step(256);int gain=dp[0].osc[0].gain;assert(gain>0&&d_osc_running(&dp[0],0)&&d_osc_running(&dp[0],2));step(256);assert(dp[0].osc[0].gain==gain);
 d_note_off(0,48,0);assert(!d_osc_running(&dp[0],0)&&d_osc_running(&dp[0],2));step(32);d_note_off(0,55,0);assert(!d_osc_running(&dp[0],0)&&d_osc_running(&dp[0],2));for(int i=0;i<180;i++)step(256);assert(!d_active);
 // Notes released on the same sample form one shared release tail.
 d_note_on(0,48,100,0);d_note_on(0,55,100,0);step(32);d_note_off(0,48,0);d_note_off(0,55,0);assert(d_osc_running(&dp[0],0)&&d_osc_running(&dp[0],2));assert(dp[0].post_env.stage==4);step(32);d_note_on(0,60,100,0);assert(!d_release[0].tail);d_full_panic();assert(!d_active&&!d_release[0].tail);
 // Sustain retains gates and releases the whole chord together on pedal-up.
 d_note_on(0,48,100,0);d_note_on(0,55,100,0);d_midi(0xb0,64,127,0);step(32);d_note_off(0,48,0);d_note_off(0,55,0);assert(dp[0].slot[0].gate&&dp[0].slot[1].gate);step(32);d_midi(0xb0,64,0,0);assert(dp[0].post_env.stage==4&&d_osc_running(&dp[0],0)&&d_osc_running(&dp[0],2));d_full_panic();
 // Disabled OSC ENV IDs/amp destinations fail closed; shared routes work.
 d_route_set(&dp[0],3,0,0);assert(d_route_target(&dp[0],3)==20&&d_route_amount(&dp[0],3)==0);d_route_set(&dp[0],0,0,4);assert(d_route_target(&dp[0],0)==20);d_route_set(&dp[0],11,0,2);assert(d_route_target(&dp[0],11)==2);
 panel_nav(1,0);assert(d_page_count(1)==2);dub_control c[4];panel_controls(c);assert(c[0].source==8);panel_nav(1,1);panel_controls(c);assert(c[0].source==9);panel_nav(7,0);panel_turn(2,3);assert(panel.matrix_source==11);panel_turn(2,1);assert(panel.matrix_source==12);panel_turn(2,1);assert(!panel.matrix_source);assert(panel_key_source(6)==11&&panel_key_source(7)==12&&panel_key_source(9)==-1);assert(d_target_step(3,1)==12&&d_target_step(12,-1)==3);
 puts("PASS no runtime OSC envelopes, static gain/legacy-inert PCM, per-note gates, same-frame chord release, no old-note resurrection, shared release/sustain/retrigger/panic and five-source UI");}

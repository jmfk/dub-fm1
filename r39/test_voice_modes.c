#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "panel.h"
static int32_t out[512];static void tick(void){d_render(out,256);}static int frames;
static void rect(int x,int y,int w,int h,uint32_t c){(void)c;assert(x>=0&&y>=0&&x+w<=240&&y+h<=240);if(y==95&&h==1&&w>1)frames++;}
int main(void){
 for(int mode=0;mode<3;mode++){
  d_init();panel_init();d_send(DC_VOICE,0,0,mode,0);tick();int voices=1<<mode,width=8/voices;
  for(int j=0;j<voices;j++){d_midi(0x90,60+j*3,100,j%2);tick();assert(d_active==(j+1)*width);for(int i=0;i<(j+1)*width;i++)assert(dp[0].osc[i].slot==i/width);}
  assert(dp[0].filter_triggers==1);d_midi(0x90,84,90,2);tick();assert(dp[0].slot[0].note==84&&d_active==8&&dp[0].filter_triggers==1);
  d_note_off(0,60,0);tick();assert(dp[0].slot[0].gate&&dp[0].slot[0].note==84); // stolen note-off cannot stop its replacement
  for(int j=0;j<voices;j++)d_note_off(0,60+j*3,j%2);d_note_off(0,84,2);tick();assert(!d_gates(&dp[0])&&dp[0].filter_env.stage==4);
  panel_nav(8,0);frames=0;panel_render(rect,0);assert(frames==voices);
  for(int i=0;i<1000;i++)tick();assert(!d_active);frames=0;panel_render(rect,0);assert(frames==voices);
 }
 d_init();panel_init();d_note_on(0,60,100,0);d_note_on(1,64,100,0);d_midi(0xb0,64,127,0);d_osc_config saved[8];memcpy(saved,dp[0].cfg,sizeof saved);int16_t oldp[P_COUNT];memcpy(oldp,dp[0].p,sizeof oldp);
 d_delay_buf[0][0][20]=d_delay_encode(10000);d_send(DC_VOICE,0,0,0,0);tick();assert(!d_gates(&dp[0])&&!dp[0].pedal[0]&&dp[0].voice_mode==0&&d_gates(&dp[1])&&dp[1].voice_mode==2);assert(!memcmp(saved,dp[0].cfg,sizeof saved)&&!memcmp(oldp,dp[0].p,sizeof oldp));assert(d_delay_buf[0][0][20]);
 for(int g=0;g<9;g++)if(g==0||g==1||g==5||g==7||g==8){panel_nav(g,0);panel.shift=1;const char *labels[12];panel_labels(labels);assert(!strcmp(labels[6],"VOICE")&&panel_enabled(6));panel_button(6,1,1);assert(panel.group==8);panel.shift=0;panel_turn(2,2);tick();assert(dp[0].voice_mode==2);panel_turn(2,-1);tick();assert(dp[0].voice_mode==1);panel.alt=1;panel_button(4,1,1);panel.alt=0;if(g!=8)assert(panel.group==g);}
 for(int t=0;t<4;t++)dp[t].voice_mode=t%3;preset_patch p;preset_capture(&p,"VOICE MODES");assert(preset_valid(&p));for(int t=0;t<4;t++)dp[t].voice_mode=2;preset_apply(&p);for(int t=0;t<4;t++)assert(dp[t].voice_mode==t%3);
 p.pad[0]=3;assert(!preset_valid(&p));preset_defaults(&p);preset_copy_name(p.name,"DEFAULT");assert(preset_valid(&p));preset_apply(&p);for(int t=0;t<4;t++)assert(dp[t].voice_mode==2);
 dp[0].voice_mode=0;preset_copy_name(preset_draft,"MONO PATCH");preset_request(PS_SAVE,0);tick();preset_saved(1);dp[0].voice_mode=1;preset_request(PS_PREVIEW,0);tick();assert(dp[0].voice_mode==0&&preset_preview);preset_request(PS_RESTORE,0);tick();assert(dp[0].voice_mode==1&&!preset_preview);
 panel_nav(3,0);panel.shift=1;panel_button(6,1,1);assert(panel.group==3&&panel.track==0);panel.shift=0;panel_nav(6,1);panel_preset_page(1);panel.shift=1;panel_button(6,1,1);assert(panel.group==6&&panel.preset_mode==2);
 puts("PASS fixed1x8/2x4/4x2 actual oscillator counts, independent groups, oldest steal/captured note-offs, shared legato envelopes, release/idle frames, track-only mode reset/config preservation/FX tails, SHIFT HOME physical page/back, all-track preset/preview/default/invalid codes and mixer/name-editor exceptions");
}

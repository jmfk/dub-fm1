#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "panel.h"
static int32_t pcm[512];static void tick(void){d_render(pcm,256);}static void silent(void){tick();for(int i=0;i<512;i++)assert(!pcm[i]);assert(!d_active);}
static void seed(void){for(int p=0;p<4;p++){d_note_on(p,48+p*4,100,0);d_midi(0xb0|p,64,127,0);dp[p].bend=4096;dp[p].expression=1234;}tick();d_delay_buf[0][0][0]=d_delay_encode(20000);d_rv_buf[3][10]=19000;d_predelay[11]=8000;d_diff[0][2]=7000;d_down[1][0]=6000;d_up[0][0][1]=5000;d_dry[0][1]=4000;}
int main(void){
 for(int group=0;group<D_GROUPS;group++)for(int mods=0;mods<4;mods++){
  d_init();panel_init();panel_nav(group,0);panel.shift=mods&1;panel.alt=mods>>1;seed();preset_patch before;preset_capture(&before,preset_name);int oldgroup=panel.group,oldpage=panel.page[group];panel_button(11,1,1);silent();assert(panel.group==oldgroup&&panel.page[group]==oldpage&&panel_enabled(11));
  for(int p=0;p<4;p++){assert(!d_gates(&dp[p])&&!dp[p].pedal[0]&&!dp[p].bend&&dp[p].expression==32767);for(int o=0;o<8;o++)assert(!dp[p].osc[o].env.stage&&dp[p].osc[o].slot<0);}
  for(unsigned i=0;i<sizeof d_delay_buf;i++)assert(!((uint8_t *)d_delay_buf)[i]);for(unsigned i=0;i<sizeof d_rv_buf/sizeof(int16_t);i++)assert(!((int16_t *)d_rv_buf)[i]);assert(!d_predelay[11]&&!d_diff[0][2]);
  preset_patch after;preset_capture(&after,preset_name);assert(!memcmp(&before,&after,sizeof before));for(int i=0;i<8;i++)silent();panel_button(11,0,2);
 }
 d_init();panel_init();seed();for(int i=0;i<128;i++)assert(d_send(DC_NOTE,0,60,100,0));assert(d_send(DC_PANIC,0,0,0,0));silent();assert(d_qr==d_qw);for(int i=0;i<32;i++)silent();d_send(DC_NOTE,0,60,100,0);tick();assert(d_active);
 d_init();panel_init();preset_copy_name(preset_draft,"PREVIEW");dp[0].p[P_CUT]=20;preset_request(PS_SAVE,0);tick();preset_saved(1);dp[0].p[P_CUT]=99;preset_listen=1;preset_request(PS_PREVIEW,0);tick();assert(preset_preview&&dp[0].p[P_CUT]==20);panel_nav(6,0);panel.preset_mode=1;panel_button(11,1,1);silent();assert(!preset_preview&&!preset_listen&&dp[0].p[P_CUT]==99&&panel.group==6&&panel.preset_mode==1);
 preset_copy_name(preset_draft,"PENDING");preset_request(PS_SAVE,1);assert(preset_busy);panel_button(11,1,1);silent();assert(!preset_busy&&preset_io<0&&!preset_bank[1].tag);
 preset_copy_name(preset_draft,"STAGED SAVE");preset_request(PS_SAVE,1);tick();assert(preset_io==1);panel_button(11,1,1);silent();assert(preset_io==1);preset_saved(1);assert(preset_bank[1].tag==PRESET_TAG);
 preset_request(PS_PREVIEW,0);panel_button(11,1,1);silent();assert(!preset_preview&&!preset_listen&&dp[0].p[P_CUT]==99);
 panel_nav(3,0);panel.alt=1;panel_button(4,1,1);assert(panel.group==6); /* MIX remembers its origin */
 panel_nav(7,0);panel.alt=0;panel.shift=1;panel_key(0,1);panel_button(11,1,1);silent();assert(!panel.shortcut_held[0]&&!panel.key_held[0]);
 puts("PASS REC panic every view/modifier: exact next-block silence, all voices/sustain/FX cleared, settings preserved, full-queue priority/no resurrection, fresh notes, preview restore/cancel, pending-save cancellation and ownership cleanup");
}

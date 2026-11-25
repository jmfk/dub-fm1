#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
static int16_t captured[17];static unsigned calls;
static void tap(const int16_t *p){memcpy(captured,p,34);calls++;}
#define D_CAPTURE_HOOK(frame) tap(frame)
#include "engine.h"
#include "panel.h"
static int32_t out[512];
static void hit(void){d_drum_clear();d_drum_on(36,100,0);}
int main(void){d_init();panel_init();d_render(out,256);assert(!calls);d_capture_active=1;d_render(out,256);assert(calls==256);
 ds_pattern_at(0)->drums=1;hit();d_render(out,64);int16_t raw=captured[6];assert(raw!=0&&captured[5]!=0);for(int i=7;i<17;i++)assert(!captured[i]);
 // Pad mute and parent level do not affect isolated drum PCM.
 d_pad[0].mute=1;dp[3].p[P_LEVEL]=0;hit();d_render(out,64);assert(captured[6]==raw);d_render(out,64);assert(captured[5]==0);
 // Track4 stem is the pad mix, before parent track volume/mute.
 d_pad[0].mute=0;d_pad[0].level=127;hit();d_render(out,128);assert(captured[5]!=0&&captured[6]!=0);
 d_usb_groups=1;hit();d_render(out,64);for(int i=2;i<17;i++)assert(!captured[i]);
 d_usb_groups=6;hit();d_render(out,64);assert(!captured[0]&&!captured[1]&&captured[6]);
 d_capture_active=0;unsigned before=calls;d_render(out,256);assert(calls==before);
 d_send(DC_DRUM_MIX,0,2,80,0);d_render(out,32);assert(d_pad[0].send[0]==80);d_send(DC_DRUM_SAVE,0,0,0,0);d_render(out,32);assert(d_mix_io);d_pad[0].send[0]=0;assert(d_mix_import()&&d_pad[0].send[0]==80);d_mix_work.sum++;assert(!d_mix_import());
 // Physical USB and mixer navigation + controls.
 panel_nav(4,0);panel_button(5,1,1);panel_button(7,1,2);panel_button(7,0,3);panel_button(5,0,4);assert(panel.group==10);panel_turn(2,1);panel_turn(3,-1);d_render(out,32);assert(d_usb_mode&&!(d_usb_groups&1));panel_turn(3,1);d_render(out,32);assert(d_usb_groups&1);
 panel_nav(3,0);panel_button(5,1,1);panel_button(9,1,2);panel_button(9,0,3);panel_button(5,0,4);assert(panel.group==11&&panel.track==3);panel_turn(1,10);assert(panel.drum_pad==10);panel_control(19,1,0,0);assert(panel.drum_pad==0);d_mix_io=0;panel_turn(2,-7);d_render(out,32);assert(d_pad[0].level==120);
 d_send(DC_NOTE,0,48,100,0);d_send(DC_NOTE,0,52,100,1);d_send(DC_NOTE,0,55,100,2);d_send(DC_USB_RELEASE,0,0,0,0);d_render(out,32);assert(!dp[0].held[0][48]&&dp[0].held[1][52]&&dp[0].held[2][55]);
 puts("PASS17 taps, group gates, track4 drum mix, pre-mix drum isolation, mixer validation and physical controls; no device access");return 0;}

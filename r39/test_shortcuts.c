#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "panel.h"
static int32_t pcm[512];static void tick(void){d_render(pcm,256);}
int main(void){d_init();panel_init();
 for(int view=0;view<2;view++){
  panel_nav(7,view);int tr=panel.track,oct=panel.octave,osc=panel.osc;panel.matrix_source=0;dp[tr].p[P_LFO_TARGET]=0;
  panel.shift=1;panel_control(19,-1,0,0);assert(panel.matrix_source==12);panel_control(19,1,0,0);assert(panel.matrix_source==0);
  panel.shift=0;panel.alt=1;panel_control(19,1,0,0);tick();assert(d_route_target(&dp[tr],0)==1);panel_control(19,-1,0,0);tick();assert(d_route_target(&dp[tr],0)==0);
  panel.shift=1;panel_control(19,1,0,0);tick();assert(panel.matrix_source==0&&d_route_target(&dp[tr],0)==1);panel.alt=0;
  for(int src=0;src<13;src++){int key=d_mod_source_keys[src];uint32_t before=d_qw;panel_key(key,1);assert(panel.matrix_source==src&&d_qw==before&&!panel.key_held[key]&&panel.shortcut_held[key]);panel.shift=0;panel_key(key,0);assert(!panel.shortcut_held[key]&&d_qw==before);panel.shift=1;}
  int src=panel.matrix_source;panel_key(1,1);panel_key(1,0);assert(panel.matrix_source==src&&!d_gates(&dp[tr]));
  assert(panel.track==tr&&panel.octave==oct&&panel.osc==osc);
  panel.shift=0;for(int b=6;b<=8;b++){panel_button(b,1,1);tick();assert(d_route_target(&dp[tr],src)==(b==6?22:b==7?32:41));}panel.shift=1;for(int b=7;b<=8;b++){panel_button(b,1,1);tick();assert(d_route_target(&dp[tr],src)==47+b-6);}
 }
 panel.shift=0;panel_nav(7,0);panel_key(0,1);tick();assert(d_gates(&dp[panel.track]));panel.shift=1;panel_key(0,0);tick();assert(!d_gates(&dp[panel.track]));
 panel_key(2,1);panel.shift=0;panel_nav(2,0);panel_key(2,0);tick();assert(!panel.shortcut_held[2]&&!d_gates(&dp[panel.track]));
 panel_nav(7,1);panel.shift=1;panel_key(4,1);panel_control(20,0,0,0);assert(!panel.shortcut_held[4]&&!panel.shift);
 panel_nav(0,0);panel.alt=0;panel.shift=1;int tr=panel.track;panel_control(19,1,0,0);assert(panel.track==(tr+1)%4);panel.shift=0;int oct=panel.octave;panel_control(19,1,0,0);assert(panel.octave==oct+1);
 puts("PASS MOD list/matrix modifier OCT navigation, precedence, all13 source keys, silent unassigned keys, modifier/view/blur releases, existing musical ownership, visible FX destinations and unchanged outside navigation");
}

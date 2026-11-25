#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "panel.h"
static uint32_t pixels[240][240];static int32_t pcm[512];
static void rect(int x,int y,int w,int h,uint32_t c){assert(x>=0&&y>=0&&w>=0&&h>=0&&x+w<=240&&y+h<=240);for(int j=y;j<y+h;j++)for(int i=x;i<x+w;i++)pixels[j][i]=c;}
static void tap(int i){panel_button(i,1,1);panel_button(i,0,2);}
int main(void){d_init();panel_init();
 panel_nav(1,1);panel_button(5,1,1);tap(0);panel_button(5,0,2);assert(panel.group==3);
 for(int t=0;t<4;t++){
  tap(6+t);assert(panel.track==t);dub_control c[4];panel_controls(c);int ids[4]={P_LEVEL,P_DLY_A,P_DLY_B,P_REVERB};
  for(int k=0;k<4;k++){assert(c[k].index==ids[k]);int before=dp[t].p[ids[k]];panel_turn(k+2,-1);d_render(pcm,256);assert(dp[t].p[ids[k]]==(before?before-1:0));}
  panel_render(rect,0);
 }
 panel.alt=1;tap(4);panel.alt=0;assert(panel.group==1&&panel.page[1]==1);
 panel_button(10,1,1);assert(panel.alt);int tr=panel.track,oct=panel.octave;panel_turn(0,1);panel_control(19,1,0,0);assert(panel.osc==2&&panel.track==tr&&panel.octave==oct);panel_button(10,0,2);assert(!panel.alt&&!panel.playing);
 for(int g=0;g<D_GROUPS;g++)for(int page=0;page<d_page_count(g);page++){
  panel_nav(g,page);for(int layer=0;layer<4;layer++){panel.shift=layer&1;panel.alt=layer>>1;panel_render(rect,0);dub_control c[4];panel_controls(c);}
 }
 panel.shift=panel.alt=0;panel_nav(2,0);tap(9);assert(panel.page[2]==3);dub_control c[4];panel_controls(c);assert(c[0].op==DC_GLOBAL);
 panel_nav(4,0);tap(8);panel_controls(c);assert(c[0].index==P_PAN&&c[1].index==P_DRIVE&&c[2].index==P_MUTE);
 panel_button(10,1,1);panel_control(20,0,0,0);assert(!panel.alt);
 puts("PASS mixer channel shortcuts, volume/sends per channel, REC back context, PLAY hold ALT/release, oscillator navigation, all menu/layer screen bounds, preserved returns/pan/drive/mute");
}

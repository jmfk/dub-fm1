#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "panel.h"
static uint32_t pixels[240][240];
static int groups;
static void rect(int x,int y,int w,int h,uint32_t c){assert(x>=0&&y>=0&&x+w<=240&&y+h<=240);if(y==95&&h==1&&w>1)groups++;for(int j=y;j<y+h;j++)for(int i=x;i<x+w;i++)pixels[j][i]=c;}
static int32_t pcm[512];
int main(void){
 d_init();panel_init();panel_render(rect,0);assert(groups==4);
 for(int i=0;i<8;i++)assert(pixels[99][12+i*28]==UC_BG);
 for(int n=1;n<=4;n++){
  d_note_on(0,48+n*4,100,0);d_render(pcm,256);groups=0;panel_render(rect,0);assert(groups==4);
  for(int i=0;i<n*2;i++)assert(pixels[99][12+i*28]!=UC_BG);
 }
 for(int n=1;n<=4;n++)d_note_off(0,48+n*4,0);
 d_render(pcm,256);groups=0;panel_render(rect,0);assert(groups==4); /* release stays visible */
 for(int i=0;i<1000;i++)d_render(pcm,256);
 panel_render(rect,0);for(int i=0;i<8;i++)assert(pixels[99][12+i*28]==UC_BG);
 panel_nav(0,0);panel_render(rect,0);assert(pixels[151][76]==UC_TILE&&pixels[173][76]==UC_TILE);
 d_init();panel_init();d_note_on(0,60,100,0);d_note_on(0,64,100,0);d_render(pcm,256);panel.osc=4;panel_render(rect,0);assert(pixels[95][6]==UC_DIM&&pixels[95][118]==UC_AMBER);
 panel_nav(0,1);panel_render(rect,0);assert(pixels[151][76]==UC_CYAN&&pixels[173][76]==UC_TILE);
 panel_nav(1,1);panel_render(rect,0);assert(pixels[151][76]==UC_CYAN);
 panel_nav(1,2);panel_render(rect,0);assert(pixels[173][76]==UC_CYAN&&pixels[151][76]==UC_TILE);
 panel_nav(5,0);dp[0].p[P_LFO_TARGET]=2;panel_render(rect,0);assert(pixels[173][76]==UC_CYAN&&pixels[151][76]==UC_TILE);
 dp[0].env_target[8]=4;panel_nav(1,1);panel_render(rect,0);assert(pixels[119][20]==UC_CYAN&&pixels[151][76]==UC_TILE);
 panel_nav(7,1);panel.matrix_source=11;panel_render(rect,0);assert(pixels[98+11*7][43+4*9]==UC_AMBER);
 int tr=panel.track,oct=panel.octave;panel.alt=1;
 for(int group=0;group<6;group++){
  panel_nav(group,0);panel.osc=7;panel_turn(0,1);assert(panel.osc==0);panel_control(19,-1,0,0);assert(panel.osc==7);
  panel.shift=1;panel_control(19,1,0,0);assert(panel.osc==0);panel.shift=0;
  assert(panel.track==tr&&panel.octave==oct);
 }
 panel.alt=0;panel_turn(0,1);assert(panel.track==1);panel_control(19,1,0,0);assert(panel.octave==1);
 puts("PASS ALT SELECT/OCT wraps eight oscillators in all menus, preserves track/octave, fixed four-group allocation frames, activity/release/idle fills");
}

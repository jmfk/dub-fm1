#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "panel.h"
static int32_t pcm[512];static void advance(uint32_t n){while(n){uint32_t k=n>256?256:n;d_render(pcm,k);n-=k;}}
int main(void){d_init();panel_init();static const uint8_t num[16]={8,4,2,3,4,1,3,2,1,3,1,1,1,1,1,1},den[16]={1,1,1,2,3,1,4,3,2,8,3,4,6,8,12,16};d_clock_set(0,D_CLK_INT);
 for(int bpm=30;bpm<=240;bpm++){dg[G_TEMPO]=bpm;uint32_t previous=0;for(int div=0;div<16;div++){int code=D_LFO_INIT|D_LFO_SYNC|(div<<7)|64;uint32_t actual=d_lfo_increment(code);uint64_t denominator=60ull*D_FS*num[div],numerator=(1ull<<32)*bpm*den[div];uint32_t expected=(uint32_t)((numerator+denominator/2)/denominator);assert(actual+1>=expected&&actual<=expected+1&&actual>=previous);previous=actual;}}
 for(int track=0;track<4;track++)for(int lfo=0;lfo<3;lfo++){
  panel.track=track;panel_nav(5,lfo);int id=d_lfo_ids[lfo][0],free=dp[track].p[id]&127;
  panel.shift=1;panel_turn(2,1);advance(256);int code=dp[track].p[id];assert((code&D_LFO_SYNC)&&((code>>7)&15)==5&&(code&127)==free);
  panel.shift=0;panel_turn(2,2);advance(256);assert(((dp[track].p[id]>>7)&15)==7);panel.shift=1;panel_turn(2,-1);advance(256);assert(!(dp[track].p[id]&D_LFO_SYNC)&&(dp[track].p[id]&127)==free);
  panel.shift=0;panel_turn(2,7);advance(256);panel.shift=1;panel_turn(2,1);advance(256);assert(((dp[track].p[id]>>7)&15)==7&&(dp[track].p[id]&127)==free+7);
 }
 int code=D_LFO_SYNC|D_LFO_INIT|(5<<7)|32;dg[G_TEMPO]=240;d_clock_set(0,D_CLK_EXT);d_midi(0xf8,0,0,0);advance(919);d_midi(0xf8,0,0,0);assert(d_clock_tempo()==120);uint32_t ext=d_lfo_increment(code),free=d_lfo_increment(32);assert(ext>free);
 d_clock_set(0,D_CLK_AUTO);d_midi(0xf8,0,0,0);advance(919);d_midi(0xf8,0,0,0);assert(d_lfo_increment(code)==ext);advance(22051);uint32_t internal=d_lfo_increment(code);assert(internal+1>=ext*2&&internal<=ext*2+1&&d_lfo_increment(32)==free);
 preset_patch p;preset_capture(&p,"SYNC TEST");assert(preset_valid(&p));int saved=dp[2].p[P_LFO2_RATE];dp[2].p[P_LFO2_RATE]=0;preset_apply(&p);assert(dp[2].p[P_LFO2_RATE]==saved);p.part[0].p[P_LFO_RATE]=8192;assert(!preset_valid(&p));
 d_send(DC_PARAM,0,P_LFO_RATE,99999,0);advance(256);assert(dp[0].p[P_LFO_RATE]==8191);d_send(DC_PARAM,0,P_LFO_RATE,-1,0);advance(256);assert(!dp[0].p[P_LFO_RATE]);
 d_init();dg[G_TEMPO]=120;dp[0].p[P_LFO_RATE]=code;uint32_t step=d_lfo_increment(code);advance(256);assert(dp[0].lfo[0]==step*256u&&dp[0].lfo[1]==d_lfo_inc[48]*256u);
 puts("PASS all16 beat divisions at30–240BPM within one phase bit, monotonic rate, all12 independent mode controls/value memory, INT/EXT/AUTO follow/fallback, free-Hz independence, patch roundtrip/validation/clamps and actual phase progression");
}

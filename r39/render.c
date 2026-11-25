/* SPDX-License-Identifier: GPL-3.0-only; offline MIDI stimulus -> WAV. */
#include <stdio.h>
#include <time.h>
#include "engine.h"
static void u16(FILE *f,uint32_t v){fputc(v&255,f);fputc((v>>8)&255,f);}static void u32(FILE *f,uint32_t v){u16(f,v);u16(f,v>>16);}
int main(int argc,char **argv){
 const uint32_t frames=44100u*12u;FILE *f=fopen(argc>1?argv[1]:"demo.wav","wb");if(!f)return 1;
 fwrite("RIFF",1,4,f);u32(f,36+frames*4);fwrite("WAVEfmt ",1,8,f);u32(f,16);u16(f,1);u16(f,2);u32(f,44100);u32(f,176400);u16(f,4);u16(f,16);fwrite("data",1,4,f);u32(f,frames*4);
 d_init();d_cpu_set(3,1);for(int p=0;p<4;p++){
  dp[p].p[P_MODE]=p==0?1:0;dp[p].p[P_CUT]=60+p*7;dp[p].p[P_DLY_B]=24;dp[p].p[P_PAN]=32+p*21;
  for(int i=0;i<8;i++)dp[p].cfg[i].wave=p;
 }
 int32_t out[512];uint32_t peak=0,active=0;clock_t start=clock();
 static const int chords[4][4]={{48,55,58,63},{53,60,63,67},{46,53,56,60},{48,55,58,62}};
 for(uint32_t pos=0;pos<frames;){
  uint32_t n=frames-pos<256?frames-pos:256;
  for(uint32_t i=0;i<n;i++){
   uint32_t at=pos+i,sec=at/44100u,part=sec<4?sec:0;
   if(at%44100u==0&&sec<4)for(int k=0;k<4;k++)d_send(DC_MIDI,0x90|part,chords[part][k],85,0);
   if(at%44100u==22050&&sec<4)for(int k=0;k<4;k++)d_send(DC_MIDI,0x80|part,chords[part][k],0,0);
   if(at==44100u*4u)for(int p=0;p<4;p++)for(int k=0;k<4;k++)d_send(DC_MIDI,0x90|p,chords[p][k],78,0);
   if(at==44100u*6u)for(int p=0;p<4;p++)d_send(DC_MIDI,0xb0|p,123,0,0);
  }
  d_render(out,n);if(d_active>active)active=d_active;
  for(uint32_t i=0;i<n*2;i++){uint32_t a=(uint32_t)da(out[i]);if(a>peak)peak=a;u16(f,(uint16_t)out[i]);}pos+=n;
 }
 fclose(f);printf("{\"frames\":%u,\"seconds\":12,\"peak\":%u,\"output_clip_samples\":%u,\"fx_safety_clamps\":%u,\"max_active_oscillators\":%u,\"host_render_seconds\":%.4f,\"hardware_cpu_measured\":false}\n",frames,peak,d_clips,d_fx_limits,active,(double)(clock()-start)/CLOCKS_PER_SEC);return peak&&active==32?0:1;
}

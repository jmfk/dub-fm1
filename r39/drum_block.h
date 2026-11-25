/* SPDX-License-Identifier: GPL-3.0-only; active-pad block mixer.
 * Flash PCM preparation stays outside the call-free SRAM mixer. Raw samples
 * share the existing drum-stem columns; the final USB pass applies group masks. */
#pragma once
static uint8_t d_drum_sound;
static D_SAMPLE_RAM void d_drum_mix_block(d_part *t,unsigned n,const uint8_t *pads,unsigned count,int32_t mix[][5],int16_t raw[][15]){
 int32_t buses[32][9];
 /* Pad-major: gain/step, pan and send coefficients are read once per pad. */
 for(unsigned a=0;a<count;a++){
  unsigned k=pads[a];int32_t gain=d_pad_gain[k],step=d_pad_step[k];
  unsigned gl=d_pad_coeff[k][0],gr=d_pad_coeff[k][1],sa=d_pad_coeff[k][2],sb=d_pad_coeff[k][3],sr=d_pad_coeff[k][4];
  unsigned inherit=(!d_pad[k].send[0])|((!d_pad[k].send[1])<<1)|((!d_pad[k].send[2])<<2);
  for(unsigned f=0;f<n;f++){
   int32_t x=raw[f][4+k];gain+=step;int32_t amp=d_pad_scale(x,gain);
   int32_t v[9]={d_pad_scale(amp,gl),d_pad_scale(amp,gr),(inherit&1)?amp:0,(inherit&2)?amp:0,(inherit&4)?amp:0,(inherit&1)?0:d_pad_scale(amp,sa),(inherit&2)?0:d_pad_scale(amp,sb),(inherit&4)?0:d_pad_scale(amp,sr),amp};
   if(!a){for(unsigned j=0;j<9;j++)buses[f][j]=v[j];}
   else{for(unsigned j=0;j<9;j++)buses[f][j]+=v[j];}
  }
  d_pad_gain[k]=gain;
 }
 unsigned drive=t->p[P_DRIVE];
 for(unsigned f=0;f<n;f++){
  for(unsigned j=0;j<9;j++)buses[f][j]/=4;int32_t mono=buses[f][8],x=d_drive(mono,drive);
  for(unsigned i=0;i<5;i++)t->gain[i]+=t->gainstep[i];
  if((uint32_t)da(x)>t->peak)t->peak=(uint32_t)da(x);
  if(d_capture_active&&(d_usb_groups&2))raw[f][3]=(int16_t)dc(buses[f][8],-32767,32767);
  int32_t wet[8];for(unsigned j=0;j<8;j++){
   if(D_SPARSE_FAST&&j>=2&&!((j-2)%3<2?d_cpu[2]:d_cpu[3]))continue;
   if(!D_FULL_FAST)wet[j]=d_drive(buses[f][j],drive);
   else if(!buses[f][j])wet[j]=0;
   else if(buses[f][j]==mono)wet[j]=x;
   else if(j&&buses[f][j]==buses[f][0])wet[j]=wet[0];
   else wet[j]=d_drive(buses[f][j],drive);
  }
  int32_t l=dm(wet[0],t->gain[0]>>8),r=dm(wet[1],t->gain[1]>>8);mix[f][0]+=l;mix[f][1]+=r;if(d_bench_active&&(l||r))d_drum_sound=8;
  for(unsigned j=0;j<3;j++)if(!D_SPARSE_FAST||(j<2?d_cpu[2]:d_cpu[3]))mix[f][2+j]+=dm(wet[2+j],t->gain[2+j]>>8)+dm(wet[5+j],(t->gain[0]+t->gain[1])>>8);
  if(f+1==n){for(unsigned j=0;j<9;j++)d_drum_bus[j]=buses[f][j];d_drum_source_mono=mono;}
 }
 for(unsigned a=0;a<count;a++)d_drum_raw[pads[a]]=raw[n-1][4+pads[a]];
}
#if defined(D_PLATFORM_MEMORY) && !defined(DINGHY_HOST)
static void d_drum_mix_call(d_part *t,unsigned n,const uint8_t *pads,unsigned count,int32_t bus[][5]){void (*volatile entry)(d_part *,unsigned,const uint8_t *,unsigned,int32_t (*)[5],int16_t (*)[15])=d_drum_mix_block;entry(t,n,pads,count,bus,d_block_stem);}
#else
#define d_drum_mix_call(t,n,pads,count,bus) d_drum_mix_block(t,n,pads,count,bus,d_block_stem)
#endif
static unsigned d_drum_prepare(d_part *t,unsigned n,uint8_t pads[11]){
 unsigned count=0;d_drum_sound=0;
 
 if(!d_sample_limit||(!d_sample_active_count()&&!d_pcm_tails())){
  memset(d_drum_bus,0,sizeof d_drum_bus);d_drum_source_mono=0;
  if(d_sample_limit)for(unsigned i=0;i<11;i++)d_pad_gain[i]+=d_pad_step[i]*(int32_t)n;
  for(unsigned i=0;i<5;i++)t->gain[i]+=t->gainstep[i]*(int32_t)n;return 0;
 }
 for(unsigned i=0;i<11;i++){
  if(!d_drum_voice[i].active&&!d_pcm_tail[i]){d_drum_raw[i]=0;d_pad_gain[i]+=d_pad_step[i]*(int32_t)n;continue;}
  pads[count++]=(uint8_t)i;uint32_t position=d_drum_voice[i].position,length=d_drum_length(i);int32_t gain=d_drum_voice[i].gain;unsigned alive=d_drum_voice[i].active,release=d_drum_voice[i].release,loop=d_sample_views[i].loop;
  for(unsigned f=0;f<n;f++){
   int32_t x=0;
   if(alive){unsigned at=position>>1;if(at>=length){if(loop)position%=length*2u;else {alive=0;d_pcm_tail[i]=f?d_block_stem[f-1][4+i]:d_drum_raw[i];d_pcm_tail_left[i]=32;}}
    if(alive){x=d_pad_scale(d_drum_read(i,position,length),gain);position++;if(release){gain-=256;if(gain<=0)alive=0;}}
   }
   if(d_pcm_attack[i]){x=x*(32-d_pcm_attack[i])/32;d_pcm_attack[i]--;}
   x+=d_pcm_tail[i];if(d_pcm_tail_left[i]){d_pcm_tail[i]-=d_pcm_tail[i]/d_pcm_tail_left[i];if(!--d_pcm_tail_left[i])d_pcm_tail[i]=0;}
   d_block_stem[f][4+i]=(int16_t)dc(x,-32768,32767);
  }
  d_drum_voice[i].position=position;d_drum_voice[i].gain=gain;d_drum_voice[i].active=(uint8_t)alive;
 }
 return count;
}
static void d_drum_block(d_part *t,unsigned n){uint8_t pads[11];unsigned count=d_drum_prepare(t,n,pads);if(count)d_drum_mix_call(t,n,pads,count,d_block_bus);d_sound_mask|=d_drum_sound;
}

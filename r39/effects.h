/* SPDX-License-Identifier: GPL-3.0-only; shared half-rate stereo FX.
 * FIR down/up conversion; dry path aligned by 14 samples. No allocation. */
#pragma once
#define DD_N 24576u
#define RV_N 2448u
static int16_t d_delay_buf[2][2][DD_N] D_POOL;
static int16_t d_rv_buf[8][RV_N] D_POOL;
static int16_t d_diff[2][256] D_POOL;
static int16_t d_predelay[2048] D_POOL;
static int32_t d_down[3][16],d_up[3][2][16],d_dry[2][16];
static uint32_t d_fpos,d_dpos[2],d_rpos[8],d_diffpos[2],d_predpos,d_fxphase,d_fxlfo,d_rvlfo;
static int32_t d_dlp[2][2],d_ddc[2][2],d_rvdamp[8],d_rvdc,d_time[2],d_rvout[2];
static const uint16_t d_rlen[8]={1493,1601,1747,1867,1999,2137,2281,2411};
/* Delay-only two-stage /4 conversion; reverb retains the half-rate clock. */
static int32_t d_rate_down[2][2][16],d_rate_up[2][2][2][16];
static uint32_t d_rate_frame;
#ifndef D_ZERO_FIR
#define D_ZERO_FIR 1
#endif
static uint16_t d_down_nz[3],d_up_nz[3][2],d_rate_down_nz[2][2],d_rate_up_nz[2][2][2];
static __attribute__((always_inline)) inline void d_fir_store(int32_t *h,uint16_t *mask,uint32_t pos,int32_t value){h[pos]=value;uint16_t bit=(uint16_t)(1u<<pos);*mask=value?(uint16_t)(*mask|bit):(uint16_t)(*mask&~bit);}

static uint32_t d_hold_tq[8];
static int d_rv_frozen;
static uint32_t d_fx_limits,d_delay_valid[2];
static int32_t d_lo_sum[2],d_lo_out[2][2],d_fb_hold[2][2];
static uint8_t d_lo_count[2],d_fb_count[2];
static uint8_t d_delay_excited;
/* All engine histories are below2^20 in magnitude: four synth sends are
 * bounded by post-filter clamps/gains, drum input by eleven signed16 samples;
 * FIR L1=42216/32768 and bounded delay/reverb feedback keep later stages below
 * this bound. Pair sums fit signed32; products/accumulation stay signed64. */
static __attribute__((always_inline)) inline int64_t d_fir_sum(const int32_t *h,uint32_t p){
 if(!D_FULL_FAST){int64_t sum=0;for(int k=0;k<15;k++)if(d_fir[k])sum+=(int64_t)h[(p+16u-(uint32_t)k)&15u]*d_fir[k];return sum;}
 return (int64_t)(h[p]+h[(p+2u)&15u])*-120
       +(int64_t)(h[(p+14u)&15u]+h[(p+4u)&15u])*530
       +(int64_t)(h[(p+12u)&15u]+h[(p+6u)&15u])*-2242
       +(int64_t)(h[(p+10u)&15u]+h[(p+8u)&15u])*9993
       +(int64_t)h[(p+9u)&15u]*16446;
}
/* Interpolation histories ONLY: odd ring positions are zero because every
 * /2 input is zero-stuffed at odd frames. Never use for decimation inputs. */
static __attribute__((always_inline)) inline int64_t d_fir_interp_sum(const int32_t *h,uint32_t p){
 if(!D_SPARSE_FAST)return d_fir_sum(h,p);
 if(p&1u)return (int64_t)h[(p+9u)&15u]*16446;
 return (int64_t)(h[p]+h[(p+2u)&15u])*-120
       +(int64_t)(h[(p+14u)&15u]+h[(p+4u)&15u])*530
       +(int64_t)(h[(p+12u)&15u]+h[(p+6u)&15u])*-2242
       +(int64_t)(h[(p+10u)&15u]+h[(p+8u)&15u])*9993;
}
static __attribute__((always_inline)) inline int32_t d_interp_masked(const int32_t *h,uint16_t mask,uint32_t p){return D_ZERO_FIR&&!mask?0:(int32_t)((d_fir_interp_sum(h,p)*2)>>15);}
static __attribute__((always_inline)) inline int32_t d_delay_fir(int32_t *history,uint32_t pos,int gain){return (int32_t)((d_fir_sum(history,pos)*gain)>>15);}
static __attribute__((always_inline)) inline int32_t d_delay_masked(int32_t *h,uint16_t mask,uint32_t pos,int gain){return D_ZERO_FIR&&!mask?0:d_delay_fir(h,pos,gain);}
static __attribute__((always_inline)) inline int32_t d_safe(int32_t x){if(x>32767||x< -32767)d_fx_limits++;return dc(x,-32767,32767);}
static void d_fx_init(void){
 memset(d_down_nz,0,sizeof d_down_nz);memset(d_up_nz,0,sizeof d_up_nz);memset(d_rate_down_nz,0,sizeof d_rate_down_nz);memset(d_rate_up_nz,0,sizeof d_rate_up_nz);
 d_delay_excited=0;d_rv_frozen=0;memset(d_hold_tq,0,sizeof d_hold_tq);
 memset(d_delay_buf,0,sizeof d_delay_buf);memset(d_rv_buf,0,sizeof d_rv_buf);memset(d_diff,0,sizeof d_diff);memset(d_predelay,0,sizeof d_predelay);
 memset(d_rate_down,0,sizeof d_rate_down);memset(d_rate_up,0,sizeof d_rate_up);d_rate_frame=0;
 memset(d_down,0,sizeof d_down);memset(d_up,0,sizeof d_up);memset(d_dry,0,sizeof d_dry);
 memset(d_dlp,0,sizeof d_dlp);memset(d_ddc,0,sizeof d_ddc);memset(d_rvdamp,0,sizeof d_rvdamp);memset(d_rpos,0,sizeof d_rpos);memset(d_diffpos,0,sizeof d_diffpos);
 memset(d_lo_sum,0,sizeof d_lo_sum);memset(d_lo_out,0,sizeof d_lo_out);memset(d_lo_count,0,sizeof d_lo_count);memset(d_fb_hold,0,sizeof d_fb_hold);memset(d_fb_count,0,sizeof d_fb_count);d_dpos[0]=d_dpos[1]=d_delay_valid[0]=d_delay_valid[1]=0;d_fpos=d_predpos=d_fxphase=d_fxlfo=d_rvlfo=d_fx_limits=0;d_rvdc=0;d_rvout[0]=d_rvout[1]=0;
 for(int j=0;j<2;j++)d_time[j]=d_delay_ms_q8(j,dd[j][D_TIME_MS]);
}
static __attribute__((always_inline)) inline int32_t d_delay_read(int bus,int ch,uint32_t time){
 uint32_t n=time>>8,frac=time&255u,cells=d_delay_cells(bus),at=d_dpos[bus]+cells-n;if(at>=cells)at-=cells;
 int32_t a=n>d_delay_valid[bus]?0:d_delay_buf[bus][ch][at],b=n+1u>d_delay_valid[bus]?0:d_delay_buf[bus][ch][at?at-1u:cells-1u];return a+((b-a)*(int32_t)frac>>8);
}
static __attribute__((always_inline)) inline int32_t d_delay_target(int bus){
 int sync=d_fxdd[bus][D_SYNC],ms=d_fxdd[bus][D_TIME_MS];if(sync)ms=d_delay_sync_ms(sync,d_fxg[G_TEMPO]);
 return d_delay_ms_q8(bus,ms);
}
static __attribute__((always_inline)) inline void d_delay_tick(int bus,int32_t input,int32_t result[2]){
 int16_t *p=d_fxdd[bus];int32_t target=d_delay_target(bus);
 int32_t diff=target-d_time[bus];if(diff)d_time[bus]+=diff/256+(diff>0?1:-1);
 int32_t mod=p[D_MODE]?d_wave_ram[3][7][d_fxlfo>>24]*p[D_MOD]/16384:0;
 int32_t echo[2];for(int ch=0;ch<2;ch++)echo[ch]=d_delay_read(bus,ch,(uint32_t)dc(d_time[bus]+(ch?-mod:mod)+(p[D_ROUTE]==1&&ch?d_time[bus]/11:0),256,(int32_t)(d_delay_cells(bus)-2u)*256));
 int32_t feedback[2];for(int ch=0;ch<2;ch++){
  int32_t x=echo[ch];
  if(p[D_MODE]){d_ddc[bus][ch]+=(x-d_ddc[bus][ch])/64;x-=d_ddc[bus][ch];d_dlp[bus][ch]+=dm(x-d_dlp[bus][ch],dc(4*(1500+(127-p[D_TONE])*220),0,32767));x=d_dlp[bus][ch];}
  feedback[ch]=x;
 }
 if(d_delay_cfg[bus][0]&&d_delay_cfg[bus][3]){
  unsigned amount=d_delay_cfg[bus][3],period=1u+amount*7u/127u,drop=4u+amount*8u/127u;
  if(!d_fb_count[bus])for(int ch=0;ch<2;ch++){int32_t x=dc(feedback[ch],-32767,32767);d_fb_hold[bus][ch]=(x<0?-((-x)>>drop):x>>drop)*(int32_t)(1u<<drop);}
  if(++d_fb_count[bus]>=period)d_fb_count[bus]=0;feedback[0]=d_fb_hold[bus][0];feedback[1]=d_fb_hold[bus][1];
 }
 for(int ch=0;ch<2;ch++){
  int other=p[D_ROUTE]==2?ch^1:ch;int32_t feed=feedback[other]*p[D_FEEDBACK]/100;
  int32_t in=input;if(p[D_ROUTE]==2&&ch)in=0; /* excite left; cross-feedback creates alternating echoes */
  int32_t x=in+feed;if(p[D_MODE])x=d_drive(x,p[D_DRIVE]);x=d_safe(x);unsigned drop=4u*d_delay_cfg[bus][2];if(drop)x=(x<0?-((-x)>>drop):x>>drop)*(int32_t)(1u<<drop);d_delay_buf[bus][ch][d_dpos[bus]]=(int16_t)x;
 }
 int32_t mid=(echo[0]+echo[1])/2,side=(echo[0]-echo[1])/2;
 if(p[D_ROUTE]==0){result[0]=result[1]=mid;}else{result[0]=mid+side*p[D_WIDTH]/127;result[1]=mid-side*p[D_WIDTH]/127;}
}
static __attribute__((always_inline)) inline void d_delay_step(int bus,int32_t input,int32_t echo[2]){
 d_delay_tick(bus,input,echo);unsigned cells=d_delay_cells(bus);if(d_delay_valid[bus]<cells)d_delay_valid[bus]++;if(++d_dpos[bus]==cells)d_dpos[bus]=0;
}
/* Invalidate the ring in constant time; preserve the other delay and reverb. */
static void d_delay_config_set(int bus,int field,int value){
 if(bus<0||bus>1||field<0||field>3)return;value=dc(value,0,field==1?5:field==2?3:field==3?127:1);if(d_delay_cfg[bus][field]==value)return;d_delay_cfg[bus][field]=(uint8_t)value;
 d_dpos[bus]=d_delay_valid[bus]=0;d_time[bus]=d_delay_ms_q8(bus,dd[bus][D_TIME_MS]);d_lo_sum[bus]=d_lo_out[bus][0]=d_lo_out[bus][1]=0;d_lo_count[bus]=d_fb_count[bus]=0;d_fb_hold[bus][0]=d_fb_hold[bus][1]=0;
 memset(d_rate_down[bus],0,sizeof d_rate_down[bus]);memset(d_rate_up[bus],0,sizeof d_rate_up[bus]);memset(d_rate_down_nz[bus],0,sizeof d_rate_down_nz[bus]);memset(d_rate_up_nz[bus],0,sizeof d_rate_up_nz[bus]);
 memset(d_down[bus],0,sizeof d_down[bus]);memset(d_up[bus],0,sizeof d_up[bus]);d_down_nz[bus]=0;memset(d_up_nz[bus],0,sizeof d_up_nz[bus]);memset(d_dlp[bus],0,sizeof d_dlp[bus]);memset(d_ddc[bus],0,sizeof d_ddc[bus]);
}
/* Called at22050Hz. Delay tick/write pointer advance only every fourth call. */
static __attribute__((always_inline)) inline void d_delay_convert(int bus,int32_t input,int32_t result[2]){
 unsigned shift=d_delay_shift(bus);
 if(d_delay_cfg[bus][0]){d_lo_sum[bus]+=input;if(++d_lo_count[bus]>=(1u<<shift)){d_delay_step(bus,d_lo_sum[bus]/(int32_t)(1u<<shift),d_lo_out[bus]);d_lo_sum[bus]=0;d_lo_count[bus]=0;}result[0]=d_lo_out[bus][0];result[1]=d_lo_out[bus][1];return;}
 if(!shift){d_delay_step(bus,input,result);return;}
 uint32_t p0=d_rate_frame&15u,p1=(d_rate_frame>>1)&15u;
 if(shift==1){d_fir_store(d_rate_down[bus][0],&d_rate_down_nz[bus][0],p0,input);int32_t echo[2]={0,0};if(!(d_rate_frame&1u))d_delay_step(bus,d_delay_masked(d_rate_down[bus][0],d_rate_down_nz[bus][0],p0,1),echo);for(int ch=0;ch<2;ch++){d_fir_store(d_rate_up[bus][0][ch],&d_rate_up_nz[bus][0][ch],p0,echo[ch]);result[ch]=d_interp_masked(d_rate_up[bus][0][ch],d_rate_up_nz[bus][0][ch],p0);}return;}

 d_fir_store(d_rate_down[bus][0],&d_rate_down_nz[bus][0],p0,input);
 for(int ch=0;ch<2;ch++)d_fir_store(d_rate_up[bus][0][ch],&d_rate_up_nz[bus][0][ch],p0,0);
 if(!(d_rate_frame&1u)){
  d_fir_store(d_rate_down[bus][1],&d_rate_down_nz[bus][1],p1,d_delay_masked(d_rate_down[bus][0],d_rate_down_nz[bus][0],p0,1));
  int32_t echo[2]={0,0};
  if(!(d_rate_frame&3u))d_delay_step(bus,d_delay_masked(d_rate_down[bus][1],d_rate_down_nz[bus][1],p1,1),echo);
  for(int ch=0;ch<2;ch++){
   d_fir_store(d_rate_up[bus][1][ch],&d_rate_up_nz[bus][1][ch],p1,echo[ch]);
   d_fir_store(d_rate_up[bus][0][ch],&d_rate_up_nz[bus][0][ch],p0,d_interp_masked(d_rate_up[bus][1][ch],d_rate_up_nz[bus][1][ch],p1));
  }
 }
 for(int ch=0;ch<2;ch++)result[ch]=d_interp_masked(d_rate_up[bus][0][ch],d_rate_up_nz[bus][0][ch],p0);
}
static __attribute__((always_inline)) inline int32_t d_allpass(int idx,int32_t x,int32_t gain){
 uint32_t pos=d_diffpos[idx],len=idx?211:149;int32_t old=d_diff[idx][pos],y=old-dm(x,gain);
 d_diff[idx][pos]=(int16_t)d_safe(x+dm(y,gain));d_diffpos[idx]=(pos+1u==len)?0:pos+1u;return y;
}
static __attribute__((always_inline)) inline void d_reverb_tick(int32_t input,int32_t result[2]){
 int hold=d_fxdv[V_DECAY]==100;
 input/=2; /* tank headroom, compensated at the stereo output */
 int dc_error=input-d_rvdc,dc_step=dc_error/((127-d_fxdv[V_LOW_CUT])*12+32);if(hold&&!dc_step&&dc_error)dc_step=dc_error>0?1:-1;d_rvdc+=dc_step;input-=d_rvdc;
 uint32_t delay=(uint32_t)d_fxdv[V_PREDELAY]*22050u/1000u;
 d_predelay[d_predpos]=(int16_t)d_safe(input);input=d_predelay[(d_predpos+2048u-delay)&2047u];d_predpos=(d_predpos+1u)&2047u;
 int32_t diffusion=d_fxdv[V_DIFFUSION]*190;input=d_allpass(0,input,diffusion);input=d_allpass(1,input,diffusion);
 int32_t y[8],h[8];int32_t mod=d_wave_ram[3][7][d_rvlfo>>24]*d_fxdv[V_MOD]/8192;
 for(int j=0;j<8;j++){
  uint32_t len=d_rlen[j],position=d_rpos[j],tq=(len-4u)*256u+(uint32_t)(j&1?512-mod:512+mod),n,f;if(hold){if(!d_rv_frozen)d_hold_tq[j]=tq;tq=d_hold_tq[j];}n=tq>>8;f=tq&255u;
  uint32_t a=(position+len-n)%len,b=a?a-1u:len-1u;
  int32_t x=d_rv_buf[j][a]+((d_rv_buf[j][b]-d_rv_buf[j][a])*(int32_t)f>>8);
  d_rvdamp[j]+=dm(x-d_rvdamp[j],1200+(127-d_fxdv[V_DAMPING])*220);y[j]=h[j]=d_rvdamp[j];
 }
 /* Orthogonal eight-line Hadamard feedback, normalized by 1/sqrt(8). */
 if(!hold)for(int stride=1;stride<8;stride*=2)for(int start=0;start<8;start+=2*stride)for(int j=0;j<stride;j++){
  int32_t a=h[start+j],b=h[start+j+stride];h[start+j]=a+b;h[start+j+stride]=a-b;
 }
 for(int j=0;j<8;j++){
  if(!hold){int32_t x=input/4+dm(h[j],11585)*d_fxdv[V_DECAY]/100;d_rv_buf[j][d_rpos[j]]=(int16_t)d_safe(x);}
  else if(da(input)>8){int32_t x=d_rv_buf[j][d_rpos[j]]+input/4;
   if(x>24576)x=24576+(x-24576)/4;else if(x< -24576)x=-24576+(x+24576)/4;
   d_rv_buf[j][d_rpos[j]]=(int16_t)d_safe(x);
  }
  if(++d_rpos[j]==d_rlen[j])d_rpos[j]=0;
 }
 result[0]=(y[0]+y[2]-y[4]+y[6])/2;result[1]=(y[1]+y[3]-y[5]+y[7])/2;
 d_rv_frozen=hold;if(!hold)d_rvlfo+=68174u;
}
static __attribute__((always_inline)) inline void d_fx_core(int32_t dry_l,int32_t dry_r,const int32_t send[3],int32_t *left,int32_t *right){
 uint32_t pos=d_fpos;d_dry[0][pos]=dry_l;d_dry[1][pos]=dry_r;
 if(d_cpu[2]&&(send[0]||send[1]))d_delay_excited=1;
 /* Only pristine tanks may sleep. Once excited, tails are never truncated.
  * Preserve all converter clocks and delay-time slew while the tank is empty. */
 if(!d_cpu[3]&&(!d_cpu[2]||!d_delay_excited)){
  if(!d_fxphase){d_fxlfo+=68174u;for(int b=0;b<2;b++){
   unsigned shift=d_delay_shift(b),step=d_delay_cfg[b][0]?(++d_lo_count[b]>=(1u<<shift)):!(d_rate_frame&((1u<<shift)-1u));if(!step)continue;if(d_delay_cfg[b][0])d_lo_count[b]=0;
   if(d_cpu[2]){int32_t diff=d_delay_target(b)-d_time[b];if(diff)d_time[b]+=diff/256+(diff>0?1:-1);}
   if(++d_dpos[b]==d_delay_cells(b))d_dpos[b]=0;
  }d_rate_frame++;}
  *left=d_dry[0][(pos+2u)&15u];*right=d_dry[1][(pos+2u)&15u];d_fxphase^=1u;d_fpos=(pos+1u)&15u;return;
 }
 for(int b=0;b<3;b++){if(D_SPARSE_FAST&&!(b<2?d_cpu[2]:d_cpu[3]))continue;d_fir_store(d_down[b],&d_down_nz[b],pos,send[b]);for(int ch=0;ch<2;ch++)d_fir_store(d_up[b][ch],&d_up_nz[b][ch],pos,0);}
 if(!d_fxphase){
  int32_t in[3]={0,0,0};for(int b=0;b<3;b++){if(!(b<2?d_cpu[2]:d_cpu[3])||(D_ZERO_FIR&&!d_down_nz[b]))continue;in[b]=(int32_t)(d_fir_sum(d_down[b],pos)>>15);}
  int32_t echo[2][2]={{0,0},{0,0}},rv[2]={0,0};d_fxlfo+=68174u;
  for(int b=0;b<2;b++){if(!d_cpu[2])continue;d_delay_convert(b,in[b],echo[b]);in[2]+=(echo[b][0]+echo[b][1])*d_fxdd[b][D_TO_REV]/254;}
  if(d_cpu[3])d_reverb_tick(in[2],rv);d_rate_frame++;
  for(int ch=0;ch<2;ch++){for(int b=0;b<2;b++)if(!D_SPARSE_FAST||d_cpu[2])d_fir_store(d_up[b][ch],&d_up_nz[b][ch],pos,echo[b][ch]?echo[b][ch]*d_fxg[G_DLY_A_RET+b]/127:0);if(!D_SPARSE_FAST||d_cpu[3])d_fir_store(d_up[2][ch],&d_up_nz[2][ch],pos,rv[ch]?rv[ch]*d_fxg[G_REV_RET]/127:0);}
 }
 int32_t out[2];for(int ch=0;ch<2;ch++){
  int64_t acc=0;for(int b=0;b<3;b++)if((b<2?d_cpu[2]:d_cpu[3])&&(!D_ZERO_FIR||d_up_nz[b][ch]))acc+=d_fir_interp_sum(d_up[b][ch],pos)*2;
  out[ch]=d_dry[ch][(pos+16u-14u)&15u]+(int32_t)(acc>>15);
 }
 *left=out[0];*right=out[1];d_fxphase^=1u;d_fpos=(pos+1u)&15u;
}

static D_SAMPLE_RAM void d_fx_sample(int32_t l,int32_t r,const int32_t send[3],int32_t *left,int32_t *right){d_fx_core(l,r,send,left,right);}

#if defined(D_PLATFORM_MEMORY) && !defined(DINGHY_HOST)
static __attribute__((always_inline)) inline void d_fx_tick(int32_t l,int32_t r,const int32_t send[3],int32_t *left,int32_t *right){
 void (*volatile entry)(int32_t,int32_t,const int32_t *,int32_t *,int32_t *)=d_fx_sample;entry(l,r,send,left,right);
}
#else
#define d_fx_tick d_fx_sample
#endif

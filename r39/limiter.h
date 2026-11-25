/* SPDX-License-Identifier: GPL-3.0-only
 * lim_block port from SLOOP2.8.5 fx.c, bac9c05ea7627449405aa10e48da21d75b6c8c56.
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments.
 * DUB adaptation: interleaved stereo, hold measured in frames for4..32frame blocks.
 */
#pragma once
#define D_LIM_N 32u
#define D_LIM_T 18000
static struct {int32_t line[32][2],env,gain;uint32_t pos,hold,preclip_frames;} d_lim D_POOL;
static const int16_t d_tanh[257]={0,256,512,768,1024,1279,1535,1790,2045,2300,2555,2809,3063,3317,3570,3822,4075,4326,4578,4828,5079,5328,5577,5825,6073,6320,6566,6811,7056,7299,7542,7784,8025,8265,8505,8743,8980,9216,9452,9686,9919,10151,10382,10611,10840,11067,11294,11519,11742,11965,12186,12406,12625,12842,13058,13273,13486,13698,13908,14118,14325,14532,14737,14940,15142,15343,15542,15740,15936,16130,16324,16515,16706,16894,17081,17267,17451,17634,17815,17995,18173,18349,18524,18697,18869,19040,19208,19376,19541,19706,19868,20029,20189,20347,20503,20658,20812,20964,21114,21263,21411,21557,21701,21844,21986,22126,22264,22401,22537,22671,22804,22935,23065,23193,23320,23446,23570,23693,23815,23935,24053,24171,24287,24401,24515,24627,24738,24847,24955,25062,25168,25272,25375,25477,25578,25677,25775,25872,25968,26063,26156,26248,26339,26429,26518,26606,26693,26778,26863,26946,27029,27110,27190,27269,27348,27425,27501,27576,27650,27723,27796,27867,27937,28007,28075,28143,28210,28275,28340,28404,28468,28530,28592,28652,28712,28771,28829,28887,28943,28999,29054,29109,29162,29215,29267,29319,29369,29419,29469,29517,29565,29612,29659,29705,29750,29795,29839,29882,29925,29967,30009,30050,30090,30130,30169,30208,30246,30284,30321,30357,30393,30429,30464,30498,30532,30566,30599,30631,30663,30695,30726,30757,30787,30817,30846,30875,30903,30932,30959,30986,31013,31040,31066,31092,31117,31142,31166,31191,31215,31238,31261,31284,31306,31329,31350,31372,31393,31414,31435,31455,31475,31494,31514,31533,31552,31570,31588};
static int32_t d_knee(int32_t x){int32_t a=da(x);if(a<=16384)return x;unsigned v=(unsigned)(a-16384)*2u,i=v>>8;int32_t y=i>=256?d_tanh[256]:d_tanh[i]+((d_tanh[i+1]-d_tanh[i])*(int32_t)(v&255u)>>8);a=16384+(y>>1);return x<0?-a:a;}
static void d_limiter_reset(void){memset(&d_lim,0,sizeof d_lim);d_lim.env=D_LIM_T;d_lim.gain=32768;}
static void d_limiter_block(int32_t *pcm,unsigned n){
 int32_t pk=0,e=d_lim.env,g0=d_lim.gain;
 for(unsigned i=0;i<32;i++)for(unsigned ch=0;ch<2;ch++){int32_t a=da(d_lim.line[i][ch]);if(a>pk)pk=a;}
 for(unsigned i=0;i<n;i++){int32_t l=da(pcm[2*i]),r=da(pcm[2*i+1]);if(l>32767||r>32767)d_lim.preclip_frames++;if(l>pk)pk=l;if(r>pk)pk=r;}
 if((int64_t)pk*17>=(int64_t)e*16)d_lim.hold=48u*32u;
 if(d_lim.hold)d_lim.hold=d_lim.hold>n?d_lim.hold-n:0;
 else if(e>D_LIM_T)e-=(int32_t)((((int64_t)(e-D_LIM_T)*n)>>11)+1);
 if(e<D_LIM_T)e=D_LIM_T;if(pk>e){e=pk;d_lim.hold=48u*32u;}
 d_lim.env=e;int32_t g1=e>D_LIM_T?(int32_t)(((uint32_t)D_LIM_T<<15)/(uint32_t)e):32768;d_lim.gain=g1;int32_t dg=(g1-g0)/(int32_t)n;
 for(unsigned i=0;i<n;i++){int32_t gain=i+1==n?g1:g0+dg*(int32_t)(i+1);
  for(unsigned ch=0;ch<2;ch++){int32_t x=d_lim.line[d_lim.pos][ch];d_lim.line[d_lim.pos][ch]=pcm[2*i+ch];if(gain<32768)x=(int32_t)((int64_t)x*gain>>15);pcm[2*i+ch]=d_knee(x);}
  d_lim.pos=(d_lim.pos+1u)&31u;
 }
}

/* SPDX-License-Identifier: GPL-3.0-only; SMP1 wire and FSB1 catalog primitives. */
#pragma once
#include <stdint.h>
#include "sample_config.h"
#define D_SAMPLE_RECORD_BYTES 40u
#define D_SAMPLE_RATE 22050u
#define D_SAMPLE_CHUNK 96u
static uint32_t d_sample_rd32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void d_sample_wr36(uint8_t *p,uint32_t n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);p[2]=(uint8_t)(n>>16);p[3]=(uint8_t)(n>>24);}
static uint32_t d_sample_crc_more(uint32_t c,const uint8_t *p,uint32_t n){while(n--){c^=*p++;for(int k=0;k<8;k++)c=c&1u?(c>>1)^0xedb88320u:c>>1;}return c;}
static uint32_t d_sample_crc(const uint8_t *p,uint32_t n){return ~d_sample_crc_more(0xffffffffu,p,n);}
static unsigned d_sample_pack(const uint8_t *p,unsigned n,uint8_t *out){unsigned i=0,o=0;while(i<n){unsigned h=o++;out[h]=0;for(unsigned k=0;k<7&&i<n;k++,i++){out[h]|=(p[i]>>7)<<k;out[o++]=p[i]&127u;}}return o;}
static int d_sample_unpack(const uint8_t *p,unsigned n,uint8_t *out,unsigned max){unsigned i=0,o=0;while(i<n){unsigned h=p[i++],k=0;if(h>127||i==n)return -1;for(;k<7&&i<n;k++,i++){if(p[i]>127||o>=max)return -1;out[o++]=p[i]|((h>>k&1u)<<7);}if(h>>k)return -1;}return (int)o;}
static int d_sample_catalog_valid(const uint8_t *p,uint32_t n){
 if(n<D_SAMPLE_CATALOG_BYTES||n>D_SAMPLE_BLOB_MAX||p[0]!='F'||p[1]!='S'||p[2]!='B'||p[3]!='1'||d_sample_rd32(p+4)!=1||d_sample_rd32(p+8)!=11||d_sample_rd32(p+12)!=n)return 0;
 uint32_t used=0;for(int i=0;i<11;i++){
  const uint8_t *r=p+16+i*40;uint32_t offset=d_sample_rd32(r+24),frames=d_sample_rd32(r+28),rate=d_sample_rd32(r+32),flags=d_sample_rd32(r+36);
  if(!frames){for(int k=0;k<40;k++)if(r[k])return 0;continue;}
  if(!r[0]||r[23]||frames<2||rate!=D_SAMPLE_RATE||flags>1||offset!=used||frames>(n-D_SAMPLE_CATALOG_BYTES-used)/2u)return 0;
  int ended=0;for(int k=0;k<24;k++){if(!r[k])ended=1;else if(ended||r[k]<32||r[k]>126)return 0;}
  used+=frames*2u;
 }
 for(unsigned i=16u+11u*40u;i<D_SAMPLE_CATALOG_BYTES;i++)if(p[i])return 0;
 return n==D_SAMPLE_CATALOG_BYTES+used;
}

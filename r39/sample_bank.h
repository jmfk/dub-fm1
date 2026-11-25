/* SPDX-License-Identifier: GPL-3.0-only; immutable active XIP views, published muted. */
#pragma once
#include "sample_format.h"
typedef struct {const int16_t *pcm;uint32_t frames;uint8_t loop;} d_sample_view;
static d_sample_view d_sample_views[11] D_POOL;
static const uint8_t *d_sample_payload;
static uint32_t d_sample_generation,d_sample_payload_bytes;
/* Audio owner acknowledges1(requested) as2(muted) or3(busy). */
static volatile uint8_t d_sample_pause,d_sample_abort_requested;
static volatile uint32_t d_sample_progress;
static void d_sample_catalog_reset(void){memset(d_sample_views,0,sizeof d_sample_views);d_sample_payload=0;d_sample_generation=d_sample_payload_bytes=0;d_sample_pause=d_sample_abort_requested=0;d_sample_progress=0;}
static void d_sample_publish(const uint8_t *p,uint32_t n,uint32_t generation){
 for(int i=0;i<11;i++){
  const uint8_t *r=p+16+i*40;uint32_t frames=d_sample_rd32(r+28);
  d_sample_views[i].pcm=frames?(const int16_t *)(p+D_SAMPLE_CATALOG_BYTES+d_sample_rd32(r+24)):0;
  d_sample_views[i].frames=frames;d_sample_views[i].loop=frames?(uint8_t)d_sample_rd32(r+36):0;
 }
 d_sample_payload=p;d_sample_payload_bytes=n;d_sample_generation=generation;
}

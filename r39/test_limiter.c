/* SPDX-License-Identifier: GPL-3.0-only; limiter reference from SLOOP2.8.5 fx.c.
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments. */
#include <assert.h>
#include <stdio.h>
#include "engine.h"
#define CTL 32
#define LIM_T 18000
typedef struct {
    int32_t dl[CTL], dr[CTL];           /* the mix of the last block (the delay) */
    uint32_t w;                         /* its oldest sample (the next out) */
    int32_t env;                        /* the peak the gain fits under LIM_T (LIM_T: none) */
    int32_t g;                          /* the gain at the end of the last block, Q15 (32768: 1) */
    uint32_t hold;                      /* blocks before the release starts (LIM_HOLD after a peak near env) */
} lim_t;
/* (2.6, a whistle on loud basses) the release waits LIM_HOLD blocks after the last peak near the envelope: a held bass
 * (down to ~29 Hz, a period under 35 ms) keeps one gain, instead of the gain dipping and creeping back at every cycle
 * (a sawtooth at the bass's pitch with a corner at every block: tones around 1.4 kHz and up) */
#define LIM_HOLD 48u
#define LIM_REL 11                      /* the release after the hold: ~46 ms (12 before the hold: ~93 ms; the same loudness) */

/* l, r: n (<= CTL) samples in; out: the n samples of CTL before, limited (before the knee) */
static void lim_block(lim_t *s, int32_t *l, int32_t *r, uint32_t n)
{
    uint32_t i, w = s->w;
    int32_t pk = 0, a, g0 = s->g, g1, dg, e = s->env;
    for (i = 0; i < CTL; i++) {                         /* the samples going out now and the ones after them (the
                                                         * rest of the delay, then this block): CTL ahead */
        a = s->dl[i] < 0 ? -s->dl[i] : s->dl[i], pk = a > pk ? a : pk;
        a = s->dr[i] < 0 ? -s->dr[i] : s->dr[i], pk = a > pk ? a : pk;
    }
    for (i = 0; i < n; i++) {
        a = l[i] < 0 ? -l[i] : l[i], pk = a > pk ? a : pk;
        a = r[i] < 0 ? -r[i] : r[i], pk = a > pk ? a : pk;
    }
    if ((int64_t)pk * 17 >= (int64_t)e * 16)           /* a peak within ~0.5 dB of the envelope: the hold again */
        s->hold = LIM_HOLD;
    if (s->hold)
        s->hold--;
    else if (e > LIM_T)                                 /* the release (after the hold), a block at a time */
        e -= (int32_t)((((int64_t)(e - LIM_T) * (int32_t)n) >> LIM_REL) + 1);
    if (e < LIM_T)
        e = LIM_T;
    if (pk > e)
        e = pk, s->hold = LIM_HOLD;
    s->env = e;
    g1 = e > LIM_T ? (int32_t)(((uint32_t)LIM_T << 15) / (uint32_t)e) : 32768;
    s->g = g1;
    dg = (g1 - g0) / (int32_t)(n ? n : 1u);             /* (towards 0: the gain never above both ends' bound) */
    for (i = 0; i < n; i++) {
        int32_t xl = s->dl[w], xr = s->dr[w], g = g0 + dg * (int32_t)(i + 1u);
        s->dl[w] = l[i], s->dr[w] = r[i];
        w = (w + 1u) & (CTL - 1u);
        if (i + 1u == n)
            g = g1;
        if (g < 32768) {                                /* (64-bit: |x| may be far above Q15. 2.6 fix: x >> 4 first cut
                                                         * the limited signal to 12 bits, a whistle on loud basses) */
            xl = (int32_t)(((int64_t)xl * g) >> 15);
            xr = (int32_t)(((int64_t)xr * g) >> 15);
        }
        l[i] = xl, r[i] = xr;
    }
    s->w = w;
}

int main(void){d_init();lim_t reference={.env=LIM_T,.g=32768};int32_t pcm[64],l[32],r[32];unsigned seed=1;for(unsigned block=0;block<2500;block++){
 for(unsigned i=0;i<32;i++){seed=seed*1664525+1013904223;l[i]=pcm[2*i]=(int32_t)(seed%200001)-100000;seed=seed*1664525+1013904223;r[i]=pcm[2*i+1]=(int32_t)(seed%200001)-100000;}
 lim_block(&reference,l,r,32);d_limiter_block(pcm,32);for(unsigned i=0;i<32;i++){assert(pcm[2*i]==d_knee(l[i]));assert(pcm[2*i+1]==d_knee(r[i]));assert(da(pcm[2*i])<32767&&da(pcm[2*i+1])<32767);}}
 for(unsigned n=1;n<=32;n*=2){d_limiter_reset();for(unsigned f=0;f<128;f+=n){for(unsigned i=0;i<n;i++){pcm[2*i]=1000+(int32_t)f+(int32_t)i;pcm[2*i+1]=-pcm[2*i];}d_limiter_block(pcm,n);for(unsigned i=0;i<n;i++){int32_t expected=f+i<32?0:1000+(int32_t)f+(int32_t)i-32;assert(pcm[2*i]==expected&&pcm[2*i+1]==-expected);}}}
 d_limiter_reset();for(unsigned i=0;i<64;i++)pcm[i]=200000;d_limiter_block(pcm,32);assert(d_lim.gain<32768);d_full_panic();for(unsigned i=0;i<64;i++)pcm[i]=0;d_limiter_block(pcm,32);for(unsigned i=0;i<64;i++)assert(!pcm[i]);puts("PASS2500 blocks exact SLOOP limiter PCM parity, loud stereo bounded, quiet linear32sample latency at1/2/4/8/16/32 chunks and panic reset");}

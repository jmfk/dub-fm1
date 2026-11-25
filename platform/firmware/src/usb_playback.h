/* SPDX-License-Identifier: GPL-3.0-only */
/* UAC1 PCM16 stereo sink. TIMER5 produces, audio ISR consumes. Control-plane
 * reset runs only outside rendering. Never reset indices from the producer
 * while rendering: nested RX may interrupt a block being consumed. */
#pragma once
#define UP_N 1024u
#define UP_PRIME 512u
#define UP_MAX_FRAMES 45u
#define UP_MAX_BYTES (UP_MAX_FRAMES * 4u)
#define UP_NOMINAL 722534u                 /* round(44.1 * 2^14), frames/USB ms */
static uint32_t up_ring[UP_N];
static struct {
    volatile uint32_t w, r;
    volatile uint8_t alt;
    uint8_t running;                      /* owned by the audio consumer */
    int32_t fill_filter;
    volatile uint32_t feedback;
    uint32_t packets, frames, malformed, overflows, underruns, starts;
    uint32_t fill_min, fill_max;
    uint32_t peak_l, peak_r;
    uint32_t status_errors, length_errors, max_bytes;
} up = {.feedback = UP_NOMINAL};

static void up_stream(uint32_t alt)
{
    up.alt = 0;
    RING_PUBLISH();
    up.w = up.r = 0;
    up.running = 0;
    up.fill_filter = 0;
    up.feedback = UP_NOMINAL;
    RING_PUBLISH();
    up.alt = (uint8_t)alt;
    if (alt) {
        up.starts++;
        up.fill_min = UP_N;
        up.fill_max = 0;
    }
}

static void up_receive(const uint8_t *p, uint32_t bytes)
{
    uint32_t i, w = up.w, frames = bytes / 4u;
    if (!up.alt) return;
    if(bytes>up.max_bytes)up.max_bytes=bytes;
    if (bytes > UP_MAX_BYTES || (bytes & 3u)) { up.malformed++;up.length_errors++; return; }
    if (frames > UP_N - (w - up.r)) { up.overflows++; return; }
    for (i = 0; i < frames; i++, p += 4) {
        int32_t l = (int16_t)(p[0] | (uint16_t)p[1]<<8);
        int32_t r = (int16_t)(p[2] | (uint16_t)p[3]<<8);
        uint32_t al=(uint32_t)(l<0?-l:l), ar=(uint32_t)(r<0?-r:r);
        if(al>up.peak_l)up.peak_l=al;
        if(ar>up.peak_r)up.peak_r=ar;
        up_ring[(w + i) & (UP_N - 1u)] = p[0] | (uint32_t)p[1] << 8 |
            (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
    }
    RING_PUBLISH();
    up.w = w + frames;
    up.packets++;
    up.frames += frames;
}

/* Called AFTER the synth-only USB capture tap. MASTER scales both paths; this
 * function saturates their sum, never sends host playback back to USB capture.
 * Underflow leaves the synth untouched, discards the partial block and re-primes.
 * A 16-block low-pass of pre-consumption fill gives bounded explicit feedback;
 * a small steady-state fill offset compensates crystal error without resampling. */
static void up_mix(int32_t *out, uint32_t n, int32_t master)
{
    uint32_t i, r = up.r, fill = up.w - r;
    int32_t correction;
    if (!up.alt) return;
    if (!up.running) {
        if (fill < UP_PRIME) return;
        up.running = 1;
    }
    if (fill < n) {
        up.underruns++;
        up.running = 0;
        up.r = r + fill;
        up.fill_filter = 0;
        up.feedback = UP_NOMINAL;
        return;
    }
    if (fill < up.fill_min) up.fill_min = fill;
    if (fill > up.fill_max) up.fill_max = fill;
    up.fill_filter += ((int32_t)fill - (int32_t)UP_PRIME) - up.fill_filter / 16;
    correction = up.fill_filter;
    if (correction > 4096) correction = 4096;
    if (correction < -4096) correction = -4096;
    up.feedback = (uint32_t)((int32_t)UP_NOMINAL - correction);
    for (i = 0; i < n; i++) {
        uint32_t frame = up_ring[(r + i) & (UP_N - 1u)];
        int32_t l = (int16_t)frame, right = (int16_t)(frame >> 16);
        l = out[2u*i] + ((l * master) >> 12);
        right = out[2u*i+1u] + ((right * master) >> 12);
        out[2u*i] = l > 32767 ? 32767 : l < -32767 ? -32767 : l;
        out[2u*i+1u] = right > 32767 ? 32767 : right < -32767 ? -32767 : right;
    }
    RING_PUBLISH();
    up.r = r + n;
}

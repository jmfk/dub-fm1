/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define FELUCCA_CDC 0
#define FELUCCA_UAC 1
#define FELUCCA_OTA 0
#define HALF_FRAMES 256u
#define FM1_USB_HAL "usb_mock.h"
#ifndef FM1_USB_PLAYBACK
#define FM1_USB_PLAYBACK 1
#endif
#include "usb.c"

static void setup(uint8_t type, uint8_t request, uint16_t value, uint16_t index, uint16_t length)
{
    const uint8_t p[8] = {type, request, value, value >> 8, index, index >> 8, length, length >> 8};
    memcpy(ep0buf, p, 8);
    hw_ep[0][17] = 1;
    hw_ep[0][22] = 8;
    hw_stall = 0;
    control_len = 0;
    ep0_service();
    while (usb.e0_tx) {
        hw_ep[0][17] &= ~2u;
        ep0_service();
    }
    hw_ep[0][17] = 0;
}
static void data_rate(uint32_t rate)
{
    ep0buf[0] = (uint8_t)rate; ep0buf[1] = rate >> 8; ep0buf[2] = rate >> 16;
    hw_ep[0][17] = 1; hw_ep[0][22] = 3;
    ep0_service(); hw_ep[0][17] = 0;
}

static void descriptors(void)
{
    uint32_t at = 0, interface = 99, alt = 0, endpoints = 0, interfaces = 0;
    setup(0x80, 6, 0x0200, 0, 1024);
    assert(!hw_stall && control_len == sizeof CFG_DESC);
    assert(control_reply[2] + 256u * control_reply[3] == control_len);
    assert(control_reply[4] == 3u + FM1_USB_PLAYBACK);
    while (at < control_len) {
        const uint8_t *d = control_reply + at;
        assert(d[0] >= 2 && at + d[0] <= control_len);
        if (d[1] == 4) {
            interface = d[2]; alt = d[3];
            if (!alt) interfaces++;
            if (interface == 3 && alt == 1) assert(d[4] == 2);
        }
        if (d[1] == 5) {
            if (interface == 1) { assert((d[2] & 0x7f) == 1 && d[3] == 2); endpoints |= d[2] == 1 ? 1 : 2; }
            if (interface == 2) { assert(d[2] == 0x84 && d[3] == 5); endpoints |= 4; }
            if (interface == 3) {
                if (d[2] == 2) { assert(d[3] == 5 && d[4] == 180 && d[8] == 0x82); endpoints |= 8; }
                else { assert(d[2] == 0x82 && d[3] == 0x11 && d[4] == 3); endpoints |= 16; }
            }
        }
        if (d[1] == 0x24 && interface == 0 && d[2] == 1) {
            assert(d[5] == (FM1_USB_PLAYBACK ? 53 : 31));
            assert(d[7] == (FM1_USB_PLAYBACK ? 3 : 2));
        }
        at += d[0];
    }
    assert(interfaces == 3u + FM1_USB_PLAYBACK);
    assert(endpoints == (FM1_USB_PLAYBACK ? 31u : 7u));
}

#if FM1_USB_PLAYBACK
/* Compile the actual platform ISR, extracted unchanged by build.py, against
 * this register model. This checks routing and scaling rather than duplicating
 * the intended order in a hand-written fake render. */
#define HALF_WORDS (HALF_FRAMES * 2u)
#define FM1_AUDIO_HALF 0x80u
#define UAC_ONLY(x) x
static int32_t abuf[2u * HALF_WORDS], master_q12 = 4096;
static uint8_t in_audio;
static uint8_t fm1_audio_pending(void) { return FM1_AUDIO_HALF; }
static void fm1_audio_ack_aux(uint8_t p) { (void)p; }
static uint32_t fm1_audio_free_half(void) { return 0; }
static void fm1_audio_ack_half(void) {}
static void app_audio(int32_t *o, uint32_t n)
{ for (uint32_t i=0; i<n; i++) { o[2*i] = 1000; o[2*i+1] = -2000; } }
#include "render_irq.h"

static void packet(uint32_t n, int16_t l, int16_t r)
{
    uint32_t i,rearms=rx_rearms[2],flushes=rx_flushes[2];
    for (i=0; i<n; i++) {
        ep2rx[4*i] = l; ep2rx[4*i+1] = (uint16_t)l >> 8;
        ep2rx[4*i+2] = r; ep2rx[4*i+3] = (uint16_t)r >> 8;
    }
    hw_ep[2][20] = 1; hw_ep[2][22] = n*4; hw_ep[2][23] = (n*4) >> 8;
    up_service();
    assert(rx_rearms[2]==rearms+1 && rx_flushes[2]==flushes+1);
    assert(!(hw_ep[2][20]&1u) && hw_ep[2][21]==0x40);
}

static void capture_pcm_rates(void)
{
    uint32_t rate,i,j,nonzero;
    usb.config=usb.up=1;
    for(rate=44100;rate<=48000;rate+=3900) {
        setup(1,11,1,2,0);
        setup(0x22,1,0x0100,0x84,3);data_rate(rate);
        assert(!hw_stall && uac.r48==(rate==48000) && !uac.go);
        nonzero=0;
        for(i=0;i<120;i++) {
            hw_ep[4][17]=0;uac_service();
            if(i%6==0)fm1_alnk0_irq();
            for(j=0;j<sent_bytes[4]/4;j++) {
                const uint8_t *p=(const uint8_t *)tx_dma[4]+4*j;
                int16_t l=(int16_t)(p[0]|(uint16_t)p[1]<<8),r=(int16_t)(p[2]|(uint16_t)p[3]<<8);
                if(i>30 && (l || r)){assert(abs(l-1000)<=2 && abs(r+2000)<=2);nonzero++;}
            }
            assert(sent_bytes[4]>=(rate==48000?188u:172u) && sent_bytes[4]<=(rate==48000?196u:184u));
        }
        assert(nonzero>1000);
        setup(1,11,0,2,0);assert(!uac.alt);
    }
    setup(0x22,1,0x0100,0x84,3);data_rate(96000);assert(hw_stall && uac.r48);
    setup(0x22,1,0x0100,0x84,2);assert(hw_stall);
    puts("PASS: actual ISR/capture packets carry stereo PCM after cold start, reopen and 44.1/48k rate change");
}

static void controls_and_midi(void)
{
    uint32_t i;
    usb.up = 1;
    setup(0, 9, 1, 0, 0);
    assert(usb.config && hw_enabled == 0x16);
    assert(rx_dma[2] == ep2rx && tx_dma[2] == ep2feedback && hw_ep[2][21] == 0x40);
    assert(hw_ep[2][19] == 0xff && hw_ep[2][16] == 0xff); /* SDK USB0 convention */
    setup(1, 11, 1, 3, 0); assert(up.alt == 1 && !hw_stall);
    setup(0x81, 10, 0, 3, 1); assert(control_len == 1 && control_reply[0] == 1);
    setup(1, 11, 2, 3, 0); assert(hw_stall && up.alt == 1);
    setup(0x22, 1, 0x0100, 2, 3); assert(uac.e0_rx == 2); data_rate(44100); assert(!hw_stall);
    setup(0x22, 1, 0x0100, 2, 3); data_rate(48000); assert(hw_stall && !uac.r48);
    setup(0x22, 1, 0x0100, 0x84, 3); data_rate(48000); assert(!hw_stall && uac.r48);
    setup(0xa2, 0x81, 0x0100, 2, 3); assert(control_reply[0] == 0x44 && control_reply[1] == 0xac);
    setup(0xa2, 0x81, 0x0100, 0x84, 3); assert(control_reply[0] == 0x80 && control_reply[1] == 0xbb);
    setup(0xa2, 0x81, 0x0200, 2, 3); assert(hw_stall);
    setup(1, 11, 1, 2, 0); assert(uac.alt == 1 && up.alt == 1);
    {uint32_t w=up.w,errors=up.status_errors,flushes=rx_flushes[2],rearms=rx_rearms[2];
        hw_ep[2][20]=0x0d;hw_ep[2][22]=176;hw_ep[2][23]=0;up_service();
        assert(up.w==w && up.status_errors==errors+1);
        assert(rx_flushes[2]==flushes+1 && rx_rearms[2]==rearms+1 && !hw_ep[2][20]);
        packet(44,321,-654);assert(up.w==w+44);}
    for (i = 0; i < 50; i++) {
        uint8_t midi[8] = {9, 0x90, 60, 90, 8, 0x80, 60, 0};
        assert(ep1_take(midi, 8));
        assert(mi_w - mi_r == 2); mi_r = mi_w;
        midi_out_event(0x643c9009u); ep1_tx();
        assert(sent_bytes[1] == 4); hw_ep[1][17] = 0;
        packet(44, 1234, -2345);
        if (i % 6 == 5) { int32_t out[512] = {0}; up_mix(out, 256, 4096); }
        uac_render_start();
        { int32_t synth[64] = {0}; uac_tap(synth, 32); }
        hw_ep[4][17] = 0; uac_service();
        assert(sent_bytes[4] >= 176 && sent_bytes[4] <= 196);
    }
    assert(sent_bytes[2] == 3);
    assert((ep2feedback[0] | (uint32_t)ep2feedback[1] << 8 | (uint32_t)ep2feedback[2] << 16) == UP_NOMINAL);
    setup(1, 11, 0, 3, 0); assert(!up.alt && up.w == up.r && uac.alt == 1);
    setup(1, 11, 1, 3, 0); packet(45, 1, 2);
    hw_ep[2][20] = 1; hw_ep[2][22] = 180; /* stale DMA must not survive suspend */
    hw_global[6] = 1; usb_poll(); assert(up.w == up.r && up.alt == 1); /* suspend */
    hw_global[6] = 4; usb_poll(); assert(!up.alt && !uac.alt && !usb.config);
    setup(0, 9, 1, 0, 0); setup(1, 11, 1, 3, 0); packet(45, 3, 4);
    setup(0, 9, 0, 0, 0); assert(!up.alt && !usb.config);
}

static void actual_render_routing(void)
{
    uint32_t i, capture_start;
    usb.config = usb.up = 1;
    up_stream(1);
    for (i=0; i<12; i++) packet(45, 12000, 4000);
    uac_rate_set(44100); uac_stream(1); uac.flowing = 1;
    capture_start = ua_r + UA_PRIME;
    master_q12 = 4096;
    fm1_alnk0_irq();
    assert(!in_audio);
    for (i=0; i<256; i++) {
        uint32_t frame = ua_ring[(capture_start+i) & (UA_N-1u)];
        assert((int16_t)frame == 1000 && (int16_t)(frame >> 16) == -2000);
        assert(abuf[2*i] == 13000*128 && abuf[2*i+1] == 2000*128);
    }
    up_stream(0); uac.flowing = 0;
    fm1_alnk0_irq(); assert(abuf[0] == 1000*128 && abuf[1] == -2000*128);
    puts("PASS: actual audio ISR keeps USB capture synth-only and mixes stereo host audio to DAC");
}

static void samples_and_bounds(void)
{
    int32_t out[512];
    uint32_t i, w;
    usb.config = usb.up = 1; up_stream(1);
    memset(out, 0, sizeof out);
    packet(45, 12000, -14000); up_mix(out, 256, 4096); assert(out[0] == 0 && up.r == 0);
    for (i=0; i<11; i++) packet(45, 12000, -14000);
    up_mix(out, 256, 2048);
    for (i=0; i<256; i++) assert(out[2*i] == 6000 && out[2*i+1] == -7000);
    memset(out, 0, sizeof out); up_mix(out, 256, 4096); assert(up.r == 512);
    memset(out, 0, sizeof out); up_mix(out, 256, 4096);
    assert(up.underruns && !up.running && up.r == up.w && out[0] == 0);
    w = up.w;
    up_receive(ep2rx, 181); up_receive(ep2rx, 3); up_receive(ep2rx, 2048);
    assert(up.w == w && up.malformed >= 3);
    up_stream(1);
    for (i=0; i<22; i++) packet(45, 32767, -32768);
    w = up.w; packet(45, 1, 2); assert(up.w == w && up.overflows);
    for (i=0; i<256; i++) { out[2*i] = 20000; out[2*i+1] = -20000; }
    up_mix(out, 256, 4096); assert(out[0] == 32767 && out[1] == -32767);
    up_stream(1); up.w = up.r = UINT32_MAX-20u; /* counter wrap */
    for (i=0; i<12; i++) packet(45, -1000, 2000);
    memset(out, 0, sizeof out); up_mix(out, 256, 4096);
    assert(out[0] == -1000 && out[1] == 2000 && up.w-up.r == 284);
    up_stream(0); memset(out, 0, sizeof out); up_mix(out, 256, 4096); assert(!out[0]);
}

/* 60 seconds per case, independent DAC clock, packet quantisation and a host
 * that updates its sending rate only every 16 USB frames (one feedback refresh).
 * Fractional scheduling is integer, in 1/16384 frame units. */
static void clock_drift(int32_t ppm)
{
    uint32_t ms, send_acc=0, dac_acc=0, host_rate=UP_NOMINAL;
    uint32_t rate = (uint32_t)((int32_t)UP_NOMINAL + (int32_t)UP_NOMINAL * ppm / 1000000);
    uint32_t underruns = up.underruns, overflows = up.overflows;
    int32_t out[512];
    up_stream(1);
    for (ms=0; ms<60000; ms++) {
        uint32_t frames;
        if (!(ms & 15)) host_rate = up.feedback;
        send_acc += host_rate; frames = send_acc >> 14; send_acc &= 16383;
        assert(frames <= UP_MAX_FRAMES); packet(frames, 1200, -3400);
        hw_ep[2][17] = 0;
        dac_acc += rate;
        if (dac_acc >= 256u*16384u) {
            dac_acc -= 256u*16384u;
            memset(out, 0, sizeof out); up_mix(out, 256, 4096);
        }
        assert(up.w-up.r <= UP_N);
    }
    assert(up.running && up.underruns == underruns && up.overflows == overflows);
    assert(up.fill_min >= 300 && up.fill_max < 750);
    printf("PASS: simulated DAC drift %+d ppm, fill %u..%u, no under/overflow in 60 s\n", ppm, up.fill_min, up.fill_max);
}
#endif

int main(void)
{
    descriptors();
#if FM1_USB_PLAYBACK
    controls_and_midi(); samples_and_bounds();
    actual_render_routing();
    capture_pcm_rates();
    clock_drift(0); clock_drift(400); clock_drift(-400); clock_drift(1000); clock_drift(-1000);
    puts("PASS: actual USB driver descriptors, rates, stream lifecycle, PCM stereo, bounds and concurrent capture/MIDI");
#else
    puts("PASS: capture/MIDI-only configuration remains valid");
#endif
    return 0;
}

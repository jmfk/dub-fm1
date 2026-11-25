/* SPDX-License-Identifier: GPL-3.0-only
 * Actual eight-part engine + staged R5 USB driver + actual hardware ISR.
 * Register model only; no host MIDI/USB APIs and no flash operations. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define DINGHY_HOST 1
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define FELUCCA_CDC 0
#define FELUCCA_UAC 1
#define FELUCCA_OTA 0
#define HALF_FRAMES 256u
#define FM1_USB_HAL "usb_mock.h"
#define FM1_USB_PLAYBACK 1
#include "usb.c"
#include "engine.h"
#include "diagnostic.h"
#define HALF_WORDS 512u
#define FM1_AUDIO_HALF 0x80u
#define UAC_ONLY(x) x
static int32_t abuf[1024], master_q12 = 4096;
static uint8_t in_audio;
static uint32_t panel_fault;
static uint8_t fm1_audio_pending(void) { return FM1_AUDIO_HALF; }
static void fm1_audio_ack_aux(uint8_t p) { (void)p; }
static uint32_t fm1_audio_free_half(void) { return 0; }
static void fm1_audio_ack_half(void) {}
static uint32_t cpu_last,cpu_peak,cpu_late,synth_usb_started,fake_ticks,tick_step=5000;
static uint32_t fm1_ticks(void){fake_ticks+=tick_step;return fake_ticks;}
static uint32_t fm1_ms;
#include "diagnostic_runtime.h"
#include "synth_timing.h"
#define APP_AUDIO_BEGIN synth_usb_audio_begin
#define APP_AUDIO_END synth_usb_audio_end
#include "synth_audio.h"
#include "render_irq.h"
static void prime_host(int enabled, int16_t l, int16_t r) {
    up_stream(enabled);
    usb.config = usb.up = 1;
    if (!enabled)
        return;
    uint8_t p[180];
    for (uint32_t i = 0; i < 45; i++) {
        p[4 * i] = (uint8_t)l;
        p[4 * i + 1] = (uint16_t)l >> 8;
        p[4 * i + 2] = (uint8_t)r;
        p[4 * i + 3] = (uint16_t)r >> 8;
    }
    for (int i = 0; i < 12; i++)
        up_receive(p, 180);
}
static int16_t capture[2][120 * UA_MAXF * 2], dac[2][120 * 256 * 2];
static uint32_t capn[2], dacn[2], channels[2][8];
static void from_usb(uint32_t channel, uint32_t note, int on) {
    uint8_t event[4] = {(uint8_t)(on ? 9 : 8), (uint8_t)((on ? 0x90 : 0x80) | channel), (uint8_t)note,
                        (uint8_t)(on ? 100 : 0)};
    assert(ep1_take(event, 4));
    assert(mi_w - mi_r == 1);
    uint32_t pk = midi_in_q[mi_r++ % MQ];
    a_send(A_MIDI, 0, (pk >> 8) & 255, (pk >> 16) & 127, (pk >> 24) & 127, 1);
}
static void run_case(uint32_t rate, int host) {
    a_init();
    prime_host(host, 1200, -1700);
    uac_rate_set(rate);
    uac_stream(1);
    capn[host] = dacn[host] = 0;
    memset(channels[host], 0, sizeof channels[host]);
    for (uint32_t ms = 0; ms < 120; ms++) {
        /* Actual USB-MIDI parser -> application queue -> each synth's audio. */
        if (ms == 8)
            for (uint32_t c = 0; c < 8; c++)
                from_usb(c, 48 + c, 1);
        if (ms == 90)
            for (uint32_t c = 0; c < 8; c++)
                from_usb(c, 48 + c, 0);
        hw_ep[4][17] = 0;
        sent_bytes[4] = 0;
        uac_service();
        for (uint32_t i = 0; i < sent_bytes[4] / 2; i++) {
            assert(capn[host] < sizeof capture[host] / sizeof capture[host][0]);
            const uint8_t *p = (const uint8_t *)tx_dma[4] + 2 * i;
            capture[host][capn[host]++] = (int16_t)(p[0] | (uint16_t)p[1] << 8);
        }
        if (ms % 6 == 0) {
            if (host && ms > 0) {
                uint8_t p[180];
                for (uint32_t i = 0; i < 45; i++) {
                    p[4 * i] = 1200 & 255;
                    p[4 * i + 1] = 1200 >> 8;
                    p[4 * i + 2] = (uint16_t)-1700 & 255;
                    p[4 * i + 3] = (uint16_t)-1700 >> 8;
                }
                for (int j = 0; j < 6; j++)
                    up_receive(p, 176);
            }
            fm1_alnk0_irq();
            assert(!in_audio);
            for (uint32_t i = 0; i < 512; i++)
                dac[host][dacn[host]++] = (int16_t)(abuf[i] / 128);
        }
    }
    for (uint32_t c = 0; c < 8; c++) {
        channels[host][c] = ap[c].note_ons;
        assert(channels[host][c] == 1);
        assert(!ap[c].voice[0].gate && !ap[c].voice[1].gate);
    }
    assert(a_all_active_peak >= 8);
}
static void test_route(uint32_t rate) {
    /* Identical initial engine state: drift is deterministic within each comparison. */
    arng = 0x723aa185u;
    run_case(rate, 0);
    arng = 0x723aa185u;
    run_case(rate, 1);
    assert(capn[0] == capn[1] && dacn[0] == dacn[1]);
    assert(!memcmp(capture[0], capture[1], capn[0] * 2u));
    uint32_t energy = 0, difference = 0;
    for (uint32_t i = 0; i < capn[0]; i++)
        if (capture[0][i])
            energy++;
    for (uint32_t i = 0; i < dacn[0]; i++)
        if (dac[0][i] != dac[1][i])
            difference++;
    assert(energy > 1000 && difference > 1000);
    printf("PASS: actual eight-part synth + ISR, %uHz capture is byte-identical with/without host playback; "
           "analogue differs, MIDI1–8 sounds and note-offs retain ownership\n",
           rate);
}
int main(void) {
    uint32_t before=ad_render_blocks;
    ad_ready=0;fm1_alnk0_irq();
    assert(ad_render_blocks==before && !cpu_late && !panel_fault);
    puts("PASS: actual ISR before initialization neither renders synth nor latches overload");
    ad_ready=1;
    test_route(44100);
    test_route(48000);
    /* Channel hard stop and sustain while both USB directions are open. */
    a_init();
    prime_host(1, 1200, -1700);
    uac_rate_set(44100);
    uac_stream(1);
    from_usb(3, 60, 1);
    fm1_alnk0_irq();
    a_send(A_MIDI, 0, 0xb3, 64, 127, 1);
    from_usb(3, 60, 0);
    fm1_alnk0_irq();
    assert(ap[3].voice[0].gate);
    a_send(A_MIDI, 0, 0xb3, 64, 0, 1);
    fm1_alnk0_irq();
    assert(!ap[3].voice[0].gate);
    a_send(A_MIDI, 0, 0xb3, 120, 0, 1);
    fm1_alnk0_irq();
    assert(!ap[3].voice[0].amp);
    puts("PASS: MIDI sustain and hard-stop semantics retained during duplex audio");
    assert(cpu_last==5000 && !cpu_late && !panel_fault);
    tick_step=200000;fm1_alnk0_irq();
    tick_step=5000;fm1_alnk0_irq();
    assert(cpu_late==1 && ad_consecutive==0 && !panel_fault);
    cpu_late=0;tick_step=200000;
    fm1_alnk0_irq();fm1_alnk0_irq();fm1_alnk0_irq();
    assert(cpu_last==200000 && cpu_peak==200000 && cpu_late==3 && panel_fault);
    puts("PASS: actual whole-ISR timing hooks include USB routing and stop synth after three deadline overruns");
    return 0;
}

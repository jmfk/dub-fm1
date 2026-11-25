/* SPDX-License-Identifier: GPL-3.0-only; focused offline behavioral and DSP checks. */
#include "engine.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void blocks(int count) {
    while (count--) {
        int32_t x[512] = {0};
        a_render(x, 256);
        for (int i = 0; i < 512; i++)
            assert(x[i] >= -30000 && x[i] <= 30000);
    }
}
static void midi(int st, int n, int v) {
    a_send(A_MIDI, 0, st, n, v, 1);
    blocks(1);
}
static int gates(int t) { return ap[t].voice[0].gate + ap[t].voice[1].gate; }
static int64_t energy(int ch, int slope, int effect) {
    a_init();
    ap[ch].p[P_SLOPE] = slope;
    ap[ch].p[P_INSERT] = effect;
    ap[ch].p[P_IWET] = 100;
    a_note_on(ch, 60, 110, 1);
    int64_t sum = 0;
    for (int b = 0; b < 80; b++) {
        int32_t x[512] = {0};
        a_render(x, 256);
        for (int i = 0; i < 512; i++)
            sum += (int64_t)x[i] * x[i];
    }
    return sum;
}
int main(void) {
    a_init();
    for (int t = 0; t < 8; t++)
        midi(0x90 + t, 48 + t, 100);
    for (int t = 0; t < 8; t++) {
        assert(ap[t].note_ons == 1);
        assert(gates(t) == 1);
    }
    assert(a_active == 8);
    midi(0xb2, 64, 127);
    midi(0x82, 50, 0);
    assert(gates(2) == 1);
    midi(0xb2, 64, 0);
    assert(gates(2) == 0);
    assert(gates(1) == 1);
    midi(0xe3, 127, 127);
    assert(ap[3].bend == 8191);
    assert(!ap[0].bend);
    midi(0xb3, 1, 99);
    assert(ap[3].mod == 99 * 258);
    midi(0xb3, 74, 17);
    assert(ap[3].p[P_CUT] == 17);
    midi(0xb3, 101, 0);
    midi(0xb3, 100, 0);
    midi(0xb3, 6, 12);
    assert(ap[3].p[P_BEND] == 12);
    midi(0xb3, 99, 0);
    midi(0xb3, 98, P_FDRIVE);
    midi(0xb3, 6, 70);
    assert(ap[3].p[P_FDRIVE] == 70);
    midi(0xb3, 99, 2);
    midi(0xb3, 98, 0);
    midi(0xb3, 6, S_LFO2);
    assert(ap[3].route[0].source == S_LFO2);
    midi(0xb3, 98, 1);
    midi(0xb3, 6, D_DELAYTIME);
    assert(ap[3].route[0].dest == D_DELAYTIME);
    midi(0xb3, 98, 2);
    midi(0xb3, 6, 0);
    midi(0xb3, 38, 0);
    assert(ap[3].route[0].depth == -127);
    midi(0xb1, 66, 127);
    midi(0x81, 49, 0);
    assert(gates(1) == 1);
    midi(0xb1, 66, 0);
    assert(!gates(1));
    midi(0xb0, 123, 0);
    assert(!gates(0));
    midi(0xb4, 120, 0);
    assert(ap[4].voice[0].amp == 0);
    a_init();
    a_note_on(0, 60, 100, 2);
    a_note_on(1, 60, 100, 1);
    a_note_off(0, 60, 2);
    assert(!gates(0) && gates(1)); /* note-off owns original part */
    a_init();
    a_note_on(1, 60, 100, 1);
    blocks(8);
    uint32_t phase = ap[1].voice[0].phase[0];
    a_note_off(1, 60, 1);
    blocks(300);
    a_note_on(1, 60, 100, 1);
    assert(ap[1].voice[0].phase[0] != phase);
    assert(ap[1].voice[0].phase[0] != 0);
    a_init();
    ap[0].p[P_ALOOP] = 1;
    ap[0].p[P_ATK] = 0;
    ap[0].p[P_DEC] = 0;
    a_note_on(0, 40, 100, 1);
    blocks(30);
    assert(ap[0].voice[0].env[0].stage < 3);
    a_note_off(0, 40, 1);
    blocks(800);
    assert(!ap[0].voice[0].amp);
    a_init();
    uint32_t p = ap[1].phase[0];
    blocks(2);
    assert(ap[1].phase[0] != p);
    ap[1].p[P_L1RESET] = 1;
    a_note_on(1, 60, 100, 1);
    assert(!ap[1].voice[0].lfo_phase[0]);
    for (int fx = 0; fx < 8; fx++) {
        int64_t e = energy(3, 0, fx);
        assert(e > 1000);
    }
    assert(energy(3, 0, 0) != energy(3, 1, 0));
    a_init();
    for (int i = 0; i < 400; i++)
        a_send(A_MIDI, 0, 0x90, 60, 100, 1);
    assert(a_queue_drops > 0);
    blocks(1);
    assert(!gates(0));
    a_init();
    a_trigger();
    int32_t g = a_duck_gain(0, 2, 100);
    assert(g < 32767);
    for (int i = 0; i < 8000; i++)
        g = a_duck_gain(0, 2, 100);
    assert(g == 32767);
    /* Max drive/resonance, all inserts, high feedback, 24dB and MIDI fuzz. */
    a_init();
    for (int t = 0; t < 8; t++) {
        ap[t].p[P_SLOPE] = 1;
        ap[t].p[P_RES] = 127;
        ap[t].p[P_FDRIVE] = 127;
        ap[t].p[P_INSERT] = t;
        ap[t].p[P_ALOOP] = 1;
        a_note_on(t, 36 + t * 9, 127, 1);
        a_note_on(t, 38 + t * 9, 127, 1);
    }
    blocks(300);
    a_init();
    a_transport(1);
    blocks(12000);
    assert(!a_song_play);
    assert(a_song_pos == A_SONG_COUNT);
    assert(a_all_active_peak >= 8);
    assert(a_drum_hits > 0);
    for (int t = 0; t < 8; t++)
        assert(ap[t].note_ons > 20);
    printf("PASS: channels, pedals, RPN/NRPN, note ownership, free phase, loop/LFO, inserts, 24dB, overflow, "
           "duck, full 32-bar score. peak voices=%u ceiling=%u\n",
           a_all_active_peak, a_clamps);
    return 0;
}

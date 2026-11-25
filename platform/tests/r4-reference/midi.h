/* SPDX-License-Identifier: GPL-3.0-only; explicit MIDI 1 channel semantics. */
#pragma once
static void a_set_param(a_part_t *t, uint32_t p, int v) {
    if (p < P_COUNT)
        t->p[p] = (int16_t)ac(v, 0, p_max[p]);
}
static void a_cc_param(a_part_t *t, uint32_t p, uint32_t v) {
    a_set_param(t, p, (int)(v * (uint32_t)p_max[p] + 63u) / 127);
}
static void a_data(a_part_t *t) {
    uint32_t nr = (uint32_t)t->nrpn_m * 128u + t->nrpn_l, rpn = (uint32_t)t->rpn_m * 128u + t->rpn_l,
             v = (uint32_t)t->data_m * 128u + t->data_l;
    if (nr < 16383) {
        if (nr < P_COUNT)
            a_cc_param(t, nr, t->data_m);
        else if (nr >= 256 && nr < 280) {
            uint32_t slot = (nr - 256) / 3, field = (nr - 256) % 3;
            a_route_t *r = &t->route[slot];
            if (field == 0)
                r->source = (uint8_t)ac(t->data_m, 0, S_COUNT - 1);
            if (field == 1)
                r->dest = (uint8_t)ac(t->data_m, 0, D_COUNT - 1);
            if (field == 2)
                r->depth = (int16_t)ac(((int32_t)v - 8192) * 127 / (v < 8192u ? 8192 : 8191), -127, 127);
        }
    } else if (rpn == 0)
        t->p[P_BEND] = (int16_t)ac(t->data_m, 0, 24);
    else if (rpn == 1)
        t->fine = ((int32_t)v - 8192) / 32;
    else if (rpn == 2)
        t->coarse = ((int32_t)t->data_m - 64) * 256;
}
static void a_midi(uint32_t st, uint32_t d1, uint32_t d2, uint32_t source) {
    if (st == 0xf8) {
        if (a_clock_last) {
            uint32_t delta = a_frame - a_clock_last;
            if (delta > 300u && delta < 20000u) {
                a_clock_period = a_clock_period ? (a_clock_period * 7 + delta) / 8 : delta;
                ag[G_TEMPO] = (int16_t)ac((int32_t)(A_FS * 60u / (a_clock_period * 24u)), 30, 240);
            }
        }
        a_clock_last = a_frame;
        a_external = 1;
        a_clock_count++;
        return;
    }
    if (st == 0xf2) {
        a_song_tick = (d1 + d2 * 128u) * 24u;
        a_song_pos = 0;
        while (a_song_pos < A_SONG_COUNT && a_song[a_song_pos].tick < a_song_tick)
            a_song_pos++;
        return;
    }
    if (st == 0xfa) {
        a_transport(1);
        return;
    }
    if (st == 0xfb) {
        a_transport(2);
        return;
    }
    if (st == 0xfc) {
        a_transport(0);
        return;
    }
    if (st == 0xff) {
        a_transport(0);
        a_panic(-1, 1);
        return;
    }
    uint32_t ch = st & 15u, type = st & 0xf0u;
    d1 &= 127;
    d2 &= 127;
    if (ch == 9) {
        if (type == 0x90 && d2)
            a_drum_on(d1, d2);
        return;
    }
    if (ch == 15 && type == 0xb0) {
        if (d1 >= 20 && d1 < 20 + G_COUNT) {
            uint32_t g = d1 - 20;
            ag[g] = (int16_t)(g == G_TEMPO ? 30 + d2 * 210 / 127 : d2 * (uint32_t)g_max[g] / 127);
        }
        return;
    }
    if (ch >= 8)
        return;
    a_part_t *t = &ap[ch];
    if (type == 0x90 && d2) {
        a_note_on(ch, d1, d2, source);
        return;
    }
    if (type == 0x80 || (type == 0x90 && !d2)) {
        a_note_off(ch, d1, source);
        return;
    }
    if (type == 0xe0) {
        t->bend = (int32_t)(d1 + d2 * 128) - 8192;
        return;
    }
    if (type == 0xd0) {
        t->pressure = (int32_t)d1 * 258;
        return;
    }
    if (type == 0xa0) {
        for (uint32_t i = 0; i < 2; i++)
            if (t->voice[i].note == d1)
                t->voice[i].pressure = (int32_t)d2 * 258;
        return;
    }
    if (type == 0xc0) {
        a_patch(ch, d1);
        return;
    }
    if (type != 0xb0)
        return;
    switch (d1) {
    case 0:
    case 32:
        break; /* one eight-program bank; bank select accepted */
    case 1:
        t->mod = (int32_t)d2 * 258;
        break;
    case 2:
        t->breath = (int32_t)d2 * 258;
        break;
    case 5:
        a_cc_param(t, P_GLIDE, d2);
        break;
    case 7:
        a_cc_param(t, P_LEVEL, d2);
        break;
    case 10:
        a_cc_param(t, P_PAN, d2);
        break;
    case 11:
        t->expression = (int32_t)d2 * 258;
        break;
    case 16:
        t->cc16 = (int32_t)d2 * 258;
        break;
    case 17:
        t->cc17 = (int32_t)d2 * 258;
        break;
    case 64:
        t->sustain = d2 >= 64;
        if (!t->sustain)
            for (uint32_t i = 0; i < 2; i++)
                if (!t->voice[i].held && !t->voice[i].sost)
                    t->voice[i].gate = 0;
        break;
    case 66:
        if (d2 >= 64 && !t->sostenuto) {
            for (uint32_t i = 0; i < 2; i++)
                t->voice[i].sost = t->voice[i].held;
        } else if (d2 < 64) {
            for (uint32_t i = 0; i < 2; i++) {
                t->voice[i].sost = 0;
                if (!t->voice[i].held && !t->sustain)
                    t->voice[i].gate = 0;
            }
        }
        t->sostenuto = d2 >= 64;
        break;
    case 65:
        if (d2 < 64)
            t->p[P_GLIDE] = 0;
        else if (!t->p[P_GLIDE])
            t->p[P_GLIDE] = 24;
        break;
    case 67:
        a_cc_param(t, P_CUT, 127 - d2 / 2);
        break;
    case 70:
        a_cc_param(t, P_WAVE1, d2);
        break;
    case 71:
        a_cc_param(t, P_RES, d2);
        break;
    case 72:
        a_cc_param(t, P_REL, d2);
        break;
    case 73:
        a_cc_param(t, P_ATK, d2);
        break;
    case 74:
        a_cc_param(t, P_CUT, d2);
        break;
    case 75:
        a_cc_param(t, P_DEC, d2);
        break;
    case 76:
        a_cc_param(t, P_L1RATE, d2);
        break;
    case 77:
        a_cc_param(t, P_DETUNE, d2);
        break;
    case 78:
        a_cc_param(t, P_L2RATE, d2);
        break;
    case 79:
        a_cc_param(t, P_FAMT, d2);
        break;
    case 80:
        a_cc_param(t, P_SLOPE, d2);
        break;
    case 81:
        a_cc_param(t, P_FDRIVE, d2);
        break;
    case 82:
        a_cc_param(t, P_ADRIVE, d2);
        break;
    case 83:
        a_cc_param(t, P_PWM, d2);
        break;
    case 84:
        a_cc_param(t, P_MIX, d2);
        break;
    case 85:
        a_cc_param(t, P_WAVE2, d2);
        break;
    case 86:
        a_cc_param(t, P_INSERT, d2);
        break;
    case 87:
        a_cc_param(t, P_IAMT, d2);
        break;
    case 88:
        a_cc_param(t, P_IWET, d2);
        break;
    case 89:
        a_cc_param(t, P_DUCK, d2);
        break;
    case 91:
        a_cc_param(t, P_REV, d2);
        break;
    case 92:
        a_cc_param(t, P_FDRIVE, d2);
        break;
    case 93:
        a_cc_param(t, P_CHO, d2);
        break;
    case 94:
        a_cc_param(t, P_DLY, d2);
        break;
    case 95:
        a_cc_param(t, P_BUS, d2);
        break;
    case 99:
        t->nrpn_m = (uint8_t)d2;
        t->rpn_m = t->rpn_l = 127;
        break;
    case 98:
        t->nrpn_l = (uint8_t)d2;
        t->rpn_m = t->rpn_l = 127;
        break;
    case 101:
        t->rpn_m = (uint8_t)d2;
        t->nrpn_m = t->nrpn_l = 127;
        break;
    case 100:
        t->rpn_l = (uint8_t)d2;
        t->nrpn_m = t->nrpn_l = 127;
        break;
    case 6:
        t->data_m = (uint8_t)d2;
        a_data(t);
        break;
    case 38:
        t->data_l = (uint8_t)d2;
        a_data(t);
        break;
    case 96:
        if (t->data_m < 127)
            t->data_m++;
        a_data(t);
        break;
    case 97:
        if (t->data_m)
            t->data_m--;
        a_data(t);
        break;
    case 120:
        a_panic((int)ch, 1);
        break;
    case 121:
        t->bend = t->mod = t->pressure = t->breath = t->cc16 = t->cc17 = 0;
        t->expression = 32767;
        t->sustain = t->sostenuto = 0;
        t->rpn_m = t->rpn_l = t->nrpn_m = t->nrpn_l = 127;
        for (uint32_t i = 0; i < 2; i++) {
            t->voice[i].sost = 0;
            if (!t->voice[i].held)
                t->voice[i].gate = 0;
        }
        break;
    case 123:
    case 124:
    case 125:
    case 126:
    case 127:
        for (uint32_t i = 0; i < 2; i++) {
            t->voice[i].held = 0;
            if (!t->sustain && !t->voice[i].sost)
                t->voice[i].gate = 0;
        }
        if (d1 == 126)
            t->p[P_MONO] = 1;
        if (d1 == 127)
            t->p[P_MONO] = 0;
        break;
    default:
        break; /* Undefined controls have no invented vendor semantics. */
    }
}

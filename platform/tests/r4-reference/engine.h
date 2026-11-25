/* SPDX-License-Identifier: GPL-3.0-only
 * Analog Test R1. Integer-only portable engine, audio-thread ownership. */
#pragma once
#include "params.h"
#include "song.h"
#include "tables.h"
#include <stdint.h>
#if defined(__APPLE__) && defined(__MACH__)
#define A_POOL __attribute__((section("__DATA,__apool")))
#else
#define A_POOL __attribute__((section(".pool")))
#endif
#define A_PARTS 8
#define A_VOICES 2
#define A_CTL 32u
#define A_FS 44100u
static int32_t ac(int32_t x, int32_t l, int32_t h) { return x < l ? l : x > h ? h : x; }
static int32_t aa(int32_t x) { return x < 0 ? -x : x; }
static int32_t am(int32_t a, int32_t b) { return (int32_t)(((int64_t)a * b) >> 15); }
static int32_t aq(int32_t a, int32_t b) { return (int32_t)(((int64_t)a * b) >> 24); }
static void az(void *p, uint32_t n) {
    uint8_t *d = p;
    while (n--)
        *d++ = 0;
}
static int32_t ashape(int32_t x) {
    int32_t a = aa(x);
    if (a >= 32768)
        return x < 0 ? -32767 : 32767;
    return (int32_t)(((int64_t)x * (65536 - a)) >> 15);
}
static int32_t adrive(int32_t x, int32_t d) { return d ? ashape(ac(x + ((x * d) >> 5), -65535, 65535)) : x; }
static uint32_t arng = 0x723aa185u;
static uint32_t arand(void) {
    uint32_t x = arng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return arng = x;
}
typedef struct {
    int32_t value;
    uint8_t stage;
} a_env_t;
typedef struct {
    int32_t s1, s2, c[3];
} a_filter_t;
typedef struct {
    uint32_t phase[2], inc[2], last_frame, age;
    int32_t pitch, amp, last, steal, drift, drift_to;
    uint32_t lfo_phase[2];
    int32_t held_random[2];
    a_env_t env[2];
    a_filter_t f[2];
    uint8_t note, velocity, source, gate, held, sost;
    int32_t pressure;
    int16_t component;
} a_voice_t;
typedef struct {
    uint8_t source, dest;
    int16_t depth;
} a_route_t;
typedef struct {
    int32_t line[4], fb, dc, last;
    uint32_t phase, write;
    int16_t delay[2048];
} a_insert_t;
typedef struct {
    int16_t p[P_COUNT];
    a_voice_t voice[A_VOICES];
    a_route_t route[8];
    int32_t bend, mod, pressure, breath, expression, cc16, cc17, fine, coarse;
    uint8_t sustain, sostenuto, rpn_m, rpn_l, nrpn_m, nrpn_l, data_m, data_l, program;
    uint32_t phase[2];
    int32_t random[2], last_env[2], last_vel, last_key, drift;
    a_insert_t insert;
    int32_t gain, pan, peak;
    uint32_t note_ons;
} a_part_t;
static a_part_t ap[A_PARTS];
static int16_t ag[G_COUNT];
static uint32_t a_frame, a_age, a_active, a_peak, a_clamps, a_song_pos, a_song_tick, a_song_rem, a_song_play;
static uint32_t a_clock_last, a_clock_period, a_clock_count, a_external;
static uint32_t a_queue_drops, a_panic_request, a_guard_trips;
static uint32_t a_drum_hits, a_all_active_peak;

static int32_t a_bus_mod[D_COUNT], a_track_buf[9][A_CTL];
static int32_t a_part_mod[8][D_COUNT];
static int16_t a_wave_ram[4][8][256] A_POOL;
static int32_t a_fc_ram[128][16][3] A_POOL;
typedef struct {
    uint8_t op, ch, a, b;
    int32_t value;
    uint8_t source;
} a_cmd_t;
static a_cmd_t a_queue[256];
static volatile uint32_t a_qw, a_qr;
enum { A_MIDI = 1, A_PARAM, A_GLOBAL, A_MATRIX, A_PLAY, A_PANIC, A_PRESET };
static int a_send(uint32_t op, uint32_t ch, uint32_t a, uint32_t b, int32_t v, uint32_t source) {
    uint32_t w = a_qw;
    if (w - a_qr >= 256u) {
        a_queue_drops++;
        a_panic_request = 1;
        return 0;
    }
    a_queue[w & 255u] = (a_cmd_t){(uint8_t)op, (uint8_t)ch, (uint8_t)a, (uint8_t)b, v, (uint8_t)source};
    __asm__ volatile("" ::: "memory");
    a_qw = w + 1u;
    return 1;
}
static uint32_t a_band(uint32_t inc) {
    uint32_t lev=0,h=128;
    while(lev<7u && (uint64_t)inc*h>=0x80000000u){lev++;h>>=1;}
    return lev;
}
static int32_t a_osc(uint32_t ph, uint32_t lev, uint32_t wave, int32_t pwm) {
    uint32_t i,j,frac;
    const int16_t *t;
    int32_t x;
    t = a_wave_ram[wave & 3u][lev];
    /* Band-limited pulse from the difference of two band-limited saws. */
    if (wave == 1u) {
        uint32_t shift = (uint32_t)ac(pwm, 12, 115) * 33554432u;
        int32_t y;
        i = ph >> 24;
        j = (i + 1u) & 255u;
        frac = (ph >> 9) & 32767u;
        t = a_wave_ram[0][lev];
        x = t[i] + am(t[j] - t[i], frac);
        ph += shift;
        i = ph >> 24;
        j = (i + 1u) & 255u;
        frac = (ph >> 9) & 32767u;
        y = t[i] + am(t[j] - t[i], frac);
        return (x - y) / 2;
    }
    i = ph >> 24;
    j = (i + 1u) & 255u;
    frac = (ph >> 9) & 32767u;
    x = t[i];
    return x + am(t[j] - x, frac);
}
static uint32_t a_pitch_inc(int32_t pitch) {
    uint32_t n, f;
    pitch = ac(pitch, 0, 127 * 256);
    n = (uint32_t)pitch >> 8;
    f = (uint32_t)pitch & 255u;
    return (uint32_t)(((uint64_t)a_inc[n] * a_frac[f]) >> 16);
}
static int32_t a_env_step(a_env_t *e, int gate, const int16_t *p, int which) {
    int atk = p[which ? P_FATK : P_ATK], dec = p[which ? P_FDEC : P_DEC], sus = p[which ? P_FSUS : P_SUS],
        rel = p[which ? P_FREL : P_REL], loop = p[which ? P_FLOOP : P_ALOOP];
    int32_t step, target;
    if (!gate && e->stage < 3)
        e->stage = 3;
    switch (e->stage) {
    case 0:
        step = 32767 * 32 / (a_times[atk] * 44 + 1);
        if (step < 1)
            step = 1;
        e->value += step;
        if (e->value >= 32767) {
            e->value = 32767;
            e->stage = 1;
        }
        break;
    case 1:
        target = loop ? 0 : sus * 258;
        step = 32767 * 32 / (a_times[dec] * 44 + 1);
        if (step < 1)
            step = 1;
        e->value -= step;
        if (e->value <= target) {
            e->value = target;
            e->stage = (uint8_t)(loop && gate ? 0 : 2);
        }
        break;
    case 2:
        break;
    case 3:
        step = 32767 * 32 / (a_times[rel] * 44 + 1);
        if (step < 1)
            step = 1;
        e->value -= step;
        if (e->value <= 0) {
            e->value = 0;
            e->stage = 4;
        }
        break;
    default:
        e->value = 0;
        break;
    }
    return e->value;
}
static int32_t a_lfo_value(uint32_t ph, uint32_t shape, int32_t random) {
    uint32_t u = ph >> 16;
    switch (shape) {
    case 1:
        return u < 32768 ? ((int32_t)u * 2 - 32767) : (98303 - (int32_t)u * 2);
    case 2:
        return (int32_t)u - 32768;
    case 3:
        return ph & 0x80000000u ? 32767 : -32767;
    case 4:
        return random;
    default:
        return a_sine[ph >> 24];
    }
}
static uint32_t a_lfo_inc(int rate, int sync) {
    static const uint16_t beats_q8[16] = {4096, 2048, 1024, 768, 512, 384, 256, 192,
                                          128,  96,   64,   48,  32,  24,  16,  8};
    if (!sync)
        return a_lfoinc[ac(rate, 0, 127)];
    return ((uint32_t)ag[G_TEMPO] * 13297156u) / beats_q8[(uint32_t)rate >> 3];
}
static int32_t a_filter(a_filter_t *f, int32_t x, int nonlinear) {
    int32_t v3 = x - f->s2, v1 = aq(f->c[0], f->s1) + aq(f->c[1], v3),
            v2 = f->s2 + aq(f->c[1], f->s1) + aq(f->c[2], v3);
    f->s1 = ac(2 * v1 - f->s1, -131071, 131071);
    f->s2 = ac(2 * v2 - f->s2, -131071, 131071);
    if (nonlinear) {
        f->s1 = ashape(ac(f->s1, -65535, 65535));
        f->s2 = ashape(ac(f->s2, -65535, 65535));
    }
    return ac(v2, -65535, 65535);
}
static const char *const a_patch_names[8] = {"ROUND BASS",  "AMBER STRINGS", "SLOW PAD",   "SINGING LEAD",
                                             "GLASS PLUCK", "CLOCK ARP",     "SOFT BRASS", "AIR TEXTURE"};
static void a_patch(uint32_t ch, uint32_t program) {
    a_part_t *t;
    if (ch >= 8)
        return;
    t = &ap[ch];
    uint32_t i;
    for (i = 0; i < P_COUNT; i++)
        t->p[i] = p_defaults[i];
    for (i = 0; i < 8; i++)
        t->route[i] = (a_route_t){0, 0, 0};
    program %= 8;
    t->program = (uint8_t)program;
    t->p[P_PAN] = ch == 0 ? 64 : 32 + (int)ch * 9;
    t->route[0] = (a_route_t){S_WHEELVIB, D_PITCH, 8};
    t->route[1] = (a_route_t){S_LFO1, D_PWM, 16};
    switch (program) {
    case 0:
        t->p[P_WAVE1] = 1;
        t->p[P_WAVE2] = 2;
        t->p[P_OCT2] = 1;
        t->p[P_CUT] = 48;
        t->p[P_FAMT] = 38;
        t->p[P_MONO] = 1;
        t->p[P_GLIDE] = 10;
        t->p[P_REV] = 0;
        t->p[P_DLY] = 0;
        t->p[P_FDRIVE] = 20;
        t->p[P_LEVEL] = 100;
        break;
    case 1:
        t->p[P_ATK] = 39;
        t->p[P_REL] = 54;
        t->p[P_CUT] = 73;
        t->p[P_FAMT] = 12;
        t->p[P_DETUNE] = 26;
        t->p[P_CHO] = 50;
        t->p[P_REV] = 45;
        t->p[P_LEVEL] = 70;
        break;
    case 2:
        t->p[P_ATK] = 66;
        t->p[P_REL] = 66;
        t->p[P_CUT] = 57;
        t->p[P_FATK] = 60;
        t->p[P_FDEC] = 64;
        t->p[P_FSUS] = 60;
        t->p[P_FAMT] = 24;
        t->p[P_CHO] = 38;
        t->p[P_REV] = 50;
        t->p[P_LEVEL] = 65;
        break;
    case 3:
        t->p[P_WAVE1] = 1;
        t->p[P_MONO] = 1;
        t->p[P_GLIDE] = 18;
        t->p[P_CUT] = 88;
        t->p[P_RES] = 34;
        t->p[P_FAMT] = 18;
        t->p[P_DLY] = 35;
        t->p[P_REV] = 24;
        t->p[P_LEVEL] = 82;
        break;
    case 4:
        t->p[P_WAVE1] = 2;
        t->p[P_WAVE2] = 3;
        t->p[P_DEC] = 39;
        t->p[P_SUS] = 0;
        t->p[P_REL] = 25;
        t->p[P_FDEC] = 29;
        t->p[P_CUT] = 82;
        t->p[P_FAMT] = 35;
        t->p[P_DLY] = 50;
        t->p[P_REV] = 34;
        break;
    case 5:
        t->p[P_WAVE1] = 0;
        t->p[P_DEC] = 30;
        t->p[P_SUS] = 12;
        t->p[P_REL] = 20;
        t->p[P_CUT] = 67;
        t->p[P_RES] = 42;
        t->p[P_DLY] = 38;
        t->p[P_REV] = 22;
        t->p[P_LEVEL] = 72;
        break;
    case 6:
        t->p[P_ATK] = 18;
        t->p[P_FATK] = 20;
        t->p[P_FDEC] = 50;
        t->p[P_FSUS] = 25;
        t->p[P_CUT] = 58;
        t->p[P_FAMT] = 45;
        t->p[P_REV] = 24;
        t->p[P_ADRIVE] = 16;
        t->p[P_LEVEL] = 73;
        break;
    case 7:
        t->p[P_WAVE1] = 2;
        t->p[P_WAVE2] = 3;
        t->p[P_ATK] = 55;
        t->p[P_REL] = 70;
        t->p[P_CUT] = 90;
        t->p[P_FAMT] = 0;
        t->p[P_CHO] = 55;
        t->p[P_REV] = 62;
        t->p[P_LEVEL] = 55;
        t->route[2] = (a_route_t){S_LFO2, D_PAN, 38};
        break;
    }
}
static void a_note_on(uint32_t ch, uint32_t n, uint32_t vel, uint32_t source) {
    a_part_t *t;
    if (ch >= 8 || n > 127 || !vel)
        return;
    t = &ap[ch];
    uint32_t i, pick = 0;
    int32_t score = 0x7fffffff;
    int mono = t->p[P_MONO];
    for (i = 0; i < (mono ? 1u : 2u); i++) {
        a_voice_t *v = &t->voice[i];
        int32_t s = !v->amp && !v->gate ? 0 : (v->gate ? 100000 : 1) + v->amp;
        if (s < score) {
            score = s;
            pick = i;
        }
    }
    a_voice_t *v = &t->voice[pick];
    int legato = mono && v->gate;
    /* Preserve phase even in silence. Phase is advanced virtually at last pitch. */
    v->phase[0] += v->inc[0] * (a_frame - v->last_frame);
    v->phase[1] += v->inc[1] * (a_frame - v->last_frame);
    v->last_frame = a_frame;
    if (!legato) {
        v->steal = v->last;
        v->amp = 0;
        v->env[0] = (a_env_t){0, 0};
        v->env[1] = (a_env_t){0, 0};
        for (i = 0; i < 2; i++) {
            v->f[i].s1 = v->f[i].s2 = 0;
        }
    }
    if (!t->p[P_GLIDE] || !v->inc[0])
        v->pitch = (int32_t)n * 256;
    v->note = (uint8_t)n;
    v->velocity = (uint8_t)vel;
    v->source = (uint8_t)source;
    v->gate = 1;
    v->held = 1;
    v->sost = 0;
    v->age = ++a_age;
    for (i = 0; i < 2; i++)
        if (t->p[i ? P_L2RESET : P_L1RESET])
            v->lfo_phase[i] = 0;
    t->last_vel = (int32_t)vel * 258;
    t->last_key = (int32_t)n * 258 - 16384;
    t->note_ons++;
}
static void a_note_off(uint32_t ch, uint32_t n, uint32_t source) {
    if (ch >= 8)
        return;
    for (uint32_t i = 0; i < 2; i++) {
        a_voice_t *v = &ap[ch].voice[i];
        if (v->note == n && v->source == source) {
            v->held = 0;
            if (!ap[ch].sustain && !v->sost)
                v->gate = 0;
        }
    }
}
static void a_panic(int ch, int instant) {
    for (uint32_t t = 0; t < 8; t++)
        if (ch < 0 || (uint32_t)ch == t) {
            ap[t].sustain = ap[t].sostenuto = 0;
            for (uint32_t i = 0; i < 2; i++) {
                a_voice_t *v = &ap[t].voice[i];
                v->gate = v->held = v->sost = 0;
                if (instant) {
                    v->amp = v->last = v->steal = 0;
                    v->env[0].value = v->env[1].value = 0;
                    v->env[0].stage = v->env[1].stage = 4;
                }
            }
        }
}
static int32_t a_source(a_part_t *t, a_voice_t *v, uint32_t src, int32_t l1, int32_t l2) {
    switch (src) {
    case S_LFO1:
        return l1;
    case S_LFO2:
        return l2;
    case S_AMPENV:
        return v ? v->env[0].value : t->last_env[0];
    case S_FILTENV:
        return v ? v->env[1].value : t->last_env[1];
    case S_VELOCITY:
        return v ? v->velocity * 258 : t->last_vel;
    case S_KEY:
        return v ? (int32_t)v->note * 258 - 16384 : t->last_key;
    case S_MOD:
        return t->mod;
    case S_WHEELVIB:
        return am(t->mod, l1);
    case S_BEND:
        return t->bend * 4;
    case S_PRESS:
        return ac(t->pressure + (v ? v->pressure : 0), 0, 32767);
    case S_BREATH:
        return t->breath;
    case S_DRIFT:
        return v ? v->drift : t->drift;
    case S_EXPR:
        return t->expression;
    case S_CC16:
        return t->cc16;
    case S_CC17:
        return t->cc17;
    default:
        return 0;
    }
}
static void a_modulate(a_part_t *t, a_voice_t *v, int32_t *m, int32_t l1, int32_t l2, int globals) {
    for (uint32_t i = 0; i < D_COUNT; i++)
        m[i] = 0;
    for (uint32_t i = 0; i < 8; i++) {
        a_route_t *r = &t->route[i];
        if (r->source && r->dest && r->dest < D_COUNT) {
            int global = r->dest >= D_DELAYTIME;
            if (global != globals)
                continue;
            int32_t s = a_source(t, v, r->source, l1, l2);
            m[r->dest] += (s * r->depth) / 32768;
        }
    }
}
static void a_voice_render(a_part_t *t, a_voice_t *v, int32_t *out, uint32_t n) {
    if (!v->amp && !v->gate && v->env[0].stage == 4)
        return;
    int32_t l[2], m[D_COUNT], coeffdelta[2][3], pitch, bent, target, cut, res, pwm, mix, fd, amp0, amp1, step,
        oct;
    uint32_t i, j, inc1, inc2;
    for (i = 0; i < 2; i++) {
        int rate =
                ac(t->p[i ? P_L2RATE : P_L1RATE] + a_part_mod[t - ap][i ? D_LFO2RATE : D_LFO1RATE], 0, 127),
            sync = t->p[i ? P_L2SYNC : P_L1SYNC], reset = t->p[i ? P_L2RESET : P_L1RESET];
        uint32_t ph = reset ? v->lfo_phase[i] : t->phase[i], inc = a_lfo_inc(rate, sync);
        if (reset) {
            v->lfo_phase[i] += inc;
            if (v->lfo_phase[i] < ph)
                v->held_random[i] = (int32_t)(arand() & 65535) - 32768;
        }
        l[i] = a_lfo_value(ph, t->p[i ? P_L2SHAPE : P_L1SHAPE], reset ? v->held_random[i] : t->random[i]);
    }
    a_env_step(&v->env[0], v->gate, t->p, 0);
    a_env_step(&v->env[1], v->gate, t->p, 1);
    if ((a_frame & 4095u) == 0)
        v->drift_to = (int32_t)(arand() & 65535) - 32768;
    v->drift += (v->drift_to - v->drift) / 256;
    a_modulate(t, v, m, l[0], l[1], 0);
    target = (int32_t)v->note * 256;
    int glide = ac(t->p[P_GLIDE] + m[D_GLIDE], 0, 127);
    step = glide ? ((target - v->pitch) * 32) / (a_times[glide] * 44 + 1) : target - v->pitch;
    if (!step && target != v->pitch)
        step = target > v->pitch ? 1 : -1;
    v->pitch += step;
    pitch = v->pitch + t->fine + t->coarse + (t->p[P_TUNE] - 64) * 4 + v->component / 4 +
            (v->drift * t->p[P_DRIFT]) / 65536;
    bent = (t->bend * t->p[P_BEND]) / 32;
    pitch += bent + m[D_PITCH] * 24;
    oct = (t->p[P_OCT2] - 2) * 12 * 256;
    inc1 = a_pitch_inc(pitch);
    inc2 = a_pitch_inc(pitch + oct + t->p[P_DETUNE] + m[D_OSC2PITCH] * 24);
    cut = ac(t->p[P_CUT] + m[D_CUT] + am(v->env[1].value, t->p[P_FAMT] * 258) / 512 + v->component / 32, 0,
             127);
    res = ac(t->p[P_RES] + m[D_RES], 0, 127) >> 3;
    pwm = ac(t->p[P_PWM] + m[D_PWM], 12, 115);
    mix = ac(t->p[P_MIX] + m[D_MIX], 0, 127);
    fd = ac(t->p[P_FDRIVE] + m[D_FDRIVE], 0, 127);
    for (j = 0; j < 2; j++)
        for (i = 0; i < 3; i++) {
            int32_t to = a_fc_ram[cut][res][i];
            if (!v->inc[0])
                v->f[j].c[i] = to;
            coeffdelta[j][i] = (to - v->f[j].c[i]) / 32;
        }
    amp0 = v->amp;
    amp1 = v->env[0].stage == 4
               ? 0
               : ac(am(v->env[0].value, (int32_t)v->velocity * 258) + m[D_LEVEL] * 128, 0, 32767);
    step = (amp1 - amp0) / 32;
    uint32_t lev1=a_band(inc1),lev2=a_band(inc2);
    for (i = 0; i < n; i++) {
        int32_t x, y;
        v->phase[0] += inc1;
        v->phase[1] += inc2;
        x = a_osc(v->phase[0], lev1, t->p[P_WAVE1], pwm);
        y = a_osc(v->phase[1], lev2, t->p[P_WAVE2], pwm);
        x = (x * (127 - mix) + y * mix) / 127;
        x = adrive(x, fd);
        for (j = 0; j < 3; j++)
            v->f[0].c[j] += coeffdelta[0][j];
        x = a_filter(&v->f[0], x, fd > 32);
        if (t->p[P_SLOPE]) {
            for (j = 0; j < 3; j++)
                v->f[1].c[j] += coeffdelta[1][j];
            x = a_filter(&v->f[1], x, fd > 32);
        }
        amp0 += step;
        y = am(x, amp0) / 3; /* per-voice headroom, then mix/part gain */
        if (v->steal) {
            y += v->steal;
            v->steal -= v->steal / 32 + (v->steal > 0 ? 1 : -1);
        }
        v->last = y;
        out[i] += y;
    }
    v->amp = amp1;
    v->inc[0] = inc1;
    v->inc[1] = inc2;
    v->last_frame = a_frame + n;
}
static void a_trigger(void);
static void a_drum_on(uint32_t note, uint32_t velocity);
static void a_fx_init(void);
static void a_fx_block(int32_t *out, uint32_t n);
static void a_midi(uint32_t st, uint32_t d1, uint32_t d2, uint32_t source);
static void a_transport(uint32_t action) {
    if (action == 0) {
        a_song_play = 0;
        for (uint32_t t = 0; t < 8; t++)
            for (uint32_t i = 0; i < 2; i++)
                if (ap[t].voice[i].source == 3)
                    ap[t].voice[i].gate = ap[t].voice[i].held = 0;
    } else {
        if (action == 1) {
            a_song_pos = a_song_tick = a_song_rem = 0;
        }
        a_song_play = 1;
    }
}
static void a_commands(void) {
    if (a_panic_request) {
        a_qr = a_qw;
        a_panic(-1, 1);
        a_song_play = 0;
        a_panic_request = 0;
    }
    while (a_qr != a_qw) {
        a_cmd_t c = a_queue[a_qr & 255u];
        a_qr++;
        switch (c.op) {
        case A_MIDI:
            a_midi(c.a, c.b, (uint32_t)c.value, c.source);
            break;
        case A_PARAM:
            if (c.ch < 8 && c.a < P_COUNT)
                ap[c.ch].p[c.a] = (int16_t)ac(c.value, 0, p_max[c.a]);
            break;
        case A_GLOBAL:
            if (c.a < G_COUNT)
                ag[c.a] = (int16_t)ac(c.value, c.a == G_TEMPO ? 30 : 0, g_max[c.a]);
            break;
        case A_MATRIX:
            if (c.ch < 8 && c.a < 8) {
                a_route_t *r = &ap[c.ch].route[c.a];
                if (c.b == 0)
                    r->source = (uint8_t)ac(c.value, 0, S_COUNT - 1);
                if (c.b == 1)
                    r->dest = (uint8_t)ac(c.value, 0, D_COUNT - 1);
                if (c.b == 2)
                    r->depth = (int16_t)ac(c.value, -127, 127);
            }
            break;
        case A_PLAY:
            a_transport(c.value ? 1 : 0);
            break;
        case A_PRESET:
            a_patch(c.ch, (uint32_t)c.value);
            break;
        case A_PANIC:
            a_transport(0);
            a_panic(-1, 1);
            a_fx_init();
            break;
        }
    }
}
static void a_song_dispatch(void) {
    while (a_song_pos < A_SONG_COUNT && a_song[a_song_pos].tick <= a_song_tick) {
        const a_song_event_t *e = &a_song[a_song_pos++];
        if (e->track < 8) {
            if (e->status == 0x90)
                a_note_on(e->track, e->note, e->velocity, 3);
            else
                a_note_off(e->track, e->note, 3);
        } else if (e->status == 0x90)
            a_drum_on(e->note, e->velocity);
    }
}
static void a_render(int32_t *out, uint32_t n) {
    a_commands();
    for (uint32_t b = 0; b < n; b += A_CTL) {
        uint32_t k = n - b < A_CTL ? n - b : A_CTL;
        /* Dispatch internal events before the block containing their target tick. */
        if (a_song_play)
            a_song_dispatch();
        az(a_bus_mod, sizeof a_bus_mod);
        a_active = 0;
        for (uint32_t t = 0; t < 8; t++) {
            a_part_t *p = &ap[t];
            for (uint32_t j = 0; j < 2; j++) {
                uint32_t old = p->phase[j];
                int rate = p->p[j ? P_L2RATE : P_L1RATE];
                rate = ac(rate + a_part_mod[t][j ? D_LFO2RATE : D_LFO1RATE], 0, 127);
                p->phase[j] += a_lfo_inc(rate, p->p[j ? P_L2SYNC : P_L1SYNC]);
                if (p->phase[j] < old)
                    p->random[j] = (int32_t)(arand() & 65535) - 32768;
            }
            az(a_track_buf[t], sizeof a_track_buf[t]);
            for (uint32_t v = 0; v < 2; v++) {
                a_voice_render(p, &p->voice[v], a_track_buf[t], k);
                if (p->voice[v].amp || p->voice[v].gate)
                    a_active++;
            }
            a_voice_t *latest = p->voice[0].age >= p->voice[1].age ? &p->voice[0] : &p->voice[1];
            p->last_env[0] = latest->env[0].value;
            p->last_env[1] = latest->env[1].value;
            int32_t l1 = a_lfo_value(p->phase[0], p->p[P_L1SHAPE], p->random[0]),
                    l2 = a_lfo_value(p->phase[1], p->p[P_L2SHAPE], p->random[1]);
            a_modulate(p, 0, a_part_mod[t], l1, l2, 0);
            int32_t gmod[D_COUNT];
            a_modulate(p, 0, gmod, l1, l2, 1);
            for (uint32_t d = D_DELAYTIME; d < D_COUNT; d++)
                a_bus_mod[d] += gmod[d];
        }
        if (a_active > a_all_active_peak)
            a_all_active_peak = a_active;
#ifdef A_FX_TIMING
        A_FX_TIMING(0);
#endif
        a_fx_block(out + 2 * b, k);
#ifdef A_FX_TIMING
        A_FX_TIMING(1);
#endif
        a_frame += k;
        if (a_song_play) {
            a_song_rem += k * (uint32_t)ag[G_TEMPO] * 96u;
            while (a_song_rem >= A_FS * 60u) {
                a_song_rem -= A_FS * 60u;
                a_song_tick++;
            }
            if (a_song_tick >= A_SONG_END) {
                a_song_dispatch();
                a_transport(0);
            }
        }
        if (a_external && a_frame - a_clock_last > A_FS * 2u)
            a_external = 0;
    }
}
static void a_init(void) {
    az(ap, sizeof ap);
    az(a_queue, sizeof a_queue);
    az(a_part_mod, sizeof a_part_mod);
    a_qw = a_qr = 0;
    a_frame = a_age = a_active = a_song_play = a_song_pos = a_song_tick = a_song_rem = 0;
    a_clamps = a_peak = a_queue_drops = a_all_active_peak = a_drum_hits = 0;
    for (uint32_t i = 0; i < G_COUNT; i++)
        ag[i] = g_defaults[i];
    for (uint32_t w = 0; w < 4; w++)
        for (uint32_t l = 0; l < 8; l++)
            for (uint32_t i = 0; i < 256; i++)
                a_wave_ram[w][l][i] = a_waves[w][l][i];
    for (uint32_t c = 0; c < 128; c++)
        for (uint32_t r = 0; r < 16; r++)
            for (uint32_t k = 0; k < 3; k++)
                a_fc_ram[c][r][k] = a_fc[c][r][k];
    for (uint32_t t = 0; t < 8; t++) {
        a_patch(t, t);
        ap[t].expression = 32767;
        ap[t].rpn_m = ap[t].rpn_l = ap[t].nrpn_m = ap[t].nrpn_l = 127;
        for (uint32_t i = 0; i < 2; i++) {
            a_voice_t *v = &ap[t].voice[i];
            v->phase[0] = arand();
            v->phase[1] = arand();
            v->env[0].stage = v->env[1].stage = 4;
            v->component = (int16_t)((int)(arand() % 129u) - 64);
        }
    }
    a_fx_init();
}
#include "effects.h"
#include "midi.h"

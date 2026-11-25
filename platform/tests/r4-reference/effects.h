/* SPDX-License-Identifier: GPL-3.0-only; compact shared FX, independent wet-return ducking. */
#pragma once
static int16_t a_delay[32768] A_POOL, a_chorus[2048] A_POOL, a_reverb[9200] A_POOL;
static uint32_t a_dw, a_cw, a_cp, a_ri[4], a_revph, a_api[2];
static int16_t a_diff[2][256] A_POOL;
static int32_t a_dlp, a_rlp[4], a_dt_now;
static const uint16_t a_rlen[4] = {1559, 1931, 2389, 2791};
static const uint16_t a_roff[4] = {0, 1585, 3516, 5905};
typedef struct {
    uint32_t phase, left, inc;
    int32_t amp;
    uint8_t note;
} a_drum_t;
static a_drum_t a_drums[4];
typedef struct {
    int32_t attenuation;
    uint32_t hold;
} a_duck_t;
static a_duck_t a_ducks[12];
static int32_t a_bus_env, a_bus_gain = 32767, a_glue_env, a_glue_gain = 32767, a_source_env;
static uint32_t a_trigger_serial, a_duck_serial[12];
static int32_t a_glue_gr, a_bus_gr;
static void a_trigger(void) { a_trigger_serial++; }
static void a_drum_on(uint32_t note, uint32_t velocity) {
    uint32_t pick = 0;
    for (uint32_t i = 0; i < 4; i++)
        if (!a_drums[i].left) {
            pick = i;
            break;
        } else if (a_drums[i].amp < a_drums[pick].amp)
            pick = i;
    if (note == 42 || note == 46)
        for (uint32_t i = 0; i < 4; i++)
            if (a_drums[i].note == 42 || a_drums[i].note == 46)
                a_drums[i].left = 0;
    a_drums[pick] = (a_drum_t){0,
                               (note == 36   ? 12000u
                                : note == 38 ? 7000u
                                : note == 46 ? 9000u
                                             : 2600u),
                               a_inc[note == 36   ? 34
                                     : note == 38 ? 57
                                                  : 92],
                               (int32_t)velocity * 200, (uint8_t)note};
    a_drum_hits++;
    if (note == 36 && (ag[G_DSOURCE] == 0 || ag[G_DSOURCE] == 2))
        a_trigger();
}
static int32_t a_drum_sample(void) {
    int32_t mix = 0;
    for (uint32_t i = 0; i < 4; i++) {
        a_drum_t *d = &a_drums[i];
        if (!d->left)
            continue;
        int32_t x;
        d->phase += d->inc;
        if (d->note == 36) {
            x = a_sine[d->phase >> 24];
            if (d->inc > a_inc[30])
                d->inc -= d->inc / 2400u;
        } else {
            int32_t noise = (int32_t)(arand() & 65535) - 32768;
            x = d->note == 38 ? (noise + a_sine[d->phase >> 24]) / 2 : noise;
        }
        mix += am(x, d->amp) / 3;
        d->left--;
        d->amp -= d->amp / (d->note == 46 ? 3000 : 1500) + (d->amp > 0);
        if (d->amp < 0)
            d->amp = 0;
    }
    return mix;
}
static int32_t a_comp(int32_t peak, int amount, int32_t *env, int32_t *gain, int threshold, int attack,
                      int release) {
    int32_t e = *env, target = 32767, th = 512 + threshold * 140,
            coef = peak > e ? ac(64 + attack * 12, 64, 4096) : ac(2 + release / 4, 2, 64);
    e += am(peak - e, coef);
    if (e > th) {
        int32_t compressed = th + (e - th) * (127 - amount) / 127;
        target = ac((compressed * 32767) / (e ? e : 1), 4096, 32767);
    }
    *env = e;
    *gain += am(target - *gain, target < *gain ? 2048 : 128);
    return *gain;
}
static int32_t a_duck_gain(uint32_t id, int mode, int depth) {
    a_duck_t *d = &a_ducks[id];
    int att = ag[G_DATTACK], hold = ag[G_DHOLD], rel = ag[G_DRELEASE];
    if (id >= 9) {
        uint32_t base = G_CHOATT + (id - 9) * 4;
        att = ag[base];
        hold = ag[base + 1];
        rel = ag[base + 2];
        depth = ac(ag[base + 3] * ag[G_FXDEPTH] / 64, 0, 127);
    }
    int trigger = a_duck_serial[id] != a_trigger_serial;
    int32_t target = 0;
    if (!mode) {
        d->attenuation = 0;
        d->hold = 0;
        a_duck_serial[id] = a_trigger_serial;
        return 32767;
    }
    if (trigger) {
        a_duck_serial[id] = a_trigger_serial;
        d->hold = (uint32_t)a_times[ac(hold, 0, 127)] * 44u;
        if (!d->hold)
            d->hold = 1;
    }
    if (d->hold) {
        target = depth * 258;
        if (d->hold > 32)
            d->hold -= 32;
        else
            d->hold = 0;
    }
    int32_t ms = target > d->attenuation ? a_times[ac(att, 0, 127)] : a_times[ac(rel, 0, 127)];
    int32_t step = 32767 * 32 / (ms * 44 + 1);
    if (step < 1)
        step = 1;
    /* Compressor style: one-pole recovery; gate style: linear A/H/R. */
    if (mode == 1 && target < d->attenuation) {
        d->attenuation -= (d->attenuation * 32) / (ms * 44 + 1) + 1;
    } else if (target > d->attenuation)
        d->attenuation = ac(d->attenuation + step, 0, target);
    else
        d->attenuation = ac(d->attenuation - step, target, 32767);
    int32_t g = 32767 - d->attenuation;
    return mode == 2 && ag[G_GATEOPEN] ? d->attenuation : g;
}
static int32_t a_insert(a_part_t *t, int32_t x) {
    a_insert_t *s = &t->insert;
    int type = t->p[P_INSERT], amount = t->p[P_IAMT], wet = t->p[P_IWET];
    int32_t y = x;
    switch (type) {
    case 1:
        y = adrive(x, amount);
        break;
    case 2: {
        int bits = 4 + (127 - amount) * 10 / 127;
        int div = 1 << (16 - bits);
        y = (x / div) * div;
        break;
    }
    case 3:
        y = am(x, a_sine[s->phase >> 24]);
        s->phase += a_lfoinc[ac(amount + 40, 0, 127)] / 32u;
        break;
    case 4: {
        int32_t c = 7000 + am(a_sine[s->phase >> 24], 4500);
        s->phase += a_lfoinc[ac(amount / 2, 0, 127)] / 32u;
        for (uint32_t j = 0; j < 4; j++) {
            int32_t z = y;
            y = s->line[j] - am(c, z);
            s->line[j] = z + am(c, y);
        }
        y = (x + y) / 2;
        break;
    }
    case 5:
    case 6: {
        uint32_t base = type == 5 ? 40u : 700u, span = type == 5 ? 25u : 300u;
        int32_t l = a_sine[s->phase >> 24];
        uint32_t dq = (base << 8) + (uint32_t)((l + 32768) * (int32_t)span / 256);
        uint32_t pos = (s->write - (dq >> 8)) & 2047u;
        int32_t a = s->delay[pos], b = s->delay[(pos - 1u) & 2047u];
        y = a + ((b - a) * (int32_t)(dq & 255u) >> 8);
        s->delay[s->write & 2047u] = (int16_t)ac(x + am(y, type == 5 ? amount * 160 : 0), -32767, 32767);
        s->write++;
        s->phase += a_lfoinc[amount / 2] / 32u;
        break;
    }
    case 7: {
        int32_t target = aa(x) > amount * 100 ? 32767 : 0;
        s->fb += (target - s->fb) / 64;
        y = am(x, s->fb);
        break;
    }
    default:
        return x;
    }
    s->last += am(y - s->last, ac(t->p[P_ITONE] * 250, 512, 32700));
    y = s->last;
    return x + ((y - x) * wet) / 127;
}
static void a_fx_init(void) {
    az(a_diff, sizeof a_diff);
    az(a_api, sizeof a_api);
    az(a_delay, sizeof a_delay);
    az(a_chorus, sizeof a_chorus);
    az(a_reverb, sizeof a_reverb);
    az(a_drums, sizeof a_drums);
    az(a_ducks, sizeof a_ducks);
    az(a_ri, sizeof a_ri);
    az(a_rlp, sizeof a_rlp);
    az(a_duck_serial, sizeof a_duck_serial);
    a_dw = a_cw = a_cp = a_revph = a_trigger_serial = 0;
    a_dt_now = 11025;
    a_dlp = a_bus_env = a_glue_env = a_source_env = 0;
    a_bus_gain = a_glue_gain = 32767;
}
static int32_t a_read_delay(const int16_t *buf, uint32_t wp, uint32_t mask, uint32_t dq) {
    uint32_t p = (wp - (dq >> 8)) & mask;
    int32_t x = buf[p], y = buf[(p - 1u) & mask];
    return x + ((y - x) * (int32_t)(dq & 255u) >> 8);
}
static uint32_t a_fx_enabled=1;
static void a_fx_block(int32_t *out, uint32_t n) {
    int32_t srcpeak = 0;
    int sch = ac(ag[G_DTRACK], 0, 8);
    for (uint32_t i = 0; i < n; i++) {
        a_track_buf[8][i] = a_drum_sample();
        int32_t x = aa(a_track_buf[sch][i]);
        if (x > srcpeak)
            srcpeak = x;
    }
    if (ag[G_DSOURCE] == 1 || ag[G_DSOURCE] == 2) {
        int32_t old = a_source_env;
        a_source_env = srcpeak > old ? srcpeak : old - (old / 16);
        if (old < 1024 && a_source_env >= 1024)
            a_trigger();
    }
    if (ag[G_DSOURCE] == 3 && a_song_play && (a_song_tick % 96u) == 0u &&
        a_song_rem < n * (uint32_t)ag[G_TEMPO] * 96u)
        a_trigger();
    int32_t duck[12];
    int depth = ac(ag[G_DUCKDEPTH] + a_bus_mod[D_DUCKDEPTH], 0, 127);
    duck[0] = a_duck_gain(0, ag[G_DUCKMODE], depth);
    for (uint32_t t = 0; t < 8; t++)
        duck[t + 1] = a_duck_gain(t + 1, ap[t].p[P_DUCK], ap[t].p[P_DEPTH]);
    for (uint32_t j = 0; j < 3; j++)
        duck[9 + j] = a_duck_gain(9 + j,
                                  ag[j == 0   ? G_CHODUCK
                                     : j == 1 ? G_DLYDUCK
                                              : G_REVDUCK],
                                  ag[G_FXDEPTH]);
    int32_t dryl[32] = {0}, dryr[32] = {0}, busl[32] = {0}, busr[32] = {0}, ci[32] = {0}, di[32] = {0},
            ri[32] = {0};
    for (uint32_t t = 0; t < 9; t++) {
        a_part_t *p = t < 8 ? &ap[t] : 0;
        int pan = p ? ac(p->p[P_PAN] + a_part_mod[t][D_PAN], 0, 127) : 64;
        int32_t level = p ? ac(p->p[P_LEVEL], 0, 127) * 258 : ag[G_DRUMLEVEL] * 258;
        if (p)
            level = am(level, p->expression);
        int cho = p ? ac(p->p[P_CHO] + a_part_mod[t][D_CHOSEND], 0, 127) : 0,
            dly = p ? ac(p->p[P_DLY] + a_part_mod[t][D_DLYSEND], 0, 127) : ag[G_DRUMDLY],
            rev = p ? ac(p->p[P_REV] + a_part_mod[t][D_REVSEND], 0, 127) : ag[G_DRUMREV];
        int bus = p ? p->p[P_BUS] : ag[G_DRUMBUS];
        int32_t pk = 0;
        for (uint32_t i = 0; i < n; i++) {
            int32_t x = a_track_buf[t][i];
            if (p) {
                x = adrive(x, ac(p->p[P_ADRIVE] + a_part_mod[t][D_ADRIVE], 0, 127));
                x = a_insert(p, x);
            } else if (ag[G_DRUMGLUE]) {
                int32_t g = a_comp(aa(x), ag[G_DRUMGLUE], &a_glue_env, &a_glue_gain, 40, 22, 45);
                a_glue_gr = 32767 - g;
                x = am(x, g);
            }
            if (p) {
                p->gain += (level - p->gain) / 64;
                x = am(x, p->gain);
                x = am(x, duck[t + 1]);
            } else
                x = am(x, level);
            if (aa(x) > pk)
                pk = aa(x);
            int32_t l = (x * (127 - pan)) / 127, r = x * pan / 127;
            if (bus) {
                busl[i] += l;
                busr[i] += r;
            } else {
                dryl[i] += l;
                dryr[i] += r;
            }
            ci[i] += x * cho / 127;
            di[i] += x * dly / 127;
            ri[i] += x * rev / 127;
        }
        if (p)
            p->peak = pk;
    }
    int32_t dt = ac(ag[G_DTIME] + a_bus_mod[D_DELAYTIME], 0, 127);
    dt = 2205 + dt * 220; /* 50..684ms; fixed RAM, smoothed head */
    int32_t fb = ac(ag[G_FDBK] + a_bus_mod[D_DELAYFDBK], 0, 110) * 258;
    int32_t decay = 18000 + ac(ag[G_DECAY] + a_bus_mod[D_REVDECAY], 0, 127) * 90;
    int32_t damp = 2000 + (127 - ac(ag[G_DAMP] + a_bus_mod[D_REVDAMP], 0, 127)) * 200;
    int32_t cdep = ac(ag[G_CDEPTH] + a_bus_mod[D_CHODEPTH], 0, 127);
    for (uint32_t i = 0; i < n; i++) {
        int32_t l = dryl[i], r = dryr[i], bl = busl[i], br = busr[i];
        if (ag[G_BUSCOMP]) {
            int32_t g = a_comp(aa(bl) > aa(br) ? aa(bl) : aa(br), ag[G_RATIO], &a_bus_env, &a_bus_gain,
                               ag[G_THRESH], ag[G_ATTACK], ag[G_RELEASE]);
            a_bus_gr = 32767 - g;
            int32_t mix = ag[G_BUSCOMP] * 258;
            g = 32767 - am(32767 - g, mix);
            bl = am(bl, g);
            br = am(br, g);
        }
        bl += bl * ag[G_MAKEUP] / 127;
        br += br * ag[G_MAKEUP] / 127;
        l += am(bl, duck[0]);
        r += am(br, duck[0]);
        if (a_fx_enabled) {
        /* Mono feedback delay, independently ducked wet return. */
        a_dt_now += (dt - a_dt_now) / 256;
        if (a_dt_now != dt && aa(dt - a_dt_now) < 256)
            a_dt_now += dt > a_dt_now ? 1 : -1;
        int32_t dy = a_delay[(a_dw - (uint32_t)a_dt_now) & 32767u];
        a_dlp += am(dy - a_dlp, 3000 + ag[G_DTONE] * 200);
        a_delay[a_dw++ & 32767u] = (int16_t)ac(di[i] / 2 + am(a_dlp, fb), -32767, 32767);
        dy = am(am(dy, ag[G_DELAY] * 180), duck[10]);
        l += dy;
        r += dy;
        /* Stereo chorus from two taps in one short line. */
        int32_t mod = a_sine[a_cp >> 24];
        a_cp += a_lfoinc[ag[G_CRATE]] / 32u;
        uint32_t ca = (700u << 8) + (uint32_t)((mod + 32768) * cdep / 64),
                 cb = (700u << 8) + (uint32_t)((32768 - mod) * cdep / 64);
        int32_t cl = a_read_delay(a_chorus, a_cw, 2047, ca), cr = a_read_delay(a_chorus, a_cw, 2047, cb);
        a_chorus[a_cw++ & 2047u] = (int16_t)ac(ci[i], -32767, 32767);
        l += am(am(cl, ag[G_CHO] * 200), duck[9]);
        r += am(am(cr, ag[G_CHO] * 200), duck[9]);
        /* Four-line orthogonal feedback network, loop damping and one modulated tap. */
        int32_t z[4], h[4], rin = ri[i] / 5;
        for (uint32_t j = 0; j < 2; j++) {
            uint32_t len = j ? 181u : 127u;
            int32_t delayed = a_diff[j][a_api[j]], v = rin + delayed / 2;
            a_diff[j][a_api[j]] = (int16_t)ac(v, -32767, 32767);
            rin = delayed - v / 2;
            if (++a_api[j] >= len)
                a_api[j] = 0;
        }
        a_revph += 12000u;
        uint32_t m = (uint32_t)(a_sine[a_revph >> 24] + 32768) * 12u >> 16;
        for (uint32_t j = 0; j < 4; j++) {
            uint32_t pos = a_ri[j];
            if (!j)
                pos = (pos + m) % a_rlen[j];
            z[j] = a_reverb[a_roff[j] + pos];
            a_rlp[j] += am(z[j] - a_rlp[j], damp);
            z[j] = a_rlp[j];
        }
        h[0] = (z[0] + z[1] + z[2] + z[3]) / 2;
        h[1] = (z[0] - z[1] + z[2] - z[3]) / 2;
        h[2] = (z[0] + z[1] - z[2] - z[3]) / 2;
        h[3] = (z[0] - z[1] - z[2] + z[3]) / 2;
        for (uint32_t j = 0; j < 4; j++) {
            a_reverb[a_roff[j] + a_ri[j]] = (int16_t)ac(rin + am(h[j], decay), -32767, 32767);
            if (++a_ri[j] >= a_rlen[j])
                a_ri[j] = 0;
        }
        l += am(am((z[0] + z[2]) / 2, ag[G_REVERB] * 258), duck[11]);
        r += am(am((z[1] - z[3]) / 2, ag[G_REVERB] * 258), duck[11]);
        }
        int32_t peak = aa(l) > aa(r) ? aa(l) : aa(r);
        if ((uint32_t)peak > a_peak)
            a_peak = (uint32_t)peak;
        if (peak > 30000)
            a_clamps++;
        /* Gentle master ceiling. No unbounded feedback, and saturation is counted. */
        out[2 * i] += ac(l, -30000, 30000);
        out[2 * i + 1] += ac(r, -30000, 30000);
    }
}

/*
 * tenebrae_fx.h -- the effects, in the order the signal meets them:
 *
 *   bus glue -> Tone tilt -> Chorus / Ensemble -> Phaser -> ping-pong tape
 *   Delay -> Reverb (+ Shimmer) -> low end to centre -> [clean sub joins] ->
 *   Pump -> VHS -> Level
 *   -> limiter
 *
 * The clean sine sub comes in on its own input and skips everything up to the
 * pump: a sub that is chorused, phased or washed in reverb is a weaker sub.
 *
 * Pump and VHS sit AFTER the reverb on purpose: a sidechain pump is heard on
 * the whole mix, tails included, and a worn tape wobbles everything on it.
 *
 * No allocation here: every delay line is a fixed array inside tb_fx_t, which
 * lives in the instance.
 */
#ifndef TENEBRAE_FX_H
#define TENEBRAE_FX_H

#include "tenebrae_engine.h"

#define FX_CHO_N   2048
#define FX_DLY_N   131072
#define FX_VHS_N   1024
#define FX_SHIM_N  4096
#define FX_SHIM_W  2048.0f
#define FX_PRE_N   8192
#define FX_AP_N    1024
#define REV_LINES  8
#define REV_N      8192

static const int REV_LEN[REV_LINES] = { 1687, 1931, 2239, 2617, 2903, 3331, 3761, 4177 };
static const float REV_MOD_HZ[4] = { 0.31f, 0.47f, 0.71f, 0.93f };
static const int REV_AP_LEN[2][2] = { { 142, 379 }, { 107, 277 } };

typedef struct {
    /* smoothed controls */
    float s_cho, s_phs, s_dly, s_rev, s_shim, s_pump, s_vhs, s_tone, s_level, s_fb, s_bus;

    float tone_lp[2];
    float tone_hp[2][2];

    float cho[2][FX_CHO_N];
    float cho_in_lp[2];
    int   cho_w;
    float cho_ph1, cho_ph2, cho_ph3;

    float phs_z[2][6];
    float phs_last[2];
    float phs_ph;
    float phs_a[2];
    int   phs_tick;

    float dly[2][FX_DLY_N];
    int   dly_w;
    float dly_t;              /* current delay, samples */
    int   dly_snap;           /* take the next target at once (nothing is sounding) */
    float dly_lp[2], dly_hp[2];
    float dly_wow;

    float pre[2][FX_PRE_N];
    int   pre_w;
    float ap[2][2][FX_AP_N];
    int   ap_w;
    float rev[REV_LINES][REV_N];
    int   rev_w;
    float rev_lp[REV_LINES];
    float rev_mod[4];
    float rev_hp[2];
    float rev_g[REV_LINES];
    float rev_g_size;         /* the size the gains were computed for */

    float shim[FX_SHIM_N];
    int   shim_w;
    float shim_ph;
    float shim_hp, shim_lp1, shim_lp2;
    float shim_out;

    float xo_lo[2], xo_l[2], xo_r[2];   /* the low-end crossover (see tb_fx_process) */
    float pump_g;
    double beat;              /* free-running beat counter when the transport is stopped */

    float vhs[2][FX_VHS_N];
    int   vhs_w;
    float vhs_ph1, vhs_ph2, vhs_ph3;
    float vhs_lp[2][2];
    float vhs_env;
    float vhs_rnd, vhs_rnd_to;
    int   vhs_rnd_wait;
    uint32_t rng;
} tb_fx_t;

static void tb_fx_init(tb_fx_t *fx)
{
    memset(fx, 0, sizeof(*fx));
    fx->dly_t = 0.375f * TB_SR;
    fx->dly_snap = 1;
    fx->pump_g = 1.0f;
    fx->rev_g_size = -1.0f;
    fx->s_level = 0.7f;
    fx->rng = 0xC0FFEE11u;
}

static inline float fx_soft_limit(float x, float knee, float ceil)
{
    float a = fabsf(x);
    if (a <= knee) return x;
    float over = (a - knee) / (ceil - knee);
    float y = knee + (ceil - knee) * tb_tanh(over);
    return x < 0.0f ? -y : y;
}

static inline float fx_read(const float *buf, int mask, int w, float delay)
{
    float pos = (float)w - delay;
    int i = (int)floorf(pos);
    float fr = pos - (float)i;
    float a = buf[i & mask], b = buf[(i + 1) & mask];
    return a + (b - a) * fr;
}

/*
 * Process `n` frames in place. `sub` is the clean sub, mono, added dry.
 *   bpm   the session tempo
 *   beat  beats since the transport started, or < 0 when it is not running
 */
static void tb_fx_process(tb_fx_t *fx, const float *p, float *L, float *R, const float *sub, int n, float bpm, double beat)
{
    const float sr = TB_SR;
    if (bpm < 20.0f || bpm > 400.0f) bpm = 120.0f;

    /* ---- per block ------------------------------------------------------ */
    int cho_mode = (int)(p[P_CHO_MODE] + 0.5f);
    if (cho_mode < 0) cho_mode = 0;
    if (cho_mode >= C_COUNT) cho_mode = C_COUNT - 1;
    const float t_cho = tb_clamp01(p[P_CHO_MIX]);
    const float t_phs = tb_clamp01(p[P_PHS_DEPTH]);
    const float t_dly = tb_clamp01(p[P_DLY_MIX]);
    const float t_rev = tb_clamp01(p[P_REV_MIX]);
    const float t_shim = tb_clamp01(p[P_SHIMMER]);
    const float t_pump = tb_clamp01(p[P_PUMP]);
    const float t_vhs = tb_clamp01(p[P_VHS]);
    const float t_tone = tb_clamp(p[P_TONE], -1.0f, 1.0f);
    const float t_level = tb_clamp01(p[P_LEVEL]);
    const float t_fb = tb_clamp01(p[P_DLY_FB]) * 0.9f;
    /* The last quarter of Drive: the SUMMED voices are pushed into the bus
     * clipper, so a chord intermodulates the way it does through an amp. */
    const float t_bus = tb_clamp01((p[P_DRIVE] - 0.75f) * 4.0f);
    const float sk = 0.0015f;                 /* about 15 ms */
    /* Out of silence, the controls are simply where they were set: a preset
     * chosen while nothing sounds must not fade in from the last one. */
    if (fx->dly_snap) {
        fx->s_cho = t_cho; fx->s_phs = t_phs; fx->s_dly = t_dly; fx->s_rev = t_rev; fx->s_shim = t_shim;
        fx->s_pump = t_pump; fx->s_vhs = t_vhs; fx->s_tone = t_tone; fx->s_level = t_level; fx->s_fb = t_fb;
        fx->s_bus = t_bus;
    }

    /* Tone's low cut: 18 Hz at the centre, about 540 Hz at full */
    const float hp_c = 1.0f - expf(-2.0f * (float)M_PI * (18.0f * powf(30.0f, tb_clamp01(t_tone))) / sr);

    /* chorus rates */
    const float cho_r1 = cho_mode == C_II ? 0.863f : (cho_mode == C_ENS ? 0.63f : (cho_mode == C_DIM ? 0.25f : 0.513f));
    const float cho_r2 = cho_mode == C_ENS ? 6.1f : 0.863f;

    /* phaser */
    const float phs_rate = 0.03f * powf(200.0f, tb_clamp01(p[P_PHS_RATE]));

    /* delay time */
    int di = (int)(p[P_DLY_TIME] + 0.5f);
    if (di < 0) di = 0;
    if (di > 6) di = 6;
    float dly_target = TB_DLY_BEATS[di] * 60.0f / bpm * sr;
    dly_target = tb_clamp(dly_target, 64.0f, (float)(FX_DLY_N - 64));
    /* Coming out of silence there is nothing in the line to bend, so a tempo or
     * division changed meanwhile is simply taken; while sounding it glides. */
    if (fx->dly_snap) { fx->dly_t = dly_target; fx->dly_snap = 0; }
    const float dly_lp_c = 1.0f - expf(-2.0f * (float)M_PI * 4600.0f / sr);
    const float dly_hp_c = 1.0f - expf(-2.0f * (float)M_PI * 140.0f / sr);

    /* reverb */
    const float size = tb_clamp01(p[P_REV_SIZE]);
    if (size != fx->rev_g_size) {
        float rt = 0.4f * powf(60.0f, size);   /* 0.4 s .. 24 s */
        for (int i = 0; i < REV_LINES; i++)
            fx->rev_g[i] = powf(10.0f, -3.0f * (float)REV_LEN[i] / (rt * sr));
        fx->rev_g_size = size;
    }
    const float rt60 = 0.4f * powf(60.0f, size);
    const float pre_d = (0.004f + 0.055f * size) * sr;
    const float rtone = tb_clamp01(p[P_REV_TONE]);
    const float damp_c = 1.0f - expf(-2.0f * (float)M_PI * (1500.0f * powf(9.5f, rtone)) / sr);
    const float rev_hp_c = 1.0f - expf(-2.0f * (float)M_PI * 130.0f / sr);
    /* the longer the tail, the more it accumulates: keep the shimmer loop below 1 */
    const float shim_norm = 0.62f / sqrtf(1.0f + rt60 * 0.55f);
    const float shim_hp_c = 1.0f - expf(-2.0f * (float)M_PI * 320.0f / sr);
    const float shim_lp_c = 1.0f - expf(-2.0f * (float)M_PI * 5200.0f / sr);

    const float side_c = 1.0f - expf(-2.0f * (float)M_PI * 150.0f / sr);

    /* pump */
    int pi = (int)(p[P_PUMP_RATE] + 0.5f);
    if (pi < 0) pi = 0;
    if (pi > 3) pi = 3;
    const float pump_div = TB_PUMP_BEATS[pi];
    const double beat_inc = (double)bpm / 60.0 / (double)sr;
    if (beat >= 0.0) fx->beat = beat;
    double bt = fx->beat;
    const float pump_c = 1.0f - expf(-1.0f / (0.004f * sr));

    for (int s = 0; s < n; s++) {
        fx->s_cho += (t_cho - fx->s_cho) * sk;
        fx->s_phs += (t_phs - fx->s_phs) * sk;
        fx->s_dly += (t_dly - fx->s_dly) * sk;
        fx->s_rev += (t_rev - fx->s_rev) * sk;
        fx->s_shim += (t_shim - fx->s_shim) * sk;
        fx->s_pump += (t_pump - fx->s_pump) * sk;
        fx->s_vhs += (t_vhs - fx->s_vhs) * sk;
        fx->s_tone += (t_tone - fx->s_tone) * sk;
        fx->s_level += (t_level - fx->s_level) * sk;
        fx->s_fb += (t_fb - fx->s_fb) * sk;
        fx->s_bus += (t_bus - fx->s_bus) * sk;

        /* ---- bus glue: transparent below the knee, rounds off stacked voices */
        float bg = 1.0f + 5.0f * fx->s_bus * fx->s_bus, bk = 0.55f - 0.25f * fx->s_bus;
        float bm = 1.0f / (1.0f + 1.6f * fx->s_bus * fx->s_bus);
        float l = fx_soft_limit(L[s] * bg, bk, 1.4f - 0.5f * fx->s_bus) * bm;
        float r = fx_soft_limit(R[s] * bg, bk, 1.4f - 0.5f * fx->s_bus) * bm;

        /* ---- Tone. Down: a darker tilt with a little more weight. Up: air,
         * and a low cut that climbs to about 540 Hz -- the high-pass a pad or an
         * arp wants so it stays out of the bass. Centre carries a little air. */
        {
            const float c = 0.0950f;          /* 700 Hz one-pole */
            float tn = fx->s_tone;
            float gh = tn >= 0.0f ? 1.22f + 1.15f * tn : 1.22f + 0.95f * tn;
            float gl = tn >= 0.0f ? 1.0f : 1.0f - 0.25f * tn;
            fx->tone_lp[0] += c * (l - fx->tone_lp[0]);
            fx->tone_lp[1] += c * (r - fx->tone_lp[1]);
            l = fx->tone_lp[0] * gl + (l - fx->tone_lp[0]) * gh;
            r = fx->tone_lp[1] * gl + (r - fx->tone_lp[1]) * gh;
            if (tn > 0.001f) {
                fx->tone_hp[0][0] += hp_c * (l - fx->tone_hp[0][0]); l -= fx->tone_hp[0][0];
                fx->tone_hp[0][1] += hp_c * (l - fx->tone_hp[0][1]); l -= fx->tone_hp[0][1];
                fx->tone_hp[1][0] += hp_c * (r - fx->tone_hp[1][0]); r -= fx->tone_hp[1][0];
                fx->tone_hp[1][1] += hp_c * (r - fx->tone_hp[1][1]); r -= fx->tone_hp[1][1];
            }
        }

        /* ---- Chorus ----------------------------------------------------- */
        {
            const int mask = FX_CHO_N - 1;
            /* the line is always written, so turning the chorus up never
             * replays stale audio */
            fx->cho_in_lp[0] += 0.72f * (l - fx->cho_in_lp[0]);
            fx->cho_in_lp[1] += 0.72f * (r - fx->cho_in_lp[1]);
            fx->cho[0][fx->cho_w] = fx->cho_in_lp[0];
            fx->cho[1][fx->cho_w] = fx->cho_in_lp[1];
            float mix = fx->s_cho;
            if (mix > 0.0005f) {
                fx->cho_ph1 += cho_r1 / sr; if (fx->cho_ph1 >= 1.0f) fx->cho_ph1 -= 1.0f;
                fx->cho_ph2 += cho_r2 / sr; if (fx->cho_ph2 >= 1.0f) fx->cho_ph2 -= 1.0f;
                float wl, wr, dry_g = 1.0f - 0.30f * mix, wet_g = 0.85f * mix;
                const float ms = sr * 0.001f;
                if (cho_mode == C_ENS) {
                    float t[3];
                    for (int k = 0; k < 3; k++) {
                        float m = 1.10f * tb_sin(fx->cho_ph1 + (float)k / 3.0f)
                                + 0.17f * tb_sin(fx->cho_ph2 + (float)k / 3.0f);
                        t[k] = fx_read(fx->cho[k == 2 ? 1 : 0], mask, fx->cho_w, (5.0f + m) * ms);
                    }
                    float t1r = fx_read(fx->cho[1], mask, fx->cho_w,
                        (5.0f + 1.10f * tb_sin(fx->cho_ph1 + 1.0f / 3.0f) + 0.17f * tb_sin(fx->cho_ph2 + 1.0f / 3.0f)) * ms);
                    wl = 0.75f * t[0] + 0.5f * t[1];
                    wr = 0.75f * t[2] + 0.5f * t1r;
                    dry_g = 1.0f - 0.72f * mix; wet_g = 1.05f * mix;
                } else if (cho_mode == C_DIM) {
                    float lf = tb_tri(fx->cho_ph1);
                    float a = fx_read(fx->cho[0], mask, fx->cho_w, (7.5f + 1.5f * lf) * ms);
                    float b = fx_read(fx->cho[1], mask, fx->cho_w, (7.5f - 1.5f * lf) * ms);
                    wl = a - 0.38f * b; wr = b - 0.38f * a;
                    dry_g = 1.0f - 0.15f * mix; wet_g = 0.75f * mix;
                } else {
                    float lf = tb_tri(fx->cho_ph1);
                    if (cho_mode == C_BOTH) lf = 0.6f * lf + 0.4f * tb_tri(fx->cho_ph2);
                    wl = fx_read(fx->cho[0], mask, fx->cho_w, (3.5f + 1.85f * lf) * ms);
                    wr = fx_read(fx->cho[1], mask, fx->cho_w, (3.5f - 1.85f * lf) * ms);
                    if (cho_mode == C_BOTH) {
                        float lf2 = tb_tri(fx->cho_ph2 + 0.25f);
                        wl = 0.7f * (wl + fx_read(fx->cho[0], mask, fx->cho_w, (4.4f - 1.5f * lf2) * ms));
                        wr = 0.7f * (wr + fx_read(fx->cho[1], mask, fx->cho_w, (4.4f + 1.5f * lf2) * ms));
                    }
                }
                l = l * dry_g + wl * wet_g;
                r = r * dry_g + wr * wet_g;
            }
            fx->cho_w = (fx->cho_w + 1) & mask;
        }

        /* ---- Phaser: six all-pass stages, the two sides a quarter apart ---- */
        if (fx->s_phs > 0.0005f) {
            if (--fx->phs_tick <= 0) {
                fx->phs_tick = 16;
                fx->phs_ph += phs_rate * 16.0f / sr; if (fx->phs_ph >= 1.0f) fx->phs_ph -= 1.0f;
                for (int ch = 0; ch < 2; ch++) {
                    float lf = 0.5f + 0.5f * tb_sin(fx->phs_ph + 0.25f * (float)ch);
                    float fc = 260.0f * exp2f(3.6f * lf);
                    float t = (float)M_PI * fc / sr;
                    t = t * (1.0f + t * t * 0.3333f);
                    fx->phs_a[ch] = (t - 1.0f) / (t + 1.0f);
                }
            }
            float d = fx->s_phs;
            float fbk = 0.72f * d, wet_g = tb_clamp01(d * 1.6f) * 0.9f, dry_g = 1.0f - 0.2f * d;
            float in[2] = { l, r }, out[2];
            for (int ch = 0; ch < 2; ch++) {
                float a = fx->phs_a[ch];
                float x = in[ch] + fbk * fx->phs_last[ch];
                for (int k = 0; k < 6; k++) {
                    float y = a * x + fx->phs_z[ch][k];
                    fx->phs_z[ch][k] = x - a * y;
                    x = y;
                }
                fx->phs_last[ch] = x;
                out[ch] = in[ch] * dry_g + x * wet_g;
            }
            l = out[0]; r = out[1];
        }

        /* ---- Delay: ping-pong, tape-style (the repeats darken and sag) ------ */
        {
            const int mask = FX_DLY_N - 1;
            fx->dly_t += (dly_target - fx->dly_t) * 0.00008f;   /* a tempo change glides */
            fx->dly_wow += 0.37f / sr; if (fx->dly_wow >= 1.0f) fx->dly_wow -= 1.0f;
            float t = fx->dly_t * (1.0f + 0.0012f * tb_sin(fx->dly_wow));
            float rl = fx_read(fx->dly[0], mask, fx->dly_w, t);
            float rr = fx_read(fx->dly[1], mask, fx->dly_w, t);
            float fb = fx->s_fb;
            /* each hop goes through the loop filter and a soft clip */
            fx->dly_lp[0] += dly_lp_c * (rr - fx->dly_lp[0]);
            fx->dly_hp[0] += dly_hp_c * (fx->dly_lp[0] - fx->dly_hp[0]);
            fx->dly_lp[1] += dly_lp_c * (rl - fx->dly_lp[1]);
            fx->dly_hp[1] += dly_hp_c * (fx->dly_lp[1] - fx->dly_hp[1]);
            float to_l = tb_tanh((fx->dly_lp[0] - fx->dly_hp[0]) * fb);
            float to_r = tb_tanh((fx->dly_lp[1] - fx->dly_hp[1]) * fb);
            fx->dly[0][fx->dly_w] = 0.5f * (l + r) + to_l;
            fx->dly[1][fx->dly_w] = to_r;
            fx->dly_w = (fx->dly_w + 1) & mask;
            float mix = fx->s_dly * 0.9f;
            l += rl * mix; r += rr * mix;
        }

        /* ---- Reverb: 8-line feedback network, modulated, with shimmer ------ */
        {
            const int pmask = FX_PRE_N - 1, amask = FX_AP_N - 1, rmask = REV_N - 1;
            /* low cut on the way in, then pre-delay */
            fx->rev_hp[0] += rev_hp_c * (l - fx->rev_hp[0]);
            fx->rev_hp[1] += rev_hp_c * (r - fx->rev_hp[1]);
            fx->pre[0][fx->pre_w] = l - fx->rev_hp[0];
            fx->pre[1][fx->pre_w] = r - fx->rev_hp[1];
            float in[2];
            in[0] = fx_read(fx->pre[0], pmask, fx->pre_w, pre_d) + fx->shim_out;
            in[1] = fx_read(fx->pre[1], pmask, fx->pre_w, pre_d) + fx->shim_out;
            fx->pre_w = (fx->pre_w + 1) & pmask;
            /* input diffusion */
            for (int ch = 0; ch < 2; ch++)
                for (int k = 0; k < 2; k++) {
                    float *b = fx->ap[ch][k];
                    float d = b[(fx->ap_w - REV_AP_LEN[ch][k]) & amask];
                    float v = in[ch] - 0.62f * d;
                    b[fx->ap_w] = v;
                    in[ch] = d + 0.62f * v;
                }
            fx->ap_w = (fx->ap_w + 1) & amask;

            float y[REV_LINES], sum = 0.0f;
            for (int i = 0; i < REV_LINES; i++) {
                float v;
                if (i & 1) {
                    int m = i >> 1;
                    fx->rev_mod[m] += REV_MOD_HZ[m] / sr; if (fx->rev_mod[m] >= 1.0f) fx->rev_mod[m] -= 1.0f;
                    v = fx_read(fx->rev[i], rmask, fx->rev_w, (float)REV_LEN[i] + 9.0f * tb_sin(fx->rev_mod[m]));
                } else {
                    v = fx->rev[i][(fx->rev_w - REV_LEN[i]) & rmask];
                }
                fx->rev_lp[i] += damp_c * (v - fx->rev_lp[i]);
                y[i] = fx->rev_lp[i];
                sum += y[i];
            }
            sum *= 2.0f / (float)REV_LINES;
            for (int i = 0; i < REV_LINES; i++)
                fx->rev[i][fx->rev_w] = (y[i] - sum) * fx->rev_g[i] + 0.5f * in[i & 1];
            fx->rev_w = (fx->rev_w + 1) & rmask;

            float wl = 0.42f * (y[0] - y[2] + y[4] - y[6]);
            float wr = 0.42f * (y[1] - y[3] + y[5] - y[7]);

            /* Shimmer: the tail, an octave up, fed back into the reverb's input */
            if (fx->s_shim > 0.0005f || fabsf(fx->shim_out) > 1e-7f) {
                const int smask = FX_SHIM_N - 1;
                float m = 0.5f * (wl + wr);
                fx->shim_hp += shim_hp_c * (m - fx->shim_hp);
                fx->shim[fx->shim_w] = m - fx->shim_hp;
                fx->shim_ph += 1.0f / FX_SHIM_W; if (fx->shim_ph >= 1.0f) fx->shim_ph -= 1.0f;
                float ph2 = fx->shim_ph + 0.5f; if (ph2 >= 1.0f) ph2 -= 1.0f;
                float a = fx_read(fx->shim, smask, fx->shim_w, FX_SHIM_W * (1.0f - fx->shim_ph) + 2.0f);
                float b = fx_read(fx->shim, smask, fx->shim_w, FX_SHIM_W * (1.0f - ph2) + 2.0f);
                float ga = 1.0f - fabsf(2.0f * fx->shim_ph - 1.0f), gb = 1.0f - fabsf(2.0f * ph2 - 1.0f);
                float up = a * ga + b * gb;
                fx->shim_lp1 += shim_lp_c * (up - fx->shim_lp1);
                fx->shim_lp2 += shim_lp_c * (fx->shim_lp1 - fx->shim_lp2);
                fx->shim_w = (fx->shim_w + 1) & smask;
                fx->shim_out = tb_tanh(fx->shim_lp2 * fx->s_shim * shim_norm * 2.0f) * 0.5f;
            } else {
                fx->shim_out = 0.0f;
            }

            float mix = fx->s_rev;
            l += wl * mix; r += wr * mix;
        }

        /* Whatever the stereo effects did, the bottom goes back to the middle.
         * A two-way crossover at 150 Hz: each side keeps its own top, and both
         * get the same low band, taken from their sum. A chorus or a phaser on
         * a bass widens its body and leaves its weight alone.
         *
         * It is a crossover and not a high-pass on the L-R difference because
         * that turns the phase for two octaves above its corner and leaks one
         * side into the other well into the midrange (-16 dB at 1 kHz, as
         * measured). Here the leak is the low-pass's own stopband (-37 dB).
         * The top band is subtracted, not added: with these two-pole halves
         * that is what makes the two bands sum flat. */
        {
            float lo = 0.5f * (l + r), hl = l, hr = r;
            for (int k = 0; k < 2; k++) {
                fx->xo_lo[k] += side_c * (lo - fx->xo_lo[k]); lo = fx->xo_lo[k];
                fx->xo_l[k] += side_c * (hl - fx->xo_l[k]);   hl -= fx->xo_l[k];
                fx->xo_r[k] += side_c * (hr - fx->xo_r[k]);   hr -= fx->xo_r[k];
            }
            l = lo - hl; r = lo - hr;
        }

        l += sub[s]; r += sub[s];

        /* ---- Pump: the sidechain duck, with no kick needed ----------------- */
        {
            float tg = 1.0f;
            if (fx->s_pump > 0.0005f) {
                double ph = bt / (double)pump_div;
                float fr = (float)(ph - floor(ph));
                float x = 1.0f - tb_clamp01(fr / 0.72f);
                tg = 1.0f - fx->s_pump * 0.96f * x * x;
            }
            fx->pump_g += (tg - fx->pump_g) * pump_c;
            l *= fx->pump_g; r *= fx->pump_g;
            bt += beat_inc;
        }

        /* ---- VHS: wow, flutter, lost top, a little dirt ---------------------- */
        {
            const int mask = FX_VHS_N - 1;
            float v = fx->s_vhs, v2 = v * v;
            fx->vhs[0][fx->vhs_w] = l;
            fx->vhs[1][fx->vhs_w] = r;
            if (v > 0.0005f) {
                fx->vhs_ph1 += 0.55f / sr; if (fx->vhs_ph1 >= 1.0f) fx->vhs_ph1 -= 1.0f;
                fx->vhs_ph2 += 1.31f / sr; if (fx->vhs_ph2 >= 1.0f) fx->vhs_ph2 -= 1.0f;
                fx->vhs_ph3 += 7.3f / sr;  if (fx->vhs_ph3 >= 1.0f) fx->vhs_ph3 -= 1.0f;
                if (--fx->vhs_rnd_wait <= 0) {
                    fx->vhs_rnd_to = tb_frand(&fx->rng);
                    fx->vhs_rnd_wait = 9000 + (int)(tb_rand(&fx->rng) & 16383);
                }
                fx->vhs_rnd += (fx->vhs_rnd_to - fx->vhs_rnd) * 0.00004f;
                const float ms = sr * 0.001f;
                float wow = 3.2f * v2 * (0.7f * tb_sin(fx->vhs_ph1) + 0.3f * tb_sin(fx->vhs_ph2) + 0.5f * fx->vhs_rnd);
                float flut = 0.05f * v * tb_sin(fx->vhs_ph3);
                float d = 1.0f + (5.0f * v2 + wow + flut) * ms;
                if (d < 1.0f) d = 1.0f;
                float wl = fx_read(fx->vhs[0], mask, fx->vhs_w, d);
                float wr = fx_read(fx->vhs[1], mask, fx->vhs_w, d);
                /* the top goes: 16 kHz down to about 3.5 kHz */
                float c = 1.0f - expf(-2.0f * (float)M_PI * (16000.0f * powf(0.22f, v)) / sr);
                fx->vhs_lp[0][0] += c * (wl - fx->vhs_lp[0][0]); fx->vhs_lp[0][1] += c * (fx->vhs_lp[0][0] - fx->vhs_lp[0][1]);
                fx->vhs_lp[1][0] += c * (wr - fx->vhs_lp[1][0]); fx->vhs_lp[1][1] += c * (fx->vhs_lp[1][0] - fx->vhs_lp[1][1]);
                float dg = 1.0f + 2.2f * v, mk = 1.0f / (1.0f + 1.1f * v);
                wl = tb_tanh(fx->vhs_lp[0][1] * dg * TB_OUT_MAKEUP) * mk * (1.0f / TB_OUT_MAKEUP);
                wr = tb_tanh(fx->vhs_lp[1][1] * dg * TB_OUT_MAKEUP) * mk * (1.0f / TB_OUT_MAKEUP);
                /* hiss rides the signal, so silence stays silent */
                float lev = fabsf(wl) + fabsf(wr);
                fx->vhs_env += (lev - fx->vhs_env) * (lev > fx->vhs_env ? 0.01f : 0.00008f);
                float hg = 0.0020f * v2 * tb_clamp01(fx->vhs_env * 100.0f);
                wl += tb_frand(&fx->rng) * hg;
                wr += tb_frand(&fx->rng) * hg;
                /* blend over the first few percent so 0 is a true bypass */
                float w = tb_clamp01(v * 25.0f);
                l += (wl - l) * w; r += (wr - r) * w;
            }
            fx->vhs_w = (fx->vhs_w + 1) & mask;
        }

        /* ---- Level and the output limiter ---------------------------------- */
        {
            float g = fx->s_level * (1.0f / 0.7f);
            g *= g * TB_OUT_MAKEUP;
            L[s] = fx_soft_limit(l * g, 0.60f, 0.98f);
            R[s] = fx_soft_limit(r * g, 0.60f, 0.98f);
        }
    }
    fx->beat = bt;
}

#endif

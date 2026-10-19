/*
 * tenebrae_engine.h -- voices: macro oscillator, drive, filter, envelopes, LFO.
 *
 * Signal path of one voice:
 *
 *   stack of up to 7 detuned copies of the MODEL  --+
 *   LAYER (sub / interval copy / noise) ------------+-> pre-drive (asymmetric
 *   soft clip) -> state-variable filter -> amp envelope -> post-drive (hard
 *   clip) -> pan -> voice bus
 *
 * Where the character comes from (and what to leave alone if you want to keep
 * it):
 *   HEFT  the saw carries extra fundamental; the low-pass output runs hot into
 *         the stages after it; the amp follows the SQUARE of its envelope, so
 *         decays are punchy; a fresh note kicks the filter's band state.
 *   GRIT  oscillators are PolyBLEP, not oversampled; the filter is a Chamberlin
 *         core iterated twice per sample with a held input, not a ZDF design;
 *         Drive's upper half is a plain hard clip AFTER the amp, per voice, so
 *         a chord never intermodulates and a decaying note cleans up as it
 *         fades.
 *
 * Real-time rules: nothing here allocates, locks, logs or touches a file.
 */
#ifndef TENEBRAE_ENGINE_H
#define TENEBRAE_ENGINE_H

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include "tenebrae_params.h"

#define TB_SR        44100.0f
/* overridable from the compiler command line, so the cost of more voices can
 * be measured (tools/bench_move.sh) without editing this file */
#ifndef TB_VOICES
#define TB_VOICES    8
#endif
#define TB_STACK     7
#define TB_CTL       16          /* samples per control tick */
#define TB_SIN_N     4096
#define TB_WT_N      2048
#define TB_WT_WAVES  8
#define TB_WT_MIPS   10
#define TB_WT_HARM   512
#define TB_NOTE_STACK 16

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ------------------------------------------------------------------------ */
/* Shared read-only tables, built once per process                           */
/* ------------------------------------------------------------------------ */

static float g_vs_sin[TB_SIN_N + 1];
static float g_vs_wt[TB_WT_WAVES][TB_WT_MIPS][TB_WT_N + 1];
static pthread_once_t g_vs_tables_once = PTHREAD_ONCE_INIT;

/* Harmonic recipe of each wavetable frame, h = 1..TB_WT_HARM. The frames are
 * ordered so that Shape sweeps from round to glassy to harsh. */
static float tb_wt_harm(int wave, int h)
{
    float fh = (float)h;
    switch (wave) {
    case 0: /* soft: near-sine with a little warmth */
        return h <= 6 ? 1.0f / (fh * fh * fh * 0.5f + 0.5f) : 0.0f;
    case 1: { /* organ: drawbars */
        static const float bars[9] = { 1.0f, 0.8f, 0.45f, 0.6f, 0.0f, 0.3f, 0.0f, 0.35f, 0.0f };
        return h <= 9 ? bars[h - 1] : (h == 12 ? 0.15f : (h == 16 ? 0.12f : 0.0f));
    }
    case 2: /* hollow: odd harmonics */
        return (h & 1) ? 1.0f / fh : 0.0f;
    case 3: { /* reed: 1/h with a nasal bump around the 7th */
        float d = (fh - 7.0f) / 2.5f;
        return (1.0f / fh) * (1.0f + 3.0f * expf(-d * d));
    }
    case 4: { /* glass: sparse, slowly decaying partials */
        static const int part[10] = { 1, 4, 7, 11, 16, 22, 29, 37, 46, 56 };
        for (int i = 0; i < 10; i++) if (part[i] == h) return 1.0f / (1.0f + 0.35f * (float)i);
        return 0.0f;
    }
    case 5: { /* voice: two formant bumps */
        float d1 = (fh - 4.0f) / 1.6f, d2 = (fh - 14.0f) / 3.0f;
        return (0.25f / fh) + expf(-d1 * d1) + 0.55f * expf(-d2 * d2);
    }
    case 6: /* comb: every third harmonic missing, slow rolloff */
        return (h % 3) ? 1.0f / powf(fh, 0.75f) : 0.0f;
    default: /* spike: nearly flat spectrum */
        return 1.0f / sqrtf(fh);
    }
}

static void tb_build_tables(void)
{
    for (int i = 0; i <= TB_SIN_N; i++)
        g_vs_sin[i] = (float)sin(2.0 * M_PI * (double)i / (double)TB_SIN_N);

    static float amp[TB_WT_HARM + 1];
    for (int w = 0; w < TB_WT_WAVES; w++) {
        for (int h = 1; h <= TB_WT_HARM; h++) amp[h] = tb_wt_harm(w, h);
        float norm = 1.0f;
        for (int m = 0; m < TB_WT_MIPS; m++) {
            int maxh = TB_WT_HARM >> m;
            float *t = g_vs_wt[w][m];
            float peak = 0.0f;
            for (int n = 0; n < TB_WT_N; n++) {
                float s = 0.0f;
                for (int h = 1; h <= maxh; h++) {
                    if (amp[h] == 0.0f) continue;
                    s += amp[h] * g_vs_sin[(h * n * (TB_SIN_N / TB_WT_N)) & (TB_SIN_N - 1)];
                }
                t[n] = s;
                if (fabsf(s) > peak) peak = fabsf(s);
            }
            /* one gain per wave, taken from the full-bandwidth level, so the
             * loudness does not step as the pitch crosses a mip boundary */
            if (m == 0) norm = peak > 0.0f ? 1.0f / peak : 1.0f;
            for (int n = 0; n < TB_WT_N; n++) t[n] *= norm;
            t[TB_WT_N] = t[0];
        }
    }
}

/* ------------------------------------------------------------------------ */
/* Small helpers                                                             */
/* ------------------------------------------------------------------------ */

static inline float tb_clamp(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
static inline float tb_clamp01(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
static inline float tb_lerp(float a, float b, float t) { return a + (b - a) * t; }
static inline float tb_smoothstep(float x) { x = tb_clamp01(x); return x * x * (3.0f - 2.0f * x); }

/* sine of a phase in CYCLES, any sign */
static inline float tb_sin(float ph)
{
    ph -= (float)(int)ph;
    if (ph < 0.0f) ph += 1.0f;
    float f = ph * (float)TB_SIN_N;
    int i = (int)f;
    float fr = f - (float)i;
    return g_vs_sin[i] + (g_vs_sin[i + 1] - g_vs_sin[i]) * fr;
}

/* tanh-shaped soft clip, exact +-1 beyond |x| = 3 */
static inline float tb_tanh(float x)
{
    if (x > 3.0f) return 1.0f;
    if (x < -3.0f) return -1.0f;
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

static inline uint32_t tb_rand(uint32_t *s)
{
    uint32_t x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return *s = x;
}
/* -1..1 */
static inline float tb_frand(uint32_t *s) { return (float)(int32_t)tb_rand(s) * (1.0f / 2147483648.0f); }

static inline float tb_blep(float t, float dt, float idt)
{
    if (t < dt) { float x = t * idt; return x + x - x * x - 1.0f; }
    if (t > 1.0f - dt) { float x = (t - 1.0f) * idt; return x * x + x + x + 1.0f; }
    return 0.0f;
}

/* ------------------------------------------------------------------------ */
/* Types                                                                     */
/* ------------------------------------------------------------------------ */

typedef struct { float ph, ph2, z; } tb_osc_t;

enum { ENV_IDLE, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE };
typedef struct { int stage; float pos, value, from; } tb_env_t;

typedef struct { float band, low; } tb_svf_t;

/* what one oscillator needs to know for a control tick */
typedef struct {
    float a, b, c, d, e, f, g;
    const float *t0, *t1;
} tb_octl_t;

typedef struct {
    int      active;
    int      note;          /* sounding MIDI note (after octave) */
    int      key;           /* the key that owns the voice */
    int      gate;
    int      sustained;     /* key released while the pedal is down */
    uint32_t stamp;
    float    vel, vel_cur;
    float    pressure;
    float    pitch;         /* current pitch, semitones (glides) */
    float    target;

    tb_osc_t osc[TB_STACK];
    tb_osc_t lay;
    tb_env_t aenv, fenv;
    tb_svf_t flt[2][2];     /* [channel][stage] */
    float    F, q;          /* filter coefficients in use */
    float    fgain;

    float    drift, drift_to;   /* per-voice pitch jitter, cents: a new target every 15-110 ms */
    int      drift_wait;
    float    wob_ph1, wob_ph2;  /* per-voice slow wobble: two sines at this voice's own rates */
    float    wob_r1, wob_r2;
    float    n_detune, n_cut, n_time; /* per-note scatter from Age */
    float    t_on;          /* seconds since note-on */
    float    declick;       /* 0..1 fade-in of a voice started from silence */
    float    kill;          /* >0: fading out fast (all notes off) */
    float    lfo_phase_off;
    float    sh_val, sh_cur;
    float    pan_cur;
    float    main_g, lay_g;
    uint32_t rng;
} tb_voice_t;

typedef struct {
    float p[P_COUNT];        /* parameter values as set */
    float sm_cut, sm_shape, sm_blend, sm_reso, sm_drive, sm_stack; /* smoothed copies */
    tb_voice_t v[TB_VOICES];
    uint32_t stamp;
    uint32_t rng;
    float bend;              /* semitones */
    float modwheel;          /* 0..1 */
    float chan_pressure;     /* 0..1 */
    int   sustain;
    int   octave;
    float lfo_ph;
    float lfo_sh;            /* current sample & hold value */
    float vib_ph;
    int   snap;              /* take the smoothed controls at once (waking from silence) */
    float bpm;               /* session tempo, set by the wrapper each block */
    double beat;             /* beats since the transport started, < 0 when stopped */
    double ramp_beat;        /* free-running beat counter for the synced ramp */
    float ramp_val;
    uint8_t mono_notes[TB_NOTE_STACK];
    uint8_t mono_vels[TB_NOTE_STACK];
    int   mono_n;
    float dc_x[2], dc_y[2];
    int   active_voices;
} tb_engine_t;

/* Per control tick, the same for every voice */
typedef struct {
    int   model, layer, flt_type, lfo_dest, mono, legato;
    float glide_coef;
    float shape, shape_env, age, age2;
    float main_g, lay_g;
    int   ncopies;
    float width;
    float det[TB_STACK], gain[TB_STACK], panw[TB_STACK];
    float cut, q, q2, res_gain;
    float pre_gain, pre_bias, pre_dc, pre_make;
    int   post_on;
    float post_gain, post_ceil, post_make;
    float fenv_oct, f_a, f_d, f_s;
    float a_a, a_d, a_s, a_r;
    float lfo_tri_ph, lfo_depth, lfo_sh, lfo_ramp;
    float vib;               /* extra vibrato from wheel / pressure, -1..1 scaled */
    float velk;
    float bend;
    float zap_depth, zap_tau;
    float model_gain;
} tb_frame_t;

/* ------------------------------------------------------------------------ */
/* Envelope: finite quadratic ramps (the amp takes the square of this)       */
/* ------------------------------------------------------------------------ */

static inline float tb_attack_step(float k)  { return 1.0f / (0.001f * powf(8000.0f, tb_clamp01(k)) * TB_SR); }
static inline float tb_decay_step(float k)   { return 1.0f / (0.012f * powf(2000.0f, tb_clamp01(k)) * TB_SR); }

static inline void tb_env_gate_on(tb_env_t *e)
{
    float v = tb_clamp01(e->value);
    e->stage = ENV_ATTACK;
    e->pos = 1.0f - sqrtf(1.0f - v);   /* carry on up the curve from where it is */
}

static inline void tb_env_gate_off(tb_env_t *e)
{
    if (e->stage == ENV_IDLE) return;
    e->stage = ENV_RELEASE;
    e->from = e->value;
    e->pos = 0.0f;
}

static inline float tb_env_tick(tb_env_t *e, float a, float d, float s, float r)
{
    float x;
    switch (e->stage) {
    case ENV_ATTACK:
        e->pos += a;
        if (e->pos >= 1.0f) { e->value = 1.0f; e->stage = ENV_DECAY; e->pos = 0.0f; }
        else { x = 1.0f - e->pos; e->value = 1.0f - x * x; }
        break;
    case ENV_DECAY:
        e->pos += d;
        if (e->pos >= 1.0f) { e->value = s; e->stage = ENV_SUSTAIN; }
        else { x = 1.0f - e->pos; e->value = s + (1.0f - s) * x * x; }
        break;
    case ENV_SUSTAIN:
        /* follow the knob without a step */
        e->value += (s - e->value) * 0.002f;
        break;
    case ENV_RELEASE:
        e->pos += r;
        if (e->pos >= 1.0f) { e->value = 0.0f; e->stage = ENV_IDLE; }
        else { x = 1.0f - e->pos; e->value = e->from * x * x; }
        break;
    default:
        e->value = 0.0f;
        break;
    }
    return e->value;
}

/* ------------------------------------------------------------------------ */
/* Oscillator models                                                         */
/* ------------------------------------------------------------------------ */

/* Vowels oo - oh - ah - eh - ee: three formants (Hz) and their levels */
static const float TB_VOWEL_F[5][3] = {
    { 325.0f, 700.0f, 2530.0f }, { 450.0f, 800.0f, 2830.0f }, { 730.0f, 1090.0f, 2440.0f },
    { 530.0f, 1840.0f, 2480.0f }, { 270.0f, 2290.0f, 3010.0f }
};
static const float TB_VOWEL_A[5][3] = {
    { 1.0f, 0.35f, 0.06f }, { 1.0f, 0.5f, 0.10f }, { 1.0f, 0.6f, 0.22f },
    { 1.0f, 0.45f, 0.26f }, { 1.0f, 0.36f, 0.30f }
};

static const float TB_MODEL_GAIN[M_COUNT] = { 1.00f, 0.68f, 1.05f, 1.10f, 0.90f, 1.85f, 1.70f, 0.95f };
/* Stack copies a model may run. The three built from several sines per copy
 * stop at five: a seven-wide FM stack costs more than it adds. */
static const int TB_MODEL_COPIES[M_COUNT] = { 7, 7, 7, 5, 5, 7, 5, 7 };

/* Model-specific control values for an oscillator at `dt` cycles per sample */
static void tb_osc_control(int model, float shape, float dt, float note, tb_octl_t *c)
{
    memset(c, 0, sizeof(*c));
    switch (model) {
    case M_SAW:
        /* 0 triangle - 0.5 saw - 1.0 saw + square (the two-waves-on Juno tone) */
        if (shape < 0.5f) {
            float x = shape * 2.0f;
            c->a = 1.25f * (1.0f - x); c->b = x; c->c = 0.0f; c->d = 0.17f * x;
        } else {
            float x = (shape - 0.5f) * 2.0f;
            c->a = 0.0f; c->b = 1.0f - 0.30f * x; c->c = 0.62f * x; c->d = 0.17f;
        }
        break;
    case M_PULSE: {
        float w = 0.5f - 0.47f * shape;
        c->a = w; c->b = 0.85f * (1.0f + 0.9f * shape); c->c = 2.0f * w - 1.0f;
        break;
    }
    case M_SYNC: {
        float ratio = exp2f(shape * 3.3f);
        float lim = 0.45f / (dt > 1e-6f ? dt : 1e-6f);
        c->a = ratio < lim ? ratio : lim;
        if (c->a < 1.0f) c->a = 1.0f;
        break;
    }
    case M_FM: {
        /* 1:1, index up to about 7 rad; pulled back at the top of the keyboard */
        float ks = dt > 0.018f ? sqrtf(0.018f / dt) : 1.0f;
        float idx = 7.2f * powf(shape, 1.5f) * ks;
        c->a = idx * (1.0f / (2.0f * (float)M_PI));
        c->b = 0.13f * shape;
        break;
    }
    case M_BELL: {
        /* the modulators sit 3.5 and 7 times up, so they are pulled back hard
         * toward the top of the keyboard, before they fold over */
        float ks = dt > 0.010f ? 0.010f / dt : 1.0f;
        float ia = 4.6f * powf(tb_clamp01(shape * 2.0f), 1.4f) * ks;
        float ib = 3.4f * powf(tb_clamp01(shape * 2.0f - 1.0f), 1.2f) * ks * sqrtf(ks);
        c->a = ia * (1.0f / (2.0f * (float)M_PI));
        c->b = ib * (1.0f / (2.0f * (float)M_PI));
        break;
    }
    case M_WAVE: {
        float pos = shape * (float)(TB_WT_WAVES - 1);
        int w0 = (int)pos; if (w0 > TB_WT_WAVES - 2) w0 = TB_WT_WAVES - 2;
        int m = 0; float h = (float)(TB_WT_HARM * 2) * dt;
        while (h > 1.0f && m < TB_WT_MIPS - 1) { h *= 0.5f; m++; }
        c->t0 = g_vs_wt[w0][m]; c->t1 = g_vs_wt[w0 + 1][m];
        c->a = pos - (float)w0;
        break;
    }
    case M_VOX: {
        float pos = shape * 4.0f;
        int i0 = (int)pos; if (i0 > 3) i0 = 3;
        float fr = pos - (float)i0;
        float f = dt * TB_SR;
        /* formants ride up a little with the key so the top octaves stay human */
        float track = tb_clamp(1.0f + 0.12f * (note - 60.0f) / 12.0f, 0.85f, 1.35f);
        float r[3], am[3];
        for (int k = 0; k < 3; k++) {
            float fk = tb_lerp(TB_VOWEL_F[i0][k], TB_VOWEL_F[i0 + 1][k], fr) * track;
            r[k] = fk / (f > 1.0f ? f : 1.0f);
            if (r[k] < 1.0f) r[k] = 1.0f;
            am[k] = tb_lerp(TB_VOWEL_A[i0][k], TB_VOWEL_A[i0 + 1][k], fr);
        }
        c->a = r[0]; c->b = r[1]; c->c = r[2];
        c->e = am[0]; c->f = am[1]; c->g = am[2];
        break;
    }
    default:
        break;
    }
}

/* One sample of an oscillator model */
static inline float tb_osc_tick(int model, tb_osc_t *o, float dt, float idt, const tb_octl_t *c)
{
    float p, out;
    switch (model) {
    case M_SAW: {
        o->ph += dt; if (o->ph >= 1.0f) o->ph -= 1.0f;
        p = o->ph;
        out = c->b * (2.0f * p - 1.0f - tb_blep(p, dt, idt));
        if (c->a > 0.0f) out += c->a * (1.0f - 4.0f * fabsf(p - 0.5f));
        if (c->c > 0.0f) {
            float p2 = p + 0.5f; if (p2 >= 1.0f) p2 -= 1.0f;
            float sq = (p < 0.5f ? -1.0f : 1.0f) - tb_blep(p, dt, idt) + tb_blep(p2, dt, idt);
            out += c->c * sq;
        }
        if (c->d > 0.0f) out -= c->d * tb_sin(p);
        return out;
    }
    case M_PULSE: {
        o->ph += dt; if (o->ph >= 1.0f) o->ph -= 1.0f;
        p = o->ph;
        float p2 = p - c->a; if (p2 < 0.0f) p2 += 1.0f;
        float sq = (p < c->a ? 1.0f : -1.0f) + tb_blep(p, dt, idt) - tb_blep(p2, dt, idt);
        return c->b * (sq - c->c);
    }
    case M_SYNC: {
        float m = o->ph + dt, s = o->ph2, ds = dt * c->a;
        float ids = 1.0f / ds;
        if (m >= 1.0f) {
            /* master wrapped inside this sample: restart the slave there */
            m -= 1.0f;
            float tau = m * idt;
            float s_at = s + ds * (1.0f - tau);
            s_at -= (float)(int)s_at;
            s = ds * tau;
            float k = 1.0f - tau;
            out = 2.0f * s - 1.0f + s_at * k * k;
        } else {
            s += ds; if (s >= 1.0f) s -= 1.0f;
            out = 2.0f * s - 1.0f - tb_blep(s, ds, ids);
            if (m > 1.0f - dt) {
                /* the restart lands in the next sample: its leading half */
                float x = (m - 1.0f) * idt;
                float s_at = s + ds * (1.0f - m) * idt;
                s_at -= (float)(int)s_at;
                float k = 1.0f + x;
                out -= s_at * k * k;
            }
        }
        o->ph = m; o->ph2 = s;
        return out;
    }
    case M_FM: {
        o->ph += dt; if (o->ph >= 1.0f) o->ph -= 1.0f;
        o->ph2 += dt; if (o->ph2 >= 1.0f) o->ph2 -= 1.0f;
        float m = tb_sin(o->ph2 + c->b * o->z);
        o->z = 0.5f * (o->z + m);
        return tb_sin(o->ph + c->a * m);
    }
    case M_BELL: {
        o->ph += dt; if (o->ph >= 1.0f) o->ph -= 1.0f;
        o->ph2 += dt * 3.5f; o->ph2 -= (float)(int)o->ph2;
        o->z += dt * 7.0711f; o->z -= (float)(int)o->z;
        return tb_sin(o->ph + c->a * tb_sin(o->ph2) + c->b * tb_sin(o->z));
    }
    case M_WAVE: {
        o->ph += dt; if (o->ph >= 1.0f) o->ph -= 1.0f;
        float f = o->ph * (float)TB_WT_N;
        int i = (int)f; float fr = f - (float)i;
        float a = c->t0[i] + (c->t0[i + 1] - c->t0[i]) * fr;
        float b = c->t1[i] + (c->t1[i + 1] - c->t1[i]) * fr;
        return a + (b - a) * c->a;
    }
    case M_VOX: {
        o->ph += dt; if (o->ph >= 1.0f) o->ph -= 1.0f;
        p = o->ph;
        float w = (1.0f - p); w *= w;
        return w * (c->e * tb_sin(p * c->a) + c->f * tb_sin(p * c->b) + c->g * tb_sin(p * c->c));
    }
    default: { /* M_ZAP: a sine with a little self-modulation for bite */
        o->ph += dt; if (o->ph >= 1.0f) o->ph -= 1.0f;
        return tb_sin(o->ph + 0.07f * tb_sin(o->ph));
    }
    }
}

/* ------------------------------------------------------------------------ */
/* Engine                                                                    */
/* ------------------------------------------------------------------------ */

static void tb_engine_init(tb_engine_t *e)
{
    pthread_once(&g_vs_tables_once, tb_build_tables);
    memset(e, 0, sizeof(*e));
    for (int i = 0; i < P_COUNT; i++) e->p[i] = TB_PARAMS[i].def;
    e->rng = 0x1234ABCDu;
    e->bpm = 120.0f;
    e->beat = -1.0;
    for (int i = 0; i < TB_VOICES; i++) {
        e->v[i].rng = 0x9E3779B9u * (uint32_t)(i + 1) + 0x7F4A7C15u;
        e->v[i].lfo_phase_off = (float)i * 0.6180339f;
        e->v[i].lfo_phase_off -= (float)(int)e->v[i].lfo_phase_off;
        /* each voice wobbles at its own pair of rates, from its own starting
         * point, and keeps running between notes: no two voices move together */
        e->v[i].wob_r1 = 0.42f * (1.0f + 0.30f * tb_frand(&e->v[i].rng));
        e->v[i].wob_r2 = 1.05f * (1.0f + 0.25f * tb_frand(&e->v[i].rng));
        e->v[i].wob_ph1 = 0.5f + 0.5f * tb_frand(&e->v[i].rng);
        e->v[i].wob_ph2 = 0.5f + 0.5f * tb_frand(&e->v[i].rng);
    }
    e->sm_cut = e->p[P_CUTOFF]; e->sm_shape = e->p[P_SHAPE]; e->sm_blend = e->p[P_BLEND];
    e->sm_reso = e->p[P_RESO]; e->sm_drive = e->p[P_DRIVE]; e->sm_stack = e->p[P_STACK];
}

static inline int tb_voice_mode(const tb_engine_t *e)
{
    int v = (int)(e->p[P_VOICE] + 0.5f);
    return v < 0 ? 0 : (v >= TB_VOICE_OPTION_COUNT ? TB_VOICE_OPTION_COUNT - 1 : v);
}

static void tb_voice_start(tb_engine_t *e, tb_voice_t *v, int key, float vel, int retrigger)
{
    int note = key + e->octave * 12;
    if (note < 0) note = 0;
    if (note > 127) note = 127;
    float age = e->p[P_AGE];
    int fresh = !v->active || v->aenv.stage == ENV_IDLE;

    v->key = key;
    v->note = note;
    v->target = (float)note;
    v->gate = 1;
    v->sustained = 0;
    v->stamp = ++e->stamp;
    v->vel = vel;
    v->kill = 0.0f;
    v->pressure = 0.0f;

    if (fresh) {
        /* Phase start: Age 0 restarts every oscillator in step (tight, the same
         * attack every time); from about 12 % up they start anywhere, as free
         * running oscillators do. */
        float spread = tb_clamp01(age * 8.0f);
        for (int i = 0; i < TB_STACK; i++) {
            v->osc[i].ph = spread * (0.5f + 0.5f * tb_frand(&v->rng));
            v->osc[i].ph2 = v->osc[i].ph;
            v->osc[i].z = 0.0f;
        }
        v->lay.ph = spread * (0.5f + 0.5f * tb_frand(&v->rng));
        v->lay.ph2 = v->lay.ph; v->lay.z = 0.0f;
        memset(v->flt, 0, sizeof(v->flt));
        v->aenv.value = 0.0f; v->fenv.value = 0.0f;
        v->declick = 0.0f;
        v->pitch = (float)note;
        v->vel_cur = vel;
        v->F = 0.0f;
        v->sh_cur = v->sh_val = e->lfo_sh;
        v->pan_cur = 0.0f;
    }
    if (fresh || retrigger) {
        tb_env_gate_on(&v->aenv);
        tb_env_gate_on(&v->fenv);
        v->t_on = 0.0f;
        /* per-note scatter */
        v->n_detune = tb_frand(&v->rng) * 14.0f * age * age;
        v->n_cut = tb_frand(&v->rng) * 0.30f * age;
        v->n_time = 1.0f + tb_frand(&v->rng) * 0.18f * age;
        if (fresh) {
            /* the note-on thump: a fresh voice kicks its filter's band state */
            float a_k = e->p[P_AMP_A];
            if (a_k < 0.12f) {
                float cut_hz = 20.0f * exp2f(10.0f * e->p[P_CUTOFF]);
                float kick = 0.16f * expf(-cut_hz / 500.0f) * (1.0f + 1.5f * e->p[P_RESO]);
                v->flt[0][0].band = kick; v->flt[1][0].band = kick;
            }
        }
    }
    v->active = 1;
}

static tb_voice_t *tb_pick_voice(tb_engine_t *e, int key)
{
    tb_voice_t *best = NULL;
    /* the same key again takes its own voice back */
    for (int i = 0; i < TB_VOICES; i++)
        if (e->v[i].active && e->v[i].key == key) return &e->v[i];
    for (int i = 0; i < TB_VOICES; i++)
        if (!e->v[i].active) return &e->v[i];
    /* quietest released voice, else the oldest */
    float lo = 2.0f;
    for (int i = 0; i < TB_VOICES; i++)
        if (!e->v[i].gate && e->v[i].aenv.value < lo) { lo = e->v[i].aenv.value; best = &e->v[i]; }
    if (best) return best;
    uint32_t oldest = 0xFFFFFFFFu;
    for (int i = 0; i < TB_VOICES; i++)
        if (e->v[i].stamp < oldest) { oldest = e->v[i].stamp; best = &e->v[i]; }
    return best;
}

static void tb_mono_remove(tb_engine_t *e, int key)
{
    int w = 0;
    for (int i = 0; i < e->mono_n; i++)
        if (e->mono_notes[i] != key) { e->mono_notes[w] = e->mono_notes[i]; e->mono_vels[w] = e->mono_vels[i]; w++; }
    e->mono_n = w;
}

static void tb_note_on(tb_engine_t *e, int key, int velocity)
{
    float vel = (float)velocity / 127.0f;
    int mode = tb_voice_mode(e);
    if (mode == 0) {
        tb_voice_t *v = tb_pick_voice(e, key);
        if (v) tb_voice_start(e, v, key, vel, 1);
        return;
    }
    /* mono: voice 0 only, last note priority */
    tb_mono_remove(e, key);
    if (e->mono_n < TB_NOTE_STACK) {
        e->mono_notes[e->mono_n] = (uint8_t)key; e->mono_vels[e->mono_n] = (uint8_t)velocity; e->mono_n++;
    }
    for (int i = 1; i < TB_VOICES; i++)
        if (e->v[i].active && e->v[i].gate) { e->v[i].gate = 0; tb_env_gate_off(&e->v[i].aenv); tb_env_gate_off(&e->v[i].fenv); }
    tb_voice_t *v = &e->v[0];
    int legato = (mode >= 2) && v->active && v->gate;
    if (legato) {
        /* a key pressed over a held one slides there without a new attack */
        int note = key + e->octave * 12;
        v->key = key; v->note = note < 0 ? 0 : (note > 127 ? 127 : note);
        v->target = (float)v->note;
        v->stamp = ++e->stamp;
    } else {
        tb_voice_start(e, v, key, vel, 1);
        v->pitch = v->target;   /* separate notes never slide */
    }
}

static void tb_voice_release(tb_voice_t *v)
{
    v->gate = 0; v->sustained = 0;
    tb_env_gate_off(&v->aenv);
    tb_env_gate_off(&v->fenv);
}

static void tb_note_off(tb_engine_t *e, int key)
{
    int mode = tb_voice_mode(e);
    if (mode == 0) {
        for (int i = 0; i < TB_VOICES; i++) {
            tb_voice_t *v = &e->v[i];
            if (v->active && v->gate && v->key == key) {
                if (e->sustain) v->sustained = 1; else tb_voice_release(v);
            }
        }
        return;
    }
    tb_mono_remove(e, key);
    tb_voice_t *v = &e->v[0];
    if (!v->active || !v->gate) return;
    if (e->mono_n > 0) {
        if (v->key == key) {
            /* back to the key still held */
            int k = e->mono_notes[e->mono_n - 1];
            int note = k + e->octave * 12;
            v->key = k; v->note = note < 0 ? 0 : (note > 127 ? 127 : note);
            v->target = (float)v->note;
            if (mode == 1) v->pitch = v->target;
        }
    } else if (v->key == key) {
        if (e->sustain) v->sustained = 1; else tb_voice_release(v);
    }
}

static void tb_set_sustain(tb_engine_t *e, int on)
{
    e->sustain = on;
    if (on) return;
    for (int i = 0; i < TB_VOICES; i++)
        if (e->v[i].active && e->v[i].sustained) tb_voice_release(&e->v[i]);
}

/* Fade every voice out over a few ms (program change, CC 120/123) */
static void tb_all_notes_off(tb_engine_t *e)
{
    for (int i = 0; i < TB_VOICES; i++) {
        tb_voice_t *v = &e->v[i];
        if (!v->active) continue;
        v->gate = 0; v->sustained = 0;
        v->kill = 1.0f;
    }
    e->mono_n = 0;
}

static void tb_poly_pressure(tb_engine_t *e, int key, int value)
{
    for (int i = 0; i < TB_VOICES; i++)
        if (e->v[i].active && e->v[i].key == key) e->v[i].pressure = (float)value / 127.0f;
}

/* ---- per control tick: what every voice shares ---------------------------- */

static void tb_frame_prepare(tb_engine_t *e, tb_frame_t *f)
{
    const float *p = e->p;
    const float tick = (float)TB_CTL / TB_SR;

    /* smooth the continuous controls a knob can step (about 12 ms) */
    const float k = e->snap ? 1.0f : 0.03f;
    e->snap = 0;
    e->sm_cut += (p[P_CUTOFF] - e->sm_cut) * k;
    e->sm_shape += (p[P_SHAPE] - e->sm_shape) * k;
    e->sm_blend += (p[P_BLEND] - e->sm_blend) * k;
    e->sm_reso += (p[P_RESO] - e->sm_reso) * k;
    e->sm_drive += (p[P_DRIVE] - e->sm_drive) * k;
    e->sm_stack += (p[P_STACK] - e->sm_stack) * k;

    f->model = (int)(p[P_MODEL] + 0.5f);
    if (f->model < 0) f->model = 0;
    if (f->model >= M_COUNT) f->model = M_COUNT - 1;
    f->layer = (int)(p[P_LAYER] + 0.5f);
    if (f->layer < 0) f->layer = 0;
    if (f->layer >= L_COUNT) f->layer = L_COUNT - 1;
    f->flt_type = (int)(p[P_FLT_TYPE] + 0.5f);
    f->lfo_dest = (int)(p[P_LFO_DEST] + 0.5f);
    int mode = tb_voice_mode(e);
    f->mono = mode != 0;
    f->legato = mode >= 2;
    float gms = TB_GLIDE_MS[mode];
    f->glide_coef = gms > 0.0f ? 1.0f - expf(-tick / (gms * 0.001f * 0.35f)) : 1.0f;

    f->shape = e->sm_shape;
    f->shape_env = p[P_SHAPE_ENV];
    f->age = p[P_AGE];
    f->age2 = f->age * f->age;
    f->model_gain = TB_MODEL_GAIN[f->model];

    /* Layer blend: the layer comes up over the first half, the model goes down
     * over the second, so 50 % is both at full and 100 % is the layer alone. */
    float b = e->sm_blend;
    float bl = tb_clamp01(b * 2.0f);
    f->lay_g = bl * sqrtf(bl);
    f->main_g = b <= 0.5f ? 1.0f : tb_clamp01(2.0f - b * 2.0f);

    /* Stack: one knob from a single oscillator to seven.
     *   to ~10 %   three oscillators a few cents apart: analog beating
     *   ~10-30 %   the same three pulled wide (to about +-20 cents): a reese
     *   30-55 %    a second pair fills in between them
     *   55-100 %   a third pair outside: the supersaw
     * The detune of the first pair rises fast and then levels off, so the top
     * of the knob adds voices and width rather than going out of tune. */
    float s = e->sm_stack;
    float d1 = 28.0f * (1.0f - expf(-4.0f * s));
    float g1 = tb_smoothstep(s / 0.10f);
    float g2 = tb_smoothstep((s - 0.30f) / 0.25f);
    float g3 = tb_smoothstep((s - 0.55f) / 0.25f);
    float width = tb_clamp01(s * 2.2f);
    static const float rel[TB_STACK] = { 0.0f, -1.0f, 1.03f, 0.52f, -0.57f, -1.9f, 1.84f };
    static const float pan[TB_STACK] = { 0.0f, -0.85f, 0.85f, 0.55f, -0.55f, 1.0f, -1.0f };
    if (TB_MODEL_COPIES[f->model] < 7) g3 = 0.0f;
    f->ncopies = 1;
    if (g1 > 0.0005f) f->ncopies = 3;
    if (g2 > 0.0005f) f->ncopies = 5;
    if (g3 > 0.0005f) f->ncopies = 7;
    float gains[TB_STACK] = { 1.0f, g1, g1, g2, g2, g3, g3 };
    float pw = 1.0f + 2.0f * (g1 * g1 + g2 * g2 + g3 * g3);
    float norm = powf(pw, -0.40f);    /* a little short of equal power: thicker is a little louder */
    for (int i = 0; i < TB_STACK; i++) {
        f->det[i] = rel[i] * d1;
        f->gain[i] = gains[i] * norm;
        f->panw[i] = pan[i] * width;
    }
    f->width = width;

    /* Filter */
    float r = e->sm_reso;
    f->cut = e->sm_cut;
    f->q = 0.03f + 1.47f * powf(1.0f - r, 2.2f);
    f->q2 = 0.6f * f->q + 0.55f;
    f->res_gain = 1.14f * f->q / (f->q + 0.22f);

    /* Drive: the first half pushes the mixer into an asymmetric soft clip in
     * front of the filter; the second half adds a hard clip after the amp. */
    float d = e->sm_drive;
    float da = tb_clamp01(d * 2.0f), db = tb_clamp01(d * 2.0f - 1.0f);
    float dc = da * sqrtf(da);
    f->pre_gain = 0.42f + 2.4f * dc + 1.2f * db;
    f->pre_bias = 0.18f * da;
    f->pre_dc = tb_tanh(f->pre_bias);
    f->pre_make = 1.0f / (0.42f + 0.55f * dc);
    f->post_on = db > 0.0f;
    f->post_gain = 1.0f + 7.0f * db;
    f->post_ceil = 3.2f - 2.1f * db;
    f->post_make = 1.0f / (1.0f + 1.9f * db);

    /* Envelopes */
    float fe = p[P_FLT_ENV];
    f->fenv_oct = 8.0f * fe * fabsf(fe);
    f->f_a = tb_attack_step(p[P_FLT_A]);
    f->f_d = tb_decay_step(p[P_FLT_D]);
    f->f_s = tb_clamp01(p[P_FLT_S]);
    f->a_a = tb_attack_step(p[P_AMP_A]);
    f->a_d = tb_decay_step(p[P_AMP_D]);
    f->a_s = sqrtf(tb_clamp01(p[P_AMP_S]));   /* the amp squares it again */
    f->a_r = tb_decay_step(p[P_AMP_R]);

    /* LFO: 0.05 Hz .. 25 Hz */
    float rate = 0.05f * powf(500.0f, tb_clamp01(p[P_LFO_RATE]));
    e->lfo_ph += rate * tick;
    if (e->lfo_ph >= 1.0f) {
        e->lfo_ph -= (float)(int)e->lfo_ph;
        e->lfo_sh = tb_frand(&e->rng);
    }
    f->lfo_tri_ph = e->lfo_ph;
    f->lfo_depth = tb_clamp01(p[P_LFO_DEPTH]);
    f->lfo_sh = e->lfo_sh;

    /* Sync Ramp: a falling ramp locked to the tempo -- the filter opens on the
     * division and closes through it. Follows the transport while it runs and
     * free-runs at the session tempo when it does not. */
    {
        int di = (int)(tb_clamp01(p[P_LFO_RATE]) * ((float)TB_RAMP_DIVS - 0.001f));
        double adv = (double)(e->bpm * (1.0f / 60.0f) * tick);
        if (e->beat >= 0.0) { e->ramp_beat = e->beat; e->beat += adv; }
        double ph = e->ramp_beat / (double)TB_RAMP_BEATS[di];
        float fr = (float)(ph - floor(ph));
        float target = (1.0f - fr) * (1.0f - fr);
        e->ramp_val += (target - e->ramp_val) * 0.35f;   /* a millisecond or so on the reset */
        e->ramp_beat += adv;
        f->lfo_ramp = e->ramp_val;
    }

    /* performance vibrato (wheel, pressure): its own 5.6 Hz sine */
    e->vib_ph += 5.6f * tick; if (e->vib_ph >= 1.0f) e->vib_ph -= 1.0f;
    f->vib = tb_sin(e->vib_ph);

    f->velk = tb_clamp01(p[P_VEL]);
    f->bend = e->bend;

    /* Zap: the pitch falls onto the note from above */
    f->zap_depth = 72.0f * f->shape * f->shape;
    f->zap_tau = 0.012f + 0.14f * f->shape;
}

static inline float tb_tri(float ph)
{
    ph -= (float)(int)ph;
    return ph < 0.5f ? 4.0f * ph - 1.0f : 3.0f - 4.0f * ph;
}

/* One control tick of one voice: `n` samples added to outL / outR */
static void tb_voice_render(tb_engine_t *e, tb_voice_t *v, const tb_frame_t *f, float *outL, float *outR, float *outSub, int n)
{
    const float tick = (float)n / TB_SR;
    const int model = f->model;

    /* ---- pitch ---- */
    v->pitch += (v->target - v->pitch) * f->glide_coef;
    if (fabsf(v->target - v->pitch) < 0.001f) v->pitch = v->target;

    if (--v->drift_wait <= 0) {
        v->drift_to = tb_frand(&v->rng) * 22.0f * f->age2;
        v->drift_wait = 40 + (int)(tb_rand(&v->rng) & 255);
    }
    v->drift += (v->drift_to - v->drift) * 0.012f;

    float lfo_t = tb_tri(f->lfo_tri_ph);
    float lfo_v = tb_tri(f->lfo_tri_ph + v->lfo_phase_off);   /* each voice at its own phase */
    float dep = f->lfo_depth;

    /* Age's slow wobble is per voice: the notes of a chord
     * drift against each other, as separate oscillators do. A wobble the whole
     * instrument shares is what VHS is for. */
    v->wob_ph1 += v->wob_r1 * tick; if (v->wob_ph1 >= 1.0f) v->wob_ph1 -= 1.0f;
    v->wob_ph2 += v->wob_r2 * tick; if (v->wob_ph2 >= 1.0f) v->wob_ph2 -= 1.0f;
    float wobble = f->age2 * (5.0f * tb_sin(v->wob_ph1) + 2.0f * tb_sin(v->wob_ph2));

    float cents = v->drift + v->n_detune + wobble;
    float semis = v->pitch + f->bend;
    /* pressure and wheel: vibrato */
    float press = v->pressure > e->chan_pressure ? v->pressure : e->chan_pressure;
    semis += f->vib * (0.45f * e->modwheel + 0.30f * press * press);
    if (f->lfo_dest == D_PITCH) {
        /* comes in after the attack, as a player's vibrato does */
        float fade = tb_smoothstep((v->t_on - 0.12f) / 0.35f);
        semis += tb_sin(f->lfo_tri_ph) * dep * dep * 2.0f * fade;
    }
    if (model == M_ZAP) semis += f->zap_depth * expf(-v->t_on / f->zap_tau);
    v->t_on += tick;

    float base = 440.0f * exp2f((semis - 69.0f) * (1.0f / 12.0f) + cents * (1.0f / 1200.0f));
    if (base > 16000.0f) base = 16000.0f;
    float dt0 = base * (1.0f / TB_SR);

    /* ---- shape ---- */
    float fe = v->fenv.value;
    float shape = f->shape + f->shape_env * fe;
    if (f->lfo_dest == D_SHAPE) shape += lfo_v * dep * 0.45f;
    shape = tb_clamp01(shape);

    tb_octl_t oc, lc;
    tb_osc_control(model, shape, dt0, semis, &oc);

    /* The stack's width closes up toward the bottom of the keyboard: a bass
     * stays in the middle whatever Stack is doing, a pad up top is wide. */
    float keyw = tb_smoothstep((v->pitch - 33.0f) * (1.0f / 27.0f));
    float dts[TB_STACK], idts[TB_STACK], gl[TB_STACK], gr[TB_STACK];
    int nc = f->ncopies;
    for (int i = 0; i < nc; i++) {
        dts[i] = dt0 * (1.0f + f->det[i] * 0.00057779f);
        if (dts[i] > 0.45f) dts[i] = 0.45f;
        idts[i] = 1.0f / dts[i];
        float pn = f->panw[i] * keyw;
        gl[i] = f->gain[i] * sqrtf(1.0f - pn);
        gr[i] = f->gain[i] * sqrtf(1.0f + pn);
    }
    int stereo = nc > 1 && f->width * keyw > 0.01f;

    /* ---- layer ---- */
    int layer = f->layer;
    float ldt = dt0, lidt;
    switch (layer) {
    case L_SUB_SINE: case L_SUB_SQR: case L_OCT_DN: ldt = dt0 * 0.5f; break;
    case L_SUB_2:  ldt = dt0 * 0.25f; break;
    case L_FIFTH:  ldt = dt0 * 1.498307f; break;
    case L_OCT_UP: ldt = dt0 * 2.0f; break;
    case L_OCT_2:  ldt = dt0 * 4.0f; break;
    default: break;
    }
    if (ldt > 0.45f) ldt = 0.45f;
    lidt = 1.0f / ldt;
    int lay_model = layer >= L_OCT_DN && layer <= L_OCT_2;
    if (lay_model) tb_osc_control(model, shape, ldt, semis, &lc);
    float lay_g = f->lay_g, main_g = f->main_g * f->model_gain;
    if (lay_model) lay_g *= f->model_gain * 0.9f;
    else if (layer == L_NOISE) lay_g *= 0.75f;
    else if (layer == L_SUB_SINE) lay_g *= 1.9f;   /* the clean path: no filter gain to ride on */
    int lay_on = lay_g > 0.0005f;
    float dmain = (main_g - v->main_g) / (float)n, dlay = (lay_g - v->lay_g) / (float)n;
    if (v->declick <= 0.0f) { v->main_g = main_g; v->lay_g = lay_g; dmain = dlay = 0.0f; }

    /* ---- filter ---- */
    float velk = f->velk;
    v->vel_cur += (v->vel - v->vel_cur) * 0.2f;
    float vel = v->vel_cur;
    float cut_oct = 10.0f * f->cut
                  + 0.66f * (v->pitch - 48.0f) * (1.0f / 12.0f)
                  + f->fenv_oct * fe * (1.0f - 0.5f * velk + 0.5f * velk * vel)
                  + velk * (vel - 0.6f) * 2.2f
                  + v->n_cut + 1.2f * press;
    if (f->lfo_dest == D_FILTER) cut_oct += lfo_t * dep * dep * 4.0f;
    if (f->lfo_dest == D_RAMP) cut_oct += f->lfo_ramp * dep * 5.0f;
    if (f->lfo_dest == D_SH) {
        v->sh_cur += (f->lfo_sh - v->sh_cur) * 0.5f;
        cut_oct += v->sh_cur * dep * dep * 4.0f;
    }
    float fc = 20.0f * exp2f(cut_oct);
    fc = tb_clamp(fc, 16.0f, 15500.0f);
    /* the core runs twice per sample, so its coefficient is for 2 x the rate */
    float F = 2.0f * tb_sin(fc * (0.5f / (2.0f * TB_SR)));
    float q = f->q, q2 = f->q2;
    float fmax = (sqrtf(q * q + 4.0f) - q) * 0.96f;
    if (F > fmax) F = fmax;
    if (v->F <= 0.0f) v->F = F;
    float dF = (F - v->F) / (float)n;
    float Fc = v->F;
    int ftype = f->flt_type;
    float in_gain = f->res_gain;
    float out_gain = ftype == F_BP ? 1.3f : (ftype == F_HP ? 1.0f : 1.5f);

    /* ---- amp ---- */
    float vel_gain = (1.0f - velk) + velk * (0.15f + 0.85f * vel * vel);
    float trem = 1.0f;
    if (f->lfo_dest == D_AMP) trem = 1.0f - dep * (0.5f + 0.5f * lfo_t);
    float pan = 0.0f;
    if (f->lfo_dest == D_PAN) pan = lfo_v * dep;
    float dpan = (pan - v->pan_cur) / (float)n;

    float a_a = f->a_a, a_d = f->a_d * v->n_time, a_s = f->a_s, a_r = f->a_r * v->n_time;
    float f_a = f->f_a, f_d = f->f_d * v->n_time, f_s = f->f_s;
    float kill_step = 1.0f / (0.004f * TB_SR);
    float pre_gain = f->pre_gain, pre_bias = f->pre_bias, pre_dc = f->pre_dc, pre_make = f->pre_make;

    for (int s = 0; s < n; s++) {
        /* oscillators */
        float l, r;
        float c0 = tb_osc_tick(model, &v->osc[0], dts[0], idts[0], &oc);
        l = c0 * gl[0];
        r = c0 * gr[0];
        for (int i = 1; i < nc; i++) {
            float ci = tb_osc_tick(model, &v->osc[i], dts[i], idts[i], &oc);
            l += ci * gl[i];
            r += ci * gr[i];
        }
        v->main_g += dmain; v->lay_g += dlay;
        l *= v->main_g; r *= v->main_g;
        float clean = 0.0f;
        if (lay_on || v->lay_g > 0.0005f) {
            float y;
            switch (layer) {
            case L_SUB_SINE:
                /* The sine sub is the CLEAN one: it skips the drive and the
                 * filter and meets the voice again after the clip, so the
                 * bottom stays solid however hard the rest is driven. */
                v->lay.ph += ldt; if (v->lay.ph >= 1.0f) v->lay.ph -= 1.0f;
                clean = tb_sin(v->lay.ph) * v->lay_g;
                y = 0.0f;
                break;
            case L_SUB_SQR: case L_SUB_2: {
                v->lay.ph += ldt; if (v->lay.ph >= 1.0f) v->lay.ph -= 1.0f;
                float ph = v->lay.ph, p2 = ph + 0.5f; if (p2 >= 1.0f) p2 -= 1.0f;
                y = 0.8f * ((ph < 0.5f ? 1.0f : -1.0f) + tb_blep(ph, ldt, lidt) - tb_blep(p2, ldt, lidt));
                break;
            }
            case L_NOISE:
                y = tb_frand(&v->rng);
                break;
            default:
                y = tb_osc_tick(model, &v->lay, ldt, lidt, &lc);
                break;
            }
            y *= v->lay_g;
            l += y; r += y;
        }

        /* envelopes */
        float fenv = tb_env_tick(&v->fenv, f_a, f_d, f_s, a_r);
        (void)fenv;
        float ae = tb_env_tick(&v->aenv, a_a, a_d, a_s, a_r);
        float amp = ae * ae * vel_gain * trem;
        if (v->declick < 1.0f) {
            v->declick += 1.0f / 66.0f;      /* 1.5 ms */
            if (v->declick > 1.0f) v->declick = 1.0f;
            float w = v->declick; amp *= w * w * (3.0f - 2.0f * w);
        }
        if (v->kill > 0.0f) {
            v->kill -= kill_step;
            if (v->kill <= 0.0f) { v->kill = 0.0f; v->aenv.stage = ENV_IDLE; v->aenv.value = 0.0f; amp = 0.0f; }
            else amp *= v->kill;
        }

        Fc += dF;

        /* channel 0 */
        {
            float x = (tb_tanh(l * pre_gain + pre_bias) - pre_dc) * pre_make * in_gain;
            tb_svf_t *a = &v->flt[0][0];
            float lo, hi, bd;
            lo = a->low + Fc * a->band; hi = x - lo - q * a->band; bd = a->band + Fc * hi;
            lo = lo + Fc * bd;          hi = x - lo - q * bd;      bd = bd + Fc * hi;
            a->low = lo; a->band = bd;
            float y;
            if (ftype == F_LP24) {
                tb_svf_t *b = &v->flt[0][1];
                float lo2, hi2, bd2;
                lo2 = b->low + Fc * b->band; hi2 = lo - lo2 - q2 * b->band; bd2 = b->band + Fc * hi2;
                lo2 = lo2 + Fc * bd2;        hi2 = lo - lo2 - q2 * bd2;      bd2 = bd2 + Fc * hi2;
                b->low = lo2; b->band = bd2;
                y = lo2;
            } else if (ftype == F_LP12) y = lo;
            else if (ftype == F_BP) y = bd;
            else y = hi;
            l = y * out_gain * amp;
        }
        if (stereo) {
            float x = (tb_tanh(r * pre_gain + pre_bias) - pre_dc) * pre_make * in_gain;
            tb_svf_t *a = &v->flt[1][0];
            float lo, hi, bd;
            lo = a->low + Fc * a->band; hi = x - lo - q * a->band; bd = a->band + Fc * hi;
            lo = lo + Fc * bd;          hi = x - lo - q * bd;      bd = bd + Fc * hi;
            a->low = lo; a->band = bd;
            float y;
            if (ftype == F_LP24) {
                tb_svf_t *b = &v->flt[1][1];
                float lo2, hi2, bd2;
                lo2 = b->low + Fc * b->band; hi2 = lo - lo2 - q2 * b->band; bd2 = b->band + Fc * hi2;
                lo2 = lo2 + Fc * bd2;        hi2 = lo - lo2 - q2 * bd2;      bd2 = bd2 + Fc * hi2;
                b->low = lo2; b->band = bd2;
                y = lo2;
            } else if (ftype == F_LP12) y = lo;
            else if (ftype == F_BP) y = bd;
            else y = hi;
            r = y * out_gain * amp;
        } else {
            r = l;
        }

        if (f->post_on) {
            l *= f->post_gain; r *= f->post_gain;
            float c = f->post_ceil;
            l = (l > c ? c : (l < -c ? -c : l)) * f->post_make;
            r = (r > c ? c : (r < -c ? -c : r)) * f->post_make;
        }

        outSub[s] += clean * amp;

        v->pan_cur += dpan;
        float pn = v->pan_cur;
        outL[s] += l * (pn > 0.0f ? 1.0f - pn : 1.0f);
        outR[s] += r * (pn < 0.0f ? 1.0f + pn : 1.0f);
    }
    v->F = Fc;
    v->q = q;

    /* a filter that has gone wrong is reset, not left to ring forever */
    for (int ch = 0; ch < 2; ch++)
        for (int st = 0; st < 2; st++) {
            tb_svf_t *x = &v->flt[ch][st];
            if (!(fabsf(x->band) < 64.0f) || !(fabsf(x->low) < 64.0f)) { x->band = 0.0f; x->low = 0.0f; }
        }

    if (v->aenv.stage == ENV_IDLE) {
        v->active = 0; v->gate = 0; v->sustained = 0;
        v->main_g = 0.0f; v->lay_g = 0.0f;
    }
}

/* Render `frames` samples of the dry voice mix into outL / outR, and the clean
 * sine sub (mono) into outSub. All three are overwritten. */
/* Low on purpose: eight voices must fit under the bus stage's knee, so a chord
 * is summed clean. The level comes back as TB_OUT_MAKEUP after the effects. */
#define TB_MIX_GAIN 0.06f
#define TB_OUT_MAKEUP 1.75f

static void tb_engine_render(tb_engine_t *e, float *outL, float *outR, float *outSub, int frames)
{
    memset(outSub, 0, sizeof(float) * (size_t)frames);
    memset(outL, 0, sizeof(float) * (size_t)frames);
    memset(outR, 0, sizeof(float) * (size_t)frames);
    int done = 0;
    int any = 0;
    while (done < frames) {
        int n = frames - done;
        if (n > TB_CTL) n = TB_CTL;
        tb_frame_t f;
        tb_frame_prepare(e, &f);
        for (int i = 0; i < TB_VOICES; i++) {
            if (!e->v[i].active) continue;
            tb_voice_render(e, &e->v[i], &f, outL + done, outR + done, outSub + done, n);
            any++;
        }
        done += n;
    }
    e->active_voices = 0;
    for (int i = 0; i < TB_VOICES; i++) if (e->v[i].active) e->active_voices++;
    if (!any) return;

    /* DC blocker (about 5 Hz), then the bus level */
    for (int s = 0; s < frames; s++) {
        float xl = outL[s], xr = outR[s];
        float yl = xl - e->dc_x[0] + 0.9993f * e->dc_y[0];
        float yr = xr - e->dc_x[1] + 0.9993f * e->dc_y[1];
        e->dc_x[0] = xl; e->dc_y[0] = yl;
        e->dc_x[1] = xr; e->dc_y[1] = yr;
        outL[s] = yl * TB_MIX_GAIN;
        outR[s] = yr * TB_MIX_GAIN;
        outSub[s] *= TB_MIX_GAIN;
    }
}

#endif

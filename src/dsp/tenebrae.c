/*
 * tenebrae.c -- Tenebrae for Schwung (plugin API v2): the wrapper around the
 * engine. It owns one instance per slot, serves the parameter surface and its
 * JSON (chain_params, ui_hierarchy, state), and turns MIDI into notes.
 *
 * THREADING: every entry point except instance creation runs on the audio
 * callback. Nothing here allocates, locks, logs or opens a file once an
 * instance exists; the JSON builders format straight into the caller's buffer.
 * The one other thread is the Random page's measurer (tb_rnd_worker): it runs
 * at the lowest priority there is, and the audio side never waits for it --
 * the hand-off both ways is lock-free.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <unistd.h>
#include <strings.h>
#include <stdint.h>
#include <math.h>

#include "host/plugin_api_v1.h"
#include "host/move_info.h"

#include "tenebrae_params.h"
#include "tenebrae_engine.h"
#include "tenebrae_fx.h"
#include "tenebrae_presets.h"
#include "tenebrae_random.h"
#include "tenebrae_phrases.h"
#include <pthread.h>

#define TB_BLOCK_MAX 256
/* how long the output must stay below the floor before the instance goes idle */
#define TB_IDLE_BLOCKS 172          /* about half a second */
#define TB_IDLE_FLOOR  2.0e-5f

static const host_api_v1_t *g_host = NULL;

/* One roll for the worker (or the native tool) to measure */
typedef struct {
    int         recipe;            /* -1: a Vary */
    int         cat;
    uint32_t    seed;              /* what a replacement is rolled from */
    float       p[P_COUNT];        /* the roll as it was made */
    float       anchor[P_COUNT];   /* what a Vary varied */
    const char *phrase;
    uint32_t    writes;            /* the parameter-write count when it was made */
} tb_rjob_t;

typedef struct {
    tb_engine_t eng;
    tb_fx_t     fx;
    float       bufL[TB_BLOCK_MAX], bufR[TB_BLOCK_MAX], bufSub[TB_BLOCK_MAX];
    int         preset;        /* factory preset index, -1 once it is edited away / restored */
    /* The Random page (tenebrae_random.h, and the block below) */
    uint32_t    rnd_rng;       /* the generator */
    int         rnd_cat;       /* category of the roll on now */
    int         rnd_recipe;    /* its recipe, -1 for a Vary */
    int         rnd_n;         /* how many rolls this instance has made: names them */
    int         rnd_named;     /* the sound on now is a roll (preset is -1) */
    /* The sound before the first roll of a run: Revert puts it back exactly.
     * Keep, Revert, loading a preset and restoring a state end the run. */
    int         rnd_pending;
    float       rnd_anchor[P_COUNT];
    int         rnd_a_preset, rnd_a_named, rnd_a_cat, rnd_a_n;
    long        rnd_last_cmd;  /* the command key's last value, answered back */
    uint32_t    rnd_seq;       /* counts every change the page shows */
    uint32_t    rnd_writes;    /* counts parameter writes: a knob you turn outranks a swap */
    int         rnd_busy;      /* a roll is being measured */
    /* A roll is measured, not guessed: a worker thread plays it through its
     * own engine, sets its Level, and replaces a roll that measures wrong for
     * its category. rnd_gen names the roll on now; a result for any other is
     * dropped. */
    uint32_t    rnd_gen;
    uint32_t    rnd_res_gen;   /* the roll the result below belongs to (atomic) */
    float       rnd_res_p[P_COUNT];
    int         rnd_res_swapped;
    int         rnd_sync;      /* measure inline (the native tool), no thread */
    pthread_t       rnd_th;
    pthread_mutex_t rnd_mu;
    pthread_cond_t  rnd_cv;
    int         rnd_th_on, rnd_quit;
    /* Lock-free hand-off of rnd_job: the audio side bumps rnd_job_seq to odd,
     * writes the job, bumps it to even, then publishes rnd_job_gen. The worker
     * copies the job and keeps the copy only if the sequence did not move. */
    uint32_t    rnd_job_seq;
    uint32_t    rnd_job_gen;
    uint32_t    rnd_res_writes;
    tb_rjob_t   rnd_job;       /* under rnd_mu */
    int         quiet_blocks;
    int         idle;
    float       bpm;
} tb_instance_t;

/* ------------------------------------------------------------------------ */
/* Parameters                                                                */
/* ------------------------------------------------------------------------ */

static int tb_find_param(const char *key)
{
    for (int i = 0; i < P_COUNT; i++)
        if (strcmp(TB_PARAMS[i].key, key) == 0) return i;
    return -1;
}

/* A value string for an enum: an index, or one of its option names */
static float tb_parse_enum(int id, const char *val)
{
    const char *opts = TB_PARAMS[id].options;
    int idx = 0;
    const char *start = opts;
    for (const char *c = opts; ; c++) {
        if (*c == '|' || *c == '\0') {
            size_t len = (size_t)(c - start);
            if (strlen(val) == len && strncasecmp(val, start, len) == 0) return (float)idx;
            idx++;
            start = c + 1;
            if (*c == '\0') break;
        }
    }
    return (float)atof(val);
}

static void tb_set(tb_instance_t *inst, int id, float v)
{
    const tb_param_t *pd = &TB_PARAMS[id];
    if (!isfinite(v)) return;
    if (pd->type == PT_ENUM) v = floorf(v + 0.5f);
    v = tb_clamp(v, tb_param_min(id), tb_param_max(id));
    /* leaving a mono mode: drop the key stack so nothing is left hanging */
    if (id == P_VOICE && (int)v != (int)inst->eng.p[P_VOICE]) {
        int was_mono = inst->eng.p[P_VOICE] >= 0.5f, now_mono = v >= 0.5f;
        if (was_mono != now_mono) tb_all_notes_off(&inst->eng);
    }
    inst->eng.p[id] = v;
}

static void tb_load_preset(tb_instance_t *inst, int idx)
{
    if (idx < 0) idx = 0;
    if (idx >= TB_PRESET_COUNT) idx = TB_PRESET_COUNT - 1;
    const tb_preset_t *pr = &TB_PRESETS[idx];
    tb_all_notes_off(&inst->eng);
    for (int i = 0; i < P_COUNT; i++) inst->eng.p[i] = TB_PARAMS[i].def;
    for (int i = 0; i < P_COUNT && pr->v[i].id > 0; i++) {
        int id = pr->v[i].id - 1;
        if (id >= 0 && id < P_COUNT)
            inst->eng.p[id] = tb_clamp(pr->v[i].v, tb_param_min(id), tb_param_max(id));
    }
    inst->preset = idx;
    inst->rnd_named = 0;
    inst->rnd_pending = 0;   /* choosing a preset ends a run of rolls */
    inst->rnd_busy = 0;
    inst->rnd_gen++;
    inst->rnd_seq++;
}

/* Denormals: flush to zero for the length of the render, then put the thread's
 * floating-point mode back exactly as the host had it. */
#if defined(__aarch64__)
static inline uint64_t tb_fz_on(void)
{
    uint64_t old;
    __asm__ volatile("mrs %0, fpcr" : "=r"(old));
    uint64_t now = old | (1ull << 24);
    __asm__ volatile("msr fpcr, %0" : : "r"(now));
    return old;
}
static inline void tb_fz_restore(uint64_t old) { __asm__ volatile("msr fpcr, %0" : : "r"(old)); }
#else
static inline uint64_t tb_fz_on(void) { return 0; }
static inline void tb_fz_restore(uint64_t old) { (void)old; }
#endif

/* Loudness target per phrase and the peak ceiling: tools/level_bank.py's */
static float tb_rnd_target(const char *phrase)
{
    static const struct { const char *k; float db; } T[] = {
        { "bass", -19.0f }, { "lead", -20.5f }, { "pad", -20.0f }, { "keys", -21.0f },
        { "bell", -23.0f }, { "arp", -23.0f }, { "tom", -22.0f }, { "drone", -21.0f }, { "fx", -21.0f } };
    for (unsigned i = 0; i < sizeof T / sizeof T[0]; i++) if (!strcmp(phrase, T[i].k)) return T[i].db;
    return -21.0f;
}
#define TB_RND_PEAK_MAX  -6.0f
#define TB_RND_PROBE_BEATS 8.0   /* the whole phrase, at 120 BPM */
#define TB_RND_PROBE_TAIL  1.0   /* seconds after it */

/* What a probe measures */
typedef struct {
    float level;      /* the Level that lands the phrase on its target */
    float sounding;   /* share of the phrase during which something is heard */
    float low;        /* share of the energy under 150 Hz */
    float corr;       /* left/right correlation: 1 is mono */
    int   silent;
} tb_probe_t;

/* 4th-order Butterworth low-pass at 150 Hz, 44.1 kHz, as two sections: the
 * split tests/bank_checks.py measures low end with (scipy butter(4, 150)) */
typedef struct { float z1, z2; } tb_bq_t;
static const float TB_LP150[2][5] = {   /* b0 b1 b2 a1 a2 */
    { 1.268087415e-08f, 2.53617483e-08f, 1.268087415e-08f, -1.960830456f, 0.9612783323f },
    { 1.0f, 2.0f, 1.0f, -1.983323956f, 0.9837769696f }
};
static inline float tb_bq150(tb_bq_t *s, int k, float x)
{
    const float *c = TB_LP150[k];
    float y = c[0] * x + s->z1;
    s->z1 = c[1] * x - c[3] * y + s->z2;
    s->z2 = c[2] * x - c[4] * y;
    return y;
}

/*
 * Play `phrase` through e/fx with parameters p and measure it the way
 * tools/level_bank.py and tests/bank_checks.py measure the bank. e and fx are
 * the caller's scratch: a probe never touches the instance that is playing.
 */
static void tb_rnd_probe(const float *p, const char *phrase, tb_engine_t *e, tb_fx_t *fx, tb_probe_t *out)
{
    const ev_t *ph = phrase_for(phrase);
    tb_engine_init(e);
    tb_fx_init(fx);
    for (int i = 0; i < P_COUNT; i++) e->p[i] = p[i];
    e->snap = 1; fx->dly_snap = 1;
    const float bpm = 120.0f;
    const double spb = 60.0 / bpm;
    enum { SR = 44100, B = 128, WIN = 1024, MAXW = 256 };
    int total = (int)((TB_RND_PROBE_BEATS * spb + TB_RND_PROBE_TAIL) * SR);
    int body_wins = (int)(TB_RND_PROBE_BEATS * spb * SR) / WIN;
    float L[B], R[B], S[B];
    unsigned char on[64] = { 0 }, off[64] = { 0 };
    int n_ev = 0;
    while (ph[n_ev].beat >= 0 && n_ev < 64) n_ev++;
    double acc = 0.0, acc_lo = 0.0, sll = 0.0, srr = 0.0, slr = 0.0;
    int win_n = 0, n_win = 0;
    float peak = 0.0f;
    float wins[MAXW], wlo[MAXW];
    tb_bq_t q1 = { 0, 0 }, q2 = { 0, 0 };
    for (int pos = 0; pos < total; pos += B) {
        double t0 = (double)pos / SR, t1 = (double)(pos + B) / SR;
        for (int i = 0; i < n_ev; i++) {
            if (ph[i].beat >= TB_RND_PROBE_BEATS) continue;
            double a = ph[i].beat * spb, b = ph[i].beat + ph[i].len;
            if (b > TB_RND_PROBE_BEATS) b = TB_RND_PROBE_BEATS;
            b *= spb;
            if (!off[i] && on[i] && b < t1) { tb_note_off(e, ph[i].note); off[i] = 1; }
            if (!on[i] && a < t1 && a >= t0 - 1e-9) { tb_note_on(e, ph[i].note, ph[i].vel); on[i] = 1; }
        }
        int n = total - pos < B ? total - pos : B;
        e->bpm = bpm; e->beat = t0 / spb;
        tb_engine_render(e, L, R, S, n);
        tb_fx_process(fx, e->p, L, R, S, n, bpm, t0 / spb);
        for (int k = 0; k < n; k++) {
            float l = isfinite(L[k]) ? L[k] : 0.0f, r = isfinite(R[k]) ? R[k] : 0.0f;
            float a = fabsf(l) > fabsf(r) ? fabsf(l) : fabsf(r);
            if (a > peak) peak = a;
            sll += (double)l * l; srr += (double)r * r; slr += (double)l * r;
            float m = 0.5f * (l + r);
            float lo = tb_bq150(&q2, 1, tb_bq150(&q1, 0, m));
            acc += (double)m * m;
            acc_lo += (double)lo * lo;
            if (++win_n == WIN) {
                if (n_win < MAXW) { wins[n_win] = (float)(acc / WIN); wlo[n_win] = (float)(acc_lo / WIN); n_win++; }
                acc = acc_lo = 0.0; win_n = 0;
            }
        }
    }
    float mx = 0.0f;
    for (int i = 0; i < n_win; i++) if (wins[i] > mx) mx = wins[i];
    out->level = p[P_LEVEL];
    out->silent = mx < 1e-10f || peak <= 0.0f;
    out->sounding = out->low = 0.0f;
    out->corr = 1.0f;
    if (out->silent) return;
    double sum_act = 0.0, sum_lo = 0.0;
    int n_act = 0, n_snd = 0, n_body = 0;
    for (int i = 0; i < n_win; i++) {
        if (wins[i] > mx * 0.0025f) { sum_act += wins[i]; sum_lo += wlo[i]; n_act++; }   /* above 5 % (amplitude) */
        if (i < body_wins) { n_body++; if (wins[i] > mx * 1e-4f) n_snd++; }              /* above 1 % */
    }
    out->sounding = n_body ? (float)n_snd / (float)n_body : 0.0f;
    out->low = sum_act > 0.0 ? (float)(sum_lo / sum_act) : 0.0f;
    out->corr = (sll > 0.0 && srr > 0.0) ? (float)(slr / sqrt(sll * srr)) : 1.0f;
    float rms_db = 10.0f * log10f((float)(sum_act / (n_act ? n_act : 1)) + 1e-20f);
    float peak_db = 20.0f * log10f(peak + 1e-9f);
    float g = tb_rnd_target(phrase) - rms_db;
    if (TB_RND_PEAK_MAX - peak_db < g) g = TB_RND_PEAK_MAX - peak_db;
    float level = out->level * powf(10.0f, g / 40.0f);   /* Level is a squared gain */
    out->level = level < 0.30f ? 0.30f : (level > 1.0f ? 1.0f : level);
}

/* Does a measured roll belong to its category? The rules the bank is held
 * to (tests/bank_checks.py), checked on the roll itself. Returns how far it
 * misses: 0 = it passes. */
static float tb_rnd_miss(int cat, const char *phrase, const tb_probe_t *m)
{
    /* each limit sits a little inside the bank's, because a later playing
     * of the same roll measures a little differently (voices drift) */
    if (m->silent) return 100.0f;
    float miss = 0.0f;
    if (m->sounding < 0.40f) miss += 0.40f - m->sounding + 1.0f;
    if (cat == R_BASS && m->low < 0.30f) miss += 0.30f - m->low + 1.0f;
    if (!strcmp(phrase, "pad")) {
        if (m->corr >= 0.74f) miss += m->corr - 0.74f + 1.0f;
        if (m->low >= 0.09f) miss += m->low - 0.09f + 1.0f;
    }
    return miss;
}


/*
 * Measure a roll, and if it does not belong to its category, make up to two
 * more of the same kind (same recipe, or another Vary of the same sound) and
 * keep the first that does -- or the closest. out gets the result with its
 * Level set; returns 1 when it is not the roll that was made.
 */
static int g_rnd_vet_tries;   /* how many rolls the last vet took (the native tool reports it) */
static int tb_rnd_vet(const tb_rjob_t *j, float *out, tb_engine_t *e, tb_fx_t *fx)
{
    uint32_t rng = j->seed;
    float cand[P_COUNT], best[P_COUNT];
    float best_miss = 1e9f;
    int best_try = 0;
    memcpy(cand, j->p, sizeof cand);
    for (int t = 0; t < 3; t++) {
        if (t > 0) {
            if (j->recipe >= 0) tb_rnd_roll_recipe(cand, j->recipe, &rng);
            else { memcpy(cand, j->anchor, sizeof cand); tb_rnd_vary(cand, j->cat, &rng); }
        }
        tb_probe_t m;
        tb_rnd_probe(cand, j->phrase, e, fx, &m);
        cand[P_LEVEL] = m.level;
        float miss = tb_rnd_miss(j->cat, j->phrase, &m);
        if (miss < best_miss) { best_miss = miss; best_try = t; memcpy(best, cand, sizeof best); }
        g_rnd_vet_tries = t + 1;
        if (miss == 0.0f) break;
    }
    memcpy(out, best, sizeof best);
    return best_try > 0;
}

static void *tb_rnd_worker(void *arg)
{
    tb_instance_t *inst = (tb_instance_t *)arg;
    /* The lowest priority there is: it only ever uses time nothing else
     * wants. A thread inherits its creator's scheduling, and an instance is
     * created from the host's real-time audio thread -- inherited, this ran
     * at the audio threads' own priority and starved them for the length of
     * a measurement (dropouts, and a UI that stalled for half a second). */
#if defined(__linux__) && defined(SCHED_IDLE)
    {
        struct sched_param sp; memset(&sp, 0, sizeof sp);
        if (pthread_setschedparam(pthread_self(), SCHED_IDLE, &sp) != 0) {
            pthread_setschedparam(pthread_self(), SCHED_OTHER, &sp);
            (void)nice(19);
        }
        pthread_setname_np(pthread_self(), "tenebrae-rnd");
    }
#endif
    /* the scratch engine is allocated at the first roll, so an instance
     * that never rolls never holds it */
    tb_engine_t *e = NULL;
    tb_fx_t *fx = NULL;
    uint32_t done = 0;
    tb_rjob_t job;
    float res[P_COUNT];
    for (;;) {
        /* Wait for work. The audio side wakes this when it can without
         * waiting; a wake it skipped is caught by the timeout. */
        pthread_mutex_lock(&inst->rnd_mu);
        while (!__atomic_load_n(&inst->rnd_quit, __ATOMIC_ACQUIRE) &&
               __atomic_load_n(&inst->rnd_job_gen, __ATOMIC_ACQUIRE) == done) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_nsec += 20 * 1000 * 1000;
            if (ts.tv_nsec >= 1000000000L) { ts.tv_sec++; ts.tv_nsec -= 1000000000L; }
            pthread_cond_timedwait(&inst->rnd_cv, &inst->rnd_mu, &ts);
        }
        pthread_mutex_unlock(&inst->rnd_mu);
        if (__atomic_load_n(&inst->rnd_quit, __ATOMIC_ACQUIRE)) break;

        /* copy the job; keep the copy only if it was not being rewritten */
        uint32_t gen = __atomic_load_n(&inst->rnd_job_gen, __ATOMIC_ACQUIRE);
        uint32_t s1 = __atomic_load_n(&inst->rnd_job_seq, __ATOMIC_ACQUIRE);
        if (s1 & 1u) continue;
        memcpy(&job, &inst->rnd_job, sizeof job);
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
        if (__atomic_load_n(&inst->rnd_job_seq, __ATOMIC_ACQUIRE) != s1) continue;
        if (__atomic_load_n(&inst->rnd_job_gen, __ATOMIC_ACQUIRE) != gen) continue;
        done = gen;

        if (!e) e = (tb_engine_t *)malloc(sizeof(tb_engine_t));
        if (!fx) fx = (tb_fx_t *)malloc(sizeof(tb_fx_t));
        if (!e || !fx) continue;
        uint64_t fp = tb_fz_on();
        int swapped = tb_rnd_vet(&job, res, e, fx);
        tb_fz_restore(fp);
        /* a newer roll was asked for meanwhile: this one is stale */
        if (__atomic_load_n(&inst->rnd_job_gen, __ATOMIC_ACQUIRE) != gen) continue;
        memcpy(inst->rnd_res_p, res, sizeof res);
        inst->rnd_res_swapped = swapped;
        inst->rnd_res_writes = job.writes;
        __atomic_store_n(&inst->rnd_res_gen, gen, __ATOMIC_RELEASE);
    }
    free(e); free(fx);
    return NULL;
}

/* The worker starts with the instance, not at the first roll: set_param may
 * be called on the audio thread, and creating a thread there is not. It is
 * created with ordinary scheduling, never the creator's (see the worker). */
static void tb_rnd_start(tb_instance_t *inst)
{
    pthread_mutex_init(&inst->rnd_mu, NULL);
    pthread_cond_init(&inst->rnd_cv, NULL);
    pthread_attr_t at;
    pthread_attr_init(&at);
    struct sched_param sp; memset(&sp, 0, sizeof sp);
    pthread_attr_setinheritsched(&at, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&at, SCHED_OTHER);
    pthread_attr_setschedparam(&at, &sp);
    inst->rnd_th_on = pthread_create(&inst->rnd_th, &at, tb_rnd_worker, inst) == 0;
    pthread_attr_destroy(&at);
}

/* Apply a measured result to the sound that is playing. Called on the audio
 * thread (from render), or inline by the native tool. */
static void tb_rnd_apply(tb_instance_t *inst, const float *res, int swapped, uint32_t writes)
{
    /* a replacement only if nothing has been turned since the roll was made:
     * a knob you moved is not undone by a measurement */
    if (swapped && inst->rnd_writes == writes) {
        int was_mono = inst->eng.p[P_VOICE] >= 0.5f, now_mono = res[P_VOICE] >= 0.5f;
        if (was_mono != now_mono) tb_all_notes_off(&inst->eng);
        memcpy(inst->eng.p, res, sizeof inst->eng.p);
    } else {
        inst->eng.p[P_LEVEL] = res[P_LEVEL];
    }
    inst->rnd_busy = 0;
    inst->rnd_seq++;
}

/* Hand the roll on now to the worker (or measure it inline in the tool) */
static void tb_rnd_submit(tb_instance_t *inst, int recipe, int cat, const char *phrase, uint32_t seed)
{
    inst->rnd_gen++;
    inst->rnd_busy = 1;
    if (inst->rnd_sync || !inst->rnd_th_on) {
        static tb_rjob_t j;
        static tb_engine_t e; static tb_fx_t fx;
        static float res[P_COUNT];
        j.recipe = recipe; j.cat = cat; j.seed = seed; j.phrase = phrase;
        memcpy(j.p, inst->eng.p, sizeof j.p);
        memcpy(j.anchor, inst->rnd_anchor, sizeof j.anchor);
        if (!inst->rnd_sync) { inst->rnd_busy = 0; return; }   /* no worker: the first guess stands */
        int swapped = tb_rnd_vet(&j, res, &e, &fx);
        tb_rnd_apply(inst, res, swapped, inst->rnd_writes);
        return;
    }
    /* No lock: the worker may be paused at idle priority at any moment, and
     * the audio side must never wait on it. */
    __atomic_add_fetch(&inst->rnd_job_seq, 1u, __ATOMIC_ACQ_REL);   /* odd: writing */
    inst->rnd_job.recipe = recipe; inst->rnd_job.cat = cat; inst->rnd_job.seed = seed;
    inst->rnd_job.phrase = phrase;
    memcpy(inst->rnd_job.p, inst->eng.p, sizeof inst->rnd_job.p);
    memcpy(inst->rnd_job.anchor, inst->rnd_anchor, sizeof inst->rnd_job.anchor);
    inst->rnd_job.writes = inst->rnd_writes;
    __atomic_add_fetch(&inst->rnd_job_seq, 1u, __ATOMIC_ACQ_REL);   /* even: done */
    __atomic_store_n(&inst->rnd_job_gen, inst->rnd_gen, __ATOMIC_RELEASE);
    if (pthread_mutex_trylock(&inst->rnd_mu) == 0) {
        pthread_cond_signal(&inst->rnd_cv);
        pthread_mutex_unlock(&inst->rnd_mu);
    }
}

/* The sound before the first roll of a run is what Revert puts back. */
static void tb_rnd_anchor(tb_instance_t *inst)
{
    if (inst->rnd_pending) return;
    memcpy(inst->rnd_anchor, inst->eng.p, sizeof inst->rnd_anchor);
    inst->rnd_a_preset = inst->preset;
    inst->rnd_a_named = inst->rnd_named;
    inst->rnd_a_cat = inst->rnd_cat;
    inst->rnd_a_n = inst->rnd_n;
    inst->rnd_pending = 1;
}

/* Put a new sound in place of the one playing, as a roll */
static void tb_rnd_install(tb_instance_t *inst, const float *p, int cat, int recipe)
{
    tb_all_notes_off(&inst->eng);
    for (int i = 0; i < P_COUNT; i++) tb_set(inst, i, p[i]);
    inst->preset = -1;
    inst->rnd_cat = cat;
    inst->rnd_recipe = recipe;
    inst->rnd_n = inst->rnd_n % 999 + 1;
    inst->rnd_named = 1;
    inst->rnd_seq++;
}

/* Roll a new sound from recipe `ri` */
static int tb_roll_recipe(tb_instance_t *inst, int ri)
{
    if (ri < 0 || ri >= TB_RECIPE_COUNT) return -1;
    int cat = TB_RECIPES[ri].cat;
    uint32_t seed = tb_rnd_next(&inst->rnd_rng);
    uint32_t rng = seed;
    float p[P_COUNT];
    tb_rnd_roll_recipe(p, ri, &rng);
    tb_rnd_anchor(inst);
    tb_rnd_install(inst, p, cat, ri);
    const char *phrase = TB_RECIPES[ri].phrase ? TB_RECIPES[ri].phrase : TB_RND_PHRASE[cat];
    tb_rnd_submit(inst, ri, cat, phrase, tb_rnd_next(&rng));
    return ri;
}

/* Roll a new sound of category `cat`. Returns the recipe it was made from. */
static int tb_roll(tb_instance_t *inst, int cat)
{
    if (cat < 0 || cat >= R_COUNT) return -1;
    return tb_roll_recipe(inst, tb_rnd_recipe_for(cat, &inst->rnd_rng));
}

/* The category of the sound Vary starts from */
static int tb_rnd_anchor_cat(const tb_instance_t *inst)
{
    if (inst->rnd_a_preset >= 0) {
        int c = tb_rnd_preset_cat(inst->rnd_a_preset);
        if (c >= 0) return c;
    }
    if (inst->rnd_a_preset < 0 && inst->rnd_a_named) return inst->rnd_a_cat;
    return tb_rnd_nearest_cat(inst->rnd_anchor);
}

/* A sibling of the sound you started from */
static void tb_vary(tb_instance_t *inst)
{
    tb_rnd_anchor(inst);
    int cat = tb_rnd_anchor_cat(inst);
    uint32_t seed = tb_rnd_next(&inst->rnd_rng);
    uint32_t rng = seed;
    float p[P_COUNT];
    memcpy(p, inst->rnd_anchor, sizeof p);
    tb_rnd_vary(p, cat, &rng);
    tb_rnd_install(inst, p, cat, -1);
    const char *phrase = inst->rnd_a_preset >= 0 && tb_rnd_preset_cat(inst->rnd_a_preset) >= 0
                       ? TB_PRESETS[inst->rnd_a_preset].cat : TB_RND_PHRASE[cat];
    tb_rnd_submit(inst, -1, cat, phrase, tb_rnd_next(&rng));
}

/* Keep what is playing: it is your sound now */
static void tb_rnd_keep(tb_instance_t *inst)
{
    inst->rnd_pending = 0;
    inst->rnd_seq++;
}

/* Put back the sound from before the first roll, exactly */
static void tb_rnd_revert(tb_instance_t *inst)
{
    if (!inst->rnd_pending) return;
    tb_all_notes_off(&inst->eng);
    memcpy(inst->eng.p, inst->rnd_anchor, sizeof inst->eng.p);
    inst->preset = inst->rnd_a_preset;
    inst->rnd_named = inst->rnd_a_named;
    inst->rnd_cat = inst->rnd_a_cat;
    inst->rnd_n = inst->rnd_a_n;
    inst->rnd_pending = 0;
    inst->rnd_busy = 0;
    inst->rnd_gen++;   /* a measurement in flight belongs to a roll that is gone */
    inst->rnd_seq++;
}

/*
 * The Random page's one command key. Its value is nonce * 100 + op: a host
 * that re-sends an unconfirmed write sends the same value, which is ignored,
 * and get_param("rnd_cmd") answers it back.
 */
enum { RC_KEEP = 0, RC_REVERT = 1, RC_VARY = 2, RC_ROLL = 10 };
static void tb_rnd_command(tb_instance_t *inst, long v)
{
    if (v == inst->rnd_last_cmd) return;
    inst->rnd_last_cmd = v;
    int op = (int)(v % 100);
    if (op == RC_KEEP) tb_rnd_keep(inst);
    else if (op == RC_REVERT) tb_rnd_revert(inst);
    else if (op == RC_VARY) tb_vary(inst);
    else if (op >= RC_ROLL && op < RC_ROLL + R_COUNT) tb_roll(inst, op - RC_ROLL);
}

/* ------------------------------------------------------------------------ */
/* JSON out: a bounded appender, so a short buffer truncates and never overruns */
/* ------------------------------------------------------------------------ */

typedef struct { char *buf; int cap; int len; int over; } tb_out_t;

static void out_fmt(tb_out_t *o, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
#include <stdarg.h>
static void out_fmt(tb_out_t *o, const char *fmt, ...)
{
    if (o->over) return;
    va_list ap;
    va_start(ap, fmt);
    int room = o->cap - o->len;
    int n = vsnprintf(o->buf + o->len, (size_t)(room > 0 ? room : 0), fmt, ap);
    va_end(ap);
    if (n < 0 || n >= room) { o->over = 1; if (o->cap > 0) o->buf[o->cap - 1] = '\0'; return; }
    o->len += n;
}

/* "a|b|c" -> "a","b","c" */
static void out_options(tb_out_t *o, const char *opts)
{
    const char *start = opts;
    for (const char *c = opts; ; c++) {
        if (*c == '|' || *c == '\0') {
            out_fmt(o, "%s\"%.*s\"", start == opts ? "" : ",", (int)(c - start), start);
            start = c + 1;
            if (*c == '\0') break;
        }
    }
}

static int build_chain_params(char *buf, int cap)
{
    tb_out_t o = { buf, cap, 0, 0 };
    out_fmt(&o, "[");
    for (int i = 0; i < P_COUNT; i++) {
        const tb_param_t *pd = &TB_PARAMS[i];
        out_fmt(&o, "%s{\"key\":\"%s\",\"name\":\"%s\",\"short_name\":\"%s\",",
                i ? "," : "", pd->key, pd->name, pd->short_name);
        if (pd->type == PT_ENUM) {
            /* A labelled param must say "enum": the host takes its kind from
             * the type alone, and an int carrying options stays a number. */
            out_fmt(&o, "\"type\":\"enum\",\"options\":[");
            out_options(&o, pd->options);
            out_fmt(&o, "],\"short_options\":[");
            out_options(&o, pd->short_options ? pd->short_options : pd->options);
            out_fmt(&o, "],\"default\":%d", (int)pd->def);
        } else {
            out_fmt(&o, "\"type\":\"float\",\"min\":%d,\"max\":1,\"step\":0.01,\"default\":%g,\"unit\":\"%%\"",
                    pd->type == PT_BIPOLAR ? -1 : 0, (double)pd->def);
        }
        if (pd->viz) out_fmt(&o, ",\"viz\":%s", pd->viz);
        out_fmt(&o, "}");
    }
    /* The Random page: drawn by random.js, which the host loads from the
     * module's folder. Not a parameter: nothing is saved or modulated. */
    out_fmt(&o, ",{\"key\":\"rnd_page\",\"name\":\"Random\",\"type\":\"canvas\","
                "\"canvas_script\":\"random.js\",\"as_page\":true,\"enterable\":true,"
                "\"extra_keys\":[\"rnd_status\"]}");
    out_fmt(&o, "]");
    return o.over ? -1 : o.len;
}

/*
 * The hierarchy. For a sound generator this is the ONLY copy the UI reads: the
 * chain host asks the plugin and has no module.json fallback.
 *
 * Root is the preset browser and nothing else, so the browser is the first
 * page and the six knob pages follow in jog order, each named by its own
 * level, then the Random page. A key lives on exactly one level, so nothing is deduplicated away.
 */
static int build_ui_hierarchy(char *buf, int cap)
{
    tb_out_t o = { buf, cap, 0, 0 };
    out_fmt(&o, "{\"pad_layout\":\"chromatic\",\"levels\":{\"root\":{\"name\":\"Tenebrae\","
                "\"list_param\":\"preset\",\"count_param\":\"preset_count\",\"name_param\":\"preset_name\","
                "\"params\":[");
    for (int pg = 0; pg < TB_PAGE_COUNT; pg++)
        out_fmt(&o, "%s{\"level\":\"%s\",\"label\":\"%s\"}", pg ? "," : "",
                TB_PAGES[pg].level, TB_PAGES[pg].name);
    out_fmt(&o, ",{\"level\":\"random\",\"label\":\"Random\"}]}");
    /* Random: one page the module draws (random.js), clicked into like a list */
    out_fmt(&o, ",\"random\":{\"name\":\"Random\",\"knobs\":[],\"params\":[\"rnd_page\"]}");
    for (int pg = 0; pg < TB_PAGE_COUNT; pg++) {
        out_fmt(&o, ",\"%s\":{\"name\":\"%s\",\"knobs\":[", TB_PAGES[pg].level, TB_PAGES[pg].name);
        for (int k = 0; k < tb_page_knobs(pg); k++)
            out_fmt(&o, "%s\"%s\"", k ? "," : "", TB_PARAMS[TB_PAGES[pg].knobs[k]].key);
        out_fmt(&o, "],\"params\":[");
        for (int k = 0; k < tb_page_knobs(pg); k++)
            out_fmt(&o, "%s\"%s\"", k ? "," : "", TB_PARAMS[TB_PAGES[pg].knobs[k]].key);
        out_fmt(&o, "]}");
    }
    out_fmt(&o, "}}");
    return o.over ? -1 : o.len;
}

/* The whole sound, self-contained: every parameter by key. */
static int build_state(const tb_instance_t *inst, char *buf, int cap)
{
    tb_out_t o = { buf, cap, 0, 0 };
    /* "preset" is the preset's uid, not its place in the browser: the bank can
     * be reordered without renaming what has been saved from it */
    out_fmt(&o, "{\"tenebrae\":1,\"preset\":%d,\"octave_transpose\":%d",
            inst->preset >= 0 ? TB_PRESETS[inst->preset].uid : -1, inst->eng.octave);
    /* a roll keeps its name: "Rand Bass 12" comes back as itself */
    if (inst->preset < 0 && inst->rnd_named)
        out_fmt(&o, ",\"rnd_cat\":%d,\"rnd_n\":%d", inst->rnd_cat, inst->rnd_n);
    for (int i = 0; i < P_COUNT; i++) {
        if (TB_PARAMS[i].type == PT_ENUM)
            out_fmt(&o, ",\"%s\":%d", TB_PARAMS[i].key, (int)inst->eng.p[i]);
        else
            out_fmt(&o, ",\"%s\":%.4f", TB_PARAMS[i].key, (double)inst->eng.p[i]);
    }
    out_fmt(&o, "}");
    return o.over ? -1 : o.len;
}

/* Restore from a state blob: a flat object of "key":number pairs. Also reads a
 * wrapped preset file ({name, module, version, state:{...}}), since it simply
 * takes every pair whose key it knows. A control the blob does not name goes
 * back to its default: a state saved before that control existed must sound as
 * it did when it was saved, not keep whatever the instance was set to. */
static void restore_state(tb_instance_t *inst, const char *json)
{
    const char *p = json;
    int preset = -1, rnd_cat = -1, rnd_n = 0;
    unsigned char seen[P_COUNT];
    memset(seen, 0, sizeof seen);
    tb_all_notes_off(&inst->eng);
    while ((p = strchr(p, '"')) != NULL) {
        const char *ks = ++p;
        const char *ke = strchr(p, '"');
        if (!ke) break;
        char key[40];
        int kl = (int)(ke - ks);
        if (kl >= (int)sizeof(key)) kl = (int)sizeof(key) - 1;
        memcpy(key, ks, (size_t)kl);
        key[kl] = '\0';
        p = ke + 1;
        while (*p == ' ' || *p == '\t') p++;
        if (*p != ':') continue;
        p++;
        while (*p == ' ' || *p == '\t') p++;
        char *end = NULL;
        double val = strtod(p, &end);
        if (end == p) continue;
        p = end;
        int id = tb_find_param(key);
        if (id >= 0) { tb_set(inst, id, (float)val); seen[id] = 1; }
        else if (strcmp(key, "preset") == 0) preset = (int)val;
        else if (strcmp(key, "rnd_cat") == 0) rnd_cat = (int)val;
        else if (strcmp(key, "rnd_n") == 0) rnd_n = (int)val;
        else if (strcmp(key, "octave_transpose") == 0) {
            int oc = (int)val;
            inst->eng.octave = oc < -4 ? -4 : (oc > 4 ? 4 : oc);
        }
    }
    for (int i = 0; i < P_COUNT; i++)
        if (!seen[i]) tb_set(inst, i, TB_PARAMS[i].def);
    inst->preset = -1;
    for (int i = 0; i < TB_PRESET_COUNT && preset >= 0; i++)
        if (TB_PRESETS[i].uid == preset) { inst->preset = i; break; }
    inst->rnd_gen++;
    inst->rnd_pending = 0;
    inst->rnd_busy = 0;
    inst->rnd_seq++;
    inst->rnd_named = inst->preset < 0 && rnd_cat >= 0 && rnd_cat < R_COUNT;
    if (inst->rnd_named) {
        inst->rnd_cat = rnd_cat;
        inst->rnd_n = rnd_n < 1 ? 1 : (rnd_n > 999 ? 999 : rnd_n);
    }
}

/* ------------------------------------------------------------------------ */
/* Plugin API                                                                */
/* ------------------------------------------------------------------------ */

static void *v2_create_instance(const char *module_dir, const char *json_defaults)
{
    (void)module_dir; (void)json_defaults;
    tb_instance_t *inst = (tb_instance_t *)calloc(1, sizeof(tb_instance_t));
    if (!inst) return NULL;
    tb_engine_init(&inst->eng);
    tb_fx_init(&inst->fx);
    inst->bpm = 120.0f;
    inst->idle = 1;
    /* every instance rolls its own sequence */
    inst->rnd_rng = (uint32_t)time(NULL) * 2654435761u ^ (uint32_t)(uintptr_t)inst;
    tb_load_preset(inst, 0);
    tb_rnd_start(inst);
    /* the first read resolves the host's export; keep that off the audio path */
    move_info_t mi;
    (void)move_info_read(&mi);
    return inst;
}

static void v2_destroy_instance(void *instance)
{
    tb_instance_t *inst = (tb_instance_t *)instance;
    if (inst && inst->rnd_th_on) {
        pthread_mutex_lock(&inst->rnd_mu);
        __atomic_store_n(&inst->rnd_quit, 1, __ATOMIC_RELEASE);
        pthread_cond_signal(&inst->rnd_cv);
        pthread_mutex_unlock(&inst->rnd_mu);
        pthread_join(inst->rnd_th, NULL);
    }
    if (inst) {
        pthread_cond_destroy(&inst->rnd_cv);
        pthread_mutex_destroy(&inst->rnd_mu);
    }
    free(instance);
}

static void v2_on_midi(void *instance, const uint8_t *msg, int len, int source)
{
    tb_instance_t *inst = (tb_instance_t *)instance;
    (void)source;
    if (!inst || len < 2) return;
    uint8_t st = msg[0] & 0xF0;
    uint8_t d1 = msg[1] & 0x7F;
    uint8_t d2 = len > 2 ? (msg[2] & 0x7F) : 0;
    switch (st) {
    case 0x90:
        if (d2 > 0) {
            tb_note_on(&inst->eng, d1, d2);
            if (inst->idle) { inst->fx.dly_snap = 1; inst->eng.snap = 1; }
            inst->idle = 0; inst->quiet_blocks = 0;
        }
        else tb_note_off(&inst->eng, d1);
        break;
    case 0x80:
        tb_note_off(&inst->eng, d1);
        break;
    case 0xA0:
        tb_poly_pressure(&inst->eng, d1, d2);
        break;
    case 0xD0:
        inst->eng.chan_pressure = (float)d1 / 127.0f;
        break;
    case 0xE0: {
        int v = ((int)d2 << 7 | (int)d1) - 8192;
        inst->eng.bend = (float)v / 8192.0f * 2.0f;
        break;
    }
    case 0xB0:
        if (d1 == 1) inst->eng.modwheel = (float)d2 / 127.0f;
        else if (d1 == 64) tb_set_sustain(&inst->eng, d2 >= 64);
        else if (d1 == 120 || d1 == 123) tb_all_notes_off(&inst->eng);
        break;
    default:
        break;
    }
}

static void v2_set_param(void *instance, const char *key, const char *val)
{
    tb_instance_t *inst = (tb_instance_t *)instance;
    if (!inst || !key || !val) return;

    size_t kl = strlen(key);
    if (strcmp(key, "state") == 0 || (kl >= 6 && strcmp(key + kl - 6, ":state") == 0)) {
        restore_state(inst, val);
        return;
    }
    if (strcmp(key, "preset") == 0) { tb_load_preset(inst, atoi(val)); return; }
    if (strcmp(key, "rnd_cmd") == 0) { tb_rnd_command(inst, strtol(val, NULL, 10)); return; }
    if (strcmp(key, "rnd_seed") == 0) { inst->rnd_rng = (uint32_t)strtoul(val, NULL, 10) * 2654435761u + 1u; return; }
    if (strcmp(key, "all_notes_off") == 0) { tb_all_notes_off(&inst->eng); return; }
    if (strcmp(key, "octave_transpose") == 0) {
        int oc = atoi(val);
        inst->eng.octave = oc < -4 ? -4 : (oc > 4 ? 4 : oc);
        return;
    }
    int id = tb_find_param(key);
    if (id < 0) return;
    if (id == P_LEVEL) inst->rnd_gen++;   /* a Level you set outranks a measured one */
    inst->rnd_writes++;
    float v = TB_PARAMS[id].type == PT_ENUM ? tb_parse_enum(id, val) : (float)atof(val);
    tb_set(inst, id, v);
}

/* What the Random page shows: {seq, pending, busy, name, arch, cat, anchor}.
 * seq only grows, so the page can tell a fresh answer from a stale one. */
static void tb_sound_name(const tb_instance_t *inst, int preset, int named, int cat, int n, char *out, size_t cap)
{
    if (preset >= 0) snprintf(out, cap, "%s", TB_PRESETS[preset].name);
    else if (named) snprintf(out, cap, "Rand %s %d", TB_RND_NAMES[cat], n);
    else snprintf(out, cap, "Edited");
    (void)inst;
}
static int build_rnd_status(const tb_instance_t *inst, char *buf, int cap)
{
    char cur[48], anc[48];
    tb_sound_name(inst, inst->preset, inst->rnd_named, inst->rnd_cat, inst->rnd_n, cur, sizeof cur);
    tb_sound_name(inst, inst->rnd_a_preset, inst->rnd_a_named, inst->rnd_a_cat, inst->rnd_a_n, anc, sizeof anc);
    const char *arch = inst->preset < 0 && inst->rnd_named
                     ? (inst->rnd_recipe >= 0 ? TB_RECIPES[inst->rnd_recipe].name : "Vary") : "";
    tb_out_t o = { buf, cap, 0, 0 };
    out_fmt(&o, "{\"seq\":%u,\"pending\":%d,\"busy\":%d,\"name\":\"%s\",\"arch\":\"%s\",\"cat\":%d,\"anchor\":\"%s\"}",
            (unsigned)inst->rnd_seq, inst->rnd_pending, inst->rnd_busy, cur, arch,
            inst->rnd_named ? inst->rnd_cat : -1, inst->rnd_pending ? anc : "");
    return o.over ? -1 : o.len;
}

static int v2_get_param(void *instance, const char *key, char *buf, int buf_len)
{
    tb_instance_t *inst = (tb_instance_t *)instance;
    if (!inst || !key || !buf || buf_len <= 0) return -1;
    int n = -1;

    if (strcmp(key, "chain_params") == 0) return build_chain_params(buf, buf_len);
    if (strcmp(key, "ui_hierarchy") == 0) return build_ui_hierarchy(buf, buf_len);
    if (strcmp(key, "state") == 0) return build_state(inst, buf, buf_len);

    if (strcmp(key, "name") == 0) n = snprintf(buf, (size_t)buf_len, "Tenebrae");
    else if (strcmp(key, "preset") == 0) n = snprintf(buf, (size_t)buf_len, "%d", inst->preset < 0 ? 0 : inst->preset);
    else if (strcmp(key, "preset_count") == 0) n = snprintf(buf, (size_t)buf_len, "%d", TB_PRESET_COUNT);
    else if (strcmp(key, "preset_name") == 0) {
        if (inst->preset >= 0) n = snprintf(buf, (size_t)buf_len, "%s", TB_PRESETS[inst->preset].name);
        else if (inst->rnd_named) n = snprintf(buf, (size_t)buf_len, "Rand %s %d", TB_RND_NAMES[inst->rnd_cat], inst->rnd_n);
        else n = snprintf(buf, (size_t)buf_len, "Edited");
    }
    else if (strcmp(key, "rnd_cmd") == 0) n = snprintf(buf, (size_t)buf_len, "%ld", inst->rnd_last_cmd);
    else if (strcmp(key, "rnd_status") == 0) return build_rnd_status(inst, buf, buf_len);
    else if (strncmp(key, "preset_name:", 12) == 0) {
        int i = atoi(key + 12);
        n = snprintf(buf, (size_t)buf_len, "%s", (i >= 0 && i < TB_PRESET_COUNT) ? TB_PRESETS[i].name : "");
    }
    else if (strcmp(key, "octave_transpose") == 0) n = snprintf(buf, (size_t)buf_len, "%d", inst->eng.octave);
    else if (strcmp(key, "polyphony") == 0) n = snprintf(buf, (size_t)buf_len, "%d", inst->eng.active_voices);
    if (n >= 0) return n < buf_len ? n : buf_len - 1;

    int id = tb_find_param(key);
    if (id < 0) return -1;
    if (TB_PARAMS[id].type == PT_ENUM) n = snprintf(buf, (size_t)buf_len, "%d", (int)inst->eng.p[id]);
    else n = snprintf(buf, (size_t)buf_len, "%.4f", (double)inst->eng.p[id]);
    return n < buf_len ? n : buf_len - 1;
}

static int v2_get_error(void *instance, char *buf, int buf_len)
{
    (void)instance; (void)buf; (void)buf_len;
    return 0;
}


static void v2_render_block(void *instance, int16_t *out_lr, int frames)
{
    tb_instance_t *inst = (tb_instance_t *)instance;
    if (!inst || !out_lr || frames <= 0) return;
    if (frames > TB_BLOCK_MAX) frames = TB_BLOCK_MAX;

    /* A measured roll: the worker's result, applied here, idle or not (the
     * page waits on it). Its Level, or the whole replacement roll. */
    if (__atomic_load_n(&inst->rnd_res_gen, __ATOMIC_ACQUIRE) == inst->rnd_gen && inst->rnd_gen) {
        tb_rnd_apply(inst, inst->rnd_res_p, inst->rnd_res_swapped, inst->rnd_res_writes);
        __atomic_store_n(&inst->rnd_res_gen, 0u, __ATOMIC_RELAXED);
    }

    if (inst->idle && inst->eng.active_voices == 0) {
        int any = 0;
        for (int i = 0; i < TB_VOICES; i++) if (inst->eng.v[i].active) { any = 1; break; }
        if (!any) {
            memset(out_lr, 0, sizeof(int16_t) * 2 * (size_t)frames);
            return;
        }
        inst->idle = 0;
        inst->fx.dly_snap = 1;
        inst->eng.snap = 1;
    }

    uint64_t fp = tb_fz_on();

    /* tempo and transport: the Set's own tempo when the host publishes it */
    float bpm = 0.0f;
    double beat = -1.0;
    move_info_t mi;
    if (move_info_read(&mi) && mi.valid && mi.tempo > 0.0f && mi.midi_clock_sync != 1) bpm = mi.tempo;
    if (g_host) {
        if (bpm <= 0.0f && g_host->get_bpm) bpm = g_host->get_bpm();
        if (g_host->get_beat_position) beat = g_host->get_beat_position();
    }
    if (bpm <= 0.0f) bpm = 120.0f;
    inst->bpm = bpm;

    inst->eng.bpm = bpm;
    inst->eng.beat = beat;
    tb_engine_render(&inst->eng, inst->bufL, inst->bufR, inst->bufSub, frames);
    tb_fx_process(&inst->fx, inst->eng.p, inst->bufL, inst->bufR, inst->bufSub, frames, bpm, beat);

    float peak = 0.0f;
    for (int i = 0; i < frames; i++) {
        float l = inst->bufL[i], r = inst->bufR[i];
        if (!isfinite(l)) l = 0.0f;
        if (!isfinite(r)) r = 0.0f;
        float a = fabsf(l) > fabsf(r) ? fabsf(l) : fabsf(r);
        if (a > peak) peak = a;
        out_lr[2 * i] = (int16_t)lrintf(tb_clamp(l, -1.0f, 1.0f) * 32767.0f);
        out_lr[2 * i + 1] = (int16_t)lrintf(tb_clamp(r, -1.0f, 1.0f) * 32767.0f);
    }

    /* Go idle once the voices are done and the tails have died, so a silent
     * slot costs nothing and answers with exact zeros. */
    if (inst->eng.active_voices == 0 && peak < TB_IDLE_FLOOR) {
        if (++inst->quiet_blocks >= TB_IDLE_BLOCKS) inst->idle = 1;
    } else {
        inst->quiet_blocks = 0;
    }

    tb_fz_restore(fp);
}

static plugin_api_v2_t g_api = {
    .api_version = 2,
    .create_instance = v2_create_instance,
    .destroy_instance = v2_destroy_instance,
    .on_midi = v2_on_midi,
    .set_param = v2_set_param,
    .get_param = v2_get_param,
    .get_error = v2_get_error,
    .render_block = v2_render_block,
};

__attribute__((visibility("default")))
plugin_api_v2_t *move_plugin_init_v2(const host_api_v1_t *host)
{
    g_host = host;
    return &g_api;
}

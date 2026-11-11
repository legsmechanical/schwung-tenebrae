/*
 * tenebrae_random.h -- the patch generator behind the Random page.
 *
 * A roll picks one recipe of the chosen category (tenebrae_recipes.h) and
 * builds the sound one section at a time, in the order a sound designer
 * would: oscillator, filter, amp envelope, modulation, motion effects, space
 * effects. Each section is either
 *
 *   BORROWED whole from a factory preset (of ANY category) whose section fits
 *   the recipe, or
 *   BLENDED from two fitting presets that share the section's deciding switch
 *   (model, filter type, LFO target, chorus type, delay division): every
 *   value lies on the line between two working sections, or a little past
 *   either end, so what holds a section together survives and the roll can
 *   still land somewhere no preset is.
 *
 * Between donors that fit, a section prefers the one that keeps the roll
 * inside the recipe's budgets (movement, space, grit, width), so what came
 * earlier shapes what comes later. Then the recipe's ranges are imposed, the
 * budgets are enforced, every live control is nudged a little, and the
 * couplings are tidied (no LFO delay without depth, and the like).
 *
 * Vary takes a sound that already exists and moves it: every live control
 * nudged, sometimes one of its modulation or effects sections replaced by a
 * same-category preset's. Model, voice mode, filter type and whether the
 * sound holds or dies are never changed.
 *
 * Cheap (no rendering, no allocation): safe in set_param. The plugin then
 * measures the result on its worker thread, sets its Level, and replaces a
 * roll that measures wrong for its category.
 */
#ifndef TENEBRAE_RANDOM_H
#define TENEBRAE_RANDOM_H

#include <string.h>
#include <math.h>
#include <stdint.h>
#include <pthread.h>
#include "tenebrae_params.h"
#include "tenebrae_presets.h"
#include "tenebrae_recipes.h"

/* Sections, in the order a roll builds them */
enum { G_OSC, G_FILTER, G_AMP, G_MOD, G_MOTION, G_SPACE, G_COUNT };
static const int TB_RND_GROUP[G_COUNT][9] = {
    [G_OSC]    = { P_MODEL, P_SHAPE, P_SHAPE_ENV, P_STACK, P_LAYER, P_BLEND, P_AGE, P_VOICE, -1 },
    [G_FILTER] = { P_FLT_TYPE, P_CUTOFF, P_RESO, P_DRIVE, P_FLT_ENV, P_FLT_A, P_FLT_D, P_FLT_S, -1 },
    [G_AMP]    = { P_AMP_A, P_AMP_D, P_AMP_S, P_AMP_R, -1 },
    [G_MOD]    = { P_LFO_DEST, P_LFO_RATE, P_LFO_DEPTH, P_LFO_DELAY, P_VEL, P_PRESS, P_PRESS_DEST, P_WHEEL, -1 },
    [G_MOTION] = { P_CHO_MODE, P_CHO_MIX, P_PHS_RATE, P_PHS_DEPTH, P_PUMP_RATE, P_PUMP, P_VHS, P_TONE, -1 },
    [G_SPACE]  = { P_DLY_TIME, P_DLY_FB, P_DLY_MIX, P_REV_SIZE, P_REV_TONE, P_SHIMMER, P_REV_MIX, -1 },
};
/* the switch two blended donors must share */
static const int TB_RND_STRUCT[G_COUNT] = { P_MODEL, P_FLT_TYPE, -1, P_LFO_DEST, P_CHO_MODE, P_DLY_TIME };
#define TB_RND_BORROW 0.4f     /* a section is taken whole this often; blended otherwise */

/* ------------------------------------------------------------------------ */
/* Random numbers                                                            */
/* ------------------------------------------------------------------------ */

static inline uint32_t tb_rnd_next(uint32_t *s)
{
    uint32_t x = *s ? *s : 0x6D2B79F5u;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *s = x;
    return x;
}
static inline float tb_rnd_unit(uint32_t *s) { return (float)(tb_rnd_next(s) >> 8) * (1.0f / 16777216.0f); }
static inline int tb_rnd_pick(uint32_t *s, int n) { return n > 0 ? (int)(tb_rnd_next(s) % (uint32_t)n) : 0; }
static inline float tb_rnd_tri(uint32_t *s) { return tb_rnd_unit(s) + tb_rnd_unit(s) - 1.0f; }

static inline float tb_rnd_clamp(int id, float v)
{
    float lo = tb_param_min(id), hi = tb_param_max(id);
    if (TB_PARAMS[id].type == PT_ENUM) v = floorf(v + 0.5f);
    return v < lo ? lo : (v > hi ? hi : v);
}

static inline int tb_rnd_is_switch(int id)
{
    return id == P_MODEL || id == P_LAYER || id == P_FLT_TYPE || id == P_LFO_DEST || id == P_VOICE;
}

/* ------------------------------------------------------------------------ */
/* The bank, expanded once to full parameter vectors                         */
/* ------------------------------------------------------------------------ */

static float g_rnd_bank[TB_PRESET_COUNT][P_COUNT];
static pthread_once_t g_rnd_bank_once = PTHREAD_ONCE_INIT;

static void tb_rnd_bank_build(void)
{
    for (int b = 0; b < TB_PRESET_COUNT; b++) {
        float *out = g_rnd_bank[b];
        for (int i = 0; i < P_COUNT; i++) out[i] = TB_PARAMS[i].def;
        const tb_preset_t *pr = &TB_PRESETS[b];
        for (int i = 0; i < P_COUNT && pr->v[i].id > 0; i++) {
            int id = pr->v[i].id - 1;
            if (id >= 0 && id < P_COUNT) out[id] = tb_rnd_clamp(id, pr->v[i].v);
        }
    }
}
static inline const float *tb_rnd_bank(int b)
{
    pthread_once(&g_rnd_bank_once, tb_rnd_bank_build);
    return g_rnd_bank[b];
}

/* The category a factory preset belongs to, by its name, or -1 (Init) */
static int tb_rnd_preset_cat(int b)
{
    for (int c = 0; c < R_COUNT; c++)
        if (strncmp(TB_PRESETS[b].name, TB_RND_PREFIX[c], strlen(TB_RND_PREFIX[c])) == 0) return c;
    return -1;
}

/* ------------------------------------------------------------------------ */
/* A recipe's ranges, its switches and its budgets                           */
/* ------------------------------------------------------------------------ */

typedef struct { float lo[P_COUNT], hi[P_COUNT]; unsigned char has[P_COUNT]; } tb_rr_t;

static void tb_rnd_ranges(const tb_recipe_t *r, tb_rr_t *rr)
{
    memset(rr, 0, sizeof *rr);
    for (int k = 0; k < 10 && TB_CAT_BASE[r->cat][k].id > 0; k++) {
        int id = TB_CAT_BASE[r->cat][k].id - 1;
        rr->lo[id] = TB_CAT_BASE[r->cat][k].lo; rr->hi[id] = TB_CAT_BASE[r->cat][k].hi; rr->has[id] = 1;
    }
    for (int k = 0; k < TB_RC_MAX && r->r[k].id > 0; k++) {
        int id = r->r[k].id - 1;
        rr->lo[id] = r->r[k].lo; rr->hi[id] = r->r[k].hi; rr->has[id] = 1;
    }
}

static inline unsigned tb_rnd_vclass(float v) { return v < 0.5f ? VM_POLY : (v < 1.5f ? VM_MONO : VM_GLIDE); }

/* Does this value of a switch belong to the recipe? */
static int tb_rnd_enum_ok(const tb_recipe_t *r, int id, float v)
{
    unsigned b = 1u << (int)v;
    switch (id) {
    case P_MODEL:    return (r->models & b) != 0;
    case P_LAYER:    return (r->layers & b) != 0;
    case P_FLT_TYPE: return (r->ftypes & b) != 0;
    case P_LFO_DEST: return (r->lfos & b) != 0;
    case P_VOICE:    return (r->voices & tb_rnd_vclass(v)) != 0;
    default:         return 1;
    }
}

/* Pick an allowed value of a switch */
static float tb_rnd_enum_pick(const tb_recipe_t *r, int id, uint32_t *rng)
{
    int n = (int)tb_param_max(id) + 1, ok[32], m = 0;
    for (int v = 0; v < n && v < 32; v++) {
        if (id == P_VOICE && v > 2) break;          /* poly, mono, glide: three classes */
        if (tb_rnd_enum_ok(r, id, (float)v)) ok[m++] = v;
    }
    if (!m) return TB_PARAMS[id].def;
    int v = ok[tb_rnd_pick(rng, m)];
    /* a glide mode: one of the glide times, 30 to 270 ms */
    if (id == P_VOICE && v == 2) v = 4 + tb_rnd_pick(rng, 8);
    return (float)v;
}

/* Does a donor's section belong to the recipe? Switches must be allowed,
 * and every control the recipe ranges must sit near its range. */
static int tb_rnd_fits(const tb_recipe_t *r, const tb_rr_t *rr, int g, const float *v)
{
    for (int k = 0; TB_RND_GROUP[g][k] >= 0; k++) {
        int id = TB_RND_GROUP[g][k];
        if (id == P_LAYER && v[P_BLEND] == 0.0f) continue;          /* unheard */
        if (id == P_LFO_DEST && v[P_LFO_DEPTH] == 0.0f) continue;   /* unheard */
        if (!tb_rnd_enum_ok(r, id, v[id])) return 0;
        if (!rr->has[id]) continue;
        float slack = TB_PARAMS[id].type == PT_ENUM ? 0.0f : 0.1f;
        if (v[id] < rr->lo[id] - slack || v[id] > rr->hi[id] + slack) return 0;
    }
    return 1;
}

static inline float tb_rnd_mv(const float *p)
{
    float lfo = p[P_LFO_DEPTH] * ((int)p[P_LFO_DEST] == D_PITCH ? 0.5f : 1.0f);
    return lfo + p[P_PHS_DEPTH] + p[P_PUMP] + 0.5f * p[P_VHS];
}
static inline float tb_rnd_sp(const float *p) { return p[P_DLY_MIX] * (0.5f + p[P_DLY_FB]) + p[P_REV_MIX] + 0.5f * p[P_SHIMMER]; }
static inline float tb_rnd_gr(const float *p) { return p[P_DRIVE] + 0.5f * p[P_RESO] + 0.5f * p[P_AGE]; }
static inline float tb_rnd_wd(const float *p) { return p[P_STACK] + p[P_CHO_MIX] + p[P_SPREAD]; }

/* How far a sound so far is over its budgets (0 = inside them all) */
static float tb_rnd_over(const tb_recipe_t *r, const float *p)
{
    float o = 0.0f, d;
    if ((d = tb_rnd_mv(p) - r->mv) > 0) o += d;
    if ((d = tb_rnd_sp(p) - r->sp) > 0) o += d;
    if ((d = tb_rnd_gr(p) - r->gr) > 0) o += d;
    if ((d = tb_rnd_wd(p) - r->wd_hi) > 0) o += d;
    return o;
}

/* Hold a recipe's switches and ranged controls inside the recipe */
static void tb_rnd_impose(const tb_recipe_t *r, const tb_rr_t *rr, float *p, uint32_t *rng)
{
    for (int id = 0; id < P_COUNT; id++) {
        if (tb_rnd_is_switch(id)) {
            int heard = !(id == P_LAYER && p[P_BLEND] == 0.0f) && !(id == P_LFO_DEST && p[P_LFO_DEPTH] == 0.0f);
            if (heard && !tb_rnd_enum_ok(r, id, p[id])) p[id] = tb_rnd_enum_pick(r, id, rng);
        }
        if (!rr->has[id]) continue;
        if (p[id] < rr->lo[id] || p[id] > rr->hi[id]) {
            float v = rr->lo[id] + (rr->hi[id] - rr->lo[id]) * tb_rnd_unit(rng);
            /* out of range: land inside, near the edge it crossed */
            if (TB_PARAMS[id].type != PT_ENUM) {
                float edge = p[id] < rr->lo[id] ? rr->lo[id] : rr->hi[id];
                v = edge + (v - edge) * 0.3f;
            }
            p[id] = tb_rnd_clamp(id, v);
        }
    }
}

/* Scale a set of amounts down so their budget fits, never below a recipe's
 * own floor for them */
static void tb_rnd_shrink(const tb_rr_t *rr, float *p, const int *ids, float have, float cap)
{
    if (have <= cap || have <= 0.0f) return;
    float k = cap / have;
    for (int i = 0; ids[i] >= 0; i++) {
        int id = ids[i];
        float v = p[id] * k;
        if (rr && rr->has[id] && v < rr->lo[id]) v = rr->lo[id];
        p[id] = v;
    }
}

/* What a roll always tidies: settings nobody hears go back to their
 * defaults, so a roll reads like a preset written by hand. */
static void tb_rnd_tidy(float *p)
{
    if (p[P_LFO_DEPTH] == 0.0f) p[P_LFO_DELAY] = 0.0f;
    if (p[P_DLY_MIX] == 0.0f) p[P_DLY_FB] = TB_PARAMS[P_DLY_FB].def;
    if (p[P_CHO_MIX] == 0.0f) p[P_CHO_MODE] = TB_PARAMS[P_CHO_MODE].def;
    if (p[P_PHS_DEPTH] == 0.0f) p[P_PHS_RATE] = TB_PARAMS[P_PHS_RATE].def;
    if (p[P_PUMP] == 0.0f) p[P_PUMP_RATE] = TB_PARAMS[P_PUMP_RATE].def;
    if (p[P_VOICE] >= 0.5f) p[P_SPREAD] = 0.0f;   /* Spread places chord notes */
}

/* Nudge every control that is doing something, by up to +-amt of its range */
static void tb_rnd_nudge(float *p, float amt, uint32_t *rng)
{
    for (int i = 0; i < P_COUNT; i++) {
        if (TB_PARAMS[i].type == PT_ENUM) continue;
        if (i == P_LEVEL || i == P_VEL || i == P_PRESS || i == P_WHEEL) continue;
        if (p[i] == 0.0f) continue;   /* a control at zero stays at zero */
        float span = TB_PARAMS[i].type == PT_BIPOLAR ? 2.0f : 1.0f;
        float v = tb_rnd_clamp(i, p[i] + tb_rnd_tri(rng) * amt * span);
        if (TB_PARAMS[i].type == PT_BIPOLAR && v * p[i] < 0.0f) v = 0.0f;   /* never flips sides */
        p[i] = v;
    }
}

/* The middle of the Levels a category's presets use: a roll's first guess */
static float tb_rnd_cat_level(int cat)
{
    float sum = 0.0f; int m = 0;
    for (int b = 1; b < TB_PRESET_COUNT; b++)
        if (tb_rnd_preset_cat(b) == cat) { sum += tb_rnd_bank(b)[P_LEVEL]; m++; }
    return m ? sum / (float)m : TB_PARAMS[P_LEVEL].def;
}

/* ------------------------------------------------------------------------ */
/* Rolling                                                                   */
/* ------------------------------------------------------------------------ */

/* One of a category's recipes, at random */
static int tb_rnd_recipe_for(int cat, uint32_t *rng)
{
    int idx[16], n = 0;
    for (int i = 0; i < TB_RECIPE_COUNT && n < 16; i++) if (TB_RECIPES[i].cat == cat) idx[n++] = i;
    return n ? idx[tb_rnd_pick(rng, n)] : -1;
}

/* Build one section of p from the bank, or invent it from the recipe */
static void tb_rnd_section(const tb_recipe_t *r, const tb_rr_t *rr, int g, float *p, uint32_t *rng)
{
    int cand[TB_PRESET_COUNT], n = 0;
    for (int b = 1; b < TB_PRESET_COUNT; b++)   /* 0 is Init: nothing to learn from */
        if (tb_rnd_fits(r, rr, g, tb_rnd_bank(b))) cand[n++] = b;
    const int *ids = TB_RND_GROUP[g];

    if (n == 0) {
        /* nothing in the bank fits: invent it inside the recipe */
        for (int k = 0; ids[k] >= 0; k++) {
            int id = ids[k];
            if (rr->has[id]) p[id] = tb_rnd_clamp(id, rr->lo[id] + (rr->hi[id] - rr->lo[id]) * tb_rnd_unit(rng));
            else if (tb_rnd_is_switch(id)) p[id] = tb_rnd_enum_pick(r, id, rng);
            else p[id] = TB_PARAMS[id].def;
        }
        return;
    }

    /* the first donor: of a few tries, the one that keeps the budgets best */
    float trial[P_COUNT];
    int a = cand[tb_rnd_pick(rng, n)];
    float best = 1e9f;
    for (int t = 0; t < 6; t++) {
        int c = cand[tb_rnd_pick(rng, n)];
        memcpy(trial, p, sizeof trial);
        for (int k = 0; ids[k] >= 0; k++) trial[ids[k]] = tb_rnd_bank(c)[ids[k]];
        float o = tb_rnd_over(r, trial);
        if (o < best) { best = o; a = c; }
        if (o == 0.0f) break;
    }
    const float *A = tb_rnd_bank(a);

    /* a second donor sharing the deciding switch, to blend with */
    int b = -1;
    if (tb_rnd_unit(rng) >= TB_RND_BORROW) {
        int sid = TB_RND_STRUCT[g], m = 0, same[TB_PRESET_COUNT];
        for (int i = 0; i < n; i++)
            if (cand[i] != a && (sid < 0 || tb_rnd_bank(cand[i])[sid] == A[sid])) same[m++] = cand[i];
        if (m) b = same[tb_rnd_pick(rng, m)];
    }
    if (b < 0) {
        for (int k = 0; ids[k] >= 0; k++) p[ids[k]] = A[ids[k]];
        return;
    }
    const float *B = tb_rnd_bank(b);
    /* anywhere on the line between them, or a fifth past either end */
    float t = -0.2f + 1.4f * tb_rnd_unit(rng);
    for (int k = 0; ids[k] >= 0; k++) {
        int id = ids[k];
        if (TB_PARAMS[id].type == PT_ENUM) { p[id] = A[id]; continue; }
        float v = A[id] + t * (B[id] - A[id]);
        if (A[id] == 0.0f && B[id] == 0.0f) v = 0.0f;   /* off stays off unless a donor uses it */
        p[id] = tb_rnd_clamp(id, v);
    }
}

/*
 * Roll recipe `ri` into p[]. Level is the category's typical one; the plugin
 * measures the roll and sets it.
 */
static void tb_rnd_roll_recipe(float *p, int ri, uint32_t *rng)
{
    const tb_recipe_t *r = &TB_RECIPES[ri];
    tb_rr_t rr;
    tb_rnd_ranges(r, &rr);
    for (int i = 0; i < P_COUNT; i++) p[i] = TB_PARAMS[i].def;

    for (int g = 0; g < G_COUNT; g++) {
        tb_rnd_section(r, &rr, g, p, rng);
        tb_rnd_impose(r, &rr, p, rng);
    }

    /* Spread, for chords: half the time on a poly sound that is not a bass */
    p[P_SPREAD] = (r->cat != R_BASS && p[P_VOICE] < 0.5f && tb_rnd_unit(rng) < 0.5f)
                ? 0.15f + 0.45f * tb_rnd_unit(rng) : 0.0f;

    tb_rnd_nudge(p, 0.05f, rng);
    tb_rnd_impose(r, &rr, p, rng);

    /* the budgets, enforced */
    static const int MV[] = { P_LFO_DEPTH, P_PHS_DEPTH, P_PUMP, P_VHS, -1 };
    static const int SP[] = { P_DLY_MIX, P_REV_MIX, P_SHIMMER, -1 };
    static const int GR[] = { P_DRIVE, P_RESO, P_AGE, -1 };
    static const int WD[] = { P_STACK, P_CHO_MIX, P_SPREAD, -1 };
    tb_rnd_shrink(&rr, p, MV, tb_rnd_mv(p), r->mv);
    tb_rnd_shrink(&rr, p, SP, tb_rnd_sp(p), r->sp);
    tb_rnd_shrink(&rr, p, GR, tb_rnd_gr(p), r->gr);
    tb_rnd_shrink(&rr, p, WD, tb_rnd_wd(p), r->wd_hi);
    if (tb_rnd_wd(p) < r->wd_lo) {
        /* too narrow for what it is: the chorus widens it (Spread cannot
         * widen a chord low on the keyboard, where it closes up) */
        float need = r->wd_lo - tb_rnd_wd(p);
        p[P_CHO_MIX] = p[P_CHO_MIX] + need > 0.85f ? 0.85f : p[P_CHO_MIX] + need;
    }

    /* a pad stays out of the bass: an octave-down or sub layer gets more Tone */
    const char *phr = r->phrase ? r->phrase : TB_RND_PHRASE[r->cat];
    if (!strcmp(phr, "pad") && p[P_LAYER] <= L_OCT_DN && p[P_BLEND] > 0.15f && p[P_TONE] < 0.35f)
        p[P_TONE] = 0.35f + 0.1f * tb_rnd_unit(rng);

    tb_rnd_tidy(p);
    p[P_LEVEL] = tb_rnd_cat_level(r->cat);
    for (int i = 0; i < P_COUNT; i++) p[i] = tb_rnd_clamp(i, p[i]);
}

/* ------------------------------------------------------------------------ */
/* Varying                                                                   */
/* ------------------------------------------------------------------------ */

/* The category a sound is closest to: the nearest factory preset's */
static int tb_rnd_nearest_cat(const float *p)
{
    int best = R_KEYS;
    float bd = 1e9f;
    for (int b = 1; b < TB_PRESET_COUNT; b++) {
        int c = tb_rnd_preset_cat(b);
        if (c < 0) continue;
        const float *v = tb_rnd_bank(b);
        float d = 0.0f;
        for (int i = 0; i < P_COUNT; i++) {
            if (i == P_LEVEL) continue;
            float span = tb_param_max(i) - tb_param_min(i);
            d += TB_PARAMS[i].type == PT_ENUM ? (v[i] != p[i] ? 1.0f : 0.0f) : fabsf(v[i] - p[i]) / span;
        }
        if (d < bd) { bd = d; best = c; }
    }
    return best;
}

/*
 * Vary p[] (a sound of category `cat`) in place. What makes it the instrument
 * it is stays: model, voice mode, filter type, and whether the amp envelope
 * holds or dies. The category's base rules are applied.
 */
static void tb_rnd_vary(float *p, int cat, uint32_t *rng)
{
    float keep_model = p[P_MODEL], keep_voice = p[P_VOICE], keep_ft = p[P_FLT_TYPE];
    int held = p[P_AMP_S] >= 0.3f;
    float level = p[P_LEVEL];

    tb_rnd_nudge(p, 0.12f, rng);

    if (tb_rnd_unit(rng) < 0.35f) {
        static const int SWAP[3] = { G_MOD, G_MOTION, G_SPACE };
        int g = SWAP[tb_rnd_pick(rng, 3)];
        int idx[TB_PRESET_COUNT], n = 0;
        for (int b = 1; b < TB_PRESET_COUNT; b++) if (tb_rnd_preset_cat(b) == cat) idx[n++] = b;
        if (n) {
            const float *d = tb_rnd_bank(idx[tb_rnd_pick(rng, n)]);
            for (int k = 0; TB_RND_GROUP[g][k] >= 0; k++) p[TB_RND_GROUP[g][k]] = d[TB_RND_GROUP[g][k]];
        }
    }

    p[P_MODEL] = keep_model; p[P_VOICE] = keep_voice; p[P_FLT_TYPE] = keep_ft;
    if (held && p[P_AMP_S] < 0.3f) p[P_AMP_S] = 0.3f;
    if (!held && p[P_AMP_S] >= 0.3f) p[P_AMP_S] = 0.29f;
    for (int k = 0; k < 10 && TB_CAT_BASE[cat][k].id > 0; k++) {
        int id = TB_CAT_BASE[cat][k].id - 1;
        if (p[id] < TB_CAT_BASE[cat][k].lo) p[id] = TB_CAT_BASE[cat][k].lo;
        if (p[id] > TB_CAT_BASE[cat][k].hi) p[id] = TB_CAT_BASE[cat][k].hi;
    }
    tb_rnd_tidy(p);
    p[P_LEVEL] = level;
    for (int i = 0; i < P_COUNT; i++) p[i] = tb_rnd_clamp(i, p[i]);
}

#endif

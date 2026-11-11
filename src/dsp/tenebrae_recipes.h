/*
 * tenebrae_recipes.h -- what each kind of sound on the Random page IS.
 *
 * A recipe is one archetype of a category: a reese bass, an ensemble pad, a
 * sync lead. It says which oscillator models, layers, filter types, LFO
 * targets and voice modes belong to it, the range each defining control must
 * sit in, and four budgets that keep the sections from piling up on each
 * other:
 *
 *   movement  LFO depth (vibrato counts half), phaser, pump, half of VHS
 *   space     delay (more with feedback), reverb, half of shimmer
 *   grit      drive, half the resonance, half of Age
 *   width     stack, chorus, Spread (a floor and a ceiling)
 *
 * A control a recipe does not name is free: it comes from whichever preset
 * donated its section. Every category also has base rules, applied to all of
 * its recipes (a bass stays dry and centred; a pad stays out of the bass).
 *
 * Ranges are on the wire scale: 0..1, -1..1 for the bipolar controls.
 */
#ifndef TENEBRAE_RECIPES_H
#define TENEBRAE_RECIPES_H

#include "tenebrae_params.h"

enum { R_BASS, R_LEAD, R_PAD, R_KEYS, R_PLUCK, R_BRASS, R_VOICE, R_STAB, R_SEQ, R_FX, R_COUNT };

static const char *const TB_RND_NAMES[R_COUNT] = {
    "Bass", "Lead", "Pad", "Keys", "Pluck", "Brass", "Voice", "Stab", "Sequence", "FX"
};
/* the bank's name prefix for each category */
static const char *const TB_RND_PREFIX[R_COUNT] = {
    "BS ", "LD ", "PD ", "KY ", "PL ", "BR ", "VX ", "ST ", "SQ ", "FX "
};
/* the audition phrase a sound of each category is measured with, when its
 * recipe does not say */
static const char *const TB_RND_PHRASE[R_COUNT] = {
    "bass", "lead", "pad", "keys", "arp", "keys", "pad", "keys", "drone", "fx"
};

#define MB(x) (1u << (x))
#define ANY 0xFFFFu
/* voice modes */
#define VM_POLY  1u
#define VM_MONO  2u
#define VM_GLIDE 4u

typedef struct { int id; float lo, hi; } tb_rng_t;
#define RC(id, lo, hi) { (id) + 1, (lo), (hi) }   /* +1 so a zero entry ends the list */
#define TB_RC_MAX 18

typedef struct {
    const char *name;
    int         cat;
    const char *phrase;        /* 0 = the category's */
    unsigned    models, layers, ftypes, lfos, voices;
    float       mv, sp, gr;    /* budget ceilings */
    float       wd_lo, wd_hi;  /* width floor and ceiling */
    tb_rng_t    r[TB_RC_MAX];
} tb_recipe_t;

/* Base ranges for every recipe of a category (a recipe's own range for the
 * same control wins). */
static const tb_rng_t TB_CAT_BASE[R_COUNT][10] = {
    [R_BASS]  = { RC(P_REV_MIX, 0, 0.15f), RC(P_SHIMMER, 0, 0), RC(P_TONE, -0.4f, 0.1f),
                  RC(P_DLY_MIX, 0, 0.12f), RC(P_AMP_R, 0.08f, 0.35f) },
    [R_LEAD]  = { RC(P_AMP_A, 0, 0.12f), RC(P_AMP_S, 0.5f, 1), RC(P_REV_MIX, 0, 0.45f) },
    [R_PAD]   = { RC(P_AMP_A, 0.3f, 0.85f), RC(P_AMP_S, 0.6f, 1), RC(P_AMP_R, 0.45f, 0.85f),
                  RC(P_TONE, 0.2f, 0.6f), RC(P_REV_MIX, 0.2f, 0.7f) },
    [R_KEYS]  = { RC(P_AMP_A, 0, 0.04f), RC(P_AMP_R, 0.2f, 0.55f), RC(P_REV_MIX, 0.05f, 0.45f) },
    [R_PLUCK] = { RC(P_AMP_A, 0, 0.01f), RC(P_AMP_S, 0, 0.12f), RC(P_AMP_D, 0.28f, 0.55f),
                  RC(P_AMP_R, 0.2f, 0.5f), RC(P_TONE, 0, 0.5f) },
    [R_BRASS] = { RC(P_AMP_A, 0.02f, 0.15f), RC(P_AMP_S, 0.55f, 0.95f), RC(P_AMP_R, 0.2f, 0.5f),
                  RC(P_FLT_ENV, 0.3f, 0.7f), RC(P_FLT_A, 0.04f, 0.3f), RC(P_FLT_S, 0.25f, 0.65f) },
    [R_VOICE] = { RC(P_REV_MIX, 0.15f, 0.65f) },
    [R_STAB]  = { RC(P_AMP_A, 0, 0.01f), RC(P_AMP_S, 0, 0.3f), RC(P_AMP_D, 0.32f, 0.55f),
                  RC(P_AMP_R, 0.2f, 0.45f) },
    [R_SEQ]   = { RC(P_AMP_S, 0.5f, 1) },
    [R_FX]    = { RC(P_AMP_S, 0, 1) },
};

static const tb_recipe_t TB_RECIPES[] = {
    /* ---- Bass --------------------------------------------------------- */
    { "Sub", R_BASS, 0, MB(M_SAW) | MB(M_PULSE) | MB(M_WAVE), MB(L_SUB_SINE) | MB(L_SUB_SQR),
      MB(F_LP24) | MB(F_LP12), MB(D_FILTER) | MB(D_SHAPE), VM_MONO | VM_GLIDE,
      0.35f, 0.15f, 0.6f, 0.0f, 0.45f,
      { RC(P_BLEND, 0.35f, 0.6f), RC(P_STACK, 0, 0.1f), RC(P_CUTOFF, 0.3f, 0.52f), RC(P_RESO, 0, 0.25f),
        RC(P_DRIVE, 0.05f, 0.4f), RC(P_FLT_ENV, 0.2f, 0.55f), RC(P_FLT_D, 0.2f, 0.45f), RC(P_FLT_S, 0, 0.3f),
        RC(P_AMP_A, 0, 0.02f), RC(P_AMP_S, 0.45f, 0.9f) } },
    { "Reese", R_BASS, 0, MB(M_SAW) | MB(M_PULSE), MB(L_SUB_SINE) | MB(L_SUB_SQR) | MB(L_OCT_DN),
      MB(F_LP24) | MB(F_LP12), MB(D_FILTER) | MB(D_SHAPE) | MB(D_PITCH), VM_MONO | VM_GLIDE,
      0.45f, 0.15f, 0.9f, 0.35f, 0.9f,
      { RC(P_STACK, 0.35f, 0.7f), RC(P_BLEND, 0.2f, 0.45f), RC(P_CUTOFF, 0.4f, 0.65f), RC(P_RESO, 0.05f, 0.3f),
        RC(P_DRIVE, 0.3f, 0.65f), RC(P_FLT_ENV, 0.1f, 0.4f), RC(P_AMP_A, 0, 0.03f), RC(P_AMP_S, 0.6f, 1) } },
    { "Acid", R_BASS, 0, MB(M_SAW) | MB(M_PULSE), MB(L_SUB_SINE) | MB(L_SUB_SQR),
      MB(F_LP24), MB(D_FILTER) | MB(D_PITCH), VM_MONO | VM_GLIDE,
      0.3f, 0.15f, 1.1f, 0.0f, 0.3f,
      { RC(P_STACK, 0, 0.05f), RC(P_BLEND, 0, 0.25f), RC(P_CUTOFF, 0.25f, 0.45f), RC(P_RESO, 0.55f, 0.85f),
        RC(P_FLT_ENV, 0.5f, 0.85f), RC(P_FLT_A, 0, 0.02f), RC(P_FLT_D, 0.18f, 0.4f), RC(P_FLT_S, 0, 0.15f),
        RC(P_DRIVE, 0.3f, 0.7f), RC(P_AMP_A, 0, 0.01f), RC(P_AMP_S, 0.4f, 0.85f) } },
    { "FM", R_BASS, 0, MB(M_FM), MB(L_SUB_SINE) | MB(L_SUB_SQR),
      MB(F_LP12) | MB(F_LP24), MB(D_SHAPE) | MB(D_FILTER), VM_MONO | VM_GLIDE | VM_POLY,
      0.3f, 0.15f, 0.6f, 0.0f, 0.45f,
      { RC(P_SHAPE, 0.28f, 0.55f), RC(P_SHAPE_ENV, 0.3f, 0.7f), RC(P_BLEND, 0.2f, 0.45f), RC(P_STACK, 0, 0.08f),
        RC(P_CUTOFF, 0.6f, 0.85f), RC(P_RESO, 0, 0.2f), RC(P_DRIVE, 0.1f, 0.4f), RC(P_FLT_ENV, 0, 0.25f),
        RC(P_AMP_A, 0, 0.01f), RC(P_AMP_D, 0.3f, 0.6f), RC(P_AMP_S, 0.2f, 0.6f) } },
    { "Distorted", R_BASS, 0, MB(M_SAW) | MB(M_PULSE) | MB(M_SYNC), MB(L_SUB_SINE),
      MB(F_LP24) | MB(F_LP12), MB(D_SHAPE) | MB(D_FILTER) | MB(D_RAMP), VM_MONO | VM_GLIDE,
      0.6f, 0.15f, 1.6f, 0.1f, 0.7f,
      { RC(P_DRIVE, 0.7f, 1), RC(P_BLEND, 0.3f, 0.5f), RC(P_STACK, 0.1f, 0.4f), RC(P_CUTOFF, 0.45f, 0.7f),
        RC(P_RESO, 0.1f, 0.35f), RC(P_FLT_ENV, 0.15f, 0.5f), RC(P_AMP_A, 0, 0.02f), RC(P_AMP_S, 0.5f, 1) } },

    /* ---- Lead --------------------------------------------------------- */
    { "Sync Scream", R_LEAD, 0, MB(M_SYNC), ANY, MB(F_LP24) | MB(F_LP12), MB(D_PITCH) | MB(D_SHAPE),
      VM_MONO | VM_GLIDE, 0.6f, 0.7f, 1.3f, 0.0f, 0.8f,
      { RC(P_SHAPE, 0.3f, 0.8f), RC(P_SHAPE_ENV, 0.15f, 0.6f), RC(P_DRIVE, 0.4f, 0.85f),
        RC(P_CUTOFF, 0.6f, 0.88f), RC(P_RESO, 0, 0.3f), RC(P_BLEND, 0, 0.35f) } },
    { "Square Glide", R_LEAD, 0, MB(M_PULSE), MB(L_SUB_SQR) | MB(L_OCT_DN) | MB(L_SUB_SINE) | MB(L_OCT_UP),
      MB(F_LP24) | MB(F_LP12), MB(D_PITCH), VM_GLIDE, 0.4f, 0.8f, 0.9f, 0.0f, 0.6f,
      { RC(P_SHAPE, 0.35f, 0.62f), RC(P_STACK, 0, 0.2f), RC(P_BLEND, 0, 0.35f), RC(P_CUTOFF, 0.5f, 0.78f),
        RC(P_RESO, 0, 0.35f), RC(P_LFO_DEPTH, 0.05f, 0.3f), RC(P_LFO_DELAY, 0.2f, 0.6f),
        RC(P_DLY_MIX, 0.12f, 0.38f) } },
    { "Supersaw", R_LEAD, 0, MB(M_SAW), ANY, MB(F_LP24) | MB(F_LP12), MB(D_PITCH) | MB(D_FILTER),
      VM_MONO | VM_GLIDE | VM_POLY, 0.5f, 0.9f, 0.9f, 0.45f, 1.4f,
      { RC(P_STACK, 0.4f, 0.75f), RC(P_CUTOFF, 0.6f, 0.88f), RC(P_RESO, 0, 0.25f), RC(P_DRIVE, 0.1f, 0.5f),
        RC(P_CHO_MIX, 0.15f, 0.5f) } },
    { "FM Lead", R_LEAD, 0, MB(M_FM) | MB(M_BELL), ANY, MB(F_LP12) | MB(F_LP24) | MB(F_BP),
      MB(D_PITCH) | MB(D_SHAPE), VM_MONO | VM_GLIDE, 0.6f, 0.8f, 0.9f, 0.0f, 0.8f,
      { RC(P_SHAPE, 0.3f, 0.62f), RC(P_SHAPE_ENV, -0.3f, 0.5f), RC(P_CUTOFF, 0.62f, 0.92f),
        RC(P_RESO, 0, 0.3f), RC(P_DRIVE, 0.05f, 0.5f) } },
    { "Wavetable", R_LEAD, 0, MB(M_WAVE), ANY, MB(F_LP24) | MB(F_LP12) | MB(F_BP), MB(D_SHAPE) | MB(D_PITCH),
      VM_MONO | VM_GLIDE, 0.7f, 0.8f, 1.0f, 0.0f, 0.9f,
      { RC(P_SHAPE, 0.15f, 0.9f), RC(P_SHAPE_ENV, -0.4f, 0.5f), RC(P_CUTOFF, 0.55f, 0.88f),
        RC(P_RESO, 0, 0.4f), RC(P_DRIVE, 0.1f, 0.6f) } },

    /* ---- Pad ---------------------------------------------------------- */
    { "Supersaw", R_PAD, 0, MB(M_SAW), ANY & ~MB(L_SUB_SINE) & ~MB(L_SUB_SQR) & ~MB(L_SUB_2),
      MB(F_LP24) | MB(F_LP12), ANY, VM_POLY, 0.6f, 1.1f, 0.6f, 0.8f, 1.8f,
      { RC(P_STACK, 0.45f, 0.85f), RC(P_CUTOFF, 0.48f, 0.75f), RC(P_RESO, 0, 0.25f), RC(P_DRIVE, 0, 0.3f),
        RC(P_CHO_MIX, 0.2f, 0.6f) } },
    { "Ensemble", R_PAD, 0, MB(M_SAW) | MB(M_PULSE), ANY & ~MB(L_SUB_SINE) & ~MB(L_SUB_SQR) & ~MB(L_SUB_2),
      MB(F_LP24) | MB(F_LP12), MB(D_PITCH) | MB(D_FILTER) | MB(D_SHAPE), VM_POLY, 0.5f, 1.0f, 0.5f, 0.7f, 1.4f,
      { RC(P_STACK, 0.12f, 0.38f), RC(P_CHO_MODE, C_ENS, C_ENS), RC(P_CHO_MIX, 0.45f, 0.85f),
        RC(P_CUTOFF, 0.42f, 0.7f), RC(P_RESO, 0, 0.2f), RC(P_DRIVE, 0, 0.25f) } },
    { "Glass", R_PAD, 0, MB(M_WAVE) | MB(M_BELL), ANY & ~MB(L_SUB_SINE) & ~MB(L_SUB_SQR) & ~MB(L_SUB_2),
      MB(F_LP12) | MB(F_HP) | MB(F_BP), MB(D_SHAPE) | MB(D_FILTER) | MB(D_PAN), VM_POLY,
      0.8f, 1.3f, 0.5f, 0.8f, 1.6f,
      { RC(P_SHAPE, 0.15f, 0.9f), RC(P_CUTOFF, 0.5f, 0.85f), RC(P_RESO, 0, 0.35f), RC(P_DRIVE, 0, 0.2f),
        RC(P_REV_MIX, 0.35f, 0.7f), RC(P_SHIMMER, 0, 0.45f), RC(P_CHO_MIX, 0.5f, 0.85f), RC(P_STACK, 0.1f, 0.5f) } },
    { "Choir", R_PAD, 0, MB(M_VOX), ANY & ~MB(L_SUB_SINE) & ~MB(L_SUB_SQR) & ~MB(L_SUB_2),
      MB(F_LP24) | MB(F_LP12) | MB(F_BP), MB(D_SHAPE) | MB(D_PITCH) | MB(D_FILTER), VM_POLY,
      0.6f, 1.2f, 0.5f, 0.95f, 1.7f,
      { RC(P_SHAPE, 0.15f, 0.85f), RC(P_STACK, 0.25f, 0.6f), RC(P_CUTOFF, 0.5f, 0.82f), RC(P_RESO, 0, 0.3f),
        RC(P_DRIVE, 0, 0.2f), RC(P_CHO_MIX, 0.5f, 0.85f) } },
    { "Dark Drift", R_PAD, 0, MB(M_SAW) | MB(M_PULSE) | MB(M_WAVE), ANY & ~MB(L_SUB_SINE) & ~MB(L_SUB_SQR) & ~MB(L_SUB_2) & ~MB(L_OCT_DN),
      MB(F_LP24) | MB(F_LP12), MB(D_FILTER) | MB(D_SHAPE) | MB(D_PITCH), VM_POLY, 0.9f, 1.2f, 1.0f, 0.6f, 1.5f,
      { RC(P_CUTOFF, 0.28f, 0.5f), RC(P_AGE, 0.35f, 0.7f), RC(P_VHS, 0.1f, 0.45f), RC(P_STACK, 0.2f, 0.6f),
        RC(P_RESO, 0, 0.3f), RC(P_CHO_MIX, 0.3f, 0.7f), RC(P_TONE, 0.3f, 0.5f) } },

    /* ---- Keys --------------------------------------------------------- */
    { "Electric", R_KEYS, 0, MB(M_FM), MB(L_OCT_UP) | MB(L_FIFTH) | MB(L_SUB_SINE) | MB(L_OCT_DN),
      MB(F_LP12) | MB(F_LP24), MB(D_AMP) | MB(D_PAN) | MB(D_PITCH), VM_POLY, 0.6f, 0.7f, 0.5f, 0.2f, 1.0f,
      { RC(P_SHAPE, 0.22f, 0.45f), RC(P_SHAPE_ENV, 0.2f, 0.55f), RC(P_CUTOFF, 0.68f, 0.9f), RC(P_RESO, 0, 0.15f),
        RC(P_DRIVE, 0, 0.35f), RC(P_AMP_D, 0.45f, 0.72f), RC(P_AMP_S, 0.15f, 0.5f), RC(P_STACK, 0, 0.15f) } },
    { "Organ", R_KEYS, 0, MB(M_PULSE) | MB(M_WAVE), MB(L_OCT_UP) | MB(L_FIFTH) | MB(L_OCT_2) | MB(L_OCT_DN),
      MB(F_LP12) | MB(F_LP24), MB(D_AMP) | MB(D_PITCH) | MB(D_PAN), VM_POLY, 0.6f, 0.6f, 0.6f, 0.3f, 1.1f,
      { RC(P_BLEND, 0.25f, 0.55f), RC(P_CUTOFF, 0.6f, 0.86f), RC(P_RESO, 0, 0.15f), RC(P_FLT_ENV, -0.1f, 0.2f),
        RC(P_AMP_S, 0.8f, 1), RC(P_AMP_R, 0.1f, 0.3f), RC(P_STACK, 0, 0.2f), RC(P_CHO_MIX, 0.2f, 0.6f) } },
    { "Poly Synth", R_KEYS, 0, MB(M_SAW) | MB(M_PULSE), ANY, MB(F_LP24) | MB(F_LP12),
      MB(D_PITCH) | MB(D_FILTER) | MB(D_SHAPE), VM_POLY, 0.5f, 0.7f, 0.7f, 0.3f, 1.2f,
      { RC(P_STACK, 0.1f, 0.38f), RC(P_CUTOFF, 0.38f, 0.65f), RC(P_RESO, 0, 0.35f), RC(P_FLT_ENV, 0.3f, 0.6f),
        RC(P_FLT_D, 0.3f, 0.55f), RC(P_AMP_D, 0.45f, 0.68f), RC(P_AMP_S, 0.3f, 0.7f) } },
    { "Bell", R_KEYS, "bell", MB(M_BELL) | MB(M_FM), MB(L_OCT_UP) | MB(L_FIFTH) | MB(L_SUB_SINE) | MB(L_OCT_2),
      MB(F_LP12) | MB(F_LP24) | MB(F_HP), MB(D_AMP) | MB(D_PITCH) | MB(D_PAN), VM_POLY, 0.5f, 1.0f, 0.4f, 0.1f, 1.0f,
      { RC(P_SHAPE, 0.2f, 0.7f), RC(P_CUTOFF, 0.65f, 0.92f), RC(P_RESO, 0, 0.2f), RC(P_DRIVE, 0, 0.25f),
        RC(P_AMP_D, 0.5f, 0.78f), RC(P_AMP_S, 0, 0.2f), RC(P_AMP_R, 0.35f, 0.6f) } },

    /* ---- Pluck -------------------------------------------------------- */
    { "Saw Pluck", R_PLUCK, 0, MB(M_SAW) | MB(M_PULSE), ANY, MB(F_LP24) | MB(F_LP12), ANY, VM_POLY,
      0.5f, 0.9f, 0.7f, 0.1f, 1.0f,
      { RC(P_CUTOFF, 0.28f, 0.55f), RC(P_RESO, 0.05f, 0.45f), RC(P_FLT_ENV, 0.4f, 0.8f), RC(P_FLT_A, 0, 0.02f),
        RC(P_FLT_D, 0.2f, 0.42f), RC(P_FLT_S, 0, 0.15f), RC(P_DLY_MIX, 0.15f, 0.42f), RC(P_STACK, 0, 0.4f) } },
    { "Bell Pluck", R_PLUCK, "bell", MB(M_BELL), ANY & ~MB(L_SUB_2) & ~MB(L_NOISE), MB(F_LP12) | MB(F_LP24) | MB(F_HP),
      ANY, VM_POLY, 0.5f, 1.0f, 0.4f, 0.1f, 1.0f,
      { RC(P_SHAPE, 0.2f, 0.75f), RC(P_CUTOFF, 0.62f, 0.92f), RC(P_RESO, 0, 0.2f), RC(P_DRIVE, 0, 0.3f),
        RC(P_DLY_MIX, 0.12f, 0.4f) } },
    { "FM Pluck", R_PLUCK, 0, MB(M_FM), ANY & ~MB(L_NOISE), MB(F_LP12) | MB(F_LP24), ANY, VM_POLY,
      0.5f, 0.9f, 0.6f, 0.1f, 1.0f,
      { RC(P_SHAPE, 0.25f, 0.6f), RC(P_SHAPE_ENV, 0.3f, 0.75f), RC(P_CUTOFF, 0.55f, 0.85f), RC(P_RESO, 0, 0.25f),
        RC(P_DLY_MIX, 0.12f, 0.4f) } },
    { "Wave Pluck", R_PLUCK, 0, MB(M_WAVE), ANY & ~MB(L_NOISE), MB(F_LP12) | MB(F_LP24) | MB(F_BP), ANY, VM_POLY,
      0.6f, 0.9f, 0.6f, 0.1f, 1.1f,
      { RC(P_SHAPE, 0.15f, 0.9f), RC(P_SHAPE_ENV, -0.5f, 0.6f), RC(P_CUTOFF, 0.4f, 0.75f), RC(P_RESO, 0.05f, 0.45f),
        RC(P_FLT_ENV, 0.25f, 0.7f), RC(P_FLT_S, 0, 0.2f), RC(P_DLY_MIX, 0.15f, 0.42f) } },

    /* ---- Brass -------------------------------------------------------- */
    { "Synth Brass", R_BRASS, 0, MB(M_SAW), ANY & ~MB(L_NOISE), MB(F_LP24) | MB(F_LP12), MB(D_PITCH) | MB(D_FILTER),
      VM_POLY, 0.4f, 0.6f, 0.8f, 0.3f, 1.1f,
      { RC(P_STACK, 0.15f, 0.42f), RC(P_CUTOFF, 0.35f, 0.6f), RC(P_RESO, 0, 0.25f), RC(P_DRIVE, 0.1f, 0.45f) } },
    { "Pulse Brass", R_BRASS, 0, MB(M_PULSE), ANY & ~MB(L_NOISE), MB(F_LP24) | MB(F_LP12), MB(D_PITCH) | MB(D_SHAPE),
      VM_POLY, 0.4f, 0.6f, 0.8f, 0.2f, 1.0f,
      { RC(P_SHAPE, 0.3f, 0.6f), RC(P_STACK, 0.05f, 0.35f), RC(P_CUTOFF, 0.35f, 0.6f), RC(P_RESO, 0, 0.25f),
        RC(P_DRIVE, 0.1f, 0.45f) } },
    { "Fanfare", R_BRASS, "lead", MB(M_SAW) | MB(M_SYNC), MB(L_FIFTH) | MB(L_OCT_UP) | MB(L_OCT_DN),
      MB(F_LP24) | MB(F_LP12), MB(D_PITCH), VM_POLY | VM_GLIDE, 0.4f, 0.7f, 1.0f, 0.2f, 1.1f,
      { RC(P_BLEND, 0.25f, 0.5f), RC(P_STACK, 0.1f, 0.4f), RC(P_CUTOFF, 0.42f, 0.68f), RC(P_RESO, 0, 0.25f),
        RC(P_DRIVE, 0.2f, 0.6f) } },

    /* ---- Voice -------------------------------------------------------- */
    { "Choir", R_VOICE, "pad", MB(M_VOX), ANY & ~MB(L_SUB_SINE) & ~MB(L_SUB_SQR) & ~MB(L_SUB_2) & ~MB(L_NOISE),
      MB(F_LP24) | MB(F_LP12) | MB(F_BP), MB(D_SHAPE) | MB(D_PITCH), VM_POLY, 0.6f, 1.2f, 0.4f, 0.95f, 1.7f,
      { RC(P_SHAPE, 0.15f, 0.85f), RC(P_STACK, 0.25f, 0.6f), RC(P_CUTOFF, 0.5f, 0.82f), RC(P_RESO, 0, 0.25f),
        RC(P_DRIVE, 0, 0.2f), RC(P_AMP_A, 0.25f, 0.7f), RC(P_AMP_S, 0.7f, 1), RC(P_AMP_R, 0.45f, 0.8f),
        RC(P_TONE, 0.15f, 0.5f), RC(P_CHO_MIX, 0.5f, 0.85f) } },
    { "Vox Lead", R_VOICE, "lead", MB(M_VOX), ANY & ~MB(L_NOISE), MB(F_LP24) | MB(F_LP12) | MB(F_BP),
      MB(D_PITCH) | MB(D_SHAPE), VM_MONO | VM_GLIDE, 0.6f, 0.9f, 0.9f, 0.0f, 0.8f,
      { RC(P_SHAPE, 0.15f, 0.85f), RC(P_CUTOFF, 0.55f, 0.88f), RC(P_RESO, 0, 0.3f), RC(P_DRIVE, 0.05f, 0.5f),
        RC(P_AMP_A, 0, 0.15f), RC(P_AMP_S, 0.6f, 1), RC(P_LFO_DEPTH, 0.05f, 0.35f), RC(P_LFO_DELAY, 0.1f, 0.6f) } },
    { "Vox Keys", R_VOICE, "keys", MB(M_VOX), ANY & ~MB(L_NOISE), MB(F_LP24) | MB(F_LP12) | MB(F_BP),
      MB(D_SHAPE) | MB(D_PAN) | MB(D_AMP), VM_POLY, 0.6f, 0.8f, 0.6f, 0.2f, 1.1f,
      { RC(P_SHAPE, 0.15f, 0.85f), RC(P_CUTOFF, 0.5f, 0.85f), RC(P_RESO, 0, 0.3f), RC(P_AMP_A, 0, 0.04f),
        RC(P_AMP_D, 0.4f, 0.7f), RC(P_AMP_S, 0.25f, 0.65f), RC(P_AMP_R, 0.25f, 0.55f) } },

    /* ---- Stab --------------------------------------------------------- */
    { "Saw Stab", R_STAB, 0, MB(M_SAW) | MB(M_PULSE), ANY & ~MB(L_NOISE), MB(F_LP24) | MB(F_LP12), ANY, VM_POLY,
      0.5f, 0.9f, 0.9f, 0.3f, 1.3f,
      { RC(P_STACK, 0.3f, 0.7f), RC(P_CUTOFF, 0.45f, 0.72f), RC(P_RESO, 0, 0.3f), RC(P_FLT_ENV, 0.3f, 0.65f),
        RC(P_FLT_D, 0.2f, 0.42f), RC(P_DRIVE, 0.1f, 0.55f) } },
    { "Hit", R_STAB, 0, MB(M_FM) | MB(M_ZAP) | MB(M_BELL), ANY, MB(F_LP12) | MB(F_LP24) | MB(F_BP) | MB(F_HP),
      ANY, VM_POLY, 0.5f, 1.0f, 1.0f, 0.1f, 1.1f,
      { RC(P_SHAPE, 0.2f, 0.7f), RC(P_CUTOFF, 0.5f, 0.85f), RC(P_RESO, 0, 0.35f), RC(P_DRIVE, 0.2f, 0.7f),
        RC(P_REV_MIX, 0.2f, 0.6f) } },
    { "Pumped", R_STAB, 0, MB(M_SAW) | MB(M_PULSE) | MB(M_WAVE), ANY & ~MB(L_NOISE), MB(F_LP24) | MB(F_LP12),
      ANY, VM_POLY, 1.0f, 0.9f, 0.8f, 0.3f, 1.3f,
      { RC(P_STACK, 0.2f, 0.6f), RC(P_CUTOFF, 0.45f, 0.72f), RC(P_FLT_ENV, 0.25f, 0.6f), RC(P_PUMP, 0.4f, 0.8f),
        RC(P_AMP_S, 0.3f, 0.6f), RC(P_AMP_D, 0.4f, 0.6f) } },

    /* ---- Sequence: a held note that plays itself ---------------------- */
    { "Sample & Hold", R_SEQ, 0, MB(M_SAW) | MB(M_PULSE) | MB(M_WAVE) | MB(M_SYNC), ANY & ~MB(L_NOISE),
      MB(F_LP24) | MB(F_LP12) | MB(F_BP), MB(D_SH), ANY, 1.2f, 1.0f, 1.0f, 0.0f, 1.2f,
      { RC(P_LFO_DEPTH, 0.5f, 0.9f), RC(P_LFO_RATE, 0.45f, 0.8f), RC(P_CUTOFF, 0.3f, 0.6f), RC(P_RESO, 0.3f, 0.65f),
        RC(P_LFO_DELAY, 0, 0) } },
    { "Ramp", R_SEQ, 0, MB(M_SAW) | MB(M_PULSE) | MB(M_SYNC) | MB(M_FM), ANY & ~MB(L_NOISE),
      MB(F_LP24) | MB(F_LP12) | MB(F_BP), MB(D_RAMP), ANY, 1.2f, 1.0f, 1.0f, 0.0f, 1.2f,
      { RC(P_LFO_DEPTH, 0.5f, 0.9f), RC(P_CUTOFF, 0.25f, 0.55f), RC(P_RESO, 0.25f, 0.6f), RC(P_LFO_DELAY, 0, 0) } },
    { "Gated", R_SEQ, 0, MB(M_SAW) | MB(M_PULSE) | MB(M_WAVE), ANY & ~MB(L_NOISE), MB(F_LP24) | MB(F_LP12),
      ANY, ANY, 1.3f, 1.1f, 0.9f, 0.0f, 1.3f,
      { RC(P_PUMP, 0.6f, 1), RC(P_PUMP_RATE, 2, 3), RC(P_DLY_MIX, 0.15f, 0.45f), RC(P_CUTOFF, 0.4f, 0.72f) } },
    { "Echo Line", R_SEQ, 0, MB(M_PULSE) | MB(M_SAW) | MB(M_FM) | MB(M_BELL), ANY & ~MB(L_NOISE),
      MB(F_LP24) | MB(F_LP12), MB(D_AMP) | MB(D_FILTER), ANY, 1.0f, 1.4f, 0.8f, 0.0f, 1.2f,
      { RC(P_LFO_DEPTH, 0.45f, 0.85f), RC(P_LFO_RATE, 0.5f, 0.8f), RC(P_DLY_MIX, 0.35f, 0.6f),
        RC(P_DLY_FB, 0.4f, 0.7f), RC(P_LFO_DELAY, 0, 0) } },

    /* ---- FX ----------------------------------------------------------- */
    { "Laser", R_FX, "tom", MB(M_ZAP), ANY, ANY, ANY, VM_POLY | VM_MONO, 0.6f, 1.0f, 1.2f, 0.0f, 1.2f,
      { RC(P_SHAPE, 0.3f, 1), RC(P_AMP_A, 0, 0.01f), RC(P_AMP_S, 0, 0.3f), RC(P_AMP_D, 0.3f, 0.6f),
        RC(P_CUTOFF, 0.5f, 0.92f) } },
    { "Noise Sweep", R_FX, "fx", ANY, MB(L_NOISE), MB(F_LP24) | MB(F_LP12) | MB(F_BP) | MB(F_HP),
      MB(D_FILTER) | MB(D_PAN) | MB(D_SH), ANY, 1.2f, 1.4f, 1.0f, 0.0f, 1.4f,
      { RC(P_BLEND, 0.55f, 1), RC(P_RESO, 0.25f, 0.7f), RC(P_LFO_DEPTH, 0.35f, 0.85f), RC(P_LFO_RATE, 0.1f, 0.45f),
        RC(P_AMP_A, 0.1f, 0.6f), RC(P_AMP_S, 0.6f, 1) } },
    { "Haunted Drone", R_FX, "drone", MB(M_SAW) | MB(M_WAVE) | MB(M_VOX) | MB(M_SYNC), ANY & ~MB(L_NOISE),
      ANY, MB(D_PITCH) | MB(D_FILTER) | MB(D_SHAPE), ANY, 1.3f, 1.5f, 1.4f, 0.2f, 1.5f,
      { RC(P_AGE, 0.45f, 0.9f), RC(P_VHS, 0.2f, 0.7f), RC(P_AMP_A, 0.2f, 0.7f), RC(P_AMP_S, 0.7f, 1),
        RC(P_REV_MIX, 0.35f, 0.75f), RC(P_SHIMMER, 0, 0.6f) } },
    { "Metal", R_FX, "tom", MB(M_BELL) | MB(M_FM) | MB(M_SYNC), ANY, ANY, ANY, VM_POLY, 0.7f, 1.2f, 1.4f, 0.0f, 1.3f,
      { RC(P_SHAPE, 0.4f, 1), RC(P_SHAPE_ENV, -0.8f, 0.8f), RC(P_AMP_A, 0, 0.02f), RC(P_AMP_S, 0, 0.25f),
        RC(P_AMP_D, 0.35f, 0.7f), RC(P_DRIVE, 0.3f, 0.8f) } },
};
#define TB_RECIPE_COUNT ((int)(sizeof TB_RECIPES / sizeof TB_RECIPES[0]))

#endif

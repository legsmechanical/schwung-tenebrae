/*
 * tenebrae_presets.h -- the factory bank: 163 presets, grouped by category.
 *
 *   26 bass, 26 lead, 30 pad, 18 keys, 24 pluck, 11 brass, 9 voices,
 *   6 stabs, 5 sequences, 8 FX.
 *
 * Inside each category, in this order:
 *   - the staples: the sounds these genres are built from, polished;
 *   - the ones where an effect IS the sound: a pump that makes the rhythm, a
 *     delay that plays the second half of the line, a tail an octave up, a
 *     worn tape;
 *   - the bigger and wilder ones, still melodic rather than sound effects.
 *     These go where the first two groups did not: the Bell, Vox and Zap
 *     models as instruments, high-pass and band-pass, envelopes
 *     that run backwards, the top of the Drive knob, and three new
 *     categories -- VX voices, ST stabs and hits, SQ sequences;
 *   - horror: retro and modern horror-soundtrack instruments, with only two
 *     sound effects among them.
 *
 * THE ORDER IS FREE; THE `uid` IS NOT. A saved state names its preset by uid,
 * so presets can be moved or inserted anywhere, but a uid is never changed
 * and never reused. A new preset takes the next unused number (164, 165, ...).
 * 0-31 are the staples, 32-63 the effects presets, 64-127 the wilder ones,
 * 128-159 the horror ones, in the order they were first written; 160 on are
 * single additions. 22 (KY Iron Bell) was dropped and is retired: a state
 * saved with it keeps its sound and shows no preset name.
 *
 * Each preset lists only what it changes from the defaults in tenebrae_params.h.
 *
 * `cat` is the phrase tools/tenebrae_tool.c plays to audition the preset; it is
 * not shown on the device.
 *
 * Rules these were written to (each is a way a generated bank can go wrong):
 *   - a modulation DEPTH is never set without its target, and never the reverse
 *   - no two presets share an effects setup: the space is part of the sound
 *   - basses keep their low end dry and centred; pads and plucks are cut low
 *     (Tone above centre) so they stay out of the bass
 *   - LEVEL is set by measurement (tools/level_bank.py), not by ear-free guess
 */
#ifndef TENEBRAE_PRESETS_H
#define TENEBRAE_PRESETS_H

#include "tenebrae_params.h"

typedef struct { int id; float v; } tb_pv_t;
typedef struct { const char *name; const char *cat; int uid; tb_pv_t v[P_COUNT]; } tb_preset_t;

/* ids are stored +1 so that the zero-filled rest of the array ends the list */
#define S(id, val) { (id) + 1, (val) }

/* Voice / Glide options */
#define V_POLY 0
#define V_MONO 1
#define V_G30  4
#define V_G45  5
#define V_G60  6
#define V_G80  7
#define V_G110 8
#define V_G150 9
#define V_G200 10
#define V_G360 12
#define V_G650 14
/* Delay times */
#define T_16  0
#define T_8T  1
#define T_8   2
#define T_8D  3
#define T_4   4
#define T_4D  5
#define T_2   6
/* Pump divisions */
#define PU_2  0
#define PU_4  1
#define PU_8  2
#define PU_16 3

static const tb_preset_t TB_PRESETS[] = {

/* ======================================================================= */
/* BASS                                                                    */
/* ======================================================================= */

/* The rolling sixteenth-note bass that drives an outrun track: one saw and a
 * square sub, a snappy filter envelope, ducking a little on the beat. */
{ "BS Night Drive", "bass", 0, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_STACK, 0.05f), S(P_LAYER, L_SUB_SQR), S(P_BLEND, 0.28f),
    S(P_AGE, 0.15f), S(P_VOICE, V_MONO),
    S(P_FLT_TYPE, F_LP24), S(P_CUTOFF, 0.52f), S(P_RESO, 0.22f), S(P_DRIVE, 0.38f),
    S(P_FLT_ENV, 0.52f), S(P_FLT_A, 0.0f), S(P_FLT_D, 0.36f), S(P_FLT_S, 0.12f),
    S(P_VEL, 0.60f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.55f), S(P_AMP_S, 0.45f), S(P_AMP_R, 0.22f),
    S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.20f), S(P_PUMP, 0.30f), S(P_PUMP_RATE, PU_4),
    S(P_REV_MIX, 0.06f), S(P_REV_SIZE, 0.30f), S(P_LEVEL, 0.76f) } },

/* Dreamwave plucked bass: saw and sub through a closed filter with the chorus
 * on, played as a melody rather than a pedal. */
{ "BS Coastline", "bass", 1, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_LAYER, L_SUB_SQR), S(P_BLEND, 0.42f), S(P_AGE, 0.25f),
    S(P_CUTOFF, 0.48f), S(P_RESO, 0.12f), S(P_DRIVE, 0.22f),
    S(P_FLT_ENV, 0.55f), S(P_FLT_D, 0.42f), S(P_FLT_S, 0.25f),
    S(P_VEL, 0.55f), S(P_AMP_A, 0.01f), S(P_AMP_D, 0.50f), S(P_AMP_S, 0.35f), S(P_AMP_R, 0.40f),
    S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.50f),
    S(P_REV_MIX, 0.12f), S(P_REV_SIZE, 0.45f), S(P_LEVEL, 0.69f) } },

/* Two saws an octave apart with the mixer pushed into the filter: the fat,
 * sustained bass of a slow chorus. */
{ "BS Tailfin", "bass", 2, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_STACK, 0.04f), S(P_LAYER, L_OCT_DN), S(P_BLEND, 0.42f),
    S(P_AGE, 0.20f), S(P_VOICE, V_G30),
    S(P_CUTOFF, 0.58f), S(P_RESO, 0.20f), S(P_DRIVE, 0.50f),
    S(P_FLT_ENV, 0.45f), S(P_FLT_D, 0.40f), S(P_FLT_S, 0.10f),
    S(P_VEL, 0.50f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.70f), S(P_AMP_R, 0.20f),
    S(P_TONE, -0.10f), S(P_REV_MIX, 0.05f), S(P_REV_SIZE, 0.30f), S(P_LEVEL, 0.75f) } },

/* The tight FM bass of the late eighties: the bite is an envelope on the FM
 * amount, not on the filter, over a clean sine sub. */
{ "BS Solid State", "bass", 3, {
    S(P_MODEL, M_FM), S(P_SHAPE, 0.42f), S(P_SHAPE_ENV, 0.50f), S(P_LAYER, L_SUB_SINE), S(P_BLEND, 0.30f),
    S(P_AGE, 0.05f), S(P_VOICE, V_MONO),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.72f), S(P_RESO, 0.10f), S(P_DRIVE, 0.28f),
    S(P_FLT_ENV, 0.15f), S(P_FLT_D, 0.33f), S(P_FLT_S, 0.0f),
    S(P_VEL, 0.70f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.55f), S(P_AMP_S, 0.40f), S(P_AMP_R, 0.20f),
    S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.35f),
    S(P_REV_MIX, 0.06f), S(P_REV_SIZE, 0.30f), S(P_LEVEL, 0.71f) } },

/* Darksynth: five detuned pulses with moving width, driven hard after the
 * filter, pumping in eighths, over a sub that stays clean. */
{ "BS Chrome Teeth", "bass", 4, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.35f), S(P_STACK, 0.50f), S(P_LAYER, L_SUB_SINE), S(P_BLEND, 0.40f),
    S(P_AGE, 0.12f), S(P_VOICE, V_G60),
    S(P_CUTOFF, 0.68f), S(P_RESO, 0.18f), S(P_DRIVE, 0.90f),
    S(P_FLT_ENV, 0.30f), S(P_FLT_D, 0.50f), S(P_FLT_S, 0.50f),
    S(P_LFO_DEST, D_SHAPE), S(P_LFO_RATE, 0.45f), S(P_LFO_DEPTH, 0.35f),
    S(P_VEL, 0.30f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.85f), S(P_AMP_R, 0.18f),
    S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.40f), S(P_PUMP, 0.45f), S(P_PUMP_RATE, PU_8),
    S(P_TONE, 0.10f), S(P_REV_MIX, 0.05f), S(P_REV_SIZE, 0.35f), S(P_LEVEL, 0.70f) } },

/* A reese: three saws pulled about twenty cents apart, mono with glide, the
 * filter breathing slowly under a phaser. */
{ "BS Undertow", "bass", 5, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_STACK, 0.28f), S(P_LAYER, L_SUB_SINE), S(P_BLEND, 0.45f),
    S(P_AGE, 0.30f), S(P_VOICE, V_G110),
    S(P_CUTOFF, 0.60f), S(P_RESO, 0.20f), S(P_DRIVE, 0.62f),
    S(P_FLT_ENV, 0.15f), S(P_FLT_D, 0.60f), S(P_FLT_S, 0.50f),
    S(P_LFO_DEST, D_FILTER), S(P_LFO_RATE, 0.30f), S(P_LFO_DEPTH, 0.30f),
    S(P_VEL, 0.30f), S(P_AMP_A, 0.20f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.30f),
    S(P_PHS_DEPTH, 0.22f), S(P_PHS_RATE, 0.22f),
    S(P_TONE, -0.15f), S(P_REV_MIX, 0.08f), S(P_REV_SIZE, 0.40f), S(P_LEVEL, 0.50f) } },

/* Darkwave: a single saw-and-square voice with a square sub, the filter down
 * low with a slight attack. Dry, cold, made for a sequencer. */
{ "BS Cold Sequence", "bass", 6, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.85f), S(P_LAYER, L_SUB_SQR), S(P_BLEND, 0.32f),
    S(P_AGE, 0.30f), S(P_VOICE, V_MONO),
    S(P_CUTOFF, 0.46f), S(P_RESO, 0.30f), S(P_DRIVE, 0.25f),
    S(P_FLT_ENV, 0.50f), S(P_FLT_A, 0.08f), S(P_FLT_D, 0.34f), S(P_FLT_S, 0.10f),
    S(P_VEL, 0.50f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.45f), S(P_AMP_S, 0.30f), S(P_AMP_R, 0.15f),
    S(P_DLY_TIME, T_16), S(P_DLY_MIX, 0.12f), S(P_DLY_FB, 0.30f),
    S(P_TONE, -0.10f), S(P_REV_MIX, 0.12f), S(P_REV_SIZE, 0.45f), S(P_REV_TONE, 0.35f), S(P_LEVEL, 0.67f) } },

/* EBM: a pulse with slow width movement, both envelopes a third open and
 * nothing held. The snap of the filter envelope is the whole sound. */
{ "BS Body Work", "bass", 7, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.20f), S(P_STACK, 0.12f), S(P_AGE, 0.20f), S(P_VOICE, V_MONO),
    S(P_CUTOFF, 0.38f), S(P_RESO, 0.25f), S(P_DRIVE, 0.55f),
    S(P_FLT_ENV, 0.72f), S(P_FLT_D, 0.33f), S(P_FLT_S, 0.0f),
    S(P_LFO_DEST, D_SHAPE), S(P_LFO_RATE, 0.50f), S(P_LFO_DEPTH, 0.20f),
    S(P_VEL, 0.60f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.40f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.12f),
    S(P_REV_MIX, 0.07f), S(P_REV_SIZE, 0.30f), S(P_LEVEL, 0.76f) } },

/* ----------------------------------------------------------------------- */
/* BASS: where an effect is the sound                                      */
/* ----------------------------------------------------------------------- */

/* Hold one note: the pump plays the rhythm. A wide reese over a clean sub,
 * ducked almost to nothing on every beat. */
{ "BS Sidechain", "drone", 32, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_STACK, 0.28f), S(P_LAYER, L_SUB_SINE), S(P_BLEND, 0.40f),
    S(P_AGE, 0.20f), S(P_VOICE, V_MONO),
    S(P_CUTOFF, 0.62f), S(P_RESO, 0.15f), S(P_DRIVE, 0.45f),
    S(P_FLT_ENV, 0.20f), S(P_FLT_D, 0.50f), S(P_FLT_S, 0.60f),
    S(P_VEL, 0.30f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.25f),
    S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.50f), S(P_PUMP, 0.85f), S(P_PUMP_RATE, PU_4),
    S(P_REV_MIX, 0.12f), S(P_REV_SIZE, 0.40f), S(P_LEVEL, 0.64f) } },

/* A resonant pulse bass with a phaser sweeping slowly across it. The clean
 * sub underneath does not go through the phaser, so the bottom holds still. */
{ "BS Phase Runner", "bass", 33, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.25f), S(P_STACK, 0.10f), S(P_LAYER, L_SUB_SINE), S(P_BLEND, 0.35f),
    S(P_AGE, 0.20f), S(P_VOICE, V_G45),
    S(P_CUTOFF, 0.55f), S(P_RESO, 0.30f), S(P_DRIVE, 0.50f),
    S(P_FLT_ENV, 0.40f), S(P_FLT_D, 0.40f), S(P_FLT_S, 0.30f),
    S(P_VEL, 0.50f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.55f), S(P_AMP_S, 0.70f), S(P_AMP_R, 0.20f),
    S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.35f), S(P_PHS_DEPTH, 0.80f), S(P_PHS_RATE, 0.30f),
    S(P_REV_MIX, 0.10f), S(P_REV_SIZE, 0.30f), S(P_LEVEL, 0.63f) } },

/* A round bass off a tape that has been left in a hot car: it sags, it
 * wobbles, the top has gone. */
{ "BS Tape Sub", "bass", 34, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.15f), S(P_LAYER, L_SUB_SQR), S(P_BLEND, 0.30f),
    S(P_AGE, 0.50f), S(P_VOICE, V_G60),
    S(P_CUTOFF, 0.50f), S(P_RESO, 0.10f), S(P_DRIVE, 0.40f),
    S(P_FLT_ENV, 0.30f), S(P_FLT_D, 0.45f), S(P_FLT_S, 0.30f),
    S(P_VEL, 0.45f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.75f), S(P_AMP_R, 0.25f),
    S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.25f), S(P_VHS, 0.65f), S(P_TONE, -0.25f),
    S(P_REV_MIX, 0.10f), S(P_REV_SIZE, 0.35f), S(P_REV_TONE, 0.30f), S(P_LEVEL, 0.59f) } },

/* Hold a note and it is chopped into sixteenths, each chop thrown into a
 * dotted-eighth delay. The gate is the pump at full depth. */
{ "BS Gatecrash", "drone", 35, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.60f), S(P_STACK, 0.45f), S(P_LAYER, L_SUB_SINE), S(P_BLEND, 0.30f),
    S(P_AGE, 0.15f), S(P_VOICE, V_MONO),
    S(P_CUTOFF, 0.70f), S(P_RESO, 0.15f), S(P_DRIVE, 0.70f),
    S(P_FLT_ENV, 0.20f), S(P_FLT_D, 0.50f), S(P_FLT_S, 0.60f),
    S(P_VEL, 0.30f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.20f),
    S(P_PUMP, 0.92f), S(P_PUMP_RATE, PU_16),
    S(P_DLY_TIME, T_8D), S(P_DLY_MIX, 0.25f), S(P_DLY_FB, 0.45f),
    S(P_TONE, 0.10f), S(P_REV_MIX, 0.15f), S(P_REV_SIZE, 0.40f), S(P_LEVEL, 0.63f) } },

/* ----------------------------------------------------------------------- */
/* BASS: bigger and wilder                                                 */
/* ----------------------------------------------------------------------- */

/* Acid: one saw, a squelching filter the envelope kicks open, and a slide
 * between overlapping notes -- which, as on the real thing, do not retrigger it. */
{ "BS Acid Rain", "bass", 64, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_AGE, 0.10f), S(P_VOICE, V_G60), S(P_CUTOFF, 0.40f),
    S(P_RESO, 0.62f), S(P_DRIVE, 0.58f), S(P_FLT_ENV, 0.70f), S(P_FLT_D, 0.30f), S(P_FLT_S, 0.0f),
    S(P_VEL, 0.80f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.45f), S(P_AMP_S, 0.60f), S(P_AMP_R, 0.15f),
    S(P_DLY_TIME, T_16), S(P_DLY_MIX, 0.18f), S(P_DLY_FB, 0.35f), S(P_REV_MIX, 0.08f),
    S(P_REV_SIZE, 0.32f), S(P_LEVEL, 0.74f) } },

/* Rubbery FM slap: three operators a few cents apart, the bite all in the
 * first thirty milliseconds, clipped per note. */
{ "BS Slap Circuit", "bass", 65, {
    S(P_MODEL, M_FM), S(P_SHAPE, 0.28f), S(P_SHAPE_ENV, 0.65f), S(P_STACK, 0.12f), S(P_LAYER, L_OCT_UP),
    S(P_BLEND, 0.22f), S(P_AGE, 0.08f), S(P_VOICE, V_MONO), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.76f),
    S(P_RESO, 0.08f), S(P_DRIVE, 0.56f), S(P_FLT_ENV, 0.20f), S(P_FLT_D, 0.24f), S(P_FLT_S, 0.0f),
    S(P_VEL, 0.75f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.52f), S(P_AMP_S, 0.35f), S(P_AMP_R, 0.18f),
    S(P_CHO_MODE, C_II), S(P_CHO_MIX, 0.28f), S(P_REV_MIX, 0.09f), S(P_REV_SIZE, 0.28f),
    S(P_LEVEL, 0.68f) } },

/* A talkbox bass: the envelope drags the vowel from "oo" to "ee" and back on
 * every note. "Yoi." */
{ "BS Mouth", "bass", 66, {
    S(P_MODEL, M_VOX), S(P_SHAPE, 0.08f), S(P_SHAPE_ENV, 0.72f), S(P_STACK, 0.06f),
    S(P_LAYER, L_SUB_SINE), S(P_BLEND, 0.42f), S(P_VOICE, V_G45), S(P_CUTOFF, 0.72f), S(P_RESO, 0.10f),
    S(P_DRIVE, 0.52f), S(P_FLT_ENV, 0.10f), S(P_FLT_D, 0.40f), S(P_FLT_S, 0.15f), S(P_AMP_A, 0.0f),
    S(P_AMP_D, 0.60f), S(P_AMP_S, 0.80f), S(P_AMP_R, 0.20f), S(P_PHS_DEPTH, 0.18f),
    S(P_PHS_RATE, 0.25f), S(P_REV_MIX, 0.08f), S(P_REV_SIZE, 0.35f), S(P_LEVEL, 0.64f) } },

/* Darksynth growl in triplets: detuned pulses into the bus clipper, the filter
 * snapping shut three times a beat over a clean sub. Hold the note. */
{ "BS Wolf", "bass", 67, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.15f), S(P_STACK, 0.22f), S(P_LAYER, L_SUB_SINE),
    S(P_BLEND, 0.45f), S(P_AGE, 0.15f), S(P_VOICE, V_MONO), S(P_CUTOFF, 0.34f), S(P_RESO, 0.38f),
    S(P_DRIVE, 0.80f), S(P_FLT_ENV, 0.10f), S(P_FLT_D, 0.45f), S(P_FLT_S, 0.30f), S(P_LFO_DEST, D_RAMP),
    S(P_LFO_RATE, 0.50f), S(P_LFO_DEPTH, 0.55f), S(P_VEL, 0.30f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.60f),
    S(P_AMP_S, 0.90f), S(P_AMP_R, 0.20f), S(P_TONE, -0.10f), S(P_REV_MIX, 0.07f), S(P_REV_SIZE, 0.38f),
    S(P_LEVEL, 0.62f) } },

/* The long kick that is also the bass line: a sine dropping half an octave
 * onto the note, clipped for harmonics, off a slightly tired tape. */
{ "BS Low Rider", "bass", 68, {
    S(P_MODEL, M_ZAP), S(P_SHAPE, 0.30f), S(P_AGE, 0.05f), S(P_VOICE, V_MONO), S(P_FLT_TYPE, F_LP12),
    S(P_CUTOFF, 0.68f), S(P_RESO, 0.0f), S(P_DRIVE, 0.62f), S(P_FLT_ENV, 0.25f), S(P_FLT_D, 0.40f),
    S(P_FLT_S, 0.0f), S(P_VEL, 0.55f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.74f), S(P_AMP_S, 0.0f),
    S(P_AMP_R, 0.32f), S(P_VHS, 0.18f), S(P_TONE, -0.20f), S(P_REV_MIX, 0.05f), S(P_REV_SIZE, 0.30f),
    S(P_LEVEL, 0.54f) } },

/* Italo: the note and its octave at once, so a one-finger line already bounces. */
{ "BS Octave Machine", "bass", 69, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.65f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.45f), S(P_AGE, 0.15f),
    S(P_VOICE, V_MONO), S(P_CUTOFF, 0.56f), S(P_RESO, 0.28f), S(P_DRIVE, 0.35f), S(P_FLT_ENV, 0.45f),
    S(P_FLT_D, 0.30f), S(P_FLT_S, 0.20f), S(P_VEL, 0.60f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.50f),
    S(P_AMP_S, 0.50f), S(P_AMP_R, 0.15f), S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.30f), S(P_DLY_TIME, T_8),
    S(P_DLY_MIX, 0.15f), S(P_DLY_FB, 0.30f), S(P_REV_MIX, 0.08f), S(P_REV_SIZE, 0.30f),
    S(P_LEVEL, 0.75f) } },

/* Hard sync that sweeps DOWN on every note: a bark, then the body, over a
 * clean sub. */
{ "BS Sync Jaw", "bass", 70, {
    S(P_MODEL, M_SYNC), S(P_SHAPE, 0.55f), S(P_SHAPE_ENV, -0.45f), S(P_LAYER, L_SUB_SINE),
    S(P_BLEND, 0.45f), S(P_AGE, 0.10f), S(P_VOICE, V_MONO), S(P_CUTOFF, 0.62f), S(P_RESO, 0.20f),
    S(P_DRIVE, 0.58f), S(P_FLT_ENV, 0.25f), S(P_FLT_D, 0.36f), S(P_FLT_S, 0.10f), S(P_VEL, 0.60f),
    S(P_AMP_A, 0.0f), S(P_AMP_D, 0.55f), S(P_AMP_S, 0.60f), S(P_AMP_R, 0.18f), S(P_TONE, 0.05f),
    S(P_REV_MIX, 0.06f), S(P_REV_SIZE, 0.25f), S(P_LEVEL, 0.59f) } },

/* A power chord on one key: saw and square with a fifth on top, driven until
 * the two grind against each other. */
{ "BS Power Line", "bass", 71, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.80f), S(P_STACK, 0.06f), S(P_LAYER, L_FIFTH), S(P_BLEND, 0.35f),
    S(P_VOICE, V_G30), S(P_CUTOFF, 0.50f), S(P_RESO, 0.15f), S(P_DRIVE, 0.50f), S(P_FLT_ENV, 0.40f),
    S(P_FLT_D, 0.50f), S(P_FLT_S, 0.35f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.80f),
    S(P_AMP_R, 0.20f), S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.30f), S(P_REV_MIX, 0.10f),
    S(P_REV_SIZE, 0.42f), S(P_LEVEL, 0.75f) } },

/* Digital and nasty: the wavetable scans down from spikes to a comb on every
 * note, over a clean sub, with some tape dirt. */
{ "BS Bit Reaper", "bass", 72, {
    S(P_MODEL, M_WAVE), S(P_SHAPE, 0.93f), S(P_SHAPE_ENV, -0.50f), S(P_LAYER, L_SUB_SINE),
    S(P_BLEND, 0.50f), S(P_AGE, 0.05f), S(P_VOICE, V_MONO), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.68f),
    S(P_RESO, 0.25f), S(P_DRIVE, 0.66f), S(P_FLT_ENV, 0.30f), S(P_FLT_D, 0.40f), S(P_FLT_S, 0.20f),
    S(P_VEL, 0.60f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.55f), S(P_AMP_S, 0.70f), S(P_AMP_R, 0.15f),
    S(P_VHS, 0.25f), S(P_DLY_TIME, T_16), S(P_DLY_MIX, 0.10f), S(P_DLY_FB, 0.25f), S(P_REV_MIX, 0.06f),
    S(P_REV_SIZE, 0.30f), S(P_LEVEL, 0.54f) } },

/* The slow, low pulse under a siege film: a narrow-ish pulse with a square two
 * octaves down, a filter that takes its time, a dark room. */
{ "BS Precinct 13", "bass", 73, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.10f), S(P_STACK, 0.08f), S(P_LAYER, L_SUB_2), S(P_BLEND, 0.40f),
    S(P_AGE, 0.30f), S(P_VOICE, V_G200), S(P_CUTOFF, 0.44f), S(P_RESO, 0.20f), S(P_DRIVE, 0.45f),
    S(P_FLT_ENV, 0.22f), S(P_FLT_A, 0.40f), S(P_FLT_D, 0.60f), S(P_FLT_S, 0.60f), S(P_VEL, 0.30f),
    S(P_AMP_A, 0.10f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.45f), S(P_TONE, -0.05f),
    S(P_REV_MIX, 0.22f), S(P_REV_SIZE, 0.60f), S(P_REV_TONE, 0.35f), S(P_LEVEL, 0.72f) } },

/* ----------------------------------------------------------------------- */
/* BS: horror -- retro and modern horror soundtrack sounds               */
/* ----------------------------------------------------------------------- */

/* The ostinato under a slasher's walk: a narrow pulse and a sine sub, short
 * and plain, on a tape that has been played too often. */
{ "BS Night Stalker", "bass", 128, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.20f), S(P_LAYER, L_SUB_SINE), S(P_BLEND, 0.35f), S(P_AGE, 0.25f),
    S(P_VOICE, V_MONO), S(P_CUTOFF, 0.42f), S(P_RESO, 0.25f), S(P_DRIVE, 0.30f), S(P_FLT_ENV, 0.45f),
    S(P_FLT_D, 0.30f), S(P_FLT_S, 0.10f), S(P_VEL, 0.50f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.45f),
    S(P_AMP_S, 0.30f), S(P_AMP_R, 0.20f), S(P_VHS, 0.25f), S(P_TONE, -0.05f), S(P_REV_MIX, 0.08f),
    S(P_REV_SIZE, 0.40f), S(P_REV_TONE, 0.40f), S(P_LEVEL, 0.65f) } },

/* A modern horror low end: detuned saws over a sub two octaves down, gliding,
 * the filter breathing so slowly you only notice it has moved. */
{ "BS Dread Reese", "bass", 129, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_STACK, 0.35f), S(P_LAYER, L_SUB_2), S(P_BLEND, 0.30f),
    S(P_AGE, 0.30f), S(P_VOICE, V_G150), S(P_CUTOFF, 0.40f), S(P_RESO, 0.30f), S(P_DRIVE, 0.55f),
    S(P_FLT_ENV, 0.20f), S(P_FLT_D, 0.55f), S(P_FLT_S, 0.50f), S(P_LFO_DEST, D_FILTER),
    S(P_LFO_RATE, 0.25f), S(P_LFO_DEPTH, 0.35f), S(P_VEL, 0.30f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.60f),
    S(P_AMP_S, 0.90f), S(P_AMP_R, 0.30f), S(P_PHS_DEPTH, 0.25f), S(P_PHS_RATE, 0.12f),
    S(P_TONE, -0.10f), S(P_REV_MIX, 0.06f), S(P_REV_SIZE, 0.35f), S(P_LEVEL, 0.53f) } },

/* Two thumps and a wait: an FM body that closes at once over a clean sub, with
 * one dark quarter-note echo. The heartbeat in the snow. */
{ "BS Cold Heartbeat", "bass", 130, {
    S(P_MODEL, M_FM), S(P_SHAPE, 0.25f), S(P_SHAPE_ENV, 0.40f), S(P_LAYER, L_SUB_SINE),
    S(P_BLEND, 0.50f), S(P_AGE, 0.10f), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.35f), S(P_RESO, 0.10f),
    S(P_DRIVE, 0.25f), S(P_FLT_ENV, 0.30f), S(P_FLT_D, 0.30f), S(P_FLT_S, 0.0f), S(P_VEL, 0.70f),
    S(P_AMP_A, 0.0f), S(P_AMP_D, 0.42f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.30f), S(P_DLY_TIME, T_4),
    S(P_DLY_MIX, 0.12f), S(P_DLY_FB, 0.20f), S(P_REV_MIX, 0.10f), S(P_REV_SIZE, 0.50f),
    S(P_REV_TONE, 0.30f), S(P_LEVEL, 0.83f) } },

/* A seventies exploitation-reel bass: hard sync driven into the clipper, the
 * print scratched, the sub kept clean underneath. */
{ "BS Grindhouse", "bass", 131, {
    S(P_MODEL, M_SYNC), S(P_SHAPE, 0.35f), S(P_SHAPE_ENV, 0.35f), S(P_LAYER, L_SUB_SQR),
    S(P_BLEND, 0.30f), S(P_AGE, 0.20f), S(P_VOICE, V_MONO), S(P_CUTOFF, 0.55f), S(P_RESO, 0.30f),
    S(P_DRIVE, 0.80f), S(P_FLT_ENV, 0.40f), S(P_FLT_D, 0.35f), S(P_FLT_S, 0.20f), S(P_VEL, 0.50f),
    S(P_AMP_A, 0.0f), S(P_AMP_D, 0.50f), S(P_AMP_S, 0.60f), S(P_AMP_R, 0.20f), S(P_CHO_MODE, C_DIM),
    S(P_CHO_MIX, 0.20f), S(P_VHS, 0.40f), S(P_REV_MIX, 0.05f), S(P_REV_SIZE, 0.25f), S(P_LEVEL, 0.67f) } },

/* ======================================================================= */
/* LEAD                                                                    */
/* ======================================================================= */

/* The hook: a saw with a little detune, glide between overlapping notes and
 * vibrato that arrives late, in chorus and a dotted-eighth delay. */
{ "LD Sunset", "lead", 8, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_STACK, 0.08f), S(P_VOICE, V_G80),
    S(P_CUTOFF, 0.68f), S(P_RESO, 0.20f), S(P_DRIVE, 0.25f),
    S(P_FLT_ENV, 0.30f), S(P_FLT_D, 0.50f), S(P_FLT_S, 0.50f),
    S(P_LFO_DEST, D_PITCH), S(P_LFO_RATE, 0.75f), S(P_LFO_DEPTH, 0.30f),
    S(P_VEL, 0.40f), S(P_AMP_A, 0.30f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.80f), S(P_AMP_R, 0.45f),
    S(P_CHO_MODE, C_BOTH), S(P_CHO_MIX, 0.50f),
    S(P_DLY_TIME, T_8D), S(P_DLY_MIX, 0.28f), S(P_DLY_FB, 0.42f),
    S(P_TONE, 0.10f), S(P_REV_MIX, 0.28f), S(P_REV_SIZE, 0.60f), S(P_LEVEL, 0.61f) } },

/* A pulse whose width never stops moving: hollow, vocal, a touch sad. */
{ "LD Prism", "lead", 9, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.30f), S(P_STACK, 0.06f), S(P_VOICE, V_G45),
    S(P_CUTOFF, 0.70f), S(P_RESO, 0.15f), S(P_DRIVE, 0.20f),
    S(P_FLT_ENV, 0.25f), S(P_FLT_D, 0.50f), S(P_FLT_S, 0.40f),
    S(P_LFO_DEST, D_SHAPE), S(P_LFO_RATE, 0.52f), S(P_LFO_DEPTH, 0.50f),
    S(P_VEL, 0.40f), S(P_AMP_A, 0.25f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.85f), S(P_AMP_R, 0.40f),
    S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.40f),
    S(P_DLY_TIME, T_4), S(P_DLY_MIX, 0.22f), S(P_DLY_FB, 0.35f),
    S(P_REV_MIX, 0.22f), S(P_REV_SIZE, 0.55f), S(P_LEVEL, 0.65f) } },

/* Hard sync, swept by the envelope on every note: the tearing lead. */
{ "LD Razorwire", "lead", 10, {
    S(P_MODEL, M_SYNC), S(P_SHAPE, 0.30f), S(P_SHAPE_ENV, 0.55f), S(P_STACK, 0.05f), S(P_VOICE, V_G30),
    S(P_CUTOFF, 0.85f), S(P_RESO, 0.10f), S(P_DRIVE, 0.42f),
    S(P_FLT_ENV, 0.0f), S(P_FLT_A, 0.0f), S(P_FLT_D, 0.72f), S(P_FLT_S, 0.15f),
    S(P_VEL, 0.40f), S(P_AMP_A, 0.02f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.85f), S(P_AMP_R, 0.35f),
    S(P_DLY_TIME, T_8D), S(P_DLY_MIX, 0.22f), S(P_DLY_FB, 0.40f),
    S(P_REV_MIX, 0.20f), S(P_REV_SIZE, 0.55f), S(P_LEVEL, 0.59f) } },

/* Darksynth lead: a supersaw whose filter closes as the note plays, through
 * the hard clip and into the bus, with long octave slides. */
{ "LD Black Chrome", "lead", 11, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_STACK, 0.50f), S(P_AGE, 0.15f), S(P_VOICE, V_G110),
    S(P_CUTOFF, 0.74f), S(P_RESO, 0.15f), S(P_DRIVE, 0.82f),
    S(P_FLT_ENV, 0.35f), S(P_FLT_D, 0.45f), S(P_FLT_S, 0.35f),
    S(P_LFO_DEST, D_PITCH), S(P_LFO_RATE, 0.74f), S(P_LFO_DEPTH, 0.20f),
    S(P_VEL, 0.30f), S(P_AMP_A, 0.01f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.85f), S(P_AMP_R, 0.30f),
    S(P_DLY_TIME, T_8), S(P_DLY_MIX, 0.18f), S(P_DLY_FB, 0.35f),
    S(P_TONE, 0.15f), S(P_REV_MIX, 0.20f), S(P_REV_SIZE, 0.50f), S(P_LEVEL, 0.68f) } },

/* Darkwave: thin, drifting out of tune, wobbling on a worn tape, a long
 * way back in the room. */
{ "LD Seance", "lead", 12, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.55f), S(P_STACK, 0.10f), S(P_AGE, 0.80f), S(P_VOICE, V_G45),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.60f), S(P_RESO, 0.30f), S(P_DRIVE, 0.20f),
    S(P_FLT_ENV, 0.20f), S(P_FLT_D, 0.50f), S(P_FLT_S, 0.40f),
    S(P_LFO_DEST, D_PITCH), S(P_LFO_RATE, 0.68f), S(P_LFO_DEPTH, 0.32f),
    S(P_VEL, 0.35f), S(P_AMP_A, 0.30f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.70f), S(P_AMP_R, 0.50f),
    S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.60f), S(P_VHS, 0.45f),
    S(P_DLY_TIME, T_8D), S(P_DLY_MIX, 0.34f), S(P_DLY_FB, 0.50f),
    S(P_TONE, -0.10f), S(P_REV_MIX, 0.40f), S(P_REV_SIZE, 0.70f), S(P_REV_TONE, 0.40f), S(P_LEVEL, 0.63f) } },

/* A bare sine with slow glide and wide vibrato: the theremin of a horror
 * score. */
{ "LD Phantom", "lead", 13, {
    S(P_MODEL, M_ZAP), S(P_SHAPE, 0.0f), S(P_AGE, 0.30f), S(P_VOICE, V_G200),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.90f), S(P_RESO, 0.0f), S(P_DRIVE, 0.15f), S(P_FLT_ENV, 0.0f),
    S(P_LFO_DEST, D_PITCH), S(P_LFO_RATE, 0.76f), S(P_LFO_DEPTH, 0.38f),
    S(P_VEL, 0.20f), S(P_AMP_A, 0.52f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.50f),
    S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.25f),
    S(P_DLY_TIME, T_4), S(P_DLY_MIX, 0.20f), S(P_DLY_FB, 0.40f),
    S(P_REV_MIX, 0.45f), S(P_REV_SIZE, 0.70f), S(P_LEVEL, 0.64f) } },

/* ----------------------------------------------------------------------- */
/* LEAD: where an effect is the sound                                      */
/* ----------------------------------------------------------------------- */

/* Play half the line; the delay plays the other half. Long dotted-quarter
 * repeats that darken as they go. */
{ "LD Echo Chamber", "lead", 36, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.40f), S(P_STACK, 0.06f), S(P_VOICE, V_G60),
    S(P_CUTOFF, 0.66f), S(P_RESO, 0.25f), S(P_DRIVE, 0.25f),
    S(P_FLT_ENV, 0.35f), S(P_FLT_D, 0.45f), S(P_FLT_S, 0.30f),
    S(P_LFO_DEST, D_PITCH), S(P_LFO_RATE, 0.74f), S(P_LFO_DEPTH, 0.20f),
    S(P_VEL, 0.40f), S(P_AMP_A, 0.10f), S(P_AMP_D, 0.55f), S(P_AMP_S, 0.60f), S(P_AMP_R, 0.40f),
    S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.20f),
    S(P_DLY_TIME, T_4D), S(P_DLY_MIX, 0.55f), S(P_DLY_FB, 0.72f),
    S(P_TONE, -0.05f), S(P_REV_MIX, 0.25f), S(P_REV_SIZE, 0.55f), S(P_REV_TONE, 0.40f), S(P_LEVEL, 0.80f) } },

/* A plain, soft voice in front of a tail that climbs an octave above it. */
{ "LD Shimmer Solo", "lead", 37, {
    S(P_MODEL, M_WAVE), S(P_SHAPE, 0.05f), S(P_AGE, 0.20f), S(P_VOICE, V_G110),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.80f), S(P_RESO, 0.05f), S(P_DRIVE, 0.15f), S(P_FLT_ENV, 0.0f),
    S(P_LFO_DEST, D_PITCH), S(P_LFO_RATE, 0.75f), S(P_LFO_DEPTH, 0.25f),
    S(P_VEL, 0.30f), S(P_AMP_A, 0.40f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.60f),
    S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.30f),
    S(P_DLY_TIME, T_4), S(P_DLY_MIX, 0.15f), S(P_DLY_FB, 0.40f),
    S(P_REV_MIX, 0.60f), S(P_REV_SIZE, 0.78f), S(P_REV_TONE, 0.70f), S(P_SHIMMER, 0.75f), S(P_LEVEL, 0.52f) } },

/* The melody from a film you rented once: a moving pulse, chorused, on a
 * tape that drifts a third of a semitone either way. */
{ "LD VHS Rental", "lead", 38, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.45f), S(P_STACK, 0.12f), S(P_AGE, 0.50f), S(P_VOICE, V_G45),
    S(P_CUTOFF, 0.64f), S(P_RESO, 0.20f), S(P_DRIVE, 0.20f),
    S(P_FLT_ENV, 0.20f), S(P_FLT_D, 0.50f), S(P_FLT_S, 0.50f),
    S(P_LFO_DEST, D_SHAPE), S(P_LFO_RATE, 0.50f), S(P_LFO_DEPTH, 0.35f),
    S(P_VEL, 0.35f), S(P_AMP_A, 0.25f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.80f), S(P_AMP_R, 0.45f),
    S(P_CHO_MODE, C_II), S(P_CHO_MIX, 0.50f), S(P_VHS, 0.80f), S(P_TONE, -0.10f),
    S(P_DLY_TIME, T_8), S(P_DLY_MIX, 0.22f), S(P_DLY_FB, 0.40f),
    S(P_REV_MIX, 0.25f), S(P_REV_SIZE, 0.50f), S(P_REV_TONE, 0.35f), S(P_LEVEL, 0.58f) } },

/* Short notes that bounce: sixteenth-note repeats thrown left and right
 * while the note itself swings across the field. Mono, NOT a glide mode: a
 * glide mode does not restart the envelope on overlapping notes, and with no
 * sustain that leaves a legato line silent after its first note. */
{ "LD Ping Pong", "lead", 39, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.70f), S(P_VOICE, V_MONO),
    S(P_CUTOFF, 0.58f), S(P_RESO, 0.15f), S(P_DRIVE, 0.25f),
    S(P_FLT_ENV, 0.35f), S(P_FLT_D, 0.40f), S(P_FLT_S, 0.0f),
    S(P_LFO_DEST, D_PAN), S(P_LFO_RATE, 0.60f), S(P_LFO_DEPTH, 0.60f),
    S(P_VEL, 0.50f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.52f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.35f),
    S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.30f),
    S(P_DLY_TIME, T_16), S(P_DLY_MIX, 0.60f), S(P_DLY_FB, 0.68f),
    S(P_TONE, 0.20f), S(P_REV_MIX, 0.15f), S(P_REV_SIZE, 0.40f), S(P_LEVEL, 0.66f) } },

/* A sync sweep through a fast phaser into triplet repeats. */
{ "LD Laserbeam", "lead", 40, {
    S(P_MODEL, M_SYNC), S(P_SHAPE, 0.40f), S(P_SHAPE_ENV, 0.45f), S(P_STACK, 0.05f), S(P_VOICE, V_G30),
    S(P_CUTOFF, 0.82f), S(P_RESO, 0.10f), S(P_DRIVE, 0.50f),
    S(P_FLT_ENV, 0.0f), S(P_FLT_D, 0.60f), S(P_FLT_S, 0.20f),
    S(P_VEL, 0.40f), S(P_AMP_A, 0.02f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.80f), S(P_AMP_R, 0.35f),
    S(P_PHS_DEPTH, 0.55f), S(P_PHS_RATE, 0.78f),
    S(P_DLY_TIME, T_8T), S(P_DLY_MIX, 0.30f), S(P_DLY_FB, 0.50f),
    S(P_REV_MIX, 0.18f), S(P_REV_SIZE, 0.50f), S(P_LEVEL, 0.63f) } },

/* A sung line: one voice made three by the ensemble, in a long bright room. */
{ "LD Neon Choir", "lead", 41, {
    S(P_MODEL, M_VOX), S(P_SHAPE, 0.62f), S(P_STACK, 0.30f), S(P_AGE, 0.30f), S(P_VOICE, V_G80),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.80f), S(P_RESO, 0.05f), S(P_DRIVE, 0.10f), S(P_FLT_ENV, 0.0f),
    S(P_LFO_DEST, D_SHAPE), S(P_LFO_RATE, 0.35f), S(P_LFO_DEPTH, 0.15f),
    S(P_VEL, 0.30f), S(P_AMP_A, 0.50f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.55f),
    S(P_CHO_MODE, C_ENS), S(P_CHO_MIX, 0.60f),
    S(P_DLY_TIME, T_4), S(P_DLY_MIX, 0.18f), S(P_DLY_FB, 0.35f),
    S(P_REV_MIX, 0.50f), S(P_REV_SIZE, 0.72f), S(P_REV_TONE, 0.55f), S(P_LEVEL, 0.76f) } },

/* ----------------------------------------------------------------------- */
/* LEAD: bigger and wilder                                                 */
/* ----------------------------------------------------------------------- */

/* The hoover: detuned pulses with their width thrashing, an octave under, both
 * choruses, and a long swoop between notes. */
{ "LD Hoover '91", "lead", 74, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.30f), S(P_STACK, 0.50f), S(P_LAYER, L_OCT_DN), S(P_BLEND, 0.40f),
    S(P_AGE, 0.25f), S(P_VOICE, V_G150), S(P_CUTOFF, 0.74f), S(P_RESO, 0.10f), S(P_DRIVE, 0.45f),
    S(P_FLT_ENV, 0.15f), S(P_LFO_DEST, D_SHAPE), S(P_LFO_RATE, 0.62f), S(P_LFO_DEPTH, 0.50f),
    S(P_VEL, 0.30f), S(P_AMP_A, 0.05f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.30f),
    S(P_CHO_MODE, C_BOTH), S(P_CHO_MIX, 0.70f), S(P_TONE, 0.05f), S(P_DLY_TIME, T_8),
    S(P_DLY_MIX, 0.15f), S(P_DLY_FB, 0.30f), S(P_REV_MIX, 0.25f), S(P_REV_SIZE, 0.55f),
    S(P_LEVEL, 0.67f) } },

/* Darksynth lead guitar: hard sync with a fifth on top, everything into the
 * clipper, vibrato that arrives late. */
{ "LD Shredder", "lead", 75, {
    S(P_MODEL, M_SYNC), S(P_SHAPE, 0.45f), S(P_SHAPE_ENV, 0.25f), S(P_STACK, 0.10f),
    S(P_LAYER, L_FIFTH), S(P_BLEND, 0.30f), S(P_AGE, 0.15f), S(P_VOICE, V_G45), S(P_CUTOFF, 0.70f),
    S(P_RESO, 0.25f), S(P_DRIVE, 0.92f), S(P_FLT_ENV, 0.20f), S(P_FLT_D, 0.50f), S(P_FLT_S, 0.40f),
    S(P_LFO_RATE, 0.74f), S(P_LFO_DEPTH, 0.28f), S(P_VEL, 0.35f), S(P_AMP_A, 0.01f), S(P_AMP_D, 0.60f),
    S(P_AMP_S, 0.85f), S(P_AMP_R, 0.30f), S(P_TONE, 0.10f), S(P_DLY_TIME, T_4), S(P_DLY_MIX, 0.22f),
    S(P_DLY_FB, 0.35f), S(P_REV_MIX, 0.22f), S(P_REV_SIZE, 0.50f), S(P_LEVEL, 0.62f) } },

/* The hands-in-the-air lead: seven saws and an octave above, wide open, in
 * dotted-eighth repeats. Plays chords too. */
{ "LD Anthem", "lead", 76, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_STACK, 0.85f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.20f),
    S(P_AGE, 0.20f), S(P_CUTOFF, 0.80f), S(P_RESO, 0.10f), S(P_DRIVE, 0.45f), S(P_FLT_ENV, 0.20f),
    S(P_FLT_D, 0.50f), S(P_FLT_S, 0.50f), S(P_VEL, 0.40f), S(P_AMP_A, 0.02f), S(P_AMP_D, 0.60f),
    S(P_AMP_S, 0.85f), S(P_AMP_R, 0.45f), S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.40f), S(P_TONE, 0.25f),
    S(P_DLY_TIME, T_8D), S(P_DLY_MIX, 0.30f), S(P_DLY_FB, 0.45f), S(P_REV_MIX, 0.35f),
    S(P_REV_SIZE, 0.65f), S(P_SHIMMER, 0.10f), S(P_LEVEL, 0.52f) } },

/* A talkbox solo: each note opens its mouth as it is held. */
{ "LD Talk To Me", "lead", 77, {
    S(P_MODEL, M_VOX), S(P_SHAPE, 0.30f), S(P_SHAPE_ENV, 0.55f), S(P_STACK, 0.12f), S(P_VOICE, V_G80),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.82f), S(P_RESO, 0.05f), S(P_DRIVE, 0.40f), S(P_FLT_ENV, 0.10f),
    S(P_FLT_A, 0.35f), S(P_FLT_D, 0.50f), S(P_FLT_S, 0.40f), S(P_LFO_RATE, 0.74f),
    S(P_LFO_DEPTH, 0.22f), S(P_AMP_A, 0.05f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.85f), S(P_AMP_R, 0.40f),
    S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.30f), S(P_DLY_TIME, T_4), S(P_DLY_MIX, 0.20f),
    S(P_REV_MIX, 0.20f), S(P_REV_SIZE, 0.50f), S(P_LEVEL, 0.71f) } },

/* An FM lead that snarls its way in: the FM amount rises over the first third
 * of a second, through a slow phaser. */
{ "LD FM Scream", "lead", 78, {
    S(P_MODEL, M_FM), S(P_SHAPE, 0.50f), S(P_SHAPE_ENV, 0.42f), S(P_STACK, 0.08f), S(P_LAYER, L_OCT_UP),
    S(P_BLEND, 0.18f), S(P_VOICE, V_G60), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.85f), S(P_RESO, 0.05f),
    S(P_DRIVE, 0.60f), S(P_FLT_ENV, 0.10f), S(P_FLT_A, 0.45f), S(P_FLT_D, 0.55f), S(P_FLT_S, 0.60f),
    S(P_LFO_RATE, 0.75f), S(P_LFO_DEPTH, 0.25f), S(P_AMP_A, 0.02f), S(P_AMP_D, 0.60f),
    S(P_AMP_S, 0.85f), S(P_AMP_R, 0.35f), S(P_PHS_DEPTH, 0.30f), S(P_PHS_RATE, 0.20f),
    S(P_DLY_TIME, T_8), S(P_DLY_MIX, 0.22f), S(P_REV_MIX, 0.25f), S(P_REV_SIZE, 0.55f),
    S(P_LEVEL, 0.53f) } },

/* The lead in parallel fifths: one finger, two notes, instant eighties. */
{ "LD Fifth Element", "lead", 79, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.60f), S(P_STACK, 0.14f), S(P_LAYER, L_FIFTH), S(P_BLEND, 0.50f),
    S(P_VOICE, V_G30), S(P_CUTOFF, 0.72f), S(P_RESO, 0.20f), S(P_DRIVE, 0.30f), S(P_FLT_ENV, 0.30f),
    S(P_FLT_D, 0.45f), S(P_FLT_S, 0.40f), S(P_LFO_RATE, 0.74f), S(P_LFO_DEPTH, 0.18f),
    S(P_AMP_A, 0.01f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.85f), S(P_AMP_R, 0.35f), S(P_CHO_MODE, C_II),
    S(P_CHO_MIX, 0.45f), S(P_TONE, 0.10f), S(P_DLY_TIME, T_8D), S(P_DLY_MIX, 0.32f), S(P_DLY_FB, 0.50f),
    S(P_REV_MIX, 0.30f), S(P_REV_SIZE, 0.60f), S(P_LEVEL, 0.62f) } },

/* The sampled bamboo flute of every eighties soundtrack: a near-triangle, a
 * breath of noise, a chiff at the front. */
{ "LD Pan Pipes", "lead", 80, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.05f), S(P_LAYER, L_NOISE), S(P_BLEND, 0.22f), S(P_AGE, 0.35f),
    S(P_VOICE, V_G30), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.70f), S(P_RESO, 0.15f), S(P_DRIVE, 0.15f),
    S(P_FLT_ENV, 0.35f), S(P_FLT_D, 0.20f), S(P_FLT_S, 0.20f), S(P_LFO_RATE, 0.72f),
    S(P_LFO_DEPTH, 0.22f), S(P_VEL, 0.60f), S(P_AMP_A, 0.30f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.85f),
    S(P_AMP_R, 0.40f), S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.30f), S(P_TONE, 0.20f), S(P_DLY_TIME, T_4),
    S(P_DLY_MIX, 0.18f), S(P_DLY_FB, 0.35f), S(P_REV_MIX, 0.45f), S(P_REV_SIZE, 0.70f),
    S(P_REV_TONE, 0.60f), S(P_LEVEL, 0.71f) } },

/* A square wave through a filter on the edge of howling, kicked by the
 * envelope: the lead that cuts through anything. */
{ "LD Screamer", "lead", 81, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.50f), S(P_AGE, 0.15f), S(P_VOICE, V_G80), S(P_CUTOFF, 0.55f),
    S(P_RESO, 0.78f), S(P_DRIVE, 0.55f), S(P_FLT_ENV, 0.55f), S(P_FLT_D, 0.45f), S(P_FLT_S, 0.30f),
    S(P_LFO_RATE, 0.75f), S(P_LFO_DEPTH, 0.20f), S(P_AMP_A, 0.01f), S(P_AMP_D, 0.60f),
    S(P_AMP_S, 0.85f), S(P_AMP_R, 0.30f), S(P_DLY_TIME, T_8D), S(P_DLY_MIX, 0.35f), S(P_DLY_FB, 0.55f),
    S(P_REV_MIX, 0.20f), S(P_REV_SIZE, 0.50f), S(P_LEVEL, 0.67f) } },

/* A digital lead in octaves, the wavetable drifting between a voice and a
 * comb, in the ensemble and triplet repeats. */
{ "LD Octavia", "lead", 82, {
    S(P_MODEL, M_WAVE), S(P_SHAPE, 0.70f), S(P_STACK, 0.20f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.35f),
    S(P_VOICE, V_G45), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.78f), S(P_RESO, 0.10f), S(P_DRIVE, 0.25f),
    S(P_FLT_ENV, 0.15f), S(P_LFO_DEST, D_SHAPE), S(P_LFO_RATE, 0.35f), S(P_LFO_DEPTH, 0.30f),
    S(P_AMP_A, 0.02f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.85f), S(P_AMP_R, 0.40f), S(P_CHO_MODE, C_ENS),
    S(P_CHO_MIX, 0.35f), S(P_DLY_TIME, T_8T), S(P_DLY_MIX, 0.25f), S(P_DLY_FB, 0.45f),
    S(P_REV_MIX, 0.30f), S(P_REV_SIZE, 0.60f), S(P_SHIMMER, 0.20f), S(P_LEVEL, 0.74f) } },

/* Every note falls onto its pitch from a few semitones up: a melody played on
 * laser toms, thrown side to side. */
{ "LD Raygun Melody", "lead", 83, {
    S(P_MODEL, M_ZAP), S(P_SHAPE, 0.22f), S(P_STACK, 0.18f), S(P_AGE, 0.10f), S(P_VOICE, V_MONO),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.85f), S(P_RESO, 0.05f), S(P_DRIVE, 0.70f), S(P_FLT_ENV, 0.20f),
    S(P_LFO_DEST, D_PAN), S(P_LFO_RATE, 0.60f), S(P_LFO_DEPTH, 0.45f), S(P_AMP_A, 0.0f),
    S(P_AMP_D, 0.55f), S(P_AMP_S, 0.50f), S(P_AMP_R, 0.30f), S(P_DLY_TIME, T_16), S(P_DLY_MIX, 0.30f),
    S(P_DLY_FB, 0.45f), S(P_REV_MIX, 0.25f), S(P_REV_SIZE, 0.50f), S(P_LEVEL, 0.65f) } },

/* ----------------------------------------------------------------------- */
/* LD: horror -- retro and modern horror soundtrack sounds               */
/* ----------------------------------------------------------------------- */

/* A fifties flying-saucer signal: an FM tone whose sidebands grow as the note
 * holds, sliding between notes, with triplet repeats and a shimmering tail. */
{ "LD Saucer Signal", "lead", 132, {
    S(P_MODEL, M_FM), S(P_SHAPE, 0.55f), S(P_AGE, 0.20f), S(P_VOICE, V_G110), S(P_FLT_TYPE, F_LP12),
    S(P_CUTOFF, 0.75f), S(P_RESO, 0.05f), S(P_DRIVE, 0.10f), S(P_FLT_ENV, 0.0f),
    S(P_LFO_DEST, D_PITCH), S(P_LFO_RATE, 0.55f), S(P_LFO_DEPTH, 0.50f), S(P_VEL, 0.40f),
    S(P_AMP_A, 0.35f), S(P_AMP_D, 0.60f), S(P_AMP_S, 1.0f), S(P_AMP_R, 0.50f), S(P_TONE, 0.10f),
    S(P_DLY_TIME, T_8T), S(P_DLY_MIX, 0.30f), S(P_DLY_FB, 0.55f), S(P_REV_MIX, 0.35f),
    S(P_REV_SIZE, 0.60f), S(P_SHAPE_ENV, -0.45f), S(P_FLT_D, 0.75f), S(P_FLT_S, 0.0f),
    S(P_SHIMMER, 0.25f), S(P_LEVEL, 0.61f) } },

/* A modern occult-thriller lead: a reed wavetable driven to the edge, its
 * timbre crawling, in a dark dotted echo. */
{ "LD Possession", "lead", 133, {
    S(P_MODEL, M_WAVE), S(P_SHAPE, 0.45f), S(P_STACK, 0.15f), S(P_LAYER, L_OCT_DN), S(P_BLEND, 0.30f),
    S(P_AGE, 0.30f), S(P_VOICE, V_G80), S(P_CUTOFF, 0.55f), S(P_RESO, 0.40f), S(P_DRIVE, 0.85f),
    S(P_FLT_ENV, 0.30f), S(P_FLT_D, 0.50f), S(P_FLT_S, 0.50f), S(P_LFO_DEST, D_SHAPE),
    S(P_LFO_RATE, 0.35f), S(P_LFO_DEPTH, 0.40f), S(P_VEL, 0.40f), S(P_AMP_A, 0.02f), S(P_AMP_D, 0.60f),
    S(P_AMP_S, 0.90f), S(P_AMP_R, 0.35f), S(P_PHS_DEPTH, 0.30f), S(P_PHS_RATE, 0.20f),
    S(P_DLY_TIME, T_8D), S(P_DLY_MIX, 0.22f), S(P_DLY_FB, 0.45f), S(P_REV_MIX, 0.25f),
    S(P_REV_SIZE, 0.60f), S(P_REV_TONE, 0.35f), S(P_LEVEL, 0.67f) } },

/* The cold square-wave melody of an eighties siege film: one oscillator, an
 * octave on top, a little chorus and a short slap. */
{ "LD Prowler", "lead", 134, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.05f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.15f), S(P_AGE, 0.20f),
    S(P_VOICE, V_MONO), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.62f), S(P_RESO, 0.15f), S(P_DRIVE, 0.20f),
    S(P_FLT_ENV, 0.25f), S(P_FLT_D, 0.45f), S(P_FLT_S, 0.60f), S(P_LFO_DEST, D_PITCH),
    S(P_LFO_RATE, 0.70f), S(P_LFO_DEPTH, 0.35f), S(P_VEL, 0.40f), S(P_AMP_A, 0.01f), S(P_AMP_D, 0.55f),
    S(P_AMP_S, 0.85f), S(P_AMP_R, 0.30f), S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.25f), S(P_VHS, 0.20f),
    S(P_DLY_TIME, T_8), S(P_DLY_MIX, 0.20f), S(P_DLY_FB, 0.30f), S(P_REV_MIX, 0.20f),
    S(P_REV_SIZE, 0.50f), S(P_LEVEL, 0.59f) } },

/* A wail from the next room: thin high-passed sync that slides and shakes,
 * its harmonics falling back as the note holds, an octave of shimmer above. */
{ "LD Banshee", "lead", 135, {
    S(P_MODEL, M_SYNC), S(P_SHAPE, 0.70f), S(P_SHAPE_ENV, -0.40f), S(P_AGE, 0.35f), S(P_VOICE, V_G650),
    S(P_FLT_TYPE, F_HP), S(P_CUTOFF, 0.50f), S(P_RESO, 0.50f), S(P_DRIVE, 0.50f), S(P_FLT_ENV, 0.0f),
    S(P_FLT_A, 0.40f), S(P_FLT_D, 0.70f), S(P_FLT_S, 0.50f), S(P_LFO_DEST, D_PITCH),
    S(P_LFO_RATE, 0.82f), S(P_LFO_DEPTH, 0.75f), S(P_VEL, 0.30f), S(P_AMP_A, 0.30f), S(P_AMP_D, 0.60f),
    S(P_AMP_S, 1.0f), S(P_AMP_R, 0.60f), S(P_TONE, 0.20f), S(P_REV_MIX, 0.50f), S(P_REV_SIZE, 0.80f),
    S(P_SHIMMER, 0.30f), S(P_LEVEL, 0.77f) } },

/* ======================================================================= */
/* PAD                                                                     */
/* ======================================================================= */

/* The chord bed: saw, a trace of sub, a slow filter, the second chorus. */
{ "PD Dusk", "pad", 14, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_STACK, 0.10f), S(P_LAYER, L_SUB_SQR), S(P_BLEND, 0.12f), S(P_AGE, 0.30f),
    S(P_CUTOFF, 0.60f), S(P_RESO, 0.10f), S(P_DRIVE, 0.15f),
    S(P_FLT_ENV, 0.20f), S(P_FLT_A, 0.66f), S(P_FLT_D, 0.70f), S(P_FLT_S, 0.70f),
    S(P_VEL, 0.25f), S(P_AMP_A, 0.62f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.85f), S(P_AMP_R, 0.62f),
    S(P_CHO_MODE, C_II), S(P_CHO_MIX, 0.60f),
    S(P_DLY_TIME, T_4), S(P_DLY_MIX, 0.10f), S(P_DLY_FB, 0.35f),
    S(P_TONE, 0.45f), S(P_REV_MIX, 0.35f), S(P_REV_SIZE, 0.65f), S(P_LEVEL, 0.45f) } },

/* High, thin strings from a pulse with its width in constant motion. */
{ "PD Glass Avenue", "pad", 15, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.32f), S(P_STACK, 0.14f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.20f), S(P_AGE, 0.25f),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.68f), S(P_RESO, 0.08f), S(P_DRIVE, 0.12f), S(P_FLT_ENV, 0.10f),
    S(P_LFO_DEST, D_SHAPE), S(P_LFO_RATE, 0.42f), S(P_LFO_DEPTH, 0.45f),
    S(P_VEL, 0.20f), S(P_AMP_A, 0.66f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.60f),
    S(P_CHO_MODE, C_II), S(P_CHO_MIX, 0.60f),
    S(P_TONE, 0.60f), S(P_REV_MIX, 0.40f), S(P_REV_SIZE, 0.65f), S(P_LEVEL, 0.41f) } },

/* Seven saws wide, opening slowly, pumping on the beat, with an octave of
 * shimmer in the tail. */
{ "PD Skyline", "pad", 16, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_STACK, 0.75f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.18f), S(P_AGE, 0.20f),
    S(P_CUTOFF, 0.62f), S(P_RESO, 0.10f), S(P_DRIVE, 0.18f),
    S(P_FLT_ENV, 0.30f), S(P_FLT_A, 0.72f), S(P_FLT_D, 0.80f), S(P_FLT_S, 0.80f),
    S(P_VEL, 0.20f), S(P_AMP_A, 0.60f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.65f),
    S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.30f), S(P_PUMP, 0.40f), S(P_PUMP_RATE, PU_4),
    S(P_TONE, 0.30f), S(P_REV_MIX, 0.45f), S(P_REV_SIZE, 0.70f), S(P_SHIMMER, 0.20f), S(P_LEVEL, 0.41f) } },

/* A string machine: two registers through the three-phase ensemble and a
 * slow phaser. Cold, flat, beautiful. */
{ "PD Winter Ensemble", "pad", 17, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_STACK, 0.22f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.50f), S(P_AGE, 0.45f),
    S(P_CUTOFF, 0.58f), S(P_RESO, 0.08f), S(P_DRIVE, 0.10f), S(P_FLT_ENV, 0.05f),
    S(P_VEL, 0.15f), S(P_AMP_A, 0.68f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.60f),
    S(P_CHO_MODE, C_ENS), S(P_CHO_MIX, 0.80f), S(P_PHS_DEPTH, 0.18f), S(P_PHS_RATE, 0.20f),
    S(P_TONE, 0.35f), S(P_REV_MIX, 0.42f), S(P_REV_SIZE, 0.62f), S(P_REV_TONE, 0.45f), S(P_LEVEL, 0.44f) } },

/* A choir on "ah", five voices to a note, the vowel drifting, in a very
 * large room. */
{ "PD Cathedral", "pad", 18, {
    S(P_MODEL, M_VOX), S(P_SHAPE, 0.50f), S(P_STACK, 0.50f), S(P_AGE, 0.40f),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.78f), S(P_RESO, 0.05f), S(P_DRIVE, 0.10f), S(P_FLT_ENV, 0.0f),
    S(P_LFO_DEST, D_SHAPE), S(P_LFO_RATE, 0.30f), S(P_LFO_DEPTH, 0.12f),
    S(P_VEL, 0.20f), S(P_AMP_A, 0.70f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.62f),
    S(P_CHO_MODE, C_ENS), S(P_CHO_MIX, 0.40f),
    S(P_DLY_TIME, T_4), S(P_DLY_MIX, 0.12f), S(P_DLY_FB, 0.40f),
    S(P_REV_MIX, 0.55f), S(P_REV_SIZE, 0.75f), S(P_REV_TONE, 0.45f), S(P_LEVEL, 0.48f) } },

/* Digital glass: a sparse wavetable, stacked, with the tail an octave up. */
{ "PD Frostline", "pad", 19, {
    S(P_MODEL, M_WAVE), S(P_SHAPE, 0.57f), S(P_STACK, 0.35f), S(P_AGE, 0.20f),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.80f), S(P_RESO, 0.05f), S(P_DRIVE, 0.10f), S(P_FLT_ENV, 0.10f),
    S(P_LFO_DEST, D_SHAPE), S(P_LFO_RATE, 0.28f), S(P_LFO_DEPTH, 0.18f),
    S(P_VEL, 0.25f), S(P_AMP_A, 0.62f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.85f), S(P_AMP_R, 0.68f),
    S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.50f),
    S(P_DLY_TIME, T_4D), S(P_DLY_MIX, 0.20f), S(P_DLY_FB, 0.45f),
    S(P_TONE, 0.30f), S(P_REV_MIX, 0.50f), S(P_REV_SIZE, 0.80f), S(P_REV_TONE, 0.70f), S(P_SHIMMER, 0.50f), S(P_LEVEL, 0.38f) } },

/* ----------------------------------------------------------------------- */
/* PAD: where an effect is the sound                                       */
/* ----------------------------------------------------------------------- */

/* Mostly tail. A wavetable turning between glass and voice, high-passed so it
 * floats, fading in over a second into a very long room an octave higher. */
{ "PD Aurora", "pad", 42, {
    S(P_MODEL, M_WAVE), S(P_SHAPE, 0.70f), S(P_STACK, 0.40f), S(P_AGE, 0.25f), S(P_FLT_TYPE, F_HP),
    S(P_CUTOFF, 0.30f), S(P_RESO, 0.05f), S(P_DRIVE, 0.10f), S(P_FLT_ENV, 0.0f),
    S(P_LFO_DEST, D_SHAPE), S(P_LFO_RATE, 0.20f), S(P_LFO_DEPTH, 0.40f), S(P_VEL, 0.20f),
    S(P_AMP_A, 0.78f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.78f), S(P_CHO_MODE, C_DIM),
    S(P_CHO_MIX, 0.40f), S(P_DLY_TIME, T_4D), S(P_DLY_MIX, 0.15f), S(P_DLY_FB, 0.50f),
    S(P_TONE, 0.40f), S(P_REV_MIX, 0.70f), S(P_REV_SIZE, 0.90f), S(P_REV_TONE, 0.75f),
    S(P_SHIMMER, 0.85f), S(P_LEVEL, 0.44f) } },

/* A wide sync pad that breathes twice a bar: a deep half-note pump under a
 * slow phaser, the edge rounded off by a gentle filter. */
{ "PD Tidal", "pad", 43, {
    S(P_MODEL, M_SYNC), S(P_SHAPE, 0.30f), S(P_STACK, 0.50f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.15f),
    S(P_AGE, 0.20f), S(P_CUTOFF, 0.50f), S(P_RESO, 0.10f), S(P_DRIVE, 0.15f), S(P_FLT_ENV, 0.25f),
    S(P_FLT_A, 0.70f), S(P_FLT_D, 0.80f), S(P_FLT_S, 0.80f), S(P_VEL, 0.20f), S(P_AMP_A, 0.60f),
    S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.65f), S(P_CHO_MODE, C_II), S(P_CHO_MIX, 0.30f),
    S(P_PHS_DEPTH, 0.50f), S(P_PHS_RATE, 0.18f), S(P_PUMP, 0.75f), S(P_PUMP_RATE, PU_2),
    S(P_TONE, 0.25f), S(P_REV_MIX, 0.40f), S(P_REV_SIZE, 0.68f), S(P_FLT_TYPE, F_LP12),
    S(P_LEVEL, 0.42f) } },

/* The string machine through the phaser, built from pulses instead of saws and
 * with no ensemble at all: the widths swim and a deep phaser does the rest. */
{ "PD Solina Phase", "pad", 44, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.40f), S(P_STACK, 0.55f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.50f),
    S(P_AGE, 0.40f), S(P_CUTOFF, 0.60f), S(P_RESO, 0.08f), S(P_DRIVE, 0.10f), S(P_FLT_ENV, 0.05f),
    S(P_VEL, 0.15f), S(P_AMP_A, 0.60f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.60f),
    S(P_PHS_DEPTH, 0.85f), S(P_PHS_RATE, 0.45f), S(P_TONE, 0.30f), S(P_REV_MIX, 0.35f),
    S(P_REV_SIZE, 0.60f), S(P_REV_TONE, 0.50f), S(P_LFO_DEST, D_SHAPE), S(P_LFO_RATE, 0.30f),
    S(P_LFO_DEPTH, 0.45f), S(P_LEVEL, 0.33f) } },

/* A pad remembered rather than heard: saw and square on a worn tape, the
 * chorus still working, the top end not. */
{ "PD Betamax", "pad", 45, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.80f), S(P_STACK, 0.15f), S(P_LAYER, L_SUB_SQR), S(P_BLEND, 0.10f), S(P_AGE, 0.60f),
    S(P_CUTOFF, 0.55f), S(P_RESO, 0.15f), S(P_DRIVE, 0.20f),
    S(P_FLT_ENV, 0.20f), S(P_FLT_A, 0.60f), S(P_FLT_D, 0.70f), S(P_FLT_S, 0.60f),
    S(P_VEL, 0.20f), S(P_AMP_A, 0.60f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.85f), S(P_AMP_R, 0.60f),
    S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.55f), S(P_VHS, 0.75f), S(P_TONE, 0.10f),
    S(P_REV_MIX, 0.30f), S(P_REV_SIZE, 0.55f), S(P_REV_TONE, 0.30f), S(P_LEVEL, 0.42f) } },

/* The longest room there is: a soft bell tone that never quite stops. Play
 * slowly and let chords pile up. */
{ "PD Frozen Lake", "pad", 46, {
    S(P_MODEL, M_BELL), S(P_SHAPE, 0.12f), S(P_STACK, 0.30f), S(P_AGE, 0.20f),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.70f), S(P_RESO, 0.05f), S(P_DRIVE, 0.10f), S(P_FLT_ENV, 0.0f),
    S(P_VEL, 0.30f), S(P_AMP_A, 0.50f), S(P_AMP_D, 0.80f), S(P_AMP_S, 0.70f), S(P_AMP_R, 0.80f),
    S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.35f),
    S(P_TONE, 0.35f), S(P_REV_MIX, 0.80f), S(P_REV_SIZE, 1.0f), S(P_REV_TONE, 0.60f), S(P_SHIMMER, 0.30f), S(P_LEVEL, 0.39f) } },

/* A held chord cut into sixteenths, the pieces falling into a dotted-eighth
 * delay and a bright room. */
{ "PD Gated Heaven", "pad", 47, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_STACK, 0.60f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.20f), S(P_AGE, 0.20f),
    S(P_CUTOFF, 0.70f), S(P_RESO, 0.10f), S(P_DRIVE, 0.15f), S(P_FLT_ENV, 0.10f),
    S(P_VEL, 0.20f), S(P_AMP_A, 0.30f), S(P_AMP_D, 0.60f), S(P_AMP_S, 1.0f), S(P_AMP_R, 0.50f),
    S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.35f), S(P_PUMP, 0.95f), S(P_PUMP_RATE, PU_16),
    S(P_DLY_TIME, T_8D), S(P_DLY_MIX, 0.35f), S(P_DLY_FB, 0.55f),
    S(P_TONE, 0.30f), S(P_REV_MIX, 0.45f), S(P_REV_SIZE, 0.65f), S(P_SHIMMER, 0.15f), S(P_LEVEL, 0.40f) } },

/* Saws stacked in fifths, spread as wide as the dimension chorus will go,
 * with a quarter-note echo behind. */
{ "PD Wide Open", "pad", 48, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_STACK, 0.35f), S(P_LAYER, L_FIFTH), S(P_BLEND, 0.30f), S(P_AGE, 0.20f),
    S(P_CUTOFF, 0.68f), S(P_RESO, 0.10f), S(P_DRIVE, 0.15f), S(P_FLT_ENV, 0.15f),
    S(P_FLT_A, 0.60f), S(P_FLT_D, 0.70f), S(P_FLT_S, 0.70f),
    S(P_VEL, 0.20f), S(P_AMP_A, 0.55f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.60f),
    S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.90f),
    S(P_DLY_TIME, T_4), S(P_DLY_MIX, 0.30f), S(P_DLY_FB, 0.50f),
    S(P_TONE, 0.45f), S(P_REV_MIX, 0.30f), S(P_REV_SIZE, 0.60f), S(P_REV_TONE, 0.65f), S(P_LEVEL, 0.35f) } },

/* A choir on "oo" a long way down: a deep slow phaser, the filter drifting,
 * a dark room. */
{ "PD Undersea", "pad", 49, {
    S(P_MODEL, M_VOX), S(P_SHAPE, 0.10f), S(P_STACK, 0.45f), S(P_AGE, 0.40f),
    S(P_CUTOFF, 0.55f), S(P_RESO, 0.30f), S(P_DRIVE, 0.15f), S(P_FLT_ENV, 0.0f),
    S(P_LFO_DEST, D_FILTER), S(P_LFO_RATE, 0.30f), S(P_LFO_DEPTH, 0.35f),
    S(P_VEL, 0.20f), S(P_AMP_A, 0.70f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.70f),
    S(P_CHO_MODE, C_II), S(P_CHO_MIX, 0.30f), S(P_PHS_DEPTH, 0.70f), S(P_PHS_RATE, 0.12f),
    S(P_TONE, 0.05f), S(P_REV_MIX, 0.55f), S(P_REV_SIZE, 0.80f), S(P_REV_TONE, 0.25f), S(P_LEVEL, 0.53f) } },

/* ----------------------------------------------------------------------- */
/* PAD: bigger and wilder                                                  */
/* ----------------------------------------------------------------------- */

/* As big as it goes: seven saws at full spread with an octave under, opening
 * over two seconds into the ensemble and the longest room. */
{ "PD Monolith", "pad", 84, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.55f), S(P_STACK, 1.00f), S(P_LAYER, L_OCT_DN), S(P_BLEND, 0.30f),
    S(P_AGE, 0.30f), S(P_CUTOFF, 0.58f), S(P_RESO, 0.10f), S(P_DRIVE, 0.40f), S(P_FLT_ENV, 0.35f),
    S(P_FLT_A, 0.75f), S(P_FLT_D, 0.80f), S(P_FLT_S, 0.80f), S(P_VEL, 0.20f), S(P_AMP_A, 0.66f),
    S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.72f), S(P_CHO_MODE, C_ENS), S(P_CHO_MIX, 0.55f),
    S(P_TONE, 0.35f), S(P_REV_MIX, 0.60f), S(P_REV_SIZE, 0.90f), S(P_SHIMMER, 0.15f), S(P_LEVEL, 0.43f) } },

/* The resonant sweep: a slow LFO drags a ringing filter up and down the chord. */
{ "PD Sweep Machine", "pad", 85, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.25f), S(P_STACK, 0.30f), S(P_AGE, 0.25f), S(P_CUTOFF, 0.52f),
    S(P_RESO, 0.50f), S(P_DRIVE, 0.20f), S(P_FLT_ENV, 0.05f), S(P_LFO_DEST, D_FILTER),
    S(P_LFO_RATE, 0.22f), S(P_LFO_DEPTH, 0.65f), S(P_VEL, 0.20f), S(P_AMP_A, 0.60f), S(P_AMP_D, 0.70f),
    S(P_AMP_S, 0.90f), S(P_AMP_R, 0.62f), S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.40f), S(P_TONE, 0.35f),
    S(P_DLY_TIME, T_4D), S(P_DLY_MIX, 0.25f), S(P_DLY_FB, 0.50f), S(P_REV_MIX, 0.40f),
    S(P_REV_SIZE, 0.70f), S(P_LEVEL, 0.39f) } },

/* All air: a supersaw through a HIGH-pass, with nothing under it at all and a
 * tail an octave up. Sits over anything. */
{ "PD High Wire", "pad", 86, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_STACK, 0.55f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.30f),
    S(P_AGE, 0.25f), S(P_FLT_TYPE, F_HP), S(P_CUTOFF, 0.55f), S(P_RESO, 0.25f), S(P_DRIVE, 0.15f),
    S(P_FLT_ENV, -0.20f), S(P_FLT_A, 0.70f), S(P_FLT_D, 0.75f), S(P_FLT_S, 0.70f), S(P_VEL, 0.20f),
    S(P_AMP_A, 0.68f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.68f), S(P_CHO_MODE, C_DIM),
    S(P_CHO_MIX, 0.60f), S(P_TONE, 0.20f), S(P_REV_MIX, 0.65f), S(P_REV_SIZE, 0.85f),
    S(P_REV_TONE, 0.75f), S(P_SHIMMER, 0.45f), S(P_LEVEL, 0.62f) } },

/* A chord heard through a radio between stations: a band-pass that jumps at
 * random, tape, and echoes of it. */
{ "PD Radio Ghost", "pad", 87, {
    S(P_MODEL, M_WAVE), S(P_SHAPE, 0.64f), S(P_STACK, 0.35f), S(P_AGE, 0.40f), S(P_FLT_TYPE, F_BP),
    S(P_CUTOFF, 0.58f), S(P_RESO, 0.40f), S(P_DRIVE, 0.30f), S(P_FLT_ENV, 0.0f), S(P_LFO_DEST, D_SH),
    S(P_LFO_RATE, 0.60f), S(P_LFO_DEPTH, 0.45f), S(P_VEL, 0.20f), S(P_AMP_A, 0.50f), S(P_AMP_D, 0.70f),
    S(P_AMP_S, 0.90f), S(P_AMP_R, 0.60f), S(P_VHS, 0.40f), S(P_TONE, 0.10f), S(P_DLY_TIME, T_8),
    S(P_DLY_MIX, 0.30f), S(P_DLY_FB, 0.55f), S(P_REV_MIX, 0.45f), S(P_REV_SIZE, 0.70f),
    S(P_REV_TONE, 0.40f), S(P_LEVEL, 0.50f) } },

/* FM in fifths that breathes: the FM amount rises and falls slowly, so the
 * chord goes from a hum to glass and back. */
{ "PD FM Halo", "pad", 88, {
    S(P_MODEL, M_FM), S(P_SHAPE, 0.30f), S(P_STACK, 0.45f), S(P_LAYER, L_FIFTH), S(P_BLEND, 0.25f),
    S(P_AGE, 0.25f), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.76f), S(P_RESO, 0.05f), S(P_DRIVE, 0.12f),
    S(P_FLT_ENV, 0.0f), S(P_LFO_DEST, D_SHAPE), S(P_LFO_RATE, 0.30f), S(P_LFO_DEPTH, 0.35f),
    S(P_VEL, 0.20f), S(P_AMP_A, 0.64f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.70f),
    S(P_CHO_MODE, C_ENS), S(P_CHO_MIX, 0.50f), S(P_TONE, 0.30f), S(P_DLY_TIME, T_4D),
    S(P_DLY_MIX, 0.20f), S(P_DLY_FB, 0.45f), S(P_REV_MIX, 0.60f), S(P_REV_SIZE, 0.85f),
    S(P_REV_TONE, 0.60f), S(P_SHIMMER, 0.35f), S(P_LEVEL, 0.45f) } },

/* A sync sweep slowed to the length of a breath: each chord tears open over
 * three seconds, through a very slow phaser. */
{ "PD Sync Tide", "pad", 89, {
    S(P_MODEL, M_SYNC), S(P_SHAPE, 0.20f), S(P_SHAPE_ENV, 0.45f), S(P_STACK, 0.40f), S(P_AGE, 0.25f),
    S(P_CUTOFF, 0.68f), S(P_RESO, 0.12f), S(P_DRIVE, 0.22f), S(P_FLT_ENV, 0.10f), S(P_FLT_A, 0.80f),
    S(P_FLT_D, 0.85f), S(P_FLT_S, 0.70f), S(P_VEL, 0.20f), S(P_AMP_A, 0.62f), S(P_AMP_D, 0.70f),
    S(P_AMP_S, 0.90f), S(P_AMP_R, 0.65f), S(P_CHO_MODE, C_II), S(P_CHO_MIX, 0.35f),
    S(P_PHS_DEPTH, 0.35f), S(P_PHS_RATE, 0.12f), S(P_TONE, 0.30f), S(P_REV_MIX, 0.50f),
    S(P_REV_SIZE, 0.80f), S(P_LEVEL, 0.37f) } },

/* Backwards: it swells in over a second and a half, opening as it comes, then
 * stops dead and leaves its echoes behind. */
{ "PD Reverse Bloom", "pad", 90, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.85f), S(P_STACK, 0.35f), S(P_AGE, 0.25f), S(P_CUTOFF, 0.50f),
    S(P_RESO, 0.15f), S(P_DRIVE, 0.20f), S(P_FLT_ENV, 0.50f), S(P_FLT_A, 0.82f), S(P_FLT_D, 0.60f),
    S(P_FLT_S, 0.90f), S(P_VEL, 0.20f), S(P_AMP_A, 0.80f), S(P_AMP_D, 0.60f), S(P_AMP_S, 1.00f),
    S(P_AMP_R, 0.25f), S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.50f), S(P_TONE, 0.40f), S(P_DLY_TIME, T_4),
    S(P_DLY_MIX, 0.45f), S(P_DLY_FB, 0.55f), S(P_REV_MIX, 0.35f), S(P_REV_SIZE, 0.60f),
    S(P_LEVEL, 0.37f) } },

/* A wall of pulse-width strings, shivering: a fast tremolo under the ensemble. */
{ "PD Tremolo Sky", "pad", 91, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.35f), S(P_STACK, 0.65f), S(P_LAYER, L_FIFTH), S(P_BLEND, 0.25f),
    S(P_AGE, 0.30f), S(P_CUTOFF, 0.70f), S(P_RESO, 0.08f), S(P_DRIVE, 0.15f), S(P_FLT_ENV, 0.10f),
    S(P_LFO_DEST, D_AMP), S(P_LFO_RATE, 0.80f), S(P_LFO_DEPTH, 0.70f), S(P_VEL, 0.20f),
    S(P_AMP_A, 0.55f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.60f), S(P_CHO_MODE, C_ENS),
    S(P_CHO_MIX, 0.60f), S(P_TONE, 0.35f), S(P_REV_MIX, 0.50f), S(P_REV_SIZE, 0.75f),
    S(P_REV_TONE, 0.55f), S(P_LEVEL, 0.48f) } },

/* Wind with a chord in it: noise through a ringing, key-tracked filter over
 * soft saws, drifting from side to side, with half-note echoes. */
{ "PD Vapor Trail", "pad", 92, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.20f), S(P_STACK, 0.30f), S(P_LAYER, L_NOISE), S(P_BLEND, 0.30f),
    S(P_AGE, 0.35f), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.66f), S(P_RESO, 0.35f), S(P_DRIVE, 0.10f),
    S(P_FLT_ENV, 0.10f), S(P_LFO_DEST, D_PAN), S(P_LFO_RATE, 0.30f), S(P_LFO_DEPTH, 0.60f),
    S(P_VEL, 0.20f), S(P_AMP_A, 0.70f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.72f),
    S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.50f), S(P_TONE, 0.40f), S(P_DLY_TIME, T_2),
    S(P_DLY_MIX, 0.30f), S(P_DLY_FB, 0.60f), S(P_REV_MIX, 0.60f), S(P_REV_SIZE, 0.90f),
    S(P_REV_TONE, 0.70f), S(P_SHIMMER, 0.30f), S(P_LEVEL, 0.53f) } },

/* The wavetable scans itself while you hold the chord, organ to glass, with
 * two octaves of sparkle on top and a bright shimmering room. */
{ "PD Prism Scan", "pad", 93, {
    S(P_MODEL, M_WAVE), S(P_SHAPE, 0.30f), S(P_SHAPE_ENV, 0.50f), S(P_STACK, 0.30f),
    S(P_LAYER, L_OCT_2), S(P_BLEND, 0.12f), S(P_AGE, 0.20f), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.84f),
    S(P_RESO, 0.05f), S(P_DRIVE, 0.10f), S(P_FLT_ENV, 0.05f), S(P_FLT_A, 0.85f), S(P_FLT_D, 0.85f),
    S(P_FLT_S, 0.60f), S(P_VEL, 0.20f), S(P_AMP_A, 0.58f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f),
    S(P_AMP_R, 0.70f), S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.45f), S(P_TONE, 0.35f), S(P_REV_MIX, 0.65f),
    S(P_REV_SIZE, 0.88f), S(P_REV_TONE, 0.70f), S(P_SHIMMER, 0.60f), S(P_LEVEL, 0.38f) } },

/* ----------------------------------------------------------------------- */
/* PD: horror -- retro and modern horror soundtrack sounds               */
/* ----------------------------------------------------------------------- */

/* Something left in a box upstairs: a glass wavetable stack on a badly worn
 * tape, every voice drifting out of tune on its own. */
{ "PD Attic Dust", "pad", 136, {
    S(P_MODEL, M_WAVE), S(P_SHAPE, 0.57f), S(P_STACK, 0.50f), S(P_AGE, 0.55f), S(P_FLT_TYPE, F_LP12),
    S(P_CUTOFF, 0.55f), S(P_RESO, 0.10f), S(P_DRIVE, 0.15f), S(P_FLT_ENV, 0.20f), S(P_FLT_A, 0.60f),
    S(P_FLT_D, 0.80f), S(P_FLT_S, 0.60f), S(P_LFO_DEST, D_PITCH), S(P_LFO_RATE, 0.20f),
    S(P_LFO_DEPTH, 0.30f), S(P_VEL, 0.20f), S(P_AMP_A, 0.60f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f),
    S(P_AMP_R, 0.70f), S(P_CHO_MODE, C_ENS), S(P_CHO_MIX, 0.50f), S(P_VHS, 0.50f), S(P_TONE, 0.25f),
    S(P_REV_MIX, 0.50f), S(P_REV_SIZE, 0.75f), S(P_REV_TONE, 0.35f), S(P_LEVEL, 0.50f) } },

/* A dark low-voiced string section with a fifth above it, slow to speak, in
 * a hall far too big for it. Play it in minor. */
{ "PD Requiem", "pad", 137, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.55f), S(P_STACK, 0.45f), S(P_LAYER, L_FIFTH), S(P_BLEND, 0.35f),
    S(P_AGE, 0.25f), S(P_CUTOFF, 0.45f), S(P_RESO, 0.20f), S(P_DRIVE, 0.35f), S(P_FLT_ENV, 0.25f),
    S(P_FLT_A, 0.50f), S(P_FLT_D, 0.70f), S(P_FLT_S, 0.50f), S(P_LFO_DEST, D_FILTER),
    S(P_LFO_RATE, 0.15f), S(P_LFO_DEPTH, 0.25f), S(P_VEL, 0.30f), S(P_AMP_A, 0.55f), S(P_AMP_D, 0.70f),
    S(P_AMP_S, 0.90f), S(P_AMP_R, 0.65f), S(P_CHO_MODE, C_BOTH), S(P_CHO_MIX, 0.45f), S(P_TONE, 0.30f),
    S(P_REV_MIX, 0.45f), S(P_REV_SIZE, 0.85f), S(P_REV_TONE, 0.25f), S(P_LEVEL, 0.45f) } },

/* The big analog pad of a modern retro-horror score: a wide saw stack whose
 * filter opens over seconds, a slow phaser, a faint octave in the tail. */
{ "PD The Other Side", "pad", 138, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.60f), S(P_STACK, 0.60f), S(P_AGE, 0.25f), S(P_CUTOFF, 0.50f),
    S(P_RESO, 0.25f), S(P_DRIVE, 0.25f), S(P_FLT_ENV, 0.45f), S(P_FLT_A, 0.75f), S(P_FLT_D, 0.85f),
    S(P_FLT_S, 0.40f), S(P_VEL, 0.20f), S(P_AMP_A, 0.50f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.95f),
    S(P_AMP_R, 0.70f), S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.50f), S(P_PHS_DEPTH, 0.35f),
    S(P_PHS_RATE, 0.08f), S(P_VHS, 0.15f), S(P_TONE, 0.35f), S(P_REV_MIX, 0.55f), S(P_REV_SIZE, 0.80f),
    S(P_SHIMMER, 0.25f), S(P_LEVEL, 0.45f) } },

/* Modern sci-fi dread: bell partials an octave up, swinging slowly across the
 * room, dissolving into a huge bright shimmer. */
{ "PD Cold Storage", "pad", 139, {
    S(P_MODEL, M_BELL), S(P_SHAPE, 0.25f), S(P_STACK, 0.35f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.20f),
    S(P_AGE, 0.20f), S(P_CUTOFF, 0.60f), S(P_RESO, 0.10f), S(P_DRIVE, 0.10f), S(P_FLT_ENV, 0.30f),
    S(P_FLT_A, 0.70f), S(P_FLT_D, 0.80f), S(P_FLT_S, 0.60f), S(P_LFO_DEST, D_PAN),
    S(P_LFO_RATE, 0.30f), S(P_LFO_DEPTH, 0.50f), S(P_VEL, 0.20f), S(P_AMP_A, 0.70f), S(P_AMP_D, 0.70f),
    S(P_AMP_S, 0.85f), S(P_AMP_R, 0.80f), S(P_PHS_DEPTH, 0.20f), S(P_PHS_RATE, 0.05f),
    S(P_TONE, 0.35f), S(P_REV_MIX, 0.70f), S(P_REV_SIZE, 0.95f), S(P_REV_TONE, 0.60f),
    S(P_SHIMMER, 0.60f), S(P_LEVEL, 0.37f) } },

/* A thin, cold, flickering bed: a high-passed pulse stack whose envelope
 * closes as it opens, the filter twitching at random underneath. */
{ "PD Unease", "pad", 140, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.50f), S(P_STACK, 0.50f), S(P_AGE, 0.30f), S(P_FLT_TYPE, F_HP),
    S(P_CUTOFF, 0.35f), S(P_RESO, 0.30f), S(P_DRIVE, 0.20f), S(P_FLT_ENV, -0.30f), S(P_FLT_A, 0.60f),
    S(P_FLT_D, 0.80f), S(P_FLT_S, 0.50f), S(P_LFO_DEST, D_SH), S(P_LFO_RATE, 0.45f),
    S(P_LFO_DEPTH, 0.40f), S(P_VEL, 0.20f), S(P_AMP_A, 0.45f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f),
    S(P_AMP_R, 0.60f), S(P_CHO_MODE, C_II), S(P_CHO_MIX, 0.40f), S(P_VHS, 0.30f), S(P_TONE, 0.15f),
    S(P_DLY_TIME, T_4D), S(P_DLY_MIX, 0.18f), S(P_DLY_FB, 0.50f), S(P_REV_MIX, 0.45f),
    S(P_REV_SIZE, 0.70f), S(P_REV_TONE, 0.45f), S(P_LEVEL, 0.48f) } },

/* An empty house breathing: a comb wavetable with a third of its harmonics
 * missing, warped by Age, under a phaser that takes half a minute to turn. */
{ "PD Hollow House", "pad", 141, {
    S(P_MODEL, M_WAVE), S(P_SHAPE, 0.86f), S(P_STACK, 0.25f), S(P_AGE, 0.70f), S(P_CUTOFF, 0.50f),
    S(P_RESO, 0.35f), S(P_DRIVE, 0.25f), S(P_FLT_ENV, 0.35f), S(P_FLT_A, 0.80f), S(P_FLT_D, 0.90f),
    S(P_FLT_S, 0.40f), S(P_VEL, 0.20f), S(P_AMP_A, 0.75f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f),
    S(P_AMP_R, 0.80f), S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.30f), S(P_PHS_DEPTH, 0.45f),
    S(P_PHS_RATE, 0.04f), S(P_VHS, 0.25f), S(P_TONE, 0.25f), S(P_REV_MIX, 0.65f), S(P_REV_SIZE, 0.90f),
    S(P_REV_TONE, 0.20f), S(P_LEVEL, 0.50f) } },

/* ======================================================================= */
/* KEYS                                                                    */
/* ======================================================================= */

/* The FM electric piano: soft when played soft, barking when hit, with a
 * tine two octaves up. */
{ "KY Velvet", "keys", 20, {
    S(P_MODEL, M_FM), S(P_SHAPE, 0.20f), S(P_SHAPE_ENV, 0.35f), S(P_LAYER, L_OCT_2), S(P_BLEND, 0.14f), S(P_AGE, 0.10f),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.78f), S(P_RESO, 0.05f), S(P_DRIVE, 0.15f),
    S(P_FLT_ENV, 0.10f), S(P_FLT_D, 0.38f), S(P_FLT_S, 0.10f),
    S(P_VEL, 0.85f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.72f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.45f),
    S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.35f),
    S(P_REV_MIX, 0.25f), S(P_REV_SIZE, 0.50f), S(P_LEVEL, 0.61f) } },

/* A struck bell: inharmonic FM whose brightness falls away after the hit. */
{ "KY Chapel", "bell", 21, {
    S(P_MODEL, M_BELL), S(P_SHAPE, 0.26f), S(P_SHAPE_ENV, 0.40f), S(P_STACK, 0.10f), S(P_AGE, 0.10f),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.76f), S(P_RESO, 0.0f), S(P_DRIVE, 0.10f),
    S(P_FLT_ENV, 0.0f), S(P_FLT_D, 0.60f), S(P_FLT_S, 0.0f),
    S(P_VEL, 0.60f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.82f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.70f),
    S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.30f),
    S(P_DLY_TIME, T_4), S(P_DLY_MIX, 0.22f), S(P_DLY_FB, 0.40f),
    S(P_REV_MIX, 0.42f), S(P_REV_SIZE, 0.70f), S(P_REV_TONE, 0.65f), S(P_LEVEL, 0.56f) } },

/* The organ from the horror film: drawbars, a little overdrive, a slow
 * tremolo, in a stone room. */
{ "KY Giallo", "keys", 23, {
    S(P_MODEL, M_WAVE), S(P_SHAPE, 0.143f), S(P_STACK, 0.08f), S(P_LAYER, L_OCT_DN), S(P_BLEND, 0.30f), S(P_AGE, 0.35f),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.75f), S(P_RESO, 0.05f), S(P_DRIVE, 0.35f), S(P_FLT_ENV, 0.0f),
    S(P_LFO_DEST, D_AMP), S(P_LFO_RATE, 0.78f), S(P_LFO_DEPTH, 0.18f),
    S(P_VEL, 0.10f), S(P_AMP_A, 0.03f), S(P_AMP_D, 0.50f), S(P_AMP_S, 1.0f), S(P_AMP_R, 0.20f),
    S(P_CHO_MODE, C_ENS), S(P_CHO_MIX, 0.30f), S(P_PHS_DEPTH, 0.15f), S(P_PHS_RATE, 0.35f),
    S(P_REV_MIX, 0.40f), S(P_REV_SIZE, 0.72f), S(P_REV_TONE, 0.50f), S(P_LEVEL, 0.46f) } },

/* ----------------------------------------------------------------------- */
/* KEYS: where an effect is the sound                                      */
/* ----------------------------------------------------------------------- */

/* The electric piano in the lounge of a motel off the highway: tremolo, a
 * phaser someone left on, a little tape. */
{ "KY Tremolo Motel", "keys", 50, {
    S(P_MODEL, M_FM), S(P_SHAPE, 0.22f), S(P_SHAPE_ENV, 0.30f), S(P_AGE, 0.25f),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.75f), S(P_RESO, 0.05f), S(P_DRIVE, 0.20f),
    S(P_FLT_ENV, 0.10f), S(P_FLT_D, 0.40f), S(P_FLT_S, 0.10f),
    S(P_LFO_DEST, D_AMP), S(P_LFO_RATE, 0.74f), S(P_LFO_DEPTH, 0.50f),
    S(P_VEL, 0.80f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.72f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.45f),
    S(P_PHS_DEPTH, 0.50f), S(P_PHS_RATE, 0.40f), S(P_VHS, 0.35f),
    S(P_REV_MIX, 0.25f), S(P_REV_SIZE, 0.50f), S(P_LEVEL, 0.62f) } },

/* A music box in an empty house: small bright bells, triplet repeats, a tail
 * that rises. */
{ "KY Music Box", "bell", 51, {
    S(P_MODEL, M_BELL), S(P_SHAPE, 0.20f), S(P_SHAPE_ENV, 0.30f), S(P_AGE, 0.15f),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.80f), S(P_RESO, 0.0f), S(P_DRIVE, 0.10f),
    S(P_FLT_ENV, 0.0f), S(P_FLT_D, 0.45f), S(P_FLT_S, 0.0f),
    S(P_VEL, 0.60f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.60f),
    S(P_DLY_TIME, T_8T), S(P_DLY_MIX, 0.45f), S(P_DLY_FB, 0.60f),
    S(P_TONE, 0.30f), S(P_REV_MIX, 0.45f), S(P_REV_SIZE, 0.70f), S(P_REV_TONE, 0.75f), S(P_SHIMMER, 0.45f), S(P_LEVEL, 0.65f) } },

/* A soft piano recorded to cassette and found years later. */
{ "KY Tape Piano", "keys", 52, {
    S(P_MODEL, M_FM), S(P_SHAPE, 0.12f), S(P_SHAPE_ENV, 0.25f), S(P_STACK, 0.10f), S(P_AGE, 0.55f),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.68f), S(P_RESO, 0.05f), S(P_DRIVE, 0.25f),
    S(P_FLT_ENV, 0.10f), S(P_FLT_D, 0.50f), S(P_FLT_S, 0.10f),
    S(P_VEL, 0.75f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.68f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.50f),
    S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.30f), S(P_VHS, 0.70f), S(P_TONE, -0.15f),
    S(P_REV_MIX, 0.22f), S(P_REV_SIZE, 0.40f), S(P_REV_TONE, 0.35f), S(P_LEVEL, 0.54f) } },

/* A buzzy digital organ, pumped in eighths through the ensemble: hold a chord
 * and it plays the rhythm part. */
{ "KY Cathode", "pad", 53, {
    S(P_MODEL, M_WAVE), S(P_SHAPE, 0.86f), S(P_STACK, 0.10f), S(P_AGE, 0.20f),
    S(P_CUTOFF, 0.70f), S(P_RESO, 0.20f), S(P_DRIVE, 0.30f),
    S(P_FLT_ENV, 0.30f), S(P_FLT_D, 0.35f), S(P_FLT_S, 0.30f),
    S(P_VEL, 0.30f), S(P_AMP_A, 0.02f), S(P_AMP_D, 0.50f), S(P_AMP_S, 0.80f), S(P_AMP_R, 0.25f),
    S(P_CHO_MODE, C_ENS), S(P_CHO_MIX, 0.50f), S(P_PUMP, 0.70f), S(P_PUMP_RATE, PU_8),
    S(P_DLY_TIME, T_8), S(P_DLY_MIX, 0.20f), S(P_DLY_FB, 0.40f),
    S(P_TONE, 0.20f), S(P_REV_MIX, 0.20f), S(P_REV_SIZE, 0.45f), S(P_LEVEL, 0.55f) } },

/* ----------------------------------------------------------------------- */
/* KEYS: bigger and wilder                                                 */
/* ----------------------------------------------------------------------- */

/* The pipe organ at the end of the film: two ranks an octave apart in a room
 * with a five-second tail. Hold the chord. */
{ "KY Pipe Dream", "keys", 94, {
    S(P_MODEL, M_WAVE), S(P_SHAPE, 0.14f), S(P_STACK, 0.15f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.45f),
    S(P_AGE, 0.15f), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.85f), S(P_RESO, 0.05f), S(P_DRIVE, 0.20f),
    S(P_FLT_ENV, 0.0f), S(P_VEL, 0.10f), S(P_AMP_A, 0.08f), S(P_AMP_D, 0.50f), S(P_AMP_S, 1.00f),
    S(P_AMP_R, 0.40f), S(P_CHO_MODE, C_ENS), S(P_CHO_MIX, 0.20f), S(P_TONE, 0.10f), S(P_REV_MIX, 0.70f),
    S(P_REV_SIZE, 0.92f), S(P_REV_TONE, 0.45f), S(P_LEVEL, 0.33f) } },

/* The harpsichord from an Italian horror score: a thin pulse and its octave,
 * plucked, through a little phaser. */
{ "KY Goblin Clav", "keys", 95, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.85f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.30f), S(P_AGE, 0.20f),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.80f), S(P_RESO, 0.20f), S(P_DRIVE, 0.45f), S(P_FLT_ENV, 0.35f),
    S(P_FLT_D, 0.28f), S(P_FLT_S, 0.0f), S(P_VEL, 0.70f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.55f),
    S(P_AMP_S, 0.0f), S(P_AMP_R, 0.30f), S(P_PHS_DEPTH, 0.25f), S(P_PHS_RATE, 0.35f),
    S(P_REV_MIX, 0.30f), S(P_REV_SIZE, 0.50f), S(P_LEVEL, 0.54f) } },

/* Tubular bells: the big struck tube with a body an octave under it, on a
 * slightly worn tape. Play it slowly. */
{ "KY Tubular", "bell", 96, {
    S(P_MODEL, M_BELL), S(P_SHAPE, 0.48f), S(P_SHAPE_ENV, 0.25f), S(P_LAYER, L_OCT_DN),
    S(P_BLEND, 0.30f), S(P_AGE, 0.15f), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.72f), S(P_RESO, 0.05f),
    S(P_DRIVE, 0.15f), S(P_FLT_ENV, 0.0f), S(P_FLT_D, 0.55f), S(P_FLT_S, 0.0f), S(P_VEL, 0.70f),
    S(P_AMP_A, 0.0f), S(P_AMP_D, 0.78f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.65f), S(P_CHO_MODE, C_DIM),
    S(P_CHO_MIX, 0.20f), S(P_VHS, 0.20f), S(P_DLY_TIME, T_4), S(P_DLY_MIX, 0.18f), S(P_DLY_FB, 0.35f),
    S(P_REV_MIX, 0.50f), S(P_REV_SIZE, 0.75f), S(P_REV_TONE, 0.40f), S(P_LEVEL, 0.59f) } },

/* An electric piano with the amp turned up until chords break up, through a
 * phaser. Play it hard. */
{ "KY Wurli Burn", "keys", 97, {
    S(P_MODEL, M_FM), S(P_SHAPE, 0.30f), S(P_SHAPE_ENV, 0.45f), S(P_AGE, 0.15f), S(P_FLT_TYPE, F_LP12),
    S(P_CUTOFF, 0.70f), S(P_RESO, 0.05f), S(P_DRIVE, 0.78f), S(P_FLT_ENV, 0.10f), S(P_FLT_D, 0.40f),
    S(P_FLT_S, 0.10f), S(P_VEL, 0.90f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.65f), S(P_AMP_S, 0.0f),
    S(P_AMP_R, 0.40f), S(P_PHS_DEPTH, 0.40f), S(P_REV_MIX, 0.18f), S(P_REV_SIZE, 0.40f),
    S(P_LEVEL, 0.50f) } },

/* The digital keys of 1987: a glassy wavetable and its octave, a struck front
 * and a held body, chorus and a bright shimmering room. */
{ "KY Fantasia", "keys", 98, {
    S(P_MODEL, M_WAVE), S(P_SHAPE, 0.57f), S(P_SHAPE_ENV, -0.20f), S(P_STACK, 0.20f),
    S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.35f), S(P_AGE, 0.15f), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.85f),
    S(P_RESO, 0.05f), S(P_DRIVE, 0.10f), S(P_FLT_ENV, 0.10f), S(P_FLT_D, 0.50f), S(P_FLT_S, 0.20f),
    S(P_VEL, 0.60f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.35f), S(P_AMP_R, 0.55f),
    S(P_CHO_MODE, C_II), S(P_CHO_MIX, 0.50f), S(P_TONE, 0.20f), S(P_DLY_TIME, T_8D),
    S(P_DLY_MIX, 0.20f), S(P_REV_MIX, 0.50f), S(P_REV_SIZE, 0.75f), S(P_REV_TONE, 0.70f),
    S(P_SHIMMER, 0.40f), S(P_LEVEL, 0.41f) } },

/* The fairground organ in the abandoned park: square waves in fifths, badly
 * out of tune, shaking, on a tape that has seen better days. */
{ "KY Carousel", "keys", 99, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.02f), S(P_LAYER, L_FIFTH), S(P_BLEND, 0.20f), S(P_AGE, 0.80f),
    S(P_CUTOFF, 0.68f), S(P_RESO, 0.15f), S(P_DRIVE, 0.30f), S(P_FLT_ENV, 0.10f), S(P_LFO_RATE, 0.80f),
    S(P_LFO_DEPTH, 0.20f), S(P_VEL, 0.20f), S(P_AMP_A, 0.01f), S(P_AMP_D, 0.50f), S(P_AMP_S, 1.00f),
    S(P_AMP_R, 0.22f), S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.30f), S(P_VHS, 0.55f), S(P_TONE, -0.10f),
    S(P_REV_MIX, 0.30f), S(P_REV_SIZE, 0.50f), S(P_REV_TONE, 0.35f), S(P_LEVEL, 0.45f) } },

/* ----------------------------------------------------------------------- */
/* KY: horror -- retro and modern horror soundtrack sounds               */
/* ----------------------------------------------------------------------- */

/* The small-town-on-Halloween piano: a bright FM key with an octave on top,
 * short, in a plain room. Play it in 5/4. */
{ "KY Haddonfield", "keys", 142, {
    S(P_MODEL, M_FM), S(P_SHAPE, 0.30f), S(P_SHAPE_ENV, 0.35f), S(P_LAYER, L_OCT_UP),
    S(P_BLEND, 0.15f), S(P_AGE, 0.10f), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.70f), S(P_RESO, 0.10f),
    S(P_DRIVE, 0.15f), S(P_FLT_ENV, 0.30f), S(P_FLT_D, 0.45f), S(P_FLT_S, 0.10f), S(P_VEL, 0.60f),
    S(P_AMP_A, 0.0f), S(P_AMP_D, 0.62f), S(P_AMP_S, 0.15f), S(P_AMP_R, 0.45f), S(P_VHS, 0.10f),
    S(P_TONE, 0.05f), S(P_DLY_TIME, T_4), S(P_DLY_MIX, 0.10f), S(P_DLY_FB, 0.25f), S(P_REV_MIX, 0.30f),
    S(P_REV_SIZE, 0.55f), S(P_REV_TONE, 0.50f), S(P_LEVEL, 0.56f) } },

/* A wind-up box from the dead girl's room: bells an octave up, tines out of
 * tune with each other, played off a tape that is falling apart. */
{ "KY Cursed Music Box", "bell", 143, {
    S(P_MODEL, M_BELL), S(P_SHAPE, 0.35f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.30f), S(P_AGE, 0.45f),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.85f), S(P_RESO, 0.05f), S(P_DRIVE, 0.10f),
    S(P_FLT_ENV, 0.10f), S(P_FLT_D, 0.40f), S(P_FLT_S, 0.0f), S(P_VEL, 0.50f), S(P_AMP_A, 0.0f),
    S(P_AMP_D, 0.55f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.55f), S(P_VHS, 0.55f), S(P_TONE, 0.25f),
    S(P_REV_MIX, 0.35f), S(P_REV_SIZE, 0.60f), S(P_REV_TONE, 0.60f), S(P_LEVEL, 0.62f) } },

/* The pump organ in the parlour at the wake: a reedy wavetable with an octave
 * on top, slow to speak, the bellows wavering, on a tape in a small room. */
{ "KY Wake Harmonium", "keys", 144, {
    S(P_MODEL, M_WAVE), S(P_SHAPE, 0.43f), S(P_STACK, 0.15f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.20f),
    S(P_AGE, 0.15f), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.68f), S(P_RESO, 0.05f), S(P_DRIVE, 0.20f),
    S(P_FLT_ENV, 0.10f), S(P_FLT_D, 0.50f), S(P_FLT_S, 0.80f), S(P_LFO_DEST, D_SHAPE),
    S(P_LFO_RATE, 0.25f), S(P_LFO_DEPTH, 0.25f), S(P_VEL, 0.20f), S(P_AMP_A, 0.10f), S(P_AMP_D, 0.60f),
    S(P_AMP_S, 1.0f), S(P_AMP_R, 0.35f), S(P_TONE, -0.05f), S(P_REV_MIX, 0.30f), S(P_REV_SIZE, 0.45f),
    S(P_REV_TONE, 0.30f), S(P_VHS, 0.40f), S(P_LEVEL, 0.42f) } },

/* A modern lo-fi horror electric piano: soft FM tines, wobbling slowly out
 * of tune, the tape chewed. */
{ "KY Rotted Rhodes", "keys", 145, {
    S(P_MODEL, M_FM), S(P_SHAPE, 0.18f), S(P_SHAPE_ENV, 0.25f), S(P_AGE, 0.60f), S(P_CUTOFF, 0.55f),
    S(P_RESO, 0.10f), S(P_DRIVE, 0.25f), S(P_FLT_ENV, 0.20f), S(P_FLT_D, 0.50f), S(P_FLT_S, 0.20f),
    S(P_LFO_DEST, D_PITCH), S(P_LFO_RATE, 0.35f), S(P_LFO_DEPTH, 0.45f), S(P_VEL, 0.55f),
    S(P_AMP_A, 0.0f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.30f), S(P_AMP_R, 0.40f), S(P_CHO_MODE, C_I),
    S(P_CHO_MIX, 0.30f), S(P_VHS, 0.65f), S(P_TONE, -0.10f), S(P_REV_MIX, 0.30f), S(P_REV_SIZE, 0.50f),
    S(P_REV_TONE, 0.25f), S(P_LEVEL, 0.54f) } },

/* Tubular bells struck deep: a pure strike with an octave below, ringing a long
 * time in a hall, with quarter-note repeats. Tubular's twin. */
{ "KY Deep Tubular", "bell", 162, {
    S(P_MODEL, M_BELL), S(P_SHAPE, 0.20f), S(P_SHAPE_ENV, 0.40f), S(P_LAYER, L_OCT_DN),
    S(P_BLEND, 0.45f), S(P_AGE, 0.15f), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.72f), S(P_RESO, 0.05f),
    S(P_DRIVE, 0.15f), S(P_FLT_ENV, 0.0f), S(P_FLT_D, 0.55f), S(P_FLT_S, 0.0f), S(P_VEL, 0.70f),
    S(P_AMP_A, 0.0f), S(P_AMP_D, 0.85f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.65f), S(P_DLY_TIME, T_4),
    S(P_DLY_MIX, 0.18f), S(P_DLY_FB, 0.35f), S(P_REV_MIX, 0.50f), S(P_REV_SIZE, 0.75f),
    S(P_REV_TONE, 0.40f), S(P_LEVEL, 0.54f) } },

/* ======================================================================= */
/* PLUCK                                                                   */
/* ======================================================================= */

/* The arpeggio: a narrow pulse with a resonant blip, cut low so it sits on
 * top, in the second chorus and a dotted-eighth delay. */
{ "PL Neon Arp", "arp", 24, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.75f), S(P_LAYER, L_SUB_SQR), S(P_BLEND, 0.22f), S(P_AGE, 0.20f),
    S(P_CUTOFF, 0.58f), S(P_RESO, 0.32f), S(P_DRIVE, 0.18f),
    S(P_FLT_ENV, 0.32f), S(P_FLT_D, 0.40f), S(P_FLT_S, 0.0f),
    S(P_VEL, 0.50f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.46f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.42f),
    S(P_CHO_MODE, C_II), S(P_CHO_MIX, 0.50f),
    S(P_DLY_TIME, T_8D), S(P_DLY_MIX, 0.30f), S(P_DLY_FB, 0.45f),
    S(P_TONE, 0.50f), S(P_REV_MIX, 0.22f), S(P_REV_SIZE, 0.50f), S(P_LEVEL, 0.66f) } },

/* Two pulses a few cents apart and a sub, with no filter envelope at all: the
 * filter is opened sixteen times a bar by the synced ramp. Hold a chord. */
{ "PL Signal", "arp", 25, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.02f), S(P_STACK, 0.12f), S(P_LAYER, L_SUB_SQR), S(P_BLEND, 0.35f), S(P_AGE, 0.30f),
    S(P_CUTOFF, 0.46f), S(P_RESO, 0.20f), S(P_DRIVE, 0.22f), S(P_FLT_ENV, 0.0f),
    S(P_LFO_DEST, D_RAMP), S(P_LFO_RATE, 0.70f), S(P_LFO_DEPTH, 0.50f),
    S(P_VEL, 0.30f), S(P_AMP_A, 0.01f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.80f), S(P_AMP_R, 0.30f),
    S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.25f),
    S(P_DLY_TIME, T_8), S(P_DLY_MIX, 0.12f), S(P_DLY_FB, 0.35f),
    S(P_REV_MIX, 0.25f), S(P_REV_SIZE, 0.55f), S(P_LEVEL, 0.54f) } },

/* A percussive, metallic arp voice: the filter envelope is nearly all of it,
 * and the delay throws it side to side in sixteenths. */
{ "PL Ricochet", "arp", 26, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.35f), S(P_AGE, 0.15f),
    S(P_CUTOFF, 0.30f), S(P_RESO, 0.32f), S(P_DRIVE, 0.35f),
    S(P_FLT_ENV, 0.88f), S(P_FLT_D, 0.26f), S(P_FLT_S, 0.0f),
    S(P_VEL, 0.60f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.38f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.30f),
    S(P_DLY_TIME, T_16), S(P_DLY_MIX, 0.30f), S(P_DLY_FB, 0.50f),
    S(P_TONE, 0.20f), S(P_REV_MIX, 0.20f), S(P_REV_SIZE, 0.45f), S(P_LEVEL, 0.70f) } },

/* Chord stabs that open and fall shut, in chorus and a quarter-note delay. */
{ "PL Palm Shadows", "keys", 27, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_STACK, 0.08f), S(P_AGE, 0.25f),
    S(P_CUTOFF, 0.42f), S(P_RESO, 0.12f), S(P_DRIVE, 0.18f),
    S(P_FLT_ENV, 0.72f), S(P_FLT_A, 0.30f), S(P_FLT_D, 0.52f), S(P_FLT_S, 0.20f),
    S(P_VEL, 0.50f), S(P_AMP_A, 0.02f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.30f), S(P_AMP_R, 0.50f),
    S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.45f),
    S(P_DLY_TIME, T_4), S(P_DLY_MIX, 0.30f), S(P_DLY_FB, 0.45f),
    S(P_TONE, 0.55f), S(P_REV_MIX, 0.30f), S(P_REV_SIZE, 0.60f), S(P_LEVEL, 0.48f) } },

/* ----------------------------------------------------------------------- */
/* PLUCK: where an effect is the sound                                     */
/* ----------------------------------------------------------------------- */

/* One note in, a figure out: dotted-eighth repeats as loud as the note,
 * chorused, falling away slowly. Play slow, even notes. */
{ "PL Dotted Dream", "bell", 54, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_STACK, 0.08f), S(P_AGE, 0.20f),
    S(P_CUTOFF, 0.50f), S(P_RESO, 0.20f), S(P_DRIVE, 0.18f),
    S(P_FLT_ENV, 0.50f), S(P_FLT_D, 0.40f), S(P_FLT_S, 0.0f),
    S(P_VEL, 0.50f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.50f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.45f),
    S(P_CHO_MODE, C_II), S(P_CHO_MIX, 0.45f),
    S(P_DLY_TIME, T_8D), S(P_DLY_MIX, 0.55f), S(P_DLY_FB, 0.65f),
    S(P_TONE, 0.40f), S(P_REV_MIX, 0.30f), S(P_REV_SIZE, 0.60f), S(P_LEVEL, 0.70f) } },

/* Small glass notes landing anywhere across the field, each one echoed in
 * triplets into a long bright tail. */
{ "PL Raindrops", "bell", 55, {
    S(P_MODEL, M_BELL), S(P_SHAPE, 0.15f), S(P_SHAPE_ENV, 0.35f), S(P_AGE, 0.20f),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.80f), S(P_RESO, 0.0f), S(P_DRIVE, 0.10f),
    S(P_FLT_ENV, 0.0f), S(P_FLT_D, 0.35f), S(P_FLT_S, 0.0f),
    S(P_LFO_DEST, D_PAN), S(P_LFO_RATE, 0.62f), S(P_LFO_DEPTH, 0.80f),
    S(P_VEL, 0.60f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.50f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.50f),
    S(P_DLY_TIME, T_8T), S(P_DLY_MIX, 0.40f), S(P_DLY_FB, 0.55f),
    S(P_TONE, 0.30f), S(P_REV_MIX, 0.50f), S(P_REV_SIZE, 0.75f), S(P_REV_TONE, 0.70f), S(P_SHIMMER, 0.35f), S(P_LEVEL, 0.86f) } },

/* A blip that sets off an avalanche: sixteenth repeats with the feedback
 * most of the way up. */
{ "PL Cascade", "bell", 56, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.60f), S(P_AGE, 0.15f),
    S(P_CUTOFF, 0.60f), S(P_RESO, 0.22f), S(P_DRIVE, 0.18f),
    S(P_FLT_ENV, 0.35f), S(P_FLT_D, 0.30f), S(P_FLT_S, 0.0f),
    S(P_VEL, 0.50f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.42f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.30f),
    S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.30f),
    S(P_DLY_TIME, T_16), S(P_DLY_MIX, 0.55f), S(P_DLY_FB, 0.78f),
    S(P_TONE, 0.35f), S(P_REV_MIX, 0.35f), S(P_REV_SIZE, 0.55f), S(P_REV_TONE, 0.60f), S(P_LEVEL, 0.72f) } },

/* Hold a chord. The filter opens on every eighth, the level ducks on every
 * beat, and a dotted delay fills what is left. */
{ "PL Pulsar", "pad", 57, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_STACK, 0.30f), S(P_LAYER, L_SUB_SQR), S(P_BLEND, 0.20f), S(P_AGE, 0.20f),
    S(P_CUTOFF, 0.42f), S(P_RESO, 0.30f), S(P_DRIVE, 0.25f), S(P_FLT_ENV, 0.0f),
    S(P_LFO_DEST, D_RAMP), S(P_LFO_RATE, 0.30f), S(P_LFO_DEPTH, 0.60f),
    S(P_VEL, 0.30f), S(P_AMP_A, 0.01f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.85f), S(P_AMP_R, 0.35f),
    S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.30f), S(P_PUMP, 0.50f), S(P_PUMP_RATE, PU_4),
    S(P_DLY_TIME, T_8D), S(P_DLY_MIX, 0.25f), S(P_DLY_FB, 0.50f),
    S(P_TONE, 0.15f), S(P_REV_MIX, 0.25f), S(P_REV_SIZE, 0.55f), S(P_LEVEL, 0.44f) } },

/* A plucked string in both choruses at once, nearly all the way up. */
{ "PL Chorus Harp", "arp", 58, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.30f), S(P_STACK, 0.10f), S(P_AGE, 0.25f),
    S(P_CUTOFF, 0.60f), S(P_RESO, 0.10f), S(P_DRIVE, 0.15f),
    S(P_FLT_ENV, 0.40f), S(P_FLT_D, 0.50f), S(P_FLT_S, 0.0f),
    S(P_VEL, 0.55f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.66f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.60f),
    S(P_CHO_MODE, C_BOTH), S(P_CHO_MIX, 0.85f),
    S(P_DLY_TIME, T_4), S(P_DLY_MIX, 0.30f), S(P_DLY_FB, 0.45f),
    S(P_TONE, 0.30f), S(P_REV_MIX, 0.35f), S(P_REV_SIZE, 0.60f), S(P_REV_TONE, 0.55f), S(P_LEVEL, 0.57f) } },

/* A bright square blip in octaves, phased and repeated in sixteenths: the
 * sound of a cabinet in the corner. */
{ "PL Arcade", "arp", 59, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.05f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.25f), S(P_AGE, 0.10f),
    S(P_CUTOFF, 0.66f), S(P_RESO, 0.20f), S(P_DRIVE, 0.20f),
    S(P_FLT_ENV, 0.35f), S(P_FLT_D, 0.32f), S(P_FLT_S, 0.0f),
    S(P_VEL, 0.50f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.45f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.30f),
    S(P_PHS_DEPTH, 0.60f), S(P_PHS_RATE, 0.60f),
    S(P_DLY_TIME, T_16), S(P_DLY_MIX, 0.45f), S(P_DLY_FB, 0.50f),
    S(P_TONE, 0.25f), S(P_REV_MIX, 0.15f), S(P_REV_SIZE, 0.40f), S(P_LEVEL, 0.57f) } },

/* ----------------------------------------------------------------------- */
/* PLUCK: bigger and wilder                                                */
/* ----------------------------------------------------------------------- */

/* The chorused guitar of every darkwave record: a twangy sync pluck and its
 * octave in both choruses. */
{ "PL Jangle '85", "arp", 100, {
    S(P_MODEL, M_SYNC), S(P_SHAPE, 0.12f), S(P_SHAPE_ENV, 0.30f), S(P_STACK, 0.15f),
    S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.30f), S(P_AGE, 0.30f), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.78f),
    S(P_RESO, 0.10f), S(P_DRIVE, 0.20f), S(P_FLT_ENV, 0.25f), S(P_FLT_D, 0.30f), S(P_FLT_S, 0.0f),
    S(P_VEL, 0.60f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.45f),
    S(P_CHO_MODE, C_BOTH), S(P_CHO_MIX, 0.50f), S(P_TONE, 0.25f), S(P_DLY_TIME, T_8D),
    S(P_DLY_MIX, 0.28f), S(P_REV_MIX, 0.30f), S(P_REV_SIZE, 0.55f), S(P_LEVEL, 0.59f) } },

/* A marimba after dark: the knock of the mallet is thirty milliseconds of FM,
 * then a round wooden note. */
{ "PL Marimba Nights", "bell", 101, {
    S(P_MODEL, M_FM), S(P_SHAPE, 0.15f), S(P_SHAPE_ENV, 0.60f), S(P_LAYER, L_SUB_SINE),
    S(P_BLEND, 0.20f), S(P_AGE, 0.10f), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.74f), S(P_RESO, 0.05f),
    S(P_DRIVE, 0.15f), S(P_FLT_ENV, 0.10f), S(P_FLT_D, 0.16f), S(P_FLT_S, 0.0f), S(P_VEL, 0.70f),
    S(P_AMP_A, 0.0f), S(P_AMP_D, 0.40f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.35f), S(P_CHO_MODE, C_DIM),
    S(P_CHO_MIX, 0.25f), S(P_DLY_TIME, T_8D), S(P_DLY_MIX, 0.22f), S(P_DLY_FB, 0.35f),
    S(P_REV_MIX, 0.28f), S(P_REV_SIZE, 0.55f), S(P_LEVEL, 0.82f) } },

/* A plucked string section: a handful of short saws, chorused, in a hall. */
{ "PL Pizzicato", "arp", 102, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_STACK, 0.30f), S(P_LAYER, L_SUB_SQR), S(P_BLEND, 0.0f),
    S(P_AGE, 0.30f), S(P_CUTOFF, 0.62f), S(P_RESO, 0.10f), S(P_DRIVE, 0.20f), S(P_FLT_ENV, 0.30f),
    S(P_FLT_D, 0.22f), S(P_FLT_S, 0.0f), S(P_VEL, 0.70f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.36f),
    S(P_AMP_S, 0.0f), S(P_AMP_R, 0.30f), S(P_CHO_MODE, C_II), S(P_CHO_MIX, 0.30f), S(P_TONE, 0.30f),
    S(P_REV_MIX, 0.45f), S(P_REV_SIZE, 0.60f), S(P_REV_TONE, 0.55f), S(P_LEVEL, 0.70f) } },

/* The festival pluck: a supersaw that snaps open and shut, pumped a little,
 * its repeats climbing into a shimmer. */
{ "PL Skyrocket", "arp", 103, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_STACK, 0.80f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.20f),
    S(P_AGE, 0.20f), S(P_CUTOFF, 0.46f), S(P_RESO, 0.15f), S(P_DRIVE, 0.35f), S(P_FLT_ENV, 0.60f),
    S(P_FLT_D, 0.30f), S(P_FLT_S, 0.0f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.46f), S(P_AMP_S, 0.0f),
    S(P_AMP_R, 0.40f), S(P_PUMP, 0.25f), S(P_TONE, 0.45f), S(P_DLY_TIME, T_8D), S(P_DLY_MIX, 0.40f),
    S(P_DLY_FB, 0.55f), S(P_REV_MIX, 0.35f), S(P_REV_SIZE, 0.65f), S(P_SHIMMER, 0.15f),
    S(P_LEVEL, 0.66f) } },

/* A koto, more or less: a hard twang at the front of every note, slightly out
 * of tune, in a dark room. */
{ "PL Koto", "bell", 104, {
    S(P_MODEL, M_SYNC), S(P_SHAPE, 0.25f), S(P_SHAPE_ENV, 0.55f), S(P_AGE, 0.40f),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.80f), S(P_RESO, 0.10f), S(P_DRIVE, 0.25f), S(P_FLT_ENV, 0.20f),
    S(P_FLT_D, 0.14f), S(P_FLT_S, 0.0f), S(P_VEL, 0.80f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.50f),
    S(P_AMP_S, 0.0f), S(P_AMP_R, 0.40f), S(P_TONE, 0.15f), S(P_DLY_TIME, T_4), S(P_DLY_MIX, 0.20f),
    S(P_REV_MIX, 0.35f), S(P_REV_SIZE, 0.60f), S(P_REV_TONE, 0.40f), S(P_LEVEL, 0.77f) } },

/* The palm-muted chug: a short, driven power chord. Play it low, in
 * sixteenths. */
{ "PL Chug", "bass", 105, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.70f), S(P_STACK, 0.12f), S(P_LAYER, L_FIFTH), S(P_BLEND, 0.35f),
    S(P_AGE, 0.15f), S(P_VOICE, V_MONO), S(P_CUTOFF, 0.46f), S(P_RESO, 0.20f), S(P_DRIVE, 0.88f),
    S(P_FLT_ENV, 0.50f), S(P_FLT_D, 0.20f), S(P_FLT_S, 0.0f), S(P_VEL, 0.70f), S(P_AMP_A, 0.0f),
    S(P_AMP_D, 0.36f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.12f), S(P_REV_MIX, 0.10f), S(P_REV_SIZE, 0.30f),
    S(P_REV_TONE, 0.45f), S(P_LEVEL, 0.85f) } },

/* Notes that drop into place from above and scatter left and right in
 * sixteenth repeats. */
{ "PL Photon", "arp", 106, {
    S(P_MODEL, M_ZAP), S(P_SHAPE, 0.18f), S(P_LAYER, L_OCT_2), S(P_BLEND, 0.15f), S(P_FLT_TYPE, F_LP12),
    S(P_CUTOFF, 0.80f), S(P_RESO, 0.05f), S(P_DRIVE, 0.50f), S(P_FLT_ENV, 0.10f), S(P_LFO_DEST, D_PAN),
    S(P_LFO_RATE, 0.60f), S(P_LFO_DEPTH, 0.50f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.38f), S(P_AMP_S, 0.0f),
    S(P_AMP_R, 0.30f), S(P_TONE, 0.15f), S(P_DLY_TIME, T_16), S(P_DLY_MIX, 0.40f), S(P_DLY_FB, 0.60f),
    S(P_REV_MIX, 0.30f), S(P_REV_SIZE, 0.50f), S(P_REV_TONE, 0.65f), S(P_LEVEL, 0.86f) } },

/* A bell that evaporates: full for an instant, then a HIGH-pass climbs
 * through it and only the glitter is left for the echoes. */
{ "PL Vanishing Point", "arp", 107, {
    S(P_MODEL, M_BELL), S(P_SHAPE, 0.30f), S(P_LAYER, L_OCT_2), S(P_BLEND, 0.30f), S(P_AGE, 0.20f),
    S(P_FLT_TYPE, F_HP), S(P_CUTOFF, 0.50f), S(P_RESO, 0.35f), S(P_DRIVE, 0.15f), S(P_FLT_ENV, -0.50f),
    S(P_FLT_D, 0.35f), S(P_FLT_S, 0.0f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.50f), S(P_AMP_S, 0.0f),
    S(P_AMP_R, 0.40f), S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.30f), S(P_DLY_TIME, T_8D),
    S(P_DLY_MIX, 0.35f), S(P_DLY_FB, 0.50f), S(P_REV_MIX, 0.30f), S(P_REV_SIZE, 0.55f),
    S(P_SHAPE_ENV, 0.20f), S(P_LEVEL, 0.84f) } },

/* ----------------------------------------------------------------------- */
/* PL: horror -- retro and modern horror soundtrack sounds               */
/* ----------------------------------------------------------------------- */

/* The Italian prog-horror arpeggio: resonant sync with an octave below,
 * snapping shut, through a quick phaser and a dotted echo. */
{ "PL Profondo", "arp", 146, {
    S(P_MODEL, M_SYNC), S(P_SHAPE, 0.25f), S(P_SHAPE_ENV, 0.30f), S(P_LAYER, L_OCT_DN),
    S(P_BLEND, 0.25f), S(P_AGE, 0.15f), S(P_CUTOFF, 0.50f), S(P_RESO, 0.40f), S(P_DRIVE, 0.45f),
    S(P_FLT_ENV, 0.50f), S(P_FLT_D, 0.30f), S(P_FLT_S, 0.05f), S(P_VEL, 0.50f), S(P_AMP_A, 0.0f),
    S(P_AMP_D, 0.42f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.25f), S(P_PHS_DEPTH, 0.30f),
    S(P_PHS_RATE, 0.50f), S(P_TONE, 0.10f), S(P_DLY_MIX, 0.25f), S(P_DLY_FB, 0.35f),
    S(P_REV_MIX, 0.15f), S(P_REV_SIZE, 0.40f), S(P_LEVEL, 0.76f) } },

/* Glass ticks that never quite line up: a sparse wavetable pluck answered by
 * triplet repeats in a high stone room. */
{ "PL Clock Tower", "arp", 147, {
    S(P_MODEL, M_WAVE), S(P_SHAPE, 0.57f), S(P_AGE, 0.25f), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.65f),
    S(P_RESO, 0.20f), S(P_DRIVE, 0.10f), S(P_FLT_ENV, 0.35f), S(P_FLT_D, 0.35f), S(P_FLT_S, 0.0f),
    S(P_VEL, 0.50f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.40f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.35f),
    S(P_TONE, 0.20f), S(P_DLY_TIME, T_8T), S(P_DLY_MIX, 0.35f), S(P_DLY_FB, 0.55f),
    S(P_REV_MIX, 0.30f), S(P_REV_SIZE, 0.65f), S(P_REV_TONE, 0.55f), S(P_LEVEL, 0.76f) } },

/* The slow-walking-thing pluck of modern synth horror: a wide pulse slammed
 * into the clipper, ducking in eighths, never letting up. */
{ "PL Follower", "arp", 148, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.60f), S(P_STACK, 0.20f), S(P_AGE, 0.20f), S(P_CUTOFF, 0.48f),
    S(P_RESO, 0.50f), S(P_DRIVE, 0.75f), S(P_FLT_ENV, 0.60f), S(P_FLT_D, 0.28f), S(P_FLT_S, 0.0f),
    S(P_VEL, 0.40f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.38f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.20f),
    S(P_PUMP, 0.35f), S(P_PUMP_RATE, PU_8), S(P_VHS, 0.30f), S(P_TONE, 0.05f), S(P_DLY_TIME, T_16),
    S(P_DLY_MIX, 0.15f), S(P_DLY_FB, 0.30f), S(P_REV_MIX, 0.20f), S(P_REV_SIZE, 0.50f),
    S(P_REV_TONE, 0.30f), S(P_LEVEL, 0.64f) } },

/* Hollow wooden knocks with a fifth in them, from somewhere underground: an FM
 * pluck whose bite dies first, in a long dark cave. */
{ "PL Bone Marimba", "arp", 149, {
    S(P_MODEL, M_FM), S(P_SHAPE, 0.50f), S(P_SHAPE_ENV, 0.60f), S(P_LAYER, L_FIFTH), S(P_BLEND, 0.12f),
    S(P_AGE, 0.15f), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.60f), S(P_RESO, 0.10f), S(P_DRIVE, 0.15f),
    S(P_FLT_ENV, 0.20f), S(P_FLT_D, 0.25f), S(P_FLT_S, 0.0f), S(P_VEL, 0.60f), S(P_AMP_A, 0.0f),
    S(P_AMP_D, 0.45f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.40f), S(P_CHO_MODE, C_ENS), S(P_CHO_MIX, 0.25f),
    S(P_TONE, 0.10f), S(P_DLY_TIME, T_4), S(P_DLY_MIX, 0.12f), S(P_DLY_FB, 0.60f), S(P_REV_MIX, 0.40f),
    S(P_REV_SIZE, 0.70f), S(P_REV_TONE, 0.20f), S(P_LEVEL, 0.74f) } },

/* A hollow, harp-like pluck of odd harmonics, doubled by the two choruses at
 * once, ringing out. Chorus Harp's twin. */
{ "PL Hollow Harp", "arp", 161, {
    S(P_MODEL, M_WAVE), S(P_SHAPE, 0.29f), S(P_STACK, 0.10f), S(P_AGE, 0.25f), S(P_CUTOFF, 0.60f),
    S(P_RESO, 0.10f), S(P_DRIVE, 0.15f), S(P_FLT_ENV, 0.40f), S(P_FLT_D, 0.50f), S(P_FLT_S, 0.0f),
    S(P_VEL, 0.55f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.66f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.60f),
    S(P_CHO_MODE, C_BOTH), S(P_CHO_MIX, 0.80f), S(P_DLY_TIME, T_4), S(P_DLY_MIX, 0.30f),
    S(P_DLY_FB, 0.45f), S(P_TONE, 0.30f), S(P_REV_MIX, 0.35f), S(P_REV_SIZE, 0.60f),
    S(P_REV_TONE, 0.55f), S(P_LEVEL, 0.43f) } },

/* Raindrops' twin: water drops landing anywhere across the field, each a tiny falling blip,
 * echoed in triplets into a long tail a shade darker than Raindrops'. */
{ "PL Droplets", "bell", 160, {
    S(P_MODEL, M_ZAP), S(P_SHAPE, 0.22f), S(P_AGE, 0.20f), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.80f),
    S(P_RESO, 0.0f), S(P_DRIVE, 0.10f), S(P_FLT_ENV, 0.0f), S(P_FLT_D, 0.35f), S(P_FLT_S, 0.0f),
    S(P_LFO_DEST, D_PAN), S(P_LFO_RATE, 0.62f), S(P_LFO_DEPTH, 0.80f), S(P_VEL, 0.60f),
    S(P_AMP_A, 0.0f), S(P_AMP_D, 0.50f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.50f), S(P_DLY_TIME, T_8T),
    S(P_DLY_MIX, 0.40f), S(P_DLY_FB, 0.55f), S(P_TONE, 0.30f), S(P_REV_MIX, 0.50f),
    S(P_REV_SIZE, 0.75f), S(P_REV_TONE, 0.65f), S(P_SHIMMER, 0.35f), S(P_LEVEL, 0.88f) } },

/* ======================================================================= */
/* BRASS                                                                   */
/* ======================================================================= */

/* The big poly brass stab: saws nine cents apart, the filter nearly open,
 * gone the moment you let go. */
{ "BR Stadium", "keys", 28, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.50f), S(P_STACK, 0.11f), S(P_AGE, 0.20f),
    S(P_CUTOFF, 0.74f), S(P_RESO, 0.25f), S(P_DRIVE, 0.30f),
    S(P_FLT_ENV, 0.22f), S(P_FLT_A, 0.30f), S(P_FLT_D, 0.50f), S(P_FLT_S, 0.60f),
    S(P_VEL, 0.50f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.90f), S(P_AMP_S, 0.85f), S(P_AMP_R, 0.20f),
    S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.15f),
    S(P_TONE, 0.10f), S(P_REV_MIX, 0.30f), S(P_REV_SIZE, 0.50f), S(P_LEVEL, 0.45f) } },

/* The slow brass of a rain-soaked future: two pulses an octave apart that
 * swell open, answer to how hard you play and press, and hang in a huge room. */
{ "BR Off-World", "pad", 29, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.05f), S(P_STACK, 0.10f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.40f), S(P_AGE, 0.35f),
    S(P_CUTOFF, 0.52f), S(P_RESO, 0.15f), S(P_DRIVE, 0.20f),
    S(P_FLT_ENV, 0.42f), S(P_FLT_A, 0.72f), S(P_FLT_D, 0.90f), S(P_FLT_S, 0.70f),
    S(P_LFO_DEST, D_SHAPE), S(P_LFO_RATE, 0.40f), S(P_LFO_DEPTH, 0.25f),
    S(P_VEL, 0.75f), S(P_AMP_A, 0.60f), S(P_AMP_D, 0.75f), S(P_AMP_S, 0.65f), S(P_AMP_R, 0.55f),
    S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.30f),
    S(P_DLY_TIME, T_4D), S(P_DLY_MIX, 0.18f), S(P_DLY_FB, 0.40f),
    S(P_REV_MIX, 0.50f), S(P_REV_SIZE, 0.80f), S(P_REV_TONE, 0.55f), S(P_LEVEL, 0.46f) } },

/* ----------------------------------------------------------------------- */
/* BRASS: bigger and wilder                                                */
/* ----------------------------------------------------------------------- */

/* The brass riff everybody knows: saw and square with an octave on top, a
 * quick blat at the front. */
{ "BR Countdown", "keys", 108, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.75f), S(P_STACK, 0.22f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.25f),
    S(P_AGE, 0.20f), S(P_CUTOFF, 0.64f), S(P_RESO, 0.12f), S(P_DRIVE, 0.30f), S(P_FLT_ENV, 0.30f),
    S(P_FLT_A, 0.10f), S(P_FLT_D, 0.50f), S(P_FLT_S, 0.60f), S(P_VEL, 0.60f), S(P_AMP_A, 0.02f),
    S(P_AMP_D, 0.70f), S(P_AMP_S, 0.85f), S(P_AMP_R, 0.28f), S(P_CHO_MODE, C_II), S(P_CHO_MIX, 0.35f),
    S(P_TONE, 0.15f), S(P_DLY_TIME, T_8), S(P_DLY_MIX, 0.20f), S(P_DLY_FB, 0.35f), S(P_REV_MIX, 0.35f),
    S(P_REV_SIZE, 0.60f), S(P_LEVEL, 0.44f) } },

/* FM brass: the FM amount swells in the way a section leans into a note, an
 * octave under for weight. */
{ "BR FM Section", "keys", 109, {
    S(P_MODEL, M_FM), S(P_SHAPE, 0.22f), S(P_SHAPE_ENV, 0.50f), S(P_STACK, 0.20f), S(P_LAYER, L_OCT_DN),
    S(P_BLEND, 0.30f), S(P_AGE, 0.20f), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.78f), S(P_RESO, 0.05f),
    S(P_DRIVE, 0.25f), S(P_FLT_ENV, 0.10f), S(P_FLT_A, 0.15f), S(P_FLT_D, 0.50f), S(P_FLT_S, 0.55f),
    S(P_VEL, 0.70f), S(P_AMP_A, 0.02f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.85f), S(P_AMP_R, 0.25f),
    S(P_CHO_MODE, C_ENS), S(P_CHO_MIX, 0.30f), S(P_REV_MIX, 0.30f), S(P_REV_SIZE, 0.50f),
    S(P_LEVEL, 0.51f) } },

/* A fanfare in open fifths, a little overdriven, in a big hall. */
{ "BR Fanfare", "keys", 110, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.55f), S(P_STACK, 0.15f), S(P_LAYER, L_FIFTH), S(P_BLEND, 0.45f),
    S(P_AGE, 0.20f), S(P_CUTOFF, 0.62f), S(P_RESO, 0.12f), S(P_DRIVE, 0.45f), S(P_FLT_ENV, 0.40f),
    S(P_FLT_A, 0.20f), S(P_FLT_D, 0.50f), S(P_FLT_S, 0.55f), S(P_VEL, 0.60f), S(P_AMP_A, 0.03f),
    S(P_AMP_D, 0.70f), S(P_AMP_S, 0.85f), S(P_AMP_R, 0.30f), S(P_TONE, 0.05f), S(P_DLY_TIME, T_4),
    S(P_DLY_MIX, 0.15f), S(P_DLY_FB, 0.30f), S(P_REV_MIX, 0.45f), S(P_REV_SIZE, 0.70f),
    S(P_REV_TONE, 0.55f), S(P_LEVEL, 0.50f) } },

/* Horns with a rasp: the sync sweep stands in for the lip buzz at the front
 * of each note. */
{ "BR Sync Horns", "keys", 111, {
    S(P_MODEL, M_SYNC), S(P_SHAPE, 0.15f), S(P_SHAPE_ENV, 0.30f), S(P_STACK, 0.18f), S(P_AGE, 0.20f),
    S(P_CUTOFF, 0.64f), S(P_RESO, 0.15f), S(P_DRIVE, 0.55f), S(P_FLT_ENV, 0.30f), S(P_FLT_A, 0.20f),
    S(P_FLT_D, 0.50f), S(P_FLT_S, 0.50f), S(P_VEL, 0.70f), S(P_AMP_A, 0.02f), S(P_AMP_D, 0.70f),
    S(P_AMP_S, 0.85f), S(P_AMP_R, 0.25f), S(P_CHO_MODE, C_I), S(P_CHO_MIX, 0.30f), S(P_REV_MIX, 0.25f),
    S(P_REV_SIZE, 0.45f), S(P_LEVEL, 0.44f) } },

/* Soft horns a long way off: nearly a triangle, a slow filter, a dark hall.
 * The quiet brass for under a voice. */
{ "BR Velvet Horn", "pad", 112, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.15f), S(P_LAYER, L_OCT_DN), S(P_BLEND, 0.20f), S(P_AGE, 0.25f),
    S(P_CUTOFF, 0.50f), S(P_RESO, 0.08f), S(P_DRIVE, 0.15f), S(P_FLT_ENV, 0.30f), S(P_FLT_A, 0.40f),
    S(P_FLT_D, 0.60f), S(P_FLT_S, 0.60f), S(P_VEL, 0.70f), S(P_AMP_A, 0.45f), S(P_AMP_D, 0.70f),
    S(P_AMP_S, 0.85f), S(P_AMP_R, 0.50f), S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.30f), S(P_TONE, 0.10f),
    S(P_REV_MIX, 0.50f), S(P_REV_SIZE, 0.80f), S(P_REV_TONE, 0.40f), S(P_LEVEL, 0.57f) } },

/* One note of this is a whole section: a wide stack and its lower octave,
 * mono, gliding, pushed into the clipper. */
{ "BR Titan", "lead", 113, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.60f), S(P_STACK, 0.60f), S(P_LAYER, L_OCT_DN), S(P_BLEND, 0.40f),
    S(P_AGE, 0.25f), S(P_VOICE, V_G110), S(P_CUTOFF, 0.56f), S(P_RESO, 0.10f), S(P_DRIVE, 0.65f),
    S(P_FLT_ENV, 0.45f), S(P_FLT_A, 0.30f), S(P_FLT_D, 0.60f), S(P_FLT_S, 0.60f), S(P_AMP_A, 0.04f),
    S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.35f), S(P_DLY_TIME, T_4D), S(P_DLY_MIX, 0.20f),
    S(P_REV_MIX, 0.40f), S(P_REV_SIZE, 0.70f), S(P_LEVEL, 0.58f) } },

/* ----------------------------------------------------------------------- */
/* BR: horror -- retro and modern horror soundtrack sounds               */
/* ----------------------------------------------------------------------- */

/* The hotel's opening brass: driven saws with an octave below, a slow
 * swell and a slow vibrato, the requiem chant played on a synthesizer. */
{ "BR Overlook", "pad", 150, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.60f), S(P_STACK, 0.25f), S(P_LAYER, L_OCT_DN), S(P_BLEND, 0.35f),
    S(P_AGE, 0.20f), S(P_CUTOFF, 0.42f), S(P_RESO, 0.15f), S(P_DRIVE, 0.50f), S(P_FLT_ENV, 0.45f),
    S(P_FLT_A, 0.15f), S(P_FLT_D, 0.55f), S(P_FLT_S, 0.45f), S(P_LFO_DEST, D_PITCH),
    S(P_LFO_RATE, 0.68f), S(P_LFO_DEPTH, 0.25f), S(P_VEL, 0.40f), S(P_AMP_A, 0.12f), S(P_AMP_D, 0.60f),
    S(P_AMP_S, 0.90f), S(P_AMP_R, 0.50f), S(P_CHO_MODE, C_BOTH), S(P_CHO_MIX, 0.30f),
    S(P_TONE, -0.05f), S(P_REV_MIX, 0.50f), S(P_REV_SIZE, 0.90f), S(P_REV_TONE, 0.25f),
    S(P_LEVEL, 0.50f) } },

/* The modern trailer 'braam' as an instrument: a wide saw stack with an
 * octave below, clipped hard, the filter blooming then sinking. */
{ "BR Doom Horn", "keys", 151, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.75f), S(P_STACK, 0.45f), S(P_LAYER, L_OCT_DN), S(P_BLEND, 0.45f),
    S(P_AGE, 0.15f), S(P_CUTOFF, 0.35f), S(P_RESO, 0.30f), S(P_DRIVE, 0.90f), S(P_FLT_ENV, 0.50f),
    S(P_FLT_A, 0.25f), S(P_FLT_D, 0.75f), S(P_FLT_S, 0.35f), S(P_VEL, 0.50f), S(P_AMP_A, 0.10f),
    S(P_AMP_D, 0.70f), S(P_AMP_S, 0.80f), S(P_AMP_R, 0.60f), S(P_TONE, -0.10f), S(P_DLY_TIME, T_2),
    S(P_DLY_MIX, 0.15f), S(P_DLY_FB, 0.35f), S(P_REV_MIX, 0.45f), S(P_REV_SIZE, 0.80f),
    S(P_REV_TONE, 0.20f), S(P_LEVEL, 0.46f) } },

/* Overlook on square-wave horns: an octave below, pushed into the clipper, a
 * slow vibrato, worn tape. */
{ "BR Room 237", "pad", 163, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.25f), S(P_STACK, 0.15f), S(P_LAYER, L_OCT_DN), S(P_BLEND, 0.35f),
    S(P_AGE, 0.20f), S(P_CUTOFF, 0.42f), S(P_RESO, 0.15f), S(P_DRIVE, 0.65f), S(P_FLT_ENV, 0.45f),
    S(P_FLT_A, 0.15f), S(P_FLT_D, 0.55f), S(P_FLT_S, 0.45f), S(P_LFO_DEST, D_PITCH),
    S(P_LFO_RATE, 0.68f), S(P_LFO_DEPTH, 0.25f), S(P_VEL, 0.40f), S(P_AMP_A, 0.12f), S(P_AMP_D, 0.60f),
    S(P_AMP_S, 0.90f), S(P_AMP_R, 0.50f), S(P_CHO_MODE, C_BOTH), S(P_CHO_MIX, 0.30f),
    S(P_TONE, -0.05f), S(P_REV_MIX, 0.50f), S(P_REV_SIZE, 0.90f), S(P_REV_TONE, 0.25f),
    S(P_VHS, 0.30f), S(P_LEVEL, 0.43f) } },

/* ======================================================================= */
/* VOICES                                                                  */
/* ======================================================================= */

/* Monks, low and close, slightly overdriven, the vowel turning slowly in a
 * very dark, very long room. */
{ "VX Black Mass", "pad", 114, {
    S(P_MODEL, M_VOX), S(P_SHAPE, 0.30f), S(P_STACK, 0.50f), S(P_LAYER, L_OCT_DN), S(P_BLEND, 0.35f),
    S(P_AGE, 0.40f), S(P_CUTOFF, 0.52f), S(P_RESO, 0.10f), S(P_DRIVE, 0.50f), S(P_FLT_ENV, 0.05f),
    S(P_LFO_DEST, D_SHAPE), S(P_LFO_RATE, 0.18f), S(P_LFO_DEPTH, 0.20f), S(P_VEL, 0.20f),
    S(P_AMP_A, 0.66f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.68f), S(P_VHS, 0.20f),
    S(P_TONE, 0.20f), S(P_REV_MIX, 0.70f), S(P_REV_SIZE, 0.95f), S(P_REV_TONE, 0.20f),
    S(P_LEVEL, 0.51f) } },

/* A bright choir on "ee" with breath in it, the tail climbing an octave. */
{ "VX Angel Dust", "pad", 115, {
    S(P_MODEL, M_VOX), S(P_SHAPE, 0.78f), S(P_STACK, 0.45f), S(P_LAYER, L_NOISE), S(P_BLEND, 0.18f),
    S(P_AGE, 0.30f), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.85f), S(P_RESO, 0.05f), S(P_DRIVE, 0.10f),
    S(P_FLT_ENV, 0.0f), S(P_VEL, 0.20f), S(P_AMP_A, 0.60f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f),
    S(P_AMP_R, 0.66f), S(P_CHO_MODE, C_ENS), S(P_CHO_MIX, 0.50f), S(P_TONE, 0.40f), S(P_REV_MIX, 0.60f),
    S(P_REV_SIZE, 0.85f), S(P_REV_TONE, 0.75f), S(P_SHIMMER, 0.50f), S(P_LEVEL, 0.46f) } },

/* A machine trying to talk: the vowel flickers six times a second under
 * every chord, band-passed like a small speaker, through a phaser. */
{ "VX Robot Heart", "keys", 116, {
    S(P_MODEL, M_VOX), S(P_SHAPE, 0.50f), S(P_STACK, 0.10f), S(P_LAYER, L_OCT_DN), S(P_BLEND, 0.30f),
    S(P_AGE, 0.05f), S(P_CUTOFF, 0.62f), S(P_RESO, 0.30f), S(P_DRIVE, 0.55f), S(P_FLT_ENV, 0.10f),
    S(P_LFO_DEST, D_SHAPE), S(P_LFO_RATE, 0.78f), S(P_LFO_DEPTH, 0.70f), S(P_VEL, 0.40f),
    S(P_AMP_A, 0.01f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.85f), S(P_AMP_R, 0.25f), S(P_PHS_DEPTH, 0.30f),
    S(P_PHS_RATE, 0.50f), S(P_DLY_TIME, T_8), S(P_DLY_MIX, 0.25f), S(P_REV_MIX, 0.20f),
    S(P_REV_SIZE, 0.45f), S(P_FLT_TYPE, F_BP), S(P_LEVEL, 0.61f) } },

/* Sung plucks: a short "doh" on every note, chorused, with eighth-note
 * answers. */
{ "VX Doo-Wop", "arp", 117, {
    S(P_MODEL, M_VOX), S(P_SHAPE, 0.35f), S(P_SHAPE_ENV, 0.40f), S(P_STACK, 0.20f), S(P_AGE, 0.20f),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.85f), S(P_RESO, 0.05f), S(P_DRIVE, 0.20f), S(P_FLT_ENV, 0.10f),
    S(P_FLT_D, 0.22f), S(P_FLT_S, 0.0f), S(P_VEL, 0.60f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.42f),
    S(P_AMP_S, 0.0f), S(P_AMP_R, 0.30f), S(P_CHO_MODE, C_II), S(P_CHO_MIX, 0.35f), S(P_DLY_TIME, T_8),
    S(P_DLY_MIX, 0.30f), S(P_DLY_FB, 0.45f), S(P_REV_MIX, 0.30f), S(P_REV_SIZE, 0.50f),
    S(P_LEVEL, 0.78f) } },

/* One wordless voice, sliding between notes with a wide vibrato, on old tape
 * and in a long room: the singer the film never shows. */
{ "VX Siren Song", "lead", 118, {
    S(P_MODEL, M_VOX), S(P_SHAPE, 0.95f), S(P_SHAPE_ENV, -0.45f), S(P_AGE, 0.30f), S(P_VOICE, V_G200),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.84f), S(P_RESO, 0.05f), S(P_DRIVE, 0.30f), S(P_FLT_ENV, 0.0f),
    S(P_FLT_A, 0.30f), S(P_FLT_D, 0.60f), S(P_FLT_S, 0.30f), S(P_LFO_RATE, 0.76f),
    S(P_LFO_DEPTH, 0.34f), S(P_AMP_A, 0.30f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.55f),
    S(P_VHS, 0.30f), S(P_DLY_TIME, T_4D), S(P_DLY_MIX, 0.35f), S(P_DLY_FB, 0.50f), S(P_REV_MIX, 0.55f),
    S(P_REV_SIZE, 0.80f), S(P_SHIMMER, 0.25f), S(P_LEVEL, 0.68f) } },

/* A tape choir: the keyboard that plays a strip of tape for every key, each
 * one worn differently. */
{ "VX Mellotape", "keys", 119, {
    S(P_MODEL, M_VOX), S(P_SHAPE, 0.45f), S(P_STACK, 0.30f), S(P_AGE, 0.70f), S(P_CUTOFF, 0.62f),
    S(P_RESO, 0.08f), S(P_DRIVE, 0.30f), S(P_FLT_ENV, 0.05f), S(P_VEL, 0.30f), S(P_AMP_A, 0.20f),
    S(P_AMP_D, 0.60f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.40f), S(P_VHS, 0.70f), S(P_TONE, -0.05f),
    S(P_REV_MIX, 0.30f), S(P_REV_SIZE, 0.50f), S(P_REV_TONE, 0.30f), S(P_LEVEL, 0.46f) } },

/* ----------------------------------------------------------------------- */
/* VX: horror -- retro and modern horror soundtrack sounds               */
/* ----------------------------------------------------------------------- */

/* The wordless lullaby a horror film opens on: one small 'oo-ah' voice with a
 * tremble, on old tape, very far away. */
{ "VX Nursery Rhyme", "lead", 152, {
    S(P_MODEL, M_VOX), S(P_SHAPE, 0.30f), S(P_AGE, 0.35f), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.75f),
    S(P_RESO, 0.05f), S(P_DRIVE, 0.10f), S(P_FLT_ENV, 0.10f), S(P_FLT_A, 0.30f), S(P_FLT_D, 0.60f),
    S(P_FLT_S, 0.80f), S(P_LFO_DEST, D_PITCH), S(P_LFO_RATE, 0.72f), S(P_LFO_DEPTH, 0.40f),
    S(P_VEL, 0.30f), S(P_AMP_A, 0.40f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.95f), S(P_AMP_R, 0.60f),
    S(P_CHO_MODE, C_ENS), S(P_CHO_MIX, 0.30f), S(P_VHS, 0.35f), S(P_TONE, 0.20f), S(P_REV_MIX, 0.60f),
    S(P_REV_SIZE, 0.85f), S(P_REV_TONE, 0.50f), S(P_SHIMMER, 0.20f), S(P_LEVEL, 0.65f) } },

/* Monks singing in parallel fifths, the oldest harmony there is: an 'oh' with
 * the fifth above it, a slow tremble, echoing down a stone corridor. */
{ "VX Catacomb Monks", "pad", 153, {
    S(P_MODEL, M_VOX), S(P_SHAPE, 0.40f), S(P_STACK, 0.20f), S(P_LAYER, L_FIFTH), S(P_BLEND, 0.40f),
    S(P_AGE, 0.25f), S(P_CUTOFF, 0.62f), S(P_RESO, 0.10f), S(P_DRIVE, 0.30f), S(P_FLT_ENV, 0.15f),
    S(P_FLT_A, 0.50f), S(P_FLT_D, 0.80f), S(P_FLT_S, 0.70f), S(P_LFO_DEST, D_PITCH),
    S(P_LFO_RATE, 0.45f), S(P_LFO_DEPTH, 0.30f), S(P_VEL, 0.20f), S(P_AMP_A, 0.50f), S(P_AMP_D, 0.70f),
    S(P_AMP_S, 0.95f), S(P_AMP_R, 0.70f), S(P_CHO_MODE, C_ENS), S(P_CHO_MIX, 0.25f),
    S(P_REV_MIX, 0.55f), S(P_REV_SIZE, 0.85f), S(P_REV_TONE, 0.20f), S(P_FLT_TYPE, F_LP12),
    S(P_DLY_TIME, T_2), S(P_DLY_MIX, 0.15f), S(P_DLY_FB, 0.40f), S(P_LEVEL, 0.43f) } },

/* Witches whispering over the band: a bright vowel half drowned in breath,
 * through a band-pass that jumps at random, echoing round the room. */
{ "VX Three Mothers", "pad", 154, {
    S(P_MODEL, M_VOX), S(P_SHAPE, 0.60f), S(P_LAYER, L_NOISE), S(P_BLEND, 0.55f), S(P_AGE, 0.30f),
    S(P_FLT_TYPE, F_BP), S(P_CUTOFF, 0.62f), S(P_RESO, 0.30f), S(P_DRIVE, 0.20f), S(P_FLT_ENV, 0.20f),
    S(P_FLT_A, 0.30f), S(P_FLT_D, 0.60f), S(P_FLT_S, 0.50f), S(P_LFO_DEST, D_SH), S(P_LFO_RATE, 0.55f),
    S(P_LFO_DEPTH, 0.35f), S(P_VEL, 0.20f), S(P_AMP_A, 0.30f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.85f),
    S(P_AMP_R, 0.60f), S(P_PHS_DEPTH, 0.40f), S(P_PHS_RATE, 0.35f), S(P_TONE, 0.30f),
    S(P_DLY_TIME, T_4D), S(P_DLY_MIX, 0.30f), S(P_DLY_FB, 0.50f), S(P_REV_MIX, 0.50f),
    S(P_REV_SIZE, 0.70f), S(P_REV_TONE, 0.40f), S(P_LEVEL, 0.50f) } },

/* ======================================================================= */
/* STABS AND HITS                                                          */
/* ======================================================================= */

/* The orchestra hit: a wide stack and its lower octave, short, driven, off a
 * tape, in a small hard room. */
{ "ST Orch Hit", "keys", 120, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.80f), S(P_STACK, 0.70f), S(P_LAYER, L_OCT_DN), S(P_BLEND, 0.45f),
    S(P_AGE, 0.50f), S(P_CUTOFF, 0.70f), S(P_RESO, 0.10f), S(P_DRIVE, 0.55f), S(P_FLT_ENV, 0.45f),
    S(P_FLT_D, 0.28f), S(P_FLT_S, 0.10f), S(P_VEL, 0.60f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.40f),
    S(P_AMP_S, 0.0f), S(P_AMP_R, 0.30f), S(P_CHO_MODE, C_ENS), S(P_CHO_MIX, 0.30f), S(P_VHS, 0.30f),
    S(P_REV_MIX, 0.40f), S(P_REV_SIZE, 0.40f), S(P_LEVEL, 0.59f) } },

/* The rave stab: detuned pulses in fifths, both choruses, a resonant snap,
 * dotted-eighth repeats. */
{ "ST Rave Stab", "keys", 121, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.30f), S(P_STACK, 0.50f), S(P_LAYER, L_FIFTH), S(P_BLEND, 0.45f),
    S(P_AGE, 0.20f), S(P_CUTOFF, 0.62f), S(P_RESO, 0.18f), S(P_DRIVE, 0.66f), S(P_FLT_ENV, 0.40f),
    S(P_FLT_D, 0.25f), S(P_FLT_S, 0.0f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.42f), S(P_AMP_S, 0.0f),
    S(P_AMP_R, 0.28f), S(P_CHO_MODE, C_BOTH), S(P_CHO_MIX, 0.50f), S(P_TONE, 0.20f),
    S(P_DLY_TIME, T_8D), S(P_DLY_MIX, 0.30f), S(P_DLY_FB, 0.45f), S(P_REV_MIX, 0.30f),
    S(P_REV_SIZE, 0.50f), S(P_LEVEL, 0.46f) } },

/* A hard FM stab, all edge for fifty milliseconds, in a short bright room. */
{ "ST FM Knife", "keys", 122, {
    S(P_MODEL, M_FM), S(P_SHAPE, 0.15f), S(P_SHAPE_ENV, 0.60f), S(P_STACK, 0.25f), S(P_LAYER, L_OCT_UP),
    S(P_BLEND, 0.30f), S(P_AGE, 0.10f), S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.82f), S(P_RESO, 0.05f),
    S(P_DRIVE, 0.60f), S(P_FLT_ENV, 0.10f), S(P_FLT_D, 0.20f), S(P_FLT_S, 0.0f), S(P_VEL, 0.70f),
    S(P_AMP_A, 0.0f), S(P_AMP_D, 0.38f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.25f), S(P_DLY_TIME, T_16),
    S(P_DLY_MIX, 0.25f), S(P_REV_MIX, 0.35f), S(P_REV_SIZE, 0.35f), S(P_REV_TONE, 0.70f),
    S(P_LEVEL, 0.43f) } },

/* A chord that slams: the sync sweep falls through every note as it hits,
 * through a fast phaser. */
{ "ST Sync Slam", "keys", 123, {
    S(P_MODEL, M_SYNC), S(P_SHAPE, 0.60f), S(P_SHAPE_ENV, -0.50f), S(P_STACK, 0.30f),
    S(P_LAYER, L_OCT_DN), S(P_BLEND, 0.35f), S(P_AGE, 0.20f), S(P_CUTOFF, 0.70f), S(P_RESO, 0.15f),
    S(P_DRIVE, 0.70f), S(P_FLT_ENV, 0.20f), S(P_FLT_D, 0.30f), S(P_FLT_S, 0.0f), S(P_AMP_A, 0.0f),
    S(P_AMP_D, 0.40f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.30f), S(P_PHS_DEPTH, 0.40f), S(P_PHS_RATE, 0.60f),
    S(P_DLY_TIME, T_8), S(P_DLY_MIX, 0.20f), S(P_DLY_FB, 0.35f), S(P_REV_MIX, 0.30f),
    S(P_REV_SIZE, 0.45f), S(P_LEVEL, 0.48f) } },

/* ----------------------------------------------------------------------- */
/* ST: horror -- retro and modern horror soundtrack sounds               */
/* ----------------------------------------------------------------------- */

/* The stinger when the door opens: a cluster of detuned saws and noise
 * clipped at full force, thrown into a big bright hall. */
{ "ST Jump Scare", "keys", 155, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.60f), S(P_STACK, 0.80f), S(P_LAYER, L_NOISE), S(P_BLEND, 0.25f),
    S(P_AGE, 0.20f), S(P_CUTOFF, 0.75f), S(P_RESO, 0.20f), S(P_DRIVE, 0.85f), S(P_FLT_ENV, 0.40f),
    S(P_FLT_D, 0.50f), S(P_FLT_S, 0.20f), S(P_VEL, 0.60f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.60f),
    S(P_AMP_S, 0.20f), S(P_AMP_R, 0.70f), S(P_TONE, 0.05f), S(P_REV_MIX, 0.65f), S(P_REV_SIZE, 0.85f),
    S(P_REV_TONE, 0.55f), S(P_LEVEL, 0.44f) } },

/* Screeching string stabs: thin high-passed sync shaking fast, each hit
 * cut short, in a tiled room. */
{ "ST Shower Stabs", "keys", 156, {
    S(P_MODEL, M_SYNC), S(P_SHAPE, 0.80f), S(P_SHAPE_ENV, 0.30f), S(P_STACK, 0.35f), S(P_AGE, 0.25f),
    S(P_FLT_TYPE, F_HP), S(P_CUTOFF, 0.45f), S(P_RESO, 0.35f), S(P_DRIVE, 0.55f), S(P_FLT_ENV, 0.30f),
    S(P_FLT_D, 0.35f), S(P_FLT_S, 0.30f), S(P_LFO_DEST, D_PITCH), S(P_LFO_RATE, 0.88f),
    S(P_LFO_DEPTH, 0.50f), S(P_VEL, 0.60f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.45f), S(P_AMP_S, 0.25f),
    S(P_AMP_R, 0.35f), S(P_CHO_MODE, C_II), S(P_CHO_MIX, 0.35f), S(P_TONE, 0.25f), S(P_REV_MIX, 0.40f),
    S(P_REV_SIZE, 0.55f), S(P_REV_TONE, 0.60f), S(P_LEVEL, 0.51f) } },

/* ======================================================================= */
/* SEQUENCES: hold a chord and the tempo plays it                          */
/* ======================================================================= */

/* Hold a chord and it rolls in sixteenths: a ringing filter snapped shut by
 * the tempo, pumped on the beat, with dotted-eighth repeats between. */
{ "SQ Motorik", "drone", 124, {
    S(P_MODEL, M_SAW), S(P_SHAPE, 0.60f), S(P_STACK, 0.10f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.30f),
    S(P_AGE, 0.15f), S(P_CUTOFF, 0.30f), S(P_RESO, 0.45f), S(P_DRIVE, 0.35f), S(P_FLT_ENV, 0.0f),
    S(P_LFO_DEST, D_RAMP), S(P_LFO_RATE, 0.70f), S(P_LFO_DEPTH, 0.75f), S(P_VEL, 0.30f),
    S(P_AMP_A, 0.01f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.30f), S(P_PUMP, 0.40f),
    S(P_TONE, 0.15f), S(P_DLY_TIME, T_8D), S(P_DLY_MIX, 0.35f), S(P_DLY_FB, 0.50f), S(P_REV_MIX, 0.20f),
    S(P_REV_SIZE, 0.50f), S(P_LEVEL, 0.52f) } },

/* A choir that pulses in triplets: hold a chord and it says "wow" three
 * times a beat, in the ensemble and a hall. */
{ "SQ Triplet Ghost", "drone", 125, {
    S(P_MODEL, M_VOX), S(P_SHAPE, 0.50f), S(P_STACK, 0.40f), S(P_AGE, 0.30f), S(P_CUTOFF, 0.34f),
    S(P_RESO, 0.30f), S(P_DRIVE, 0.20f), S(P_FLT_ENV, 0.0f), S(P_LFO_DEST, D_RAMP),
    S(P_LFO_RATE, 0.50f), S(P_LFO_DEPTH, 0.70f), S(P_AMP_A, 0.05f), S(P_AMP_D, 0.60f),
    S(P_AMP_S, 0.90f), S(P_AMP_R, 0.45f), S(P_CHO_MODE, C_ENS), S(P_CHO_MIX, 0.40f), S(P_TONE, 0.20f),
    S(P_DLY_TIME, T_4), S(P_DLY_MIX, 0.30f), S(P_DLY_FB, 0.45f), S(P_REV_MIX, 0.45f),
    S(P_REV_SIZE, 0.70f), S(P_LEVEL, 0.62f) } },

/* Thirty-second notes through a band-pass, swelling twice a bar: hold a
 * chord for a strobe light. */
{ "SQ Strobe", "drone", 126, {
    S(P_MODEL, M_SYNC), S(P_SHAPE, 0.40f), S(P_STACK, 0.20f), S(P_LAYER, L_OCT_DN), S(P_BLEND, 0.25f),
    S(P_AGE, 0.15f), S(P_FLT_TYPE, F_BP), S(P_CUTOFF, 0.40f), S(P_RESO, 0.35f), S(P_DRIVE, 0.40f),
    S(P_FLT_ENV, 0.0f), S(P_LFO_DEST, D_RAMP), S(P_LFO_RATE, 0.90f), S(P_LFO_DEPTH, 0.60f),
    S(P_AMP_A, 0.01f), S(P_AMP_D, 0.60f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.30f), S(P_PHS_DEPTH, 0.30f),
    S(P_PHS_RATE, 0.10f), S(P_PUMP, 0.60f), S(P_PUMP_RATE, PU_2), S(P_TONE, 0.10f), S(P_REV_MIX, 0.30f),
    S(P_REV_SIZE, 0.60f), S(P_LEVEL, 0.79f) } },

/* The computer is thinking: hold a chord of bells and a resonant filter picks
 * notes out of it at random, scattered in sixteenth repeats. */
{ "SQ Mainframe", "drone", 127, {
    S(P_MODEL, M_BELL), S(P_SHAPE, 0.30f), S(P_STACK, 0.10f), S(P_LAYER, L_OCT_UP), S(P_BLEND, 0.25f),
    S(P_AGE, 0.10f), S(P_CUTOFF, 0.40f), S(P_RESO, 0.60f), S(P_DRIVE, 0.30f), S(P_FLT_ENV, 0.0f),
    S(P_LFO_DEST, D_SH), S(P_LFO_RATE, 0.74f), S(P_LFO_DEPTH, 0.80f), S(P_AMP_A, 0.01f),
    S(P_AMP_D, 0.60f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.30f), S(P_TONE, 0.15f), S(P_DLY_TIME, T_16),
    S(P_DLY_MIX, 0.40f), S(P_DLY_FB, 0.60f), S(P_REV_MIX, 0.25f), S(P_REV_SIZE, 0.50f),
    S(P_REV_TONE, 0.60f), S(P_LEVEL, 0.45f) } },

/* ----------------------------------------------------------------------- */
/* SQ: horror -- retro and modern horror soundtrack sounds               */
/* ----------------------------------------------------------------------- */

/* The chase: hold a chord and it runs in sixteenths, a dark pulse with an
 * octave below, ducking in sixteenths too, never slowing down. */
{ "SQ Pursuit", "drone", 157, {
    S(P_MODEL, M_PULSE), S(P_SHAPE, 0.30f), S(P_STACK, 0.15f), S(P_LAYER, L_OCT_DN), S(P_BLEND, 0.30f),
    S(P_AGE, 0.15f), S(P_CUTOFF, 0.45f), S(P_RESO, 0.35f), S(P_DRIVE, 0.50f), S(P_FLT_ENV, 0.0f),
    S(P_LFO_DEST, D_RAMP), S(P_LFO_RATE, 0.70f), S(P_LFO_DEPTH, 0.60f), S(P_AMP_A, 0.01f),
    S(P_AMP_D, 0.60f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.25f), S(P_PUMP, 0.50f), S(P_PUMP_RATE, PU_16),
    S(P_VHS, 0.20f), S(P_DLY_TIME, T_8T), S(P_DLY_MIX, 0.18f), S(P_REV_MIX, 0.20f),
    S(P_REV_SIZE, 0.45f), S(P_LEVEL, 0.55f) } },

/* ======================================================================= */
/* FX                                                                      */
/* ======================================================================= */

/* Synth toms: a sine that drops onto the note, a breath of noise, driven,
 * in a short bright room. Play them down the keyboard. */
{ "FX Laser Toms", "tom", 30, {
    S(P_MODEL, M_ZAP), S(P_SHAPE, 0.42f), S(P_LAYER, L_NOISE), S(P_BLEND, 0.10f), S(P_AGE, 0.10f),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.75f), S(P_RESO, 0.10f), S(P_DRIVE, 0.45f),
    S(P_FLT_ENV, 0.20f), S(P_FLT_D, 0.30f), S(P_FLT_S, 0.0f),
    S(P_VEL, 0.60f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.42f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.35f),
    S(P_VHS, 0.15f), S(P_REV_MIX, 0.30f), S(P_REV_SIZE, 0.32f), S(P_REV_TONE, 0.60f), S(P_LEVEL, 0.80f) } },

/* A drone for the scene before something terrible happens: stacked sync and
 * noise through a slow-moving band-pass and phaser. Hold a low fifth. */
{ "FX Event Horizon", "drone", 31, {
    S(P_MODEL, M_SYNC), S(P_SHAPE, 0.25f), S(P_SHAPE_ENV, 0.40f), S(P_STACK, 0.60f), S(P_LAYER, L_NOISE), S(P_BLEND, 0.22f), S(P_AGE, 0.60f),
    S(P_FLT_TYPE, F_BP), S(P_CUTOFF, 0.50f), S(P_RESO, 0.45f), S(P_DRIVE, 0.50f),
    S(P_FLT_ENV, 0.0f), S(P_FLT_A, 0.85f), S(P_FLT_D, 0.90f), S(P_FLT_S, 1.0f),
    S(P_LFO_DEST, D_FILTER), S(P_LFO_RATE, 0.22f), S(P_LFO_DEPTH, 0.70f),
    S(P_VEL, 0.10f), S(P_AMP_A, 0.80f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.75f),
    S(P_PHS_DEPTH, 0.50f), S(P_PHS_RATE, 0.15f), S(P_VHS, 0.35f),
    S(P_REV_MIX, 0.60f), S(P_REV_SIZE, 0.85f), S(P_REV_TONE, 0.40f), S(P_SHIMMER, 0.40f), S(P_LEVEL, 0.63f) } },

/* ----------------------------------------------------------------------- */
/* FX: where an effect is the sound                                        */
/* ----------------------------------------------------------------------- */

/* A hit that falls for two octaves and never lands: the longest room, the
 * shimmer all the way up, long repeats. One note at a time. */
{ "FX Black Hole", "tom", 60, {
    S(P_MODEL, M_ZAP), S(P_SHAPE, 0.90f), S(P_LAYER, L_NOISE), S(P_BLEND, 0.40f), S(P_AGE, 0.20f),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.70f), S(P_RESO, 0.15f), S(P_DRIVE, 0.35f),
    S(P_FLT_ENV, 0.50f), S(P_FLT_D, 0.80f), S(P_FLT_S, 0.0f),
    S(P_VEL, 0.40f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.80f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.80f),
    S(P_VHS, 0.20f),
    S(P_DLY_TIME, T_4D), S(P_DLY_MIX, 0.40f), S(P_DLY_FB, 0.70f),
    S(P_REV_MIX, 0.90f), S(P_REV_SIZE, 1.0f), S(P_REV_TONE, 0.60f), S(P_SHIMMER, 1.0f), S(P_LEVEL, 0.46f) } },

/* A slow sync sweep swinging side to side through a fast deep phaser, with
 * the delay almost feeding itself. Hold and wait. */
{ "FX Wormhole", "drone", 61, {
    S(P_MODEL, M_SYNC), S(P_SHAPE, 0.50f), S(P_SHAPE_ENV, 0.60f), S(P_STACK, 0.40f), S(P_AGE, 0.40f),
    S(P_CUTOFF, 0.72f), S(P_RESO, 0.20f), S(P_DRIVE, 0.50f),
    S(P_FLT_ENV, 0.0f), S(P_FLT_A, 0.80f), S(P_FLT_D, 0.90f), S(P_FLT_S, 0.80f),
    S(P_LFO_DEST, D_PAN), S(P_LFO_RATE, 0.50f), S(P_LFO_DEPTH, 0.90f),
    S(P_VEL, 0.10f), S(P_AMP_A, 0.60f), S(P_AMP_D, 0.70f), S(P_AMP_S, 0.90f), S(P_AMP_R, 0.70f),
    S(P_PHS_DEPTH, 0.85f), S(P_PHS_RATE, 0.75f),
    S(P_DLY_TIME, T_8T), S(P_DLY_MIX, 0.60f), S(P_DLY_FB, 0.88f),
    S(P_REV_MIX, 0.40f), S(P_REV_SIZE, 0.70f), S(P_LEVEL, 0.51f) } },

/* Nothing but noise: stepped through a band-pass at random, on the worst
 * tape there is. The channel after the station signs off. */
{ "FX Dead Channel", "fx", 62, {
    S(P_MODEL, M_SAW), S(P_LAYER, L_NOISE), S(P_BLEND, 1.0f), S(P_AGE, 0.30f),
    S(P_FLT_TYPE, F_BP), S(P_CUTOFF, 0.60f), S(P_RESO, 0.55f), S(P_DRIVE, 0.40f), S(P_FLT_ENV, 0.0f),
    S(P_LFO_DEST, D_SH), S(P_LFO_RATE, 0.80f), S(P_LFO_DEPTH, 0.70f),
    S(P_VEL, 0.10f), S(P_AMP_A, 0.20f), S(P_AMP_D, 0.60f), S(P_AMP_S, 1.0f), S(P_AMP_R, 0.40f),
    S(P_VHS, 1.0f),
    S(P_DLY_TIME, T_16), S(P_DLY_MIX, 0.25f), S(P_DLY_FB, 0.60f),
    S(P_REV_MIX, 0.30f), S(P_REV_SIZE, 0.50f), S(P_REV_TONE, 0.40f), S(P_LEVEL, 0.95f) } },

/* The tom fill at the end of every bar in 1986: driven, and mostly room -- a
 * short, bright, very loud one. */
{ "FX Thunder Toms", "tom", 63, {
    S(P_MODEL, M_ZAP), S(P_SHAPE, 0.38f), S(P_LAYER, L_NOISE), S(P_BLEND, 0.15f), S(P_AGE, 0.10f),
    S(P_FLT_TYPE, F_LP12), S(P_CUTOFF, 0.78f), S(P_RESO, 0.10f), S(P_DRIVE, 0.50f),
    S(P_FLT_ENV, 0.20f), S(P_FLT_D, 0.30f), S(P_FLT_S, 0.0f),
    S(P_VEL, 0.60f), S(P_AMP_A, 0.0f), S(P_AMP_D, 0.45f), S(P_AMP_S, 0.0f), S(P_AMP_R, 0.40f),
    S(P_CHO_MODE, C_DIM), S(P_CHO_MIX, 0.30f),
    S(P_DLY_TIME, T_8), S(P_DLY_MIX, 0.20f), S(P_DLY_FB, 0.30f),
    S(P_REV_MIX, 0.75f), S(P_REV_SIZE, 0.28f), S(P_REV_TONE, 0.70f), S(P_LEVEL, 0.88f) } },

/* ----------------------------------------------------------------------- */
/* FX: horror -- retro and modern horror soundtrack sounds               */
/* ----------------------------------------------------------------------- */

/* The swell before the scare: sync and noise climbing out of a closed
 * resonant filter over seconds, phasing faster, into shimmer. */
{ "FX Rising Dread", "fx", 158, {
    S(P_MODEL, M_SYNC), S(P_SHAPE, 0.20f), S(P_SHAPE_ENV, 0.60f), S(P_STACK, 0.40f),
    S(P_LAYER, L_NOISE), S(P_BLEND, 0.35f), S(P_AGE, 0.30f), S(P_CUTOFF, 0.15f), S(P_RESO, 0.55f),
    S(P_DRIVE, 0.60f), S(P_FLT_ENV, 0.90f), S(P_FLT_A, 0.85f), S(P_FLT_D, 0.90f), S(P_FLT_S, 1.0f),
    S(P_VEL, 0.10f), S(P_AMP_A, 0.75f), S(P_AMP_D, 0.70f), S(P_AMP_S, 1.0f), S(P_AMP_R, 0.60f),
    S(P_PHS_DEPTH, 0.60f), S(P_PHS_RATE, 0.60f), S(P_DLY_TIME, T_16), S(P_DLY_MIX, 0.20f),
    S(P_DLY_FB, 0.60f), S(P_REV_MIX, 0.60f), S(P_REV_SIZE, 0.80f), S(P_SHIMMER, 0.50f),
    S(P_LEVEL, 0.47f) } },

/* Something running below the house: a dense spiky wavetable and a deep sub,
 * a filter wandering over half a minute, on rotten tape. Hold a low note. */
{ "FX Basement", "drone", 159, {
    S(P_MODEL, M_WAVE), S(P_SHAPE, 1.0f), S(P_STACK, 0.70f), S(P_LAYER, L_SUB_2), S(P_BLEND, 0.30f),
    S(P_AGE, 0.80f), S(P_CUTOFF, 0.30f), S(P_RESO, 0.50f), S(P_DRIVE, 0.70f), S(P_FLT_ENV, 0.0f),
    S(P_LFO_DEST, D_FILTER), S(P_LFO_RATE, 0.08f), S(P_LFO_DEPTH, 0.60f), S(P_VEL, 0.10f),
    S(P_AMP_A, 0.60f), S(P_AMP_D, 0.70f), S(P_AMP_S, 1.0f), S(P_AMP_R, 0.80f), S(P_PHS_DEPTH, 0.30f),
    S(P_PHS_RATE, 0.02f), S(P_VHS, 0.60f), S(P_TONE, -0.20f), S(P_REV_MIX, 0.50f),
    S(P_REV_SIZE, 0.70f), S(P_REV_TONE, 0.15f), S(P_LEVEL, 0.42f) } },
};

#define TB_PRESET_COUNT ((int)(sizeof(TB_PRESETS) / sizeof(TB_PRESETS[0])))

#endif

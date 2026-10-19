/*
 * tenebrae_params.h -- the parameter surface, and the ONLY place it is declared.
 *
 * chain_params, ui_hierarchy, the state blob, module.json's generated
 * chain_params block and the factory presets are all built from TB_PARAMS[] and
 * TB_PAGES[]. Adding a control is one row here plus its use in the engine.
 *
 * Wire format: every continuous control is a float 0..1 (bipolar ones -1..1),
 * every enum travels as its option INDEX. set_param also accepts option names.
 */
#ifndef TENEBRAE_PARAMS_H
#define TENEBRAE_PARAMS_H

enum {
    /* Osc page */
    P_MODEL, P_SHAPE, P_SHAPE_ENV, P_STACK,
    P_LAYER, P_BLEND, P_AGE, P_VOICE,
    /* Filter page */
    P_FLT_TYPE, P_CUTOFF, P_RESO, P_DRIVE,
    P_FLT_ENV, P_FLT_A, P_FLT_D, P_FLT_S,
    /* Amp page */
    P_LFO_DEST, P_LFO_RATE, P_LFO_DEPTH, P_VEL,
    P_AMP_A, P_AMP_D, P_AMP_S, P_AMP_R,
    /* Motion FX page */
    P_CHO_MODE, P_CHO_MIX, P_PHS_RATE, P_PHS_DEPTH,
    P_PUMP_RATE, P_PUMP, P_VHS, P_TONE,
    /* Space FX page */
    P_DLY_TIME, P_DLY_FB, P_DLY_MIX, P_LEVEL,
    P_REV_SIZE, P_REV_TONE, P_SHIMMER, P_REV_MIX,
    P_COUNT
};

/* Oscillator models */
enum { M_SAW, M_PULSE, M_SYNC, M_FM, M_BELL, M_WAVE, M_VOX, M_ZAP, M_COUNT };
/* Layer (second source) */
enum { L_SUB_SINE, L_SUB_SQR, L_SUB_2, L_OCT_DN, L_FIFTH, L_OCT_UP, L_OCT_2, L_NOISE, L_COUNT };
/* Filter types */
enum { F_LP24, F_LP12, F_BP, F_HP, F_COUNT };
/* LFO destinations */
enum { D_PITCH, D_SHAPE, D_FILTER, D_AMP, D_PAN, D_SH, D_RAMP, D_COUNT };
/* Chorus modes */
enum { C_I, C_II, C_BOTH, C_ENS, C_DIM, C_COUNT };

enum { PT_FLOAT, PT_BIPOLAR, PT_ENUM };

typedef struct {
    const char *key;
    const char *name;        /* held-knob header and list rows */
    const char *short_name;  /* the 5-character knob cell */
    int         type;
    float       def;
    const char *options;       /* PT_ENUM: '|' separated */
    const char *short_options; /* PT_ENUM: '|' separated, for the option square */
    const char *viz;           /* raw JSON object for "viz", or 0 */
} tb_param_t;

#define VOICE_OPTS \
    "Poly|Mono|Glide 10ms|Glide 20ms|Glide 30ms|Glide 45ms|Glide 60ms|Glide 80ms|" \
    "Glide 110ms|Glide 150ms|Glide 200ms|Glide 270ms|Glide 360ms|Glide 480ms|" \
    "Glide 650ms|Glide 900ms|Glide 1.2s|Glide 1.6s"
#define VOICE_SHORT \
    "POLY|MONO|10|20|30|45|60|80|110|150|200|270|360|480|650|900|1.2s|1.6s"
#define TB_VOICE_OPTION_COUNT 18
/* Glide time in ms per Voice option (Poly and Mono have none) */
static const float TB_GLIDE_MS[TB_VOICE_OPTION_COUNT] = {
    0, 0, 10, 20, 30, 45, 60, 80, 110, 150, 200, 270, 360, 480, 650, 900, 1200, 1600
};

#define DLY_OPTS  "1/16|1/8T|1/8|1/8 Dot|1/4|1/4 Dot|1/2"
#define DLY_SHORT "1/16|1/8T|1/8|1/8.|1/4|1/4.|1/2"
static const float TB_DLY_BEATS[7] = { 0.25f, 1.0f / 3.0f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f };

#define PUMP_OPTS "1/2|1/4|1/8|1/16"
static const float TB_PUMP_BEATS[4] = { 2.0f, 1.0f, 0.5f, 0.25f };

/* LFO target "Filter Sync Ramp": the Rate knob picks one of these divisions */
#define TB_RAMP_DIVS 5
static const float TB_RAMP_BEATS[TB_RAMP_DIVS] = { 1.0f, 0.5f, 1.0f / 3.0f, 0.25f, 0.125f };

static const tb_param_t TB_PARAMS[P_COUNT] = {
    /* ---- Osc ------------------------------------------------------------ */
    [P_MODEL]     = { "model", "Osc Model", "MODEL", PT_ENUM, M_SAW,
                      "Saw|Pulse|Sync|FM|Bell|Wave|Vox|Zap",
                      "SAW|PLS|SNC|FM|BEL|WAV|VOX|ZAP", 0 },
    [P_SHAPE]     = { "shape", "Shape", "SHAPE", PT_FLOAT, 0.5f, 0, 0, 0 },
    [P_SHAPE_ENV] = { "shape_env", "Shape Env", "S.ENV", PT_BIPOLAR, 0.0f, 0, 0, 0 },
    [P_STACK]     = { "stack", "Stack", "STACK", PT_FLOAT, 0.0f, 0, 0, 0 },
    [P_LAYER]     = { "layer", "Layer", "LAYER", PT_ENUM, L_SUB_SQR,
                      "Sub Sine|Sub Square|Sub -2 Oct|Octave Down|Fifth Up|Octave Up|2 Oct Up|Noise",
                      "SIN|SQR|-2|-12|+7|+12|+24|NSE", 0 },
    [P_BLEND]     = { "blend", "Layer Blend", "BLEND", PT_FLOAT, 0.0f, 0, 0, 0 },
    [P_AGE]       = { "age", "Age", "AGE", PT_FLOAT, 0.2f, 0, 0, 0 },
    [P_VOICE]     = { "voice", "Voice / Glide", "GLIDE", PT_ENUM, 0, VOICE_OPTS, VOICE_SHORT, 0 },

    /* ---- Filter --------------------------------------------------------- */
    [P_FLT_TYPE]  = { "flt_type", "Filter Type", "TYPE", PT_ENUM, F_LP24,
                      "LP24|LP12|BP|HP", "LP24|LP12|BP|HP",
                      "{\"group\":\"flt\",\"role\":\"mode\"}" },
    [P_CUTOFF]    = { "cutoff", "Cutoff", "CUT", PT_FLOAT, 0.62f, 0, 0,
                      "{\"group\":\"flt\",\"role\":\"cutoff\"}" },
    [P_RESO]      = { "reso", "Resonance", "RES", PT_FLOAT, 0.15f, 0, 0,
                      "{\"group\":\"flt\",\"role\":\"resonance\"}" },
    [P_DRIVE]     = { "drive", "Drive", "DRIVE", PT_FLOAT, 0.2f, 0, 0, 0 },
    [P_FLT_ENV]   = { "flt_env", "Filter Env Amt", "ENV", PT_BIPOLAR, 0.3f, 0, 0, 0 },
    [P_FLT_A]     = { "flt_a", "Filter Attack", "ATK", PT_FLOAT, 0.0f, 0, 0,
                      "{\"group\":\"fenv\",\"role\":\"attack\"}" },
    [P_FLT_D]     = { "flt_d", "Filter Decay", "DCY", PT_FLOAT, 0.45f, 0, 0,
                      "{\"group\":\"fenv\",\"role\":\"decay\"}" },
    [P_FLT_S]     = { "flt_s", "Filter Sustain", "SUS", PT_FLOAT, 0.3f, 0, 0,
                      "{\"group\":\"fenv\",\"role\":\"sustain\"}" },

    /* ---- Amp + LFO ------------------------------------------------------ */
    [P_LFO_DEST]  = { "lfo_dest", "LFO Target", "LFO", PT_ENUM, D_PITCH,
                      "Pitch|Shape|Filter|Amp|Pan|Filter S&H|Filter Sync Ramp",
                      "PIT|SHP|FLT|AMP|PAN|S&H|RMP", 0 },
    [P_LFO_RATE]  = { "lfo_rate", "LFO Rate", "RATE", PT_FLOAT, 0.55f, 0, 0,
                      "{\"group\":\"lfo\",\"role\":\"rate\"}" },
    [P_LFO_DEPTH] = { "lfo_depth", "LFO Depth", "DEPTH", PT_FLOAT, 0.0f, 0, 0,
                      "{\"group\":\"lfo\",\"role\":\"depth\"}" },
    [P_VEL]       = { "vel", "Velocity", "VEL", PT_FLOAT, 0.5f, 0, 0, 0 },
    [P_AMP_A]     = { "amp_a", "Amp Attack", "ATK", PT_FLOAT, 0.02f, 0, 0,
                      "{\"group\":\"aenv\",\"role\":\"attack\"}" },
    [P_AMP_D]     = { "amp_d", "Amp Decay", "DCY", PT_FLOAT, 0.5f, 0, 0,
                      "{\"group\":\"aenv\",\"role\":\"decay\"}" },
    [P_AMP_S]     = { "amp_s", "Amp Sustain", "SUS", PT_FLOAT, 0.8f, 0, 0,
                      "{\"group\":\"aenv\",\"role\":\"sustain\"}" },
    [P_AMP_R]     = { "amp_r", "Amp Release", "REL", PT_FLOAT, 0.35f, 0, 0,
                      "{\"group\":\"aenv\",\"role\":\"release\"}" },

    /* ---- Motion FX ------------------------------------------------------ */
    [P_CHO_MODE]  = { "cho_mode", "Chorus Type", "CHORUS", PT_ENUM, C_I,
                      "I|II|I+II|Ensemble|Dimension", "I|II|I+II|ENS|DIM", 0 },
    [P_CHO_MIX]   = { "cho_mix", "Chorus Mix", "MIX", PT_FLOAT, 0.0f, 0, 0, 0 },
    [P_PHS_RATE]  = { "phs_rate", "Phaser Rate", "RATE", PT_FLOAT, 0.3f, 0, 0, 0 },
    [P_PHS_DEPTH] = { "phs_depth", "Phaser Depth", "PHASER", PT_FLOAT, 0.0f, 0, 0, 0 },
    [P_PUMP_RATE] = { "pump_rate", "Pump Rate", "DIV", PT_ENUM, 1, PUMP_OPTS, PUMP_OPTS, 0 },
    [P_PUMP]      = { "pump", "Pump Depth", "PUMP", PT_FLOAT, 0.0f, 0, 0, 0 },
    [P_VHS]       = { "vhs", "VHS", "VHS", PT_FLOAT, 0.0f, 0, 0, 0 },
    [P_TONE]      = { "tone", "Tone", "TONE", PT_BIPOLAR, 0.0f, 0, 0, 0 },

    /* ---- Space FX ------------------------------------------------------- */
    [P_DLY_TIME]  = { "dly_time", "Delay Time", "TIME", PT_ENUM, 3, DLY_OPTS, DLY_SHORT, 0 },
    [P_DLY_FB]    = { "dly_fb", "Delay Feedback", "FDBK", PT_FLOAT, 0.4f, 0, 0, 0 },
    [P_DLY_MIX]   = { "dly_mix", "Delay Mix", "DELAY", PT_FLOAT, 0.0f, 0, 0, 0 },
    [P_LEVEL]     = { "level", "Level", "LEVEL", PT_FLOAT, 0.7f, 0, 0,
                      "{\"kind\":\"fader\"}" },
    [P_REV_SIZE]  = { "rev_size", "Reverb Size", "SIZE", PT_FLOAT, 0.5f, 0, 0, 0 },
    [P_REV_TONE]  = { "rev_tone", "Reverb Color", "COLOR", PT_FLOAT, 0.5f, 0, 0, 0 },
    [P_SHIMMER]   = { "shimmer", "Shimmer", "SHIM", PT_FLOAT, 0.0f, 0, 0, 0 },
    [P_REV_MIX]   = { "rev_mix", "Reverb Mix", "REVERB", PT_FLOAT, 0.15f, 0, 0, 0 },
};

/* Pages, in jog order. Each is one level of the hierarchy and exactly eight
 * knobs: two rows of four. A viz group (filter response, envelope) must sit
 * inside ONE row, which is why each envelope has a row to itself. */
#define TB_PAGE_COUNT 5
typedef struct {
    const char *level;
    const char *name;
    int         knobs[8];
} tb_page_t;

static const tb_page_t TB_PAGES[TB_PAGE_COUNT] = {
    { "osc", "Osc",
      { P_MODEL, P_SHAPE, P_SHAPE_ENV, P_STACK, P_LAYER, P_BLEND, P_AGE, P_VOICE } },
    { "filter", "Filter",
      { P_FLT_TYPE, P_CUTOFF, P_RESO, P_DRIVE, P_FLT_ENV, P_FLT_A, P_FLT_D, P_FLT_S } },
    { "amp", "Amp + LFO",
      { P_LFO_DEST, P_LFO_RATE, P_LFO_DEPTH, P_VEL, P_AMP_A, P_AMP_D, P_AMP_S, P_AMP_R } },
    { "motion", "FX Motion",
      { P_CHO_MODE, P_CHO_MIX, P_PHS_DEPTH, P_PHS_RATE, P_PUMP, P_PUMP_RATE, P_VHS, P_TONE } },
    { "space", "FX Space",
      { P_DLY_MIX, P_DLY_TIME, P_DLY_FB, P_LEVEL, P_REV_MIX, P_REV_SIZE, P_REV_TONE, P_SHIMMER } },
};

/* Number of options in a '|' separated list */
static inline int tb_option_count(const char *opts)
{
    int n = 1;
    if (!opts || !*opts) return 0;
    for (const char *c = opts; *c; c++) if (*c == '|') n++;
    return n;
}

static inline float tb_param_min(int id) { return TB_PARAMS[id].type == PT_BIPOLAR ? -1.0f : 0.0f; }
static inline float tb_param_max(int id)
{
    if (TB_PARAMS[id].type == PT_ENUM) return (float)(tb_option_count(TB_PARAMS[id].options) - 1);
    return 1.0f;
}

#endif

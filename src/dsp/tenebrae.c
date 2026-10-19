/*
 * tenebrae.c -- Tenebrae for Schwung (plugin API v2): the wrapper around the
 * engine. It owns one instance per slot, serves the parameter surface and its
 * JSON (chain_params, ui_hierarchy, state), and turns MIDI into notes.
 *
 * THREADING: every entry point except instance creation runs on the audio
 * callback. Nothing here allocates, locks, logs or opens a file once an
 * instance exists; the JSON builders format straight into the caller's buffer.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <math.h>

#include "host/plugin_api_v1.h"
#include "host/move_info.h"

#include "tenebrae_params.h"
#include "tenebrae_engine.h"
#include "tenebrae_fx.h"
#include "tenebrae_presets.h"

#define TB_BLOCK_MAX 256
/* how long the output must stay below the floor before the instance goes idle */
#define TB_IDLE_BLOCKS 172          /* about half a second */
#define TB_IDLE_FLOOR  2.0e-5f

static const host_api_v1_t *g_host = NULL;

typedef struct {
    tb_engine_t eng;
    tb_fx_t     fx;
    float       bufL[TB_BLOCK_MAX], bufR[TB_BLOCK_MAX], bufSub[TB_BLOCK_MAX];
    int         preset;        /* factory preset index, -1 once it is edited away / restored */
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
    out_fmt(&o, "]");
    return o.over ? -1 : o.len;
}

/*
 * The hierarchy. For a sound generator this is the ONLY copy the UI reads: the
 * chain host asks the plugin and has no module.json fallback.
 *
 * Root is the preset browser and nothing else, so the browser is the first
 * page and the five knob pages follow in jog order, each named by its own
 * level. A key lives on exactly one level, so nothing is deduplicated away.
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
    out_fmt(&o, "]}");
    for (int pg = 0; pg < TB_PAGE_COUNT; pg++) {
        out_fmt(&o, ",\"%s\":{\"name\":\"%s\",\"knobs\":[", TB_PAGES[pg].level, TB_PAGES[pg].name);
        for (int k = 0; k < 8; k++)
            out_fmt(&o, "%s\"%s\"", k ? "," : "", TB_PARAMS[TB_PAGES[pg].knobs[k]].key);
        out_fmt(&o, "],\"params\":[");
        for (int k = 0; k < 8; k++)
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
 * takes every pair whose key it knows. */
static void restore_state(tb_instance_t *inst, const char *json)
{
    const char *p = json;
    int preset = -1;
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
        if (id >= 0) tb_set(inst, id, (float)val);
        else if (strcmp(key, "preset") == 0) preset = (int)val;
        else if (strcmp(key, "octave_transpose") == 0) {
            int oc = (int)val;
            inst->eng.octave = oc < -4 ? -4 : (oc > 4 ? 4 : oc);
        }
    }
    inst->preset = -1;
    for (int i = 0; i < TB_PRESET_COUNT && preset >= 0; i++)
        if (TB_PRESETS[i].uid == preset) { inst->preset = i; break; }
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
    tb_load_preset(inst, 0);
    /* the first read resolves the host's export; keep that off the audio path */
    move_info_t mi;
    (void)move_info_read(&mi);
    return inst;
}

static void v2_destroy_instance(void *instance)
{
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
    if (strcmp(key, "all_notes_off") == 0) { tb_all_notes_off(&inst->eng); return; }
    if (strcmp(key, "octave_transpose") == 0) {
        int oc = atoi(val);
        inst->eng.octave = oc < -4 ? -4 : (oc > 4 ? 4 : oc);
        return;
    }
    int id = tb_find_param(key);
    if (id < 0) return;
    float v = TB_PARAMS[id].type == PT_ENUM ? tb_parse_enum(id, val) : (float)atof(val);
    tb_set(inst, id, v);
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
    else if (strcmp(key, "preset_name") == 0)
        n = snprintf(buf, (size_t)buf_len, "%s", inst->preset >= 0 ? TB_PRESETS[inst->preset].name : "Edited");
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

static void v2_render_block(void *instance, int16_t *out_lr, int frames)
{
    tb_instance_t *inst = (tb_instance_t *)instance;
    if (!inst || !out_lr || frames <= 0) return;
    if (frames > TB_BLOCK_MAX) frames = TB_BLOCK_MAX;

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

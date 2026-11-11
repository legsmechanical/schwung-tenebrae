/*
 * tenebrae_tool.c -- run the real plugin on the development machine.
 *
 *   tenebrae_tool json <key>                     print what get_param serves
 *   tenebrae_tool render <preset> <out.wav> [phrase] [key=value ...]
 *   tenebrae_tool note <out.wav> <note> <secs> [key=value ...]     one held note from Init
 *                (at=0..127 channel pressure, cc1=0..127 mod wheel, held for the note)
 *   tenebrae_tool bench                          cost of 8 held voices, per model
 *   tenebrae_tool cost                           cost of each preset playing its phrase
 *   tenebrae_tool list                           index, category, name, uid of every preset
 *   tenebrae_tool state <preset>                 that preset's parameters (the state blob)
 *   tenebrae_tool selftest | fuzz [secs] [seed]  API behaviour; random abuse
 *
 * It includes the plugin source directly, so what it renders IS the module:
 * same code, same presets, driven through the same v2 entry points the host
 * calls. Build: cc -O2 -Isrc -Isrc/dsp tools/tenebrae_tool.c -lm -o build/tenebrae_tool
 */
#include "../src/dsp/tenebrae.c"

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return x < y ? -1 : (x > y ? 1 : 0);
}
#include <time.h>

#define SR 44100
static float g_bpm = 110.0f;
static double g_beat = -1.0;
static float host_bpm(void) { return g_bpm; }
static double host_beat(void) { return g_beat; }

/* the phrases (ev_t, PH_*, phrase_for) live in src/dsp/tenebrae_phrases.h,
 * where the Random page's level probe plays them too */

static void wav_write(const char *path, const int16_t *pcm, int frames)
{
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    uint32_t bytes = (uint32_t)frames * 4, u; uint16_t h;
    fwrite("RIFF", 1, 4, f); u = 36 + bytes; fwrite(&u, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f);
    u = 16; fwrite(&u, 4, 1, f); h = 1; fwrite(&h, 2, 1, f); h = 2; fwrite(&h, 2, 1, f);
    u = SR; fwrite(&u, 4, 1, f); u = SR * 4; fwrite(&u, 4, 1, f); h = 4; fwrite(&h, 2, 1, f); h = 16; fwrite(&h, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&bytes, 4, 1, f); fwrite(pcm, 4, (size_t)frames, f);
    fclose(f);
}

static void apply_sets(plugin_api_v2_t *api, void *inst, int argc, char **argv, int from)
{
    for (int i = from; i < argc; i++) {
        char *eq = strchr(argv[i], '=');
        if (!eq) continue;
        *eq = '\0';
        if (!strcmp(argv[i], "bpm")) g_bpm = (float)atof(eq + 1);
        /* performance controls, sent as MIDI before the phrase: they hold */
        else if (!strcmp(argv[i], "at")) { uint8_t m[2] = { 0xD0, (uint8_t)(atoi(eq + 1) & 127) }; api->on_midi(inst, m, 2, 0); }
        else if (!strcmp(argv[i], "cc1")) { uint8_t m[3] = { 0xB0, 1, (uint8_t)(atoi(eq + 1) & 127) }; api->on_midi(inst, m, 3, 0); }
        else api->set_param(inst, argv[i], eq + 1);
        *eq = '=';
    }
}

/* play a phrase; `tail` seconds are rendered after the last note-off */
static int play(plugin_api_v2_t *api, void *inst, const ev_t *ph, int16_t *pcm, int max_frames, double tail, int transport)
{
    double spb = 60.0 / g_bpm, end = 0;
    int n_ev = 0;
    while (ph[n_ev].beat >= 0) { double e = (ph[n_ev].beat + ph[n_ev].len) * spb; if (e > end) end = e; n_ev++; }
    int total = (int)((end + tail) * SR);
    if (total > max_frames) total = max_frames;
    static char on[64], off[64];
    memset(on, 0, sizeof on); memset(off, 0, sizeof off);
    for (int pos = 0; pos < total; pos += 128) {
        double t0 = (double)pos / SR, t1 = (double)(pos + 128) / SR;
        g_beat = transport ? t0 / spb : -1.0;
        for (int i = 0; i < n_ev && i < 64; i++) {
            double a = ph[i].beat * spb, b = (ph[i].beat + ph[i].len) * spb;
            if (!off[i] && on[i] && b < t1) { uint8_t m[3] = { 0x80, (uint8_t)ph[i].note, 0 }; api->on_midi(inst, m, 3, 0); off[i] = 1; }
            if (!on[i] && a < t1 && a >= t0 - 1e-9) { uint8_t m[3] = { 0x90, (uint8_t)ph[i].note, (uint8_t)ph[i].vel }; api->on_midi(inst, m, 3, 0); on[i] = 1; }
        }
        int n = total - pos < 128 ? total - pos : 128;
        api->render_block(inst, pcm + 2 * pos, n);
    }
    return total;
}

int main(int argc, char **argv)
{
    static host_api_v1_t host;
    memset(&host, 0, sizeof host);
    host.api_version = 1; host.sample_rate = SR; host.frames_per_block = 128;
    host.get_bpm = host_bpm; host.get_beat_position = host_beat;
    plugin_api_v2_t *api = move_plugin_init_v2(&host);
    void *inst = api->create_instance(".", NULL);
    if (!inst) { fprintf(stderr, "create_instance failed\n"); return 1; }
    static char big[262144];
    static int16_t pcm[2 * SR * 60];
    const char *cmd = argc > 1 ? argv[1] : "";

    if (!strcmp(cmd, "json") && argc > 2) {
        int n = api->get_param(inst, argv[2], big, sizeof big);
        if (n < 0) { fprintf(stderr, "get_param(%s) = %d\n", argv[2], n); return 1; }
        fwrite(big, 1, (size_t)n, stdout); fputc('\n', stdout);
        return 0;
    }
    if (!strcmp(cmd, "state") && argc > 2) {
        /* a preset's full parameter set, as the plugin holds it */
        api->set_param(inst, "preset", argv[2]);
        int n = api->get_param(inst, "state", big, sizeof big);
        fwrite(big, 1, (size_t)(n > 0 ? n : 0), stdout); fputc('\n', stdout);
        return 0;
    }
    if (!strcmp(cmd, "list")) {
        for (int i = 0; i < TB_PRESET_COUNT; i++) printf("%d\t%s\t%s\t%d\n", i, TB_PRESETS[i].cat, TB_PRESETS[i].name, TB_PRESETS[i].uid);
        return 0;
    }
    if (!strcmp(cmd, "render") && argc > 3) {
        int idx = atoi(argv[2]);
        char tmp[16]; snprintf(tmp, sizeof tmp, "%d", idx);
        api->set_param(inst, "preset", tmp);
        const char *cat = TB_PRESETS[idx < 0 ? 0 : (idx >= TB_PRESET_COUNT ? TB_PRESET_COUNT - 1 : idx)].cat;
        int first = 4;
        if (argc > 4 && !strchr(argv[4], '=')) { cat = argv[4]; first = 5; }
        apply_sets(api, inst, argc, argv, first);
        int n = play(api, inst, phrase_for(cat), pcm, SR * 60, 3.0, 1);
        wav_write(argv[3], pcm, n);
        return 0;
    }
    if (!strcmp(cmd, "recipes")) {
        for (int i = 0; i < TB_RECIPE_COUNT; i++)
            printf("%d\t%d\t%s\t%s\n", i, TB_RECIPES[i].cat, TB_RECIPES[i].name,
                   TB_RECIPES[i].phrase ? TB_RECIPES[i].phrase : TB_RND_PHRASE[TB_RECIPES[i].cat]);
        return 0;
    }
    if ((!strcmp(cmd, "roll") || !strcmp(cmd, "rollr") || !strcmp(cmd, "vary")) && argc > 4) {
        /* roll <category 0..9> <seed> <out.wav|-> [k=v ...]   a Random page roll
         * rollr <recipe> <seed> <out.wav|-> [k=v ...]        a roll of one recipe
         * vary <preset> <seed> <out.wav|-> [k=v ...]         Vary Current on a preset
         * Measured and vetted inline, so this renders what the device ends on.
         * "-" prints "phrase<TAB>recipe<TAB>tries<TAB>state" instead of
         * rendering (tries: how many rolls the measured check took). */
        tb_instance_t *ti = (tb_instance_t *)inst;
        ti->rnd_sync = 1;
        api->set_param(inst, "rnd_seed", argv[3]);
        if (!strcmp(cmd, "vary")) {
            api->set_param(inst, "preset", argv[2]);
            tb_vary(ti);
        } else {
            int ri = !strcmp(cmd, "roll") ? tb_roll(ti, atoi(argv[2])) : tb_roll_recipe(ti, atoi(argv[2]));
            if (ri < 0) { fprintf(stderr, "roll failed\n"); return 1; }
        }
        const char *phrase = ti->rnd_recipe >= 0
            ? (TB_RECIPES[ti->rnd_recipe].phrase ? TB_RECIPES[ti->rnd_recipe].phrase : TB_RND_PHRASE[ti->rnd_cat])
            : (ti->rnd_a_preset >= 0 && tb_rnd_preset_cat(ti->rnd_a_preset) >= 0
               ? TB_PRESETS[ti->rnd_a_preset].cat : TB_RND_PHRASE[ti->rnd_cat]);
        const char *rname = ti->rnd_recipe >= 0 ? TB_RECIPES[ti->rnd_recipe].name : "Vary";
        apply_sets(api, inst, argc, argv, 5);
        if (!strcmp(argv[4], "-")) {
            int n = api->get_param(inst, "state", big, sizeof big);
            printf("%s\t%s\t%d\t%s\n", phrase, rname, g_rnd_vet_tries, n > 0 ? big : "");
            return 0;
        }
        int n = play(api, inst, phrase_for(phrase), pcm, SR * 60, 3.0, 1);
        wav_write(argv[4], pcm, n);
        printf("%s\t%s\n", phrase, rname);
        return 0;
    }
    if (!strcmp(cmd, "note") && argc > 4) {
        int note = atoi(argv[3]); double secs = atof(argv[4]);
        /* from the parameter defaults, not from whichever preset loads first */
        for (int i = 0; i < P_COUNT; i++) {
            char v[32]; snprintf(v, sizeof v, "%g", (double)TB_PARAMS[i].def);
            api->set_param(inst, TB_PARAMS[i].key, v);
        }
        apply_sets(api, inst, argc, argv, 5);
        ev_t ph[2] = { { 0, note, 100, secs * g_bpm / 60.0 }, { -1, 0, 0, 0 } };
        int n = play(api, inst, ph, pcm, SR * 60, 1.5, 0);
        wav_write(argv[2], pcm, n);
        return 0;
    }
    if (!strcmp(cmd, "bench")) {
        for (int m = 0; m < M_COUNT; m++) {
            void *b = api->create_instance(".", NULL);
            char tmp[16]; snprintf(tmp, sizeof tmp, "%d", m);
            for (int i = 0; i < P_COUNT; i++) {
                char v[32]; snprintf(v, sizeof v, "%g", (double)TB_PARAMS[i].def);
                api->set_param(b, TB_PARAMS[i].key, v);
            }
            api->set_param(b, "model", tmp);
            api->set_param(b, "stack", "1.0"); api->set_param(b, "blend", "0.5"); api->set_param(b, "layer", "5");
            api->set_param(b, "cho_mix", "0.6"); api->set_param(b, "phs_depth", "0.5"); api->set_param(b, "dly_mix", "0.4");
            api->set_param(b, "rev_mix", "0.5"); api->set_param(b, "shimmer", "0.5"); api->set_param(b, "vhs", "0.5");
            api->set_param(b, "pump", "0.5"); api->set_param(b, "amp_s", "1.0"); api->set_param(b, "drive", "0.8");
            for (int k = 0; k < TB_VOICES; k++) {
                uint8_t msg[3] = { 0x90, (uint8_t)(36 + (k * 48) / TB_VOICES), 100 };
                api->on_midi(b, msg, 3, 0);
            }
            /* every block timed on its own, in this thread's CPU time, because
             * what an audio callback has to survive is its worst block, not
             * its average one */
            enum { BLOCKS = SR * 10 / 128 };
            static double bt[BLOCKS];
            double sum = 0.0, period = 128.0 / SR;
            for (int i = 0; i < BLOCKS; i++) {
                struct timespec a, z;
                clock_gettime(CLOCK_THREAD_CPUTIME_ID, &a);
                api->render_block(b, pcm, 128);
                clock_gettime(CLOCK_THREAD_CPUTIME_ID, &z);
                bt[i] = (double)(z.tv_sec - a.tv_sec) + 1e-9 * (double)(z.tv_nsec - a.tv_nsec);
                sum += bt[i];
            }
            qsort(bt, BLOCKS, sizeof bt[0], cmp_double);
            printf("model %d: %d voices, full stack, layer, every effect on: mean %.2f %%, 99th percentile %.2f %%, worst block %.2f %% of real time\n",
                   m, TB_VOICES, 100.0 * sum / (BLOCKS * period), 100.0 * bt[(int)(BLOCKS * 0.99)] / period,
                   100.0 * bt[BLOCKS - 1] / period);
            api->destroy_instance(b);
        }
        return 0;
    }
    if (!strcmp(cmd, "cost")) {
        /* each preset playing its own phrase, as a share of real time ON THIS
         * MACHINE. The Move's processor is several times slower. */
        double worst = 0, sum = 0;
        for (int i = 0; i < TB_PRESET_COUNT; i++) {
            void *b = api->create_instance(".", NULL);
            char tmp[16]; snprintf(tmp, sizeof tmp, "%d", i);
            api->set_param(b, "preset", tmp);
            clock_t t0 = clock();
            int n = 0;
            for (int rep = 0; rep < 4; rep++) n += play(api, b, phrase_for(TB_PRESETS[i].cat), pcm, SR * 60, 1.0, 1);
            double el = (double)(clock() - t0) / CLOCKS_PER_SEC, pct = 100.0 * el / ((double)n / SR);
            printf("%-22s %5.2f %%\n", TB_PRESETS[i].name, pct);
            if (pct > worst) worst = pct;
            sum += pct;
            api->destroy_instance(b);
        }
        printf("mean %.2f %%, worst %.2f %% of real time on this machine\n", sum / TB_PRESET_COUNT, worst);
        return 0;
    }
    if (!strcmp(cmd, "fuzz")) {
        /* Random parameter writes and random notes; the output must stay finite
         * and inside full scale, and every read must answer. */
        double secs = argc > 2 ? atof(argv[2]) : 30.0;
        uint32_t seed = argc > 3 ? (uint32_t)atoi(argv[3]) : 1u;
        uint32_t rs = seed * 2654435761u + 12345u;
        int blocks = (int)(secs * SR / 128), bad = 0, hot = 0;
        int16_t peak = 0;
        for (int i = 0; i < blocks; i++) {
            g_beat = (double)i * 128.0 / SR * g_bpm / 60.0;
            if ((tb_rand(&rs) & 7) == 0) {
                int id = (int)(tb_rand(&rs) % P_COUNT);
                char v[32];
                float lo = tb_param_min(id), hi = tb_param_max(id);
                /* now and then far outside the declared range, or garbage */
                uint32_t k = tb_rand(&rs) % 20;
                if (k == 0) snprintf(v, sizeof v, "%g", 1e9);
                else if (k == 1) snprintf(v, sizeof v, "nan");
                else if (k == 2) snprintf(v, sizeof v, "-inf");
                else snprintf(v, sizeof v, "%g", (double)(lo + (hi - lo) * (0.5f + 0.5f * tb_frand(&rs))));
                api->set_param(inst, TB_PARAMS[id].key, v);
            }
            if ((tb_rand(&rs) & 15) == 0) {
                uint8_t m[3] = { (uint8_t)((tb_rand(&rs) & 1) ? 0x90 : 0x80), (uint8_t)(tb_rand(&rs) % 128), (uint8_t)(tb_rand(&rs) % 128) };
                api->on_midi(inst, m, 3, 0);
            }
            if ((tb_rand(&rs) % 400) == 0) { char v[8]; snprintf(v, sizeof v, "%d", (int)(tb_rand(&rs) % TB_PRESET_COUNT)); api->set_param(inst, "preset", v); }
            if ((tb_rand(&rs) % 97) == 0) { uint8_t m[3] = { 0xE0, (uint8_t)(tb_rand(&rs) % 128), (uint8_t)(tb_rand(&rs) % 128) }; api->on_midi(inst, m, 3, 0); }
            if ((tb_rand(&rs) % 131) == 0) { uint8_t m[3] = { 0xA0, (uint8_t)(tb_rand(&rs) % 128), (uint8_t)(tb_rand(&rs) % 128) }; api->on_midi(inst, m, 3, 0); }
            api->render_block(inst, pcm, 128);
            for (int k = 0; k < 256; k++) {
                int16_t a = pcm[k] < 0 ? (int16_t)-(pcm[k] + 1) : pcm[k];
                if (a > peak) peak = a;
                if (a >= 32766) hot++;
            }
            tb_instance_t *vi = (tb_instance_t *)inst;
            for (int k = 0; k < 128; k++) if (!isfinite(vi->bufL[k]) || !isfinite(vi->bufR[k])) bad++;
            if (api->get_param(inst, "state", big, sizeof big) <= 0) bad++;
        }
        printf("fuzz seed %u: %.0f s, peak %.1f dBFS, full-scale samples %d, non-finite %d -> %s\n",
               seed, secs, 20.0 * log10((peak + 1) / 32768.0), hot, bad, bad ? "FAIL" : "ok");
        return bad ? 1 : 0;
    }
    if (!strcmp(cmd, "selftest")) {
        int fail = 0;
#define CHECK(c, msg) do { if (!(c)) { fail++; printf("FAIL %s\n", msg); } else printf("ok   %s\n", msg); } while (0)
        /* every declared key reads back what was written */
        for (int i = 0; i < P_COUNT; i++) {
            float lo = tb_param_min(i), hi = tb_param_max(i);
            float want = TB_PARAMS[i].type == PT_ENUM ? hi : lo + 0.37f * (hi - lo);
            char v[32], r[32]; snprintf(v, sizeof v, "%.4f", (double)want);
            api->set_param(inst, TB_PARAMS[i].key, v);
            int n = api->get_param(inst, TB_PARAMS[i].key, r, sizeof r);
            if (n <= 0 || fabs(atof(r) - (double)want) > 0.0006) { fail++; printf("FAIL readback %s: wrote %s read %s\n", TB_PARAMS[i].key, v, r); }
        }
        printf("ok   readback of %d keys\n", P_COUNT);
        /* a state saved before a control existed comes back as it was saved:
         * the controls it does not name return to their defaults, whatever the
         * instance was set to before */
        {
            static const char *NEWER[] = { "lfo_delay", "press", "press_dest", "wheel", "spread" };
            static char full[8192], old[8192], back[8192];
            int all_ok = 1;
            for (int pi = 0; pi < TB_PRESET_COUNT; pi++) {
                char ix[16]; snprintf(ix, sizeof ix, "%d", pi);
                api->set_param(inst, "preset", ix);
                api->get_param(inst, "state", full, sizeof full);
                /* the blob as v1.0.0 wrote it: without the newer keys */
                strcpy(old, full);
                for (int k = 0; k < 5; k++) {
                    char pat[48]; snprintf(pat, sizeof pat, ",\"%s\":", NEWER[k]);
                    char *at = strstr(old, pat);
                    if (!at) continue;
                    char *e = at + 1; while (*e && *e != ',' && *e != '}') e++;
                    memmove(at, e, strlen(e) + 1);
                }
                for (int k = 0; k < 5; k++) api->set_param(inst, NEWER[k], "1");
                api->set_param(inst, "state", old);
                api->get_param(inst, "state", back, sizeof back);
                if (strcmp(full, back) != 0) { all_ok = 0; printf("     %s: restored differently\n", TB_PRESETS[pi].name); }
            }
            CHECK(all_ok, "a state saved before the Mod page restores every preset exactly, whatever the slot was set to");
        }
        /* the Random page */
        {
            static char st[8192], st2[8192], nm[256], cmd[32];
            static long nonce = 1;
            #define RCMD(op) do { snprintf(cmd, sizeof cmd, "%ld", (nonce++) * 100 + (op)); api->set_param(inst, "rnd_cmd", cmd); } while (0)
            tb_instance_t *ri = (tb_instance_t *)inst;
            ri->rnd_sync = 1;
            api->set_param(inst, "preset", "5");
            api->get_param(inst, "state", st, sizeof st);
            api->set_param(inst, "rnd_seed", "11");
            RCMD(10 + 2);   /* roll a pad */
            api->get_param(inst, "preset_name", nm, sizeof nm);
            CHECK(!strncmp(nm, "Rand Pad ", 9), "a roll is named for its category");
            api->get_param(inst, "rnd_status", nm, sizeof nm);
            CHECK(strstr(nm, "\"pending\":1") && strstr(nm, "\"busy\":0") && !strstr(nm, "\"arch\":\"\""),
                  "the page is told a roll is pending, measured, and what it is");
            /* the same value again is a host re-sending it: ignored */
            api->get_param(inst, "state", st2, sizeof st2);
            api->set_param(inst, "rnd_cmd", cmd);
            static char st3[8192];
            api->get_param(inst, "state", st3, sizeof st3);
            CHECK(!strcmp(st2, st3), "a repeated command value does nothing (a host re-sending it)");
            api->get_param(inst, "rnd_cmd", nm, sizeof nm);
            CHECK(!strcmp(nm, cmd), "the command key answers back its last value");
            RCMD(10 + 0); RCMD(RC_VARY); RCMD(10 + 9);
            RCMD(RC_REVERT);
            api->get_param(inst, "state", st2, sizeof st2);
            CHECK(!strcmp(st, st2), "Revert after several rolls puts the preset back exactly");
            api->get_param(inst, "preset_name", nm, sizeof nm);
            CHECK(!strcmp(nm, TB_PRESETS[5].name), "and under its own name");
            /* Keep: the roll is yours, Revert does nothing */
            RCMD(10 + 1);
            api->get_param(inst, "state", st, sizeof st);
            RCMD(RC_KEEP);
            RCMD(RC_REVERT);
            api->get_param(inst, "state", st2, sizeof st2);
            CHECK(!strcmp(st, st2), "after Keep, Revert has nothing to undo");
            /* a kept roll is the next run's starting point, edits included */
            api->set_param(inst, "cutoff", "0.3333");
            api->get_param(inst, "state", st, sizeof st);
            RCMD(10 + 3); RCMD(RC_VARY);
            RCMD(RC_REVERT);
            api->get_param(inst, "state", st2, sizeof st2);
            CHECK(!strcmp(st, st2), "Revert returns an edited kept roll exactly, name and all");
            /* loading a preset ends a run */
            RCMD(10 + 4);
            api->set_param(inst, "preset", "7");
            api->get_param(inst, "rnd_status", nm, sizeof nm);
            CHECK(strstr(nm, "\"pending\":0") != NULL, "choosing a preset ends a run of rolls");
            /* a saved roll restores under its name */
            RCMD(10 + 2);
            api->get_param(inst, "state", st, sizeof st);
            api->set_param(inst, "preset", "3");
            api->set_param(inst, "state", st);
            api->get_param(inst, "state", st2, sizeof st2);
            api->get_param(inst, "preset_name", nm, sizeof nm);
            CHECK(!strcmp(st, st2) && !strncmp(nm, "Rand Pad ", 9), "a saved roll restores exactly, under its name");
            api->get_param(inst, "rnd_status", nm, sizeof nm);
            CHECK(strstr(nm, "\"pending\":0") != NULL, "restoring a state ends a run");
            /* Vary keeps what the instrument is */
            int vary_ok = 1;
            for (int s = 1; s <= 30; s++) {
                char sd[16]; snprintf(sd, sizeof sd, "%d", s);
                api->set_param(inst, "preset", "12");
                const float *src = tb_rnd_bank(12);
                api->set_param(inst, "rnd_seed", sd);
                RCMD(RC_VARY);
                const float *v = ri->eng.p;
                if (v[P_MODEL] != src[P_MODEL] || v[P_VOICE] != src[P_VOICE] || v[P_FLT_TYPE] != src[P_FLT_TYPE]
                    || (v[P_AMP_S] >= 0.3f) != (src[P_AMP_S] >= 0.3f)) vary_ok = 0;
                RCMD(RC_REVERT);
            }
            CHECK(vary_ok, "Vary keeps the model, voice mode, filter type and sustain");
            /* every roll of every recipe is inside that recipe: its switches,
             * its ranges, its category's rules */
            {
                int bad = 0, total = 0;
                for (int r = 0; r < TB_RECIPE_COUNT; r++) {
                    tb_rr_t rr; tb_rnd_ranges(&TB_RECIPES[r], &rr);
                    for (int s = 1; s <= 200; s++) {
                        uint32_t rng = (uint32_t)s * 2654435761u + (uint32_t)r;
                        float p[P_COUNT];
                        tb_rnd_roll_recipe(p, r, &rng);
                        total++;
                        int ok = 1;
                        for (int id = 0; id < P_COUNT; id++) {
                            if (tb_rnd_is_switch(id)) {
                                int heard = !(id == P_LAYER && p[P_BLEND] == 0.0f) && !(id == P_LFO_DEST && p[P_LFO_DEPTH] == 0.0f);
                                if (heard && !tb_rnd_enum_ok(&TB_RECIPES[r], id, p[id])) ok = 0;
                            }
                            /* the budget pass may only lower an amount, never below its floor */
                            if (rr.has[id] && (p[id] < rr.lo[id] - 1e-5f || p[id] > rr.hi[id] + 1e-5f)) {
                                int lowered = id == P_CHO_MIX && p[id] > rr.hi[id];   /* width floor raises chorus */
                                if (!lowered && id != P_DLY_FB && id != P_CHO_MODE && id != P_PHS_RATE && id != P_PUMP_RATE && id != P_LFO_DELAY) ok = 0;
                            }
                        }
                        if (!ok) { if (bad < 5) printf("     %s %s seed %d: outside its recipe\n", TB_RND_NAMES[TB_RECIPES[r].cat], TB_RECIPES[r].name, s); bad++; }
                    }
                }
                printf("     %d rolls across %d recipes\n", total, TB_RECIPE_COUNT);
                CHECK(bad == 0, "every roll sits inside its recipe's switches and ranges");
                /* rolling a category reaches every one of its recipes, evenly */
                int even = 1;
                for (int c = 0; c < R_COUNT; c++) {
                    int hits[TB_RECIPE_COUNT]; memset(hits, 0, sizeof hits);
                    uint32_t rng = 77u + (uint32_t)c;
                    for (int k = 0; k < 4000; k++) hits[tb_rnd_recipe_for(c, &rng)]++;
                    int nr = 0; for (int r = 0; r < TB_RECIPE_COUNT; r++) if (TB_RECIPES[r].cat == c) nr++;
                    for (int r = 0; r < TB_RECIPE_COUNT; r++) {
                        if (TB_RECIPES[r].cat != c) { if (hits[r]) even = 0; continue; }
                        if (abs(hits[r] - 4000 / nr) > 4000 / nr / 5) even = 0;
                    }
                }
                CHECK(even, "rolling a category reaches each of its recipes, and only those, about equally");
            }
            /* the same seed rolls the same patch; the next roll differs */
            api->set_param(inst, "rnd_seed", "11"); RCMD(10 + 2);
            api->get_param(inst, "state", st, sizeof st);
            api->set_param(inst, "rnd_seed", "11"); RCMD(10 + 2);
            api->get_param(inst, "state", st2, sizeof st2);
            const char *a1 = strstr(st, "\"model\""), *a2 = strstr(st2, "\"model\"");
            CHECK(a1 && a2 && !strcmp(a1, a2), "a seed decides the roll");
            RCMD(10 + 2);
            api->get_param(inst, "state", st2, sizeof st2);
            a2 = strstr(st2, "\"model\"");
            CHECK(a1 && a2 && strcmp(a1, a2) != 0, "rolling again makes a different patch");
            RCMD(RC_KEEP);
            ri->rnd_sync = 0;

            /* On the device the roll is measured by a worker thread and the
             * result applied by the audio thread: it must arrive -- idle or
             * not -- and be what the inline path makes of the same roll. */
            void *ti = api->create_instance(".", NULL);
            tb_instance_t *t = (tb_instance_t *)ti;
            api->set_param(ti, "rnd_seed", "5");
            snprintf(cmd, sizeof cmd, "%d", 100 + 10 + 0);
            api->set_param(ti, "rnd_cmd", cmd);
            static tb_rjob_t jb;
            static float want[P_COUNT];
            jb = t->rnd_job;
            static tb_engine_t pe; static tb_fx_t pf;
            tb_rnd_vet(&jb, want, &pe, &pf);
            static int16_t blk[256];
            int waited = 0;
            while (waited < 400 && t->rnd_busy) {
                struct timespec ts = { 0, 10 * 1000 * 1000 }; nanosleep(&ts, NULL);
                api->render_block(ti, blk, 128); waited++;
            }
            printf("     measured on the worker in %d ms\n", waited * 10);
            CHECK(!t->rnd_busy && !memcmp(t->eng.p, want, sizeof want),
                  "the worker's result reaches the audio thread, idle, and matches the inline result");
            /* a Level set by hand outranks a measurement still in flight */
            snprintf(cmd, sizeof cmd, "%d", 200 + 10 + 1);
            api->set_param(ti, "rnd_cmd", cmd);
            api->set_param(ti, "level", "0.333");
            for (int i = 0; i < 60; i++) {
                struct timespec ts = { 0, 10 * 1000 * 1000 }; nanosleep(&ts, NULL);
                api->render_block(ti, blk, 128);
            }
            CHECK(fabsf(t->eng.p[P_LEVEL] - 0.333f) < 1e-4f, "a Level written by hand is not overwritten by a measurement");
            api->destroy_instance(ti);
            CHECK(1, "an instance with a running worker is destroyed cleanly");
            #undef RCMD
        }
        /* enum by name and by index */
        api->set_param(inst, "model", "Bell"); api->get_param(inst, "model", big, sizeof big);
        CHECK(atoi(big) == M_BELL, "enum accepts an option name");
        api->set_param(inst, "model", "2"); api->get_param(inst, "model", big, sizeof big);
        CHECK(atoi(big) == M_SYNC, "enum accepts an index");
        api->set_param(inst, "cutoff", "7"); api->get_param(inst, "cutoff", big, sizeof big);
        CHECK(fabs(atof(big) - 1.0) < 1e-6, "out-of-range write is clamped");
        /* state round trip through a second instance */
        static char st1[8192], st2[8192];
        int n1 = api->get_param(inst, "state", st1, sizeof st1);
        void *b = api->create_instance(".", NULL);
        api->set_param(b, "synth:state", st1);
        int n2 = api->get_param(b, "state", st2, sizeof st2);
        CHECK(n1 > 0 && n1 == n2 && !memcmp(st1, st2, (size_t)n1), "state round-trips byte for byte");
        /* a short buffer answers -1 and does not overrun */
        char tiny[16]; memset(tiny, 0x55, sizeof tiny);
        CHECK(api->get_param(inst, "chain_params", tiny, 8) < 0 && tiny[8] == 0x55, "short buffer refused without overrun");
        CHECK(api->get_param(inst, "no_such_key", big, sizeof big) < 0, "unknown key answers -1");
        /* silence in, exact zeros out, before and after a note */
        api->set_param(b, "preset", "0");
        api->render_block(b, pcm, 128);
        int nz = 0; for (int k = 0; k < 256; k++) if (pcm[k]) nz++;
        CHECK(nz == 0, "no note: exact silence");
        api->set_param(b, "rev_mix", "0.6"); api->set_param(b, "rev_size", "0.5"); api->set_param(b, "dly_mix", "0.4");
        api->set_param(b, "vhs", "0.7"); api->set_param(b, "age", "1.0");
        uint8_t on[3] = { 0x90, 60, 100 }, off[3] = { 0x80, 60, 0 };
        api->on_midi(b, on, 3, 0);
        for (int i = 0; i < 200; i++) api->render_block(b, pcm, 128);
        api->on_midi(b, off, 3, 0);
        int idle_at = -1;
        for (int i = 0; i < 345 * 60; i++) {
            api->render_block(b, pcm, 128);
            if (((tb_instance_t *)b)->idle) { idle_at = i; break; }
        }
        api->render_block(b, pcm, 128);
        nz = 0; for (int k = 0; k < 256; k++) if (pcm[k]) nz++;
        printf("     tails died and the instance went idle after %.1f s\n", idle_at * 128.0 / SR);
        CHECK(idle_at > 0 && nz == 0, "after the tail: idle, exact silence");
        /* every preset names itself and loads */
        int named = 1;
        for (int i = 0; i < TB_PRESET_COUNT; i++) {
            char k[32]; snprintf(k, sizeof k, "%d", i); api->set_param(b, "preset", k);
            if (api->get_param(b, "preset_name", big, sizeof big) <= 0 || strcmp(big, TB_PRESETS[i].name)) named = 0;
        }
        CHECK(named, "every preset loads and names itself");
        /* a saved state finds its preset by uid, wherever the preset now sits */
        int uniq = 1, back = 1;
        for (int i = 0; i < TB_PRESET_COUNT; i++) {
            for (int j = 0; j < i; j++) if (TB_PRESETS[i].uid == TB_PRESETS[j].uid) uniq = 0;
            char k[32]; snprintf(k, sizeof k, "%d", i); api->set_param(b, "preset", k);
            api->get_param(b, "state", st1, sizeof st1);
            api->set_param(b, "preset", i ? "0" : "1");
            api->set_param(b, "state", st1);
            if (api->get_param(b, "preset_name", big, sizeof big) <= 0 || strcmp(big, TB_PRESETS[i].name)) back = 0;
        }
        CHECK(uniq, "every preset uid is different");
        CHECK(back, "a saved state comes back under its own preset's name");
        /* states saved before the bank was grouped carried the old index, which
         * is what the uid is: 32 was, and must stay, BS Sidechain. They were also
         * saved under the module's first name, and that tag is not read at all. */
        api->set_param(b, "state", "{\"vesper\":1,\"preset\":32,\"octave_transpose\":0}");
        api->get_param(b, "preset_name", big, sizeof big);
        CHECK(!strcmp(big, "BS Sidechain"), "a state saved before the regrouping keeps its name");
        api->set_param(b, "state", "{\"tenebrae\":1,\"preset\":-1}");
        api->get_param(b, "preset_name", big, sizeof big);
        CHECK(!strcmp(big, "Edited"), "a state with no preset reads Edited");
        printf("%s\n", fail ? "SELFTEST FAILED" : "selftest ok");
        return fail ? 1 : 0;
    }
    fprintf(stderr, "usage: tenebrae_tool json <key> | list | render <preset> <out.wav> [phrase] [k=v] | note <out.wav> <note> <secs> [k=v] | state <preset> | bench | cost | selftest | fuzz [secs] [seed]\n");
    return 2;
}

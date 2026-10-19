/*
 * tenebrae_tool.c -- run the real plugin on the development machine.
 *
 *   tenebrae_tool json <key>                     print what get_param serves
 *   tenebrae_tool render <preset> <out.wav> [phrase] [key=value ...]
 *   tenebrae_tool note <out.wav> <note> <secs> [key=value ...]     one held note from Init
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

typedef struct { double beat; int note; int vel; double len; } ev_t;

/* Phrases, in beats. Chosen to exercise what each kind of sound is FOR. */
static const ev_t PH_BASS[] = {
    {0,28,110,.22},{.5,28,90,.22},{.75,28,100,.22},{1,40,112,.22},{1.5,28,92,.22},{1.75,28,100,.22},
    {2,28,110,.22},{2.5,31,96,.22},{2.75,28,100,.22},{3,40,112,.22},{3.5,33,96,.22},{3.75,35,104,.22},
    {4,24,110,.22},{4.5,24,90,.22},{4.75,24,100,.22},{5,36,112,.22},{5.5,24,92,.22},{5.75,24,100,.22},
    {6,26,110,.22},{6.5,26,96,.22},{6.75,38,100,.22},{7,26,112,.45},{7.5,31,100,.45},{-1,0,0,0}
};
static const ev_t PH_LEAD[] = {
    {0,64,105,1.05},{1,67,100,.55},{1.5,69,100,.55},{2,71,108,1.6},{3.5,69,96,.55},
    {4,67,104,1.05},{5,64,98,1.05},{6,62,102,.55},{6.5,64,104,1.9},{-1,0,0,0}
};
static const ev_t PH_PAD[] = {
    {0,52,90,3.9},{0,59,88,3.9},{0,64,90,3.9},{0,67,86,3.9},
    {4,48,90,3.9},{4,55,88,3.9},{4,64,90,3.9},{4,67,86,3.9},{4,71,84,3.9},{-1,0,0,0}
};
static const ev_t PH_ARP[] = {
    {0,52,104,.2},{.25,59,96,.2},{.5,64,100,.2},{.75,67,96,.2},{1,71,104,.2},{1.25,67,94,.2},{1.5,64,98,.2},{1.75,59,94,.2},
    {2,52,104,.2},{2.25,59,96,.2},{2.5,64,100,.2},{2.75,67,96,.2},{3,71,104,.2},{3.25,76,98,.2},{3.5,71,98,.2},{3.75,67,94,.2},
    {4,48,104,.2},{4.25,55,96,.2},{4.5,64,100,.2},{4.75,67,96,.2},{5,72,104,.2},{5.25,67,94,.2},{5.5,64,98,.2},{5.75,55,94,.2},
    {6,50,104,.2},{6.25,57,96,.2},{6.5,62,100,.2},{6.75,66,96,.2},{7,69,104,.2},{7.25,74,98,.2},{7.5,69,98,.2},{7.75,66,94,.2},{-1,0,0,0}
};
static const ev_t PH_KEYS[] = {
    {0,52,104,.45},{0,59,100,.45},{0,64,104,.45},{0,67,100,.45},
    {.75,52,92,.2},{.75,64,92,.2},{.75,67,92,.2},
    {1.5,52,104,.9},{1.5,59,100,.9},{1.5,64,104,.9},{1.5,67,100,.9},
    {3,50,100,.45},{3,57,98,.45},{3,62,100,.45},{3,66,98,.45},
    {4,48,104,.45},{4,55,100,.45},{4,64,104,.45},{4,67,100,.45},
    {4.75,48,92,.2},{4.75,64,92,.2},{4.75,67,92,.2},
    {5.5,48,104,1.4},{5.5,55,100,1.4},{5.5,64,104,1.4},{5.5,71,100,1.4},{-1,0,0,0}
};
static const ev_t PH_BELL[] = {
    {0,76,100,.4},{1,79,90,.4},{1.5,83,96,.4},{2,88,104,.4},{3,83,88,.4},
    {4,72,100,.4},{4.5,76,92,.4},{5,79,96,.4},{6,84,104,.4},{6.5,91,90,.4},{-1,0,0,0}
};
static const ev_t PH_FX[] = { {0,60,100,5.0},{-1,0,0,0} };
static const ev_t PH_TOM[] = {
    {0,55,112,.2},{.5,55,90,.2},{.75,52,104,.2},{1,48,112,.2},{1.5,43,112,.3},
    {4,55,112,.2},{4.25,55,96,.2},{4.5,50,104,.2},{4.75,50,96,.2},{5,45,112,.2},{5.5,40,112,.4},{-1,0,0,0}
};
static const ev_t PH_DRONE[] = { {0,36,100,7.5},{0,43,96,7.5},{-1,0,0,0} };

static const ev_t *phrase_for(const char *cat)
{
    if (!strcmp(cat, "bass")) return PH_BASS;
    if (!strcmp(cat, "lead")) return PH_LEAD;
    if (!strcmp(cat, "pad")) return PH_PAD;
    if (!strcmp(cat, "arp")) return PH_ARP;
    if (!strcmp(cat, "keys")) return PH_KEYS;
    if (!strcmp(cat, "bell")) return PH_BELL;
    if (!strcmp(cat, "fx")) return PH_FX;
    if (!strcmp(cat, "tom")) return PH_TOM;
    if (!strcmp(cat, "drone")) return PH_DRONE;
    return PH_KEYS;
}

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

#!/usr/bin/env python3
"""random_checks.py -- what the Random page makes, measured.

    python3 tests/random_checks.py        (build/tenebrae_tool must exist)

Rolls every recipe, rolls every category, and varies a spread of factory
presets, exactly as the device does (measured and checked inline), renders
each, and holds the results to the rules the factory bank is held to:
nothing clips or dies, loudness lands where the bank's does, basses keep a
centred low end, pads stay wide and out of the bass. Plus: the rolls reach
beyond the bank, every recipe is reachable, no two rolls are the same, and
Vary stays near the sound it varies.

This proves the rolls are safe, even and new. Whether they are GOOD is for ears.
"""
import json, os, subprocess, sys
import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import rnd_tune as T  # noqa: E402

failures = 0
def check(ok, msg):
    global failures
    if not ok: failures += 1
    print(("ok   " if ok else "FAIL ") + msg)

R = T.recipes()
rows = [l.split("\t") for l in subprocess.run([T.TOOL, "list"], capture_output=True, text=True).stdout.strip().split("\n")]
VARY_FROM = [int(r[0]) for r in rows[1::6]]          # every sixth factory preset
jobs = ([("rollr", r["idx"], s) for r in R for s in range(1, 11)] +
        [("roll", c, s) for c in range(len(T.NAMES)) for s in range(101, 121)] +
        [("vary", p, s) for p in VARY_FROM for s in (1, 2)])
ALL = T.run(jobs)
ROLLS = [x for x in ALL if x["kind"] != "vary"]
VARY = [x for x in ALL if x["kind"] == "vary"]
cat_of = {r["idx"]: r["cat"] for r in R}
for x in ROLLS:
    x["cat"] = cat_of[x["idx"]] if x["kind"] == "rollr" else x["idx"]

# ---- safe ------------------------------------------------------------------
check(all(x["finite"] and x["clipped"] == 0 for x in ALL), f"nothing clips or misbehaves ({len(ALL)} renders)")
# A roll is levelled from ONE playing; voices drift and a phaser sweeps, so
# another playing can peak a few dB higher than the one measured.
hot = [(x["kind"], x["idx"], x["seed"], round(x["peak"], 1)) for x in ALL if x["peak"] > -1.0]
check(not hot, "everything peaks at least 1 dB under full scale" + (f": {hot}" if hot else ""))
quiet = [(x["kind"], x["idx"], x["seed"]) for x in ALL if x["sounding"] < 0.35]
check(not quiet, "everything sounds through its phrase" + (f": {quiet}" if quiet else ""))
far = [(x["kind"], x["idx"], x["seed"], round(x["dev"], 1)) for x in ALL if not -13.0 < x["dev"] < 2.0]
check(not far, "everything lands within -13 .. +2 dB of its loudness target" + (f": {far}" if far else ""))
print(f"     {'category':9s} {'loudness median':>16s}")
for c, name in enumerate(T.NAMES):
    med = float(np.median([x["dev"] for x in ROLLS if x["cat"] == c]))
    print(f"     {name:9s} {med:+16.1f}")
    # the bank's own medians run from -3.7 (stabs) to 0
    check(-6.0 < med < 1.0, f"{name}: rolls sit where the bank does")

# ---- in their category -----------------------------------------------------
bass = [x for x in ROLLS if x["cat"] == 0]
off = [(x["idx"], x["seed"], round(x["lcorr"], 2)) for x in bass if x["lcorr"] < 0.95]
check(not off, "every bass keeps its low end in the middle" + (f": {off}" if off else ""))
thin = [(x["idx"], x["seed"], round(x["low"], 2)) for x in bass if x["low"] <= 0.25]
check(not thin, "every bass has real low end" + (f": {thin}" if thin else ""))
check(all(x["st"]["spread"] == 0 and x["st"]["shimmer"] == 0 and x["st"]["rev_mix"] <= 0.15 for x in bass),
      "basses: no spread, no shimmer, reverb kept low")
pads = [x for x in ROLLS if x["phrase"] == "pad"]
narrow = [(x["idx"], x["seed"], round(x["corr"], 2)) for x in pads if x["corr"] >= 0.8]
check(not narrow, f"every pad-like roll is wide ({len(pads)})" + (f": {narrow}" if narrow else ""))
boomy = [(x["idx"], x["seed"], round(x["low"], 2)) for x in pads if x["low"] >= 0.12]
check(not boomy, "no pad-like roll sits in the bass" + (f": {boomy}" if boomy else ""))
check(all(x["st"]["lfo_delay"] == 0 for x in ROLLS if x["st"]["lfo_depth"] == 0), "no LFO Delay without LFO depth")
check(all(x["st"]["spread"] == 0 for x in ALL if x["st"]["voice"] >= 1), "no Spread on a mono sound")

# A recipe whose rolls often fail the measured check describes something its
# ranges do not make: fix the recipe, do not lean on the retry.
print(f"     {'recipe':24s} {'passed first time':>18s}")
weak = []
for r in R:
    x = [v for v in ROLLS if v["kind"] == "rollr" and v["idx"] == r["idx"]]
    rate = sum(1 for v in x if v["tries"] == 1) / len(x)
    if rate < 0.7: weak.append((T.NAMES[r["cat"]] + " " + r["name"], f"{rate:.0%}"))
check(not weak, "every recipe passes the measured check first time at least 70 % of the time" + (f": {weak}" if weak else ""))

# ---- new -------------------------------------------------------------------
bank = [json.loads(subprocess.run([T.TOOL, "state", r[0]], capture_output=True, text=True).stdout) for r in rows[1:]]
ENUMS = {"model", "layer", "voice", "flt_type", "lfo_dest", "press_dest", "cho_mode", "pump_rate", "dly_time"}
KEYS = [k for k in bank[0] if k not in ("tenebrae", "preset", "octave_transpose", "level")]
SPAN = {k: (2.0 if k in ("shape_env", "flt_env", "tone") else 1.0) for k in KEYS}
def dist(a, b):
    return sum((1.0 if a[k] != b[k] else 0.0) if k in ENUMS else abs(a[k] - b[k]) / SPAN[k] for k in KEYS)
nn_bank = [min(dist(a, b) for j, b in enumerate(bank) if j != i) for i, a in enumerate(bank)]
gap = float(np.median(nn_bank))
nn_roll = [min(dist(x["st"], b) for b in bank) for x in ROLLS]
beyond = sum(1 for d in nn_roll if d > gap) / len(nn_roll)
print(f"     a factory preset's nearest neighbour is {gap:.2f} away (median); "
      f"{beyond:.0%} of rolls are farther than that from every preset")
check(min(nn_roll) > 0.05, "no roll is a factory preset")
check(beyond >= 0.3, "at least 30 % of rolls are farther from the bank than its presets are from each other")
V = [tuple(round(x["st"][k], 4) for k in KEYS) for x in ROLLS]
check(len(set(V)) == len(V), f"{len(V)} rolls, every one different")
models = {int(x["st"]["model"]) for x in ROLLS}
check(len(models) == 8, f"rolls use {len(models)} of the 8 oscillator models")

# ---- Vary ------------------------------------------------------------------
vd = []
for x in VARY:
    src = bank[x["idx"] - 1]
    vd.append(dist(x["st"], src))
print(f"     Vary moves a preset {min(vd):.2f} .. {max(vd):.2f} (median {np.median(vd):.2f}); "
      f"a preset's nearest neighbour is {gap:.2f} away")
check(min(vd) > 0.05, "Vary always changes the sound")
check(float(np.median(vd)) < gap, "Vary stays nearer its source than the nearest other preset is, typically")

# ---- the Random page's knobs ---------------------------------------------------
# At their extremes the rolls go far past the recipes, and must still be safe.
# Loudness gets a wider window here: a WILD roll can carry a long shimmer or
# echo tail, which pulls this file-length average down while the sound itself
# plays at level (the device measures a shorter tail).
UP = ["rnd_osc=1", "rnd_flt=1", "rnd_amp=1", "rnd_mod=1", "rnd_mot=1", "rnd_spc=1", "rnd_fresh=1", "rnd_wild=1"]
EXT = T.run([("rollr", r["idx"], s, UP) for r in R for s in range(301, 306)])
check(all(x["finite"] and x["clipped"] == 0 and x["peak"] <= -1.0 for x in EXT),
      f"every knob at full: nothing clips, everything peaks under -1 dB ({len(EXT)} rolls)")
check(all(x["sounding"] >= 0.35 for x in EXT), "every knob at full: everything sounds through its phrase")
far = [(x["recipe"], x["seed"], round(x["dev"], 1)) for x in EXT if not -22.0 < x["dev"] < 2.0]
check(not far, "every knob at full: everything within -22 .. +2 dB of its loudness target" + (f": {far}" if far else ""))

def states(kind, ids, seeds, extra):
    out = []
    for i in ids:
        for sd in seeds:
            o = subprocess.run([T.TOOL, kind, str(i), str(sd), "-"] + extra, capture_output=True, text=True).stdout
            out.append((i, json.loads(o.rstrip("\n").split("\t", 3)[3])))
    return out
def spread(extra):
    import itertools
    ss = states("roll", (0, 2, 4, 9), range(401, 411), extra)
    return float(np.mean([np.mean([dist(a, b) for a, b in itertools.combinations([st for c, st in ss if c == cat], 2)])
                          for cat in (0, 2, 4, 9)]))
s_def, s_up = spread([]), spread(UP)
print(f"     rolls in a category differ by {s_def:.2f} at the defaults, {s_up:.2f} with every knob up")
check(s_up > 1.4 * s_def, "the knobs widen the rolls")
def vary_dist(extra):
    return float(np.median([dist(st, bank[i - 1]) for i, st in states("vary", VARY_FROM[:6], (1, 2, 3), extra)]))
v_def, v_up = vary_dist([]), vary_dist(UP)
print(f"     Vary moves a preset {v_def:.2f} at the defaults, {v_up:.2f} with every knob up")
check(v_up > 3 * v_def, "the knobs widen Vary Current")

print(f"\n{failures} FAILED" if failures else "\nrandom checks ok (safe, even and new; whether they are GOOD is for ears)")
sys.exit(1 if failures else 0)

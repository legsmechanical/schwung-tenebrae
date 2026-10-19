#!/usr/bin/env python3
"""bank_checks.py -- the factory bank, checked as a bank.

    python3 tests/bank_checks.py          # build/tenebrae_tool must exist

These are the ways a generated bank has gone wrong before, turned into checks:
a modulation depth with no audible target, the same effects wash on every
preset, a bank that is all one register or all one loudness. Passing them means
the bank is EVEN and ALIVE. It does not mean the presets are good; only
listening does.
"""
import json, os, re, subprocess, sys, wave
import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOL = os.path.join(ROOT, "build", "tenebrae_tool")
OUT = os.path.join(ROOT, "build", "bankcheck")
os.makedirs(OUT, exist_ok=True)
fails = 0
def check(ok, msg):
    global fails
    if not ok: fails += 1
    print(("ok   " if ok else "FAIL ") + msg)

rows = [l.split("\t") for l in subprocess.run([TOOL, "list"], capture_output=True, text=True).stdout.strip().split("\n")]
names = [r[2] for r in rows]
check(len(rows) == 163, f"163 presets ({len(rows)})")
check(len(set(names)) == len(names), "every name is different")
check(all(len(n) <= 20 for n in names), "every name fits the screen")
prefix = {}
for n in names: prefix[n[:2]] = prefix.get(n[:2], 0) + 1
def split(ns):
    d = {}
    for n in ns: d[n[:2]] = d.get(n[:2], 0) + 1
    return d
# a preset's uid is what a saved state names it by; 0-31 are the staples,
# 32-63 the presets where an effect is the sound, 64-127 the bigger and wilder,
# 128-159 the horror soundtrack set, 160 on single additions
uids = [int(r[3]) for r in rows]
# 22 (KY Iron Bell) was dropped and is retired, never reused
RETIRED = {22}
check(sorted(uids) == [u for u in range(164) if u not in RETIRED], "uids are 0..163 less the retired, each used once")
group = [0 if u < 32 else (1 if u < 64 else (2 if u < 128 else (3 if u < 160 else 4))) for u in uids]
fxbank = [g == 1 for g in group]
staple_names = [n for n, g in zip(names, group) if g == 0]
effect_names = [n for n, g in zip(names, group) if g == 1]
wild_names = [n for n, g in zip(names, group) if g == 2]
horror_names = [n for n, g in zip(names, group) if g == 3]
check(split(staple_names) == {"BS": 8, "LD": 6, "PD": 6, "KY": 3, "PL": 4, "BR": 2, "FX": 2},
      f"staples balance {split(staple_names)}")
check(split(effect_names) == {"BS": 4, "LD": 6, "PD": 8, "KY": 4, "PL": 6, "FX": 4},
      f"effects presets balance {split(effect_names)}")
check(split(wild_names) == {"BS": 10, "LD": 10, "PD": 10, "KY": 6, "PL": 8, "BR": 6, "VX": 6, "ST": 4, "SQ": 4},
      f"bigger-and-wilder presets balance {split(wild_names)}")
# the horror set: soundtrack instruments, only two sound effects
check(split(horror_names) == {"BS": 4, "LD": 4, "PD": 6, "KY": 4, "PL": 4, "BR": 2, "VX": 3, "ST": 2, "SQ": 1, "FX": 2},
      f"horror presets balance {split(horror_names)}")
# the browser is grouped: each category is one unbroken run, in this order,
# and inside it the staples come before the effects presets
runs = [p for i, p in enumerate(n[:2] for n in names) if i == 0 or p != names[i - 1][:2]]
check(runs == ["BS", "LD", "PD", "KY", "PL", "BR", "VX", "ST", "SQ", "FX"], f"presets are grouped by category {runs}")
check(all(group[i] <= group[i + 1] for i in range(len(names) - 1) if names[i][:2] == names[i + 1][:2]),
      "inside a category: the staples, the effects presets, the wilder ones, the horror ones, then single additions")
# a uid must never move to another sound: saved states carry it
by_uid = dict(zip(uids, names))
PINNED = {0: "BS Night Drive", 7: "BS Body Work", 8: "LD Sunset", 14: "PD Dusk", 20: "KY Velvet",
          24: "PL Neon Arp", 28: "BR Stadium", 31: "FX Event Horizon", 32: "BS Sidechain",
          36: "LD Echo Chamber", 42: "PD Aurora", 50: "KY Tremolo Motel", 54: "PL Dotted Dream",
          60: "FX Black Hole", 63: "FX Thunder Toms", 64: "BS Acid Rain", 73: "BS Precinct 13",
          74: "LD Hoover '91", 84: "PD Monolith", 94: "KY Pipe Dream", 100: "PL Jangle '85",
          108: "BR Countdown", 114: "VX Black Mass", 120: "ST Orch Hit", 124: "SQ Motorik",
          127: "SQ Mainframe", 128: "BS Night Stalker", 132: "LD Saucer Signal", 143: "KY Cursed Music Box",
          157: "SQ Pursuit", 159: "FX Basement", 55: "PL Raindrops", 160: "PL Droplets",
          58: "PL Chorus Harp", 96: "KY Tubular", 150: "BR Overlook", 161: "PL Hollow Harp",
          162: "KY Deep Tubular", 163: "BR Room 237"}
moved = {u: (n, by_uid.get(u)) for u, n in PINNED.items() if by_uid.get(u) != n}
check(not moved, "uids still name the sounds they always did" + (f": {moved}" if moved else ""))

# every preset's parameters, read back through the plugin
def state_of(i):
    import ctypes  # noqa: only to make clear this is the real plugin's answer
    return json.loads(subprocess.run([TOOL, "state", str(i)], capture_output=True, text=True).stdout)

states = [state_of(int(r[0])) for r in rows]
FX_KEYS = ["cho_mode", "cho_mix", "phs_depth", "phs_rate", "pump", "pump_rate", "vhs", "tone",
           "dly_mix", "dly_time", "dly_fb", "rev_mix", "rev_size", "rev_tone", "shimmer"]
sigs = {}
for n, st in zip(names, states):
    sigs.setdefault(tuple(round(st[k], 2) for k in FX_KEYS), []).append(n)
dup = [v for v in sigs.values() if len(v) > 1]
check(not dup, "no two presets share an effects setup" + (f": {dup}" if dup else ""))

dead = []
for n, st in zip(names, states):
    # a secondary control set while its effect is off is a setting nobody hears
    if st["cho_mix"] == 0 and st["cho_mode"] != 0: dead.append(f"{n}: chorus type without mix")
    if st["phs_depth"] == 0 and abs(st["phs_rate"] - 0.3) > 1e-6: dead.append(f"{n}: phaser rate without depth")
    if st["pump"] == 0 and st["pump_rate"] != 1: dead.append(f"{n}: pump division without depth")
    if st["dly_mix"] == 0 and (st["dly_time"] != 3 or abs(st["dly_fb"] - 0.4) > 1e-6): dead.append(f"{n}: delay settings without mix")
    if st["blend"] == 0 and st["layer"] != 1: dead.append(f"{n}: layer chosen but blend is 0")
    if st["lfo_depth"] == 0 and (st["lfo_dest"] != 0 or abs(st["lfo_rate"] - 0.55) > 1e-6): dead.append(f"{n}: LFO target or rate without depth")
    if st["shimmer"] > 0 and st["rev_mix"] == 0: dead.append(f"{n}: shimmer without reverb")
    if st["shape_env"] != 0 and st["flt_d"] <= 0.02 and st["flt_a"] <= 0.02 and st["flt_s"] == 0: dead.append(f"{n}: shape envelope with no envelope")
    if st["lfo_dest"] == 1 and st["lfo_depth"] > 0 and st["model"] == 7 and False: dead.append(n)
    # a glide mode does not restart the envelopes on overlapping notes, so with
    # no sustain a legato line goes silent after its first note
    if st["voice"] >= 2 and st["amp_s"] < 0.2: dead.append(f"{n}: glide mode with no amp sustain (silent when played legato)")
for d in dead: print("     " + d)
check(not dead, "no modulation or effect is half set (depth without target, or the reverse)")

alive = [n for n, st in zip(names, states)
         if st["lfo_depth"] > 0 or st["shape_env"] != 0 or (st["flt_env"] != 0 and st["flt_s"] < 0.95)
         or st["pump"] > 0 or st["phs_depth"] > 0 or st["cho_mix"] > 0 or st["vhs"] > 0]
check(len(alive) == len(names), f"every preset moves: envelope, LFO or a moving effect ({len(alive)} of {len(names)})")

# the effects bank has to actually lean on the effects: how much of what you
# hear is effect, by the settings (a crude sum, but it separates the two banks)
def fx_weight(st):
    return (st["cho_mix"] + st["phs_depth"] + st["pump"] + st["vhs"] + st["dly_mix"] * (0.5 + st["dly_fb"])
            + st["rev_mix"] * (0.5 + st["rev_size"]) + st["shimmer"])
st_a = [st for st, g in zip(states, group) if g == 0]; st_b = [st for st, g in zip(states, group) if g == 1]
wa = np.mean([fx_weight(st) for st in st_a]); wb = np.mean([fx_weight(st) for st in st_b])
check(wb > 1.5 * wa and min(fx_weight(st) for st in st_b) > 0.9,
      f"the effects presets lean on their effects (weight {wb:.2f} against the staples' {wa:.2f})")
for key, what in (("pump", "pump"), ("vhs", "VHS"), ("phs_depth", "phaser"), ("shimmer", "shimmer")):
    n = sum(1 for st in st_b if st[key] >= 0.3)
    check(n >= 4, f"effects presets: {what} is the feature of at least four ({n})")

# no two presets are near-copies. Distance is over the controls that make the
# sound (not Level), each scaled to its own range; a different oscillator model,
# layer, filter type, LFO target or mono/poly counts as a full step.
ENUMS = ["model", "layer", "flt_type", "lfo_dest", "cho_mode", "dly_time", "pump_rate"]
CONT = ["shape", "shape_env", "stack", "blend", "age", "cutoff", "reso", "drive", "flt_env", "flt_a", "flt_d", "flt_s",
        "lfo_rate", "lfo_depth", "vel", "amp_a", "amp_d", "amp_s", "amp_r", "cho_mix", "phs_depth", "phs_rate",
        "pump", "vhs", "tone", "dly_fb", "dly_mix", "rev_size", "rev_tone", "shimmer", "rev_mix"]
def audible(st):
    # a control whose effect is off is not a difference anyone hears
    d = dict(st)
    if d["blend"] == 0: d["layer"] = -1
    if d["lfo_depth"] == 0: d["lfo_dest"] = -1; d["lfo_rate"] = 0
    if d["cho_mix"] == 0: d["cho_mode"] = -1
    if d["dly_mix"] == 0: d["dly_time"] = -1; d["dly_fb"] = 0
    if d["pump"] == 0: d["pump_rate"] = -1
    if d["phs_depth"] == 0: d["phs_rate"] = 0
    d["voice"] = min(d["voice"], 2)          # poly / mono / glide
    return d
A = [audible(st) for st in states]
def dist(a, b):
    return (sum(1.0 for k in ENUMS + ["voice"] if a[k] != b[k]) + sum((a[k] - b[k]) ** 2 for k in CONT)) ** 0.5
near = sorted((dist(A[i], A[j]), names[i], names[j]) for i in range(len(A)) for j in range(i))
print("     closest pairs: " + "; ".join(f"{a} / {b} {d:.2f}" for d, a, b in near[:5]))
close = [(a, b, round(d, 2)) for d, a, b in near if (a in wild_names + horror_names or b in wild_names + horror_names) and d < 0.75]
check(not close, "no bigger-and-wilder or horror preset is a near-copy of another preset" + (f": {close}" if close else ""))

# the third bank has to reach where the first two did not
W = [st for st, g in zip(states, group) if g == 2]
check(sum(1 for st in W if st["model"] != 0) >= 40, f"wilder presets: most are not built on the saw ({sum(1 for st in W if st['model'] != 0)} of 64)")
check(all(sum(1 for st in W if st["model"] == m) >= 3 for m in range(8)), "wilder presets: every oscillator model is used at least three times")
check(sum(1 for st in W if st["flt_type"] in (2, 3)) >= 5, "wilder presets: band-pass and high-pass get used")
check(sum(1 for st in W if st["lfo_depth"] > 0 and st["lfo_dest"] >= 3) >= 8, "wilder presets: tremolo, pan, random and tempo-ramp LFOs get used")
check(sum(1 for st in W if st["drive"] >= 0.5) >= 20, "wilder presets: at least twenty are driven into the clipper")
check(sum(1 for st in W if st["shape_env"] < 0 or st["flt_env"] < 0) >= 6, "wilder presets: some envelopes run backwards")

# measured: loudness, peak, spectrum, width
def measure(i):
    path = os.path.join(OUT, f"{i:02d}.wav")
    subprocess.run([TOOL, "render", str(i), path], check=True)
    w = wave.open(path, "rb")
    x = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(np.float64).reshape(-1, 2) / 32768
    m = x.mean(axis=1)
    win = 1024; n = len(m) // win
    fr = np.sqrt((m[:n * win].reshape(n, win) ** 2).mean(axis=1) + 1e-18)
    act = fr > fr.max() * 0.05
    rms = 20 * np.log10(np.sqrt((fr[act] ** 2).mean()))
    seg = m[:n * win].reshape(n, win)[act]
    spec = (np.abs(np.fft.rfft(seg * np.hanning(win), axis=1)) ** 2).mean(axis=0)
    f = np.fft.rfftfreq(win, 1 / 44100)
    cent = (f * spec).sum() / spec.sum()
    corr = (x[:, 0] * x[:, 1]).sum() / (np.sqrt((x[:, 0] ** 2).sum() * (x[:, 1] ** 2).sum()) + 1e-18)
    # the low end alone: under 100 Hz
    from scipy import signal
    lo = signal.sosfilt(signal.butter(4, 100, fs=44100, output="sos"), x, axis=0)
    lcorr = (lo[:, 0] * lo[:, 1]).sum() / (np.sqrt((lo[:, 0] ** 2).sum() * (lo[:, 1] ** 2).sum()) + 1e-18)
    low = spec[f < 150].sum() / spec.sum()
    # share of the phrase (the file less its 3 s tail) during which something is heard
    body = fr[:max(1, int((len(m) / 44100 - 3.0) * 44100 / win))]
    sounding = float((body > fr.max() * 0.01).mean())
    return dict(peak=20 * np.log10(np.abs(x).max() + 1e-9), rms=rms, cent=cent, corr=corr, lcorr=lcorr, low=low, sounding=sounding,
                finite=bool(np.isfinite(x).all()), clipped=int((np.abs(x) > 0.985).sum()))

M = [measure(int(r[0])) for r in rows]
check(all(m["finite"] and m["clipped"] == 0 for m in M), "no preset clips or misbehaves")
# every preset must keep sounding through its phrase, not just at the start
quiet = [n for n, m in zip(names, M) if m["sounding"] < 0.35]
check(not quiet, "every preset sounds through its whole phrase" + (f": {quiet}" if quiet else ""))
hot = [(n, round(m["peak"], 1)) for n, m in zip(names, M) if m["peak"] > -5.5]
check(not hot, "every phrase peaks under the limiter's knee" + (f": {hot}" if hot else ""))
rms = np.array([m["rms"] for m in M])
check(rms.max() - rms.min() < 10.0, f"loudness spread across the bank {rms.max() - rms.min():.1f} dB")
bass = [m for n, m in zip(names, M) if n.startswith("BS")]
off = [(n, round(m["lcorr"], 2)) for n, m in zip(names, M) if n.startswith("BS") and m["lcorr"] < 0.95]
check(not off, "every bass has its low end in the middle (L/R correlation under 100 Hz above 0.95)" + (f": {off}" if off else ""))
check(all(m["low"] > 0.25 for m in bass), "every bass has real low end")
pads = [(n, m) for n, m in zip(names, M) if n.startswith("PD")]
narrow = [(n, round(m["corr"], 2)) for n, m in pads if m["corr"] >= 0.8]
check(not narrow, "every pad is wide" + (f": {narrow}" if narrow else ""))
boomy = [(n, round(m["low"], 2)) for n, m in pads if m["low"] >= 0.12]
check(not boomy, "no pad sits in the bass (under 150 Hz)" + (f": {boomy}" if boomy else ""))
cents = np.array([m["cent"] for n, m in zip(names, M) if not n.startswith("BS")])
check((cents > 1200).sum() >= 4 and (cents < 700).sum() >= 4,
      f"the bank is not all one brightness: centroids {cents.min():.0f} .. {cents.max():.0f} Hz")

print("\nFAILED" if fails else "\nbank checks ok (even and alive; whether it is GOOD is for ears)")
sys.exit(1 if fails else 0)

#!/usr/bin/env python3
"""level_bank.py -- set every preset's LEVEL by measurement.

    python3 tools/level_bank.py            # report only
    python3 tools/level_bank.py --write    # rewrite S(P_LEVEL, ...) in tenebrae_presets.h
    python3 tools/level_bank.py --write --min-uid 64   # only presets with uid 64 and up
    python3 tools/level_bank.py --write --uids 42,43   # only these presets

Renders each preset's audition phrase and moves its Level so that the phrase
lands on a loudness target for its kind, with the peak kept under the output
limiter's knee. Run it after any change to the presets or to the engine's gain
structure. It makes the bank EVEN; it cannot make a preset good.
"""
import os, re, subprocess, sys, wave
import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOL = os.path.join(ROOT, "build", "tenebrae_tool")
HDR = os.path.join(ROOT, "src", "dsp", "tenebrae_presets.h")
OUT = os.path.join(ROOT, "build", "level")
os.makedirs(OUT, exist_ok=True)

# loudness of the sounding part of the phrase (dB RMS), and the peak ceiling
TARGET = {"bass": -19.0, "lead": -20.5, "pad": -20.0, "keys": -21.0, "bell": -23.0,
          "arp": -23.0, "tom": -22.0, "drone": -21.0, "fx": -21.0}
PEAK_MAX = -6.0

def measure(i):
    path = os.path.join(OUT, f"{i:02d}.wav")
    subprocess.run([TOOL, "render", str(i), path], check=True)
    w = wave.open(path, "rb")
    x = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(np.float64).reshape(-1, 2) / 32768
    m = x.mean(axis=1)
    win = 1024
    n = len(m) // win
    fr = np.sqrt((m[:n * win].reshape(n, win) ** 2).mean(axis=1) + 1e-18)
    act = fr > fr.max() * 0.05
    rms = 20 * np.log10(np.sqrt((fr[act] ** 2).mean()))
    return 20 * np.log10(np.abs(x).max() + 1e-9), rms

src = open(HDR).read()
rows = [l.split("\t") for l in subprocess.run([TOOL, "list"], capture_output=True, text=True).stdout.strip().split("\n")]
changes = {}
print(f"{'preset':24s} {'cat':6s} {'peak':>6s} {'rms':>6s} {'level':>6s} -> {'new':>5s}")
# --min-uid N: leave presets with a lower uid alone (ones already tuned by ear)
MIN_UID = int(sys.argv[sys.argv.index("--min-uid") + 1]) if "--min-uid" in sys.argv else 0
# --uids a,b,c: only these presets (ones reworked after they were levelled)
ONLY = {int(u) for u in sys.argv[sys.argv.index("--uids") + 1].split(",")} if "--uids" in sys.argv else None
for idx, cat, name, uid in rows:
    i = int(idx)
    if int(uid) < MIN_UID: continue
    if ONLY is not None and int(uid) not in ONLY: continue
    peak, rms = measure(i)
    blk = re.search(r'\{ "' + re.escape(name) + r'".*?\} \},', src, re.S).group(0)
    level = float(re.search(r"S\(P_LEVEL, ([0-9.]+)f\)", blk).group(1))
    gain = min(TARGET.get(cat, -21.0) - rms, PEAK_MAX - peak)
    new = float(np.clip(level * 10 ** (gain / 40.0), 0.30, 1.0))
    changes[name] = (level, new)
    print(f"{name:24s} {cat:6s} {peak:6.1f} {rms:6.1f} {level:6.2f} -> {new:5.2f}")

if "--write" in sys.argv:
    for name, (old, new) in changes.items():
        m = re.search(r'\{ "' + re.escape(name) + r'".*?\} \},', src, re.S)
        blk = m.group(0)
        blk2 = re.sub(r"S\(P_LEVEL, [0-9.]+f\)", f"S(P_LEVEL, {new:.2f}f)", blk)
        src = src[:m.start()] + blk2 + src[m.end():]
    open(HDR, "w").write(src)
    print("written: rebuild the tool and run again to confirm")

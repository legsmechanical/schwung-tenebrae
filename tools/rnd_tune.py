#!/usr/bin/env python3
"""rnd_tune.py -- how each Random page recipe actually sounds, measured.

    python3 tools/rnd_tune.py [rolls-per-recipe]   (build/tenebrae_tool must exist)

Renders rolls of every recipe exactly as the device makes them (measured and
checked inline) and prints, per recipe: how often the first roll passed the
measured check, loudness against its target, the worst peak, and the
descriptors a recipe is written to -- brightness (spectral centroid), low-end
share, width (left/right correlation, 1 = mono), movement (how much the
level swings) and how much of the phrase it sounds through.

Use it to tune tenebrae_recipes.h: a recipe whose first-try pass rate is low
is describing something its ranges do not make. tests/random_checks.py
imports the measuring code from here.
"""
import json, os, subprocess, sys, wave
from concurrent.futures import ThreadPoolExecutor
import numpy as np
from scipy import signal

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOL = os.path.join(ROOT, "build", "tenebrae_tool")
OUT = os.path.join(ROOT, "build", "rnd")
os.makedirs(OUT, exist_ok=True)
NAMES = ["Bass", "Lead", "Pad", "Keys", "Pluck", "Brass", "Voice", "Stab", "Sequence", "FX"]
TARGET = {"bass": -19.0, "lead": -20.5, "pad": -20.0, "keys": -21.0, "bell": -23.0,
          "arp": -23.0, "tom": -22.0, "drone": -21.0, "fx": -21.0}
SOS = signal.butter(4, 100, fs=44100, output="sos")


def recipes():
    out = subprocess.run([TOOL, "recipes"], capture_output=True, text=True, check=True).stdout
    return [dict(idx=int(a), cat=int(b), name=c, phrase=d)
            for a, b, c, d in (l.split("\t") for l in out.strip().split("\n"))]


def measure_file(path):
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
    corr = (x[:, 0] * x[:, 1]).sum() / (np.sqrt((x[:, 0] ** 2).sum() * (x[:, 1] ** 2).sum()) + 1e-18)
    lo = signal.sosfilt(SOS, x, axis=0)
    lcorr = (lo[:, 0] * lo[:, 1]).sum() / (np.sqrt((lo[:, 0] ** 2).sum() * (lo[:, 1] ** 2).sum()) + 1e-18)
    body = fr[:max(1, int((len(m) / 44100 - 3.0) * 44100 / win))]
    sb = body[body > body.max() * 0.05] if body.max() > 0 else body
    swing = float(np.std(20 * np.log10(sb + 1e-9))) if len(sb) > 2 else 0.0
    return dict(peak=20 * np.log10(np.abs(x).max() + 1e-9), rms=rms,
                cent=float((f * spec).sum() / spec.sum()), corr=float(corr), lcorr=float(lcorr),
                low=float(spec[f < 150].sum() / spec.sum()),
                sounding=float((body > fr.max() * 0.01).mean()), swing=swing,
                finite=bool(np.isfinite(x).all()), clipped=int((np.abs(x) > 0.985).sum()))


def roll(kind, idx, seed, extra=()):
    """kind: roll (category) | rollr (recipe) | vary (preset)."""
    out = subprocess.run([TOOL, kind, str(idx), str(seed), "-"] + list(extra),
                         capture_output=True, text=True, check=True).stdout
    phrase, rname, tries, blob = out.rstrip("\n").split("\t", 3)
    path = os.path.join(OUT, f"{kind}_{idx}_{seed}.wav")
    subprocess.run([TOOL, kind, str(idx), str(seed), path] + list(extra), capture_output=True, check=True)
    r = measure_file(path)
    r.update(kind=kind, idx=idx, seed=seed, phrase=phrase, recipe=rname, tries=int(tries),
             st=json.loads(blob), dev=r["rms"] - TARGET.get(phrase, -21.0))
    return r


def run(jobs):
    with ThreadPoolExecutor(os.cpu_count() or 4) as ex:
        return list(ex.map(lambda j: roll(*j), jobs))


if __name__ == "__main__":
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 20
    R = recipes()
    res = run([("rollr", r["idx"], s) for r in R for s in range(1, n + 1)])
    print(f"{'recipe':24s} {'1st ok':>6s} {'loud med':>8s} {'min':>5s} {'peak':>5s} "
          f"{'bright':>6s} {'low':>5s} {'corr':>5s} {'swing':>5s} {'sound':>5s}")
    for r in R:
        x = [v for v in res if v["idx"] == r["idx"]]
        first = sum(1 for v in x if v["tries"] == 1) / len(x)
        med = lambda k: float(np.median([v[k] for v in x]))
        print(f"{NAMES[r['cat']] + ' ' + r['name']:24s} {first:6.0%} {med('dev'):8.1f} "
              f"{min(v['dev'] for v in x):5.1f} {max(v['peak'] for v in x):5.1f} {med('cent'):6.0f} "
              f"{med('low'):5.2f} {med('corr'):5.2f} {med('swing'):5.1f} {min(v['sounding'] for v in x):5.2f}")

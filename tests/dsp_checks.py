#!/usr/bin/env python3
"""dsp_checks.py -- does the engine do what the knobs say?

Renders single notes through build/tenebrae_tool (the real plugin, built for this
machine) and measures them: tuning, the filter's corner, envelope and delay
times, the reverb's decay, the pump, that nothing runs away.

    python3 tests/dsp_checks.py            # build/tenebrae_tool must exist
"""
import os, subprocess, sys, tempfile, wave
import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOL = os.path.join(ROOT, "build", "tenebrae_tool")
TMP = tempfile.mkdtemp(prefix="tenebrae_")
SR = 44100
fails = 0

# a dry, static starting point: nothing moves unless a test asks it to
DRY = dict(age=0, rev_mix=0, dly_mix=0, cho_mix=0, phs_depth=0, vhs=0, pump=0, tone=0,
           drive=0, flt_env=0, vel=0, amp_a=0, amp_s=1, amp_r=0.1, lfo_depth=0, stack=0, blend=0,
           cutoff=1, reso=0, level=0.7)

def render(note=60, secs=1.0, **kw):
    p = dict(DRY); p.update(kw)
    out = os.path.join(TMP, "t.wav")
    args = [TOOL, "note", out, str(note), str(secs)] + [f"{k}={v}" for k, v in p.items()]
    subprocess.run(args, check=True)
    w = wave.open(out, "rb")
    x = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(np.float64) / 32768.0
    return x.reshape(-1, 2)

def check(ok, msg):
    global fails
    if not ok: fails += 1
    print(("ok   " if ok else "FAIL ") + msg)

def peak_freq(x, lo=20, hi=20000):
    n = len(x)
    s = np.abs(np.fft.rfft(x * np.hanning(n)))
    f = np.fft.rfftfreq(n, 1 / SR)
    m = (f >= lo) & (f <= hi)
    i = np.argmax(s * m)
    # parabolic refinement
    a, b, c = np.log(s[i - 1] + 1e-12), np.log(s[i] + 1e-12), np.log(s[i + 1] + 1e-12)
    d = 0.5 * (a - c) / (a - 2 * b + c)
    return (i + d) * SR / n

def env(x, win=441):
    n = len(x) // win
    return np.sqrt((x[:n * win].reshape(n, win) ** 2).mean(axis=1) + 1e-20)

# ---- tuning ---------------------------------------------------------------
for model, name in [(0, "Saw"), (1, "Pulse"), (3, "FM"), (5, "Wave"), (6, "Vox")]:
    x = render(69, 1.5, model=model, shape=0.3)[:, 0]
    f = peak_freq(x[SR // 4:SR * 5 // 4], 300, 600)
    check(abs(f - 440.0) < 0.6, f"{name}: A4 sounds at {f:.2f} Hz")

# ---- saw: a harmonic series, and what is NOT a harmonic stays low -----------
x = render(45, 2.0, model=0, shape=0.5)[:, 0]          # A2, 110 Hz
seg = x[SR // 2:SR // 2 + 65536]
S = np.abs(np.fft.rfft(seg * np.hanning(len(seg))))
fr = np.fft.rfftfreq(len(seg), 1 / SR)
harm = np.zeros(len(S), bool)
for h in range(1, 200):
    harm |= np.abs(fr - 110.0 * h) < 6
inharm = 10 * np.log10((S[~harm & (fr > 200)] ** 2).sum() / (S[harm] ** 2).sum())
check(inharm < -35, f"saw at A2: everything between the harmonics is {inharm:.1f} dB down")
x = render(96, 1.0, model=0, shape=0.5)[:, 0]          # C7
seg = x[SR // 4:SR // 4 + 32768]
S = np.abs(np.fft.rfft(seg * np.hanning(len(seg)))); fr = np.fft.rfftfreq(len(seg), 1 / SR)
f0 = 2093.0
harm = np.zeros(len(S), bool)
for h in range(1, 11):
    harm |= np.abs(fr - f0 * h) < 25
alias = 10 * np.log10((S[~harm & (fr > 100)] ** 2).sum() / (S[harm] ** 2).sum())
print(f"     saw at C7: aliasing {alias:.1f} dB under the harmonics (PolyBLEP, by design)")

# ---- filter: the corner is where the knob says ------------------------------
# pure noise layer, 12 dB low-pass, key 48 (the key-tracking pivot) so tracking adds nothing
for cut in (0.4, 0.5, 0.6, 0.7):
    x = render(48, 3.0, layer=7, blend=1.0, flt_type=1, cutoff=cut, reso=0.0)[:, 0]
    seg = x[SR // 2:SR * 5 // 2]
    n = 4096
    k = len(seg) // n
    P = (np.abs(np.fft.rfft(seg[:k * n].reshape(k, n) * np.hanning(n), axis=1)) ** 2).mean(axis=0)
    fr = np.fft.rfftfreq(n, 1 / SR)
    want = 20.0 * 2 ** (10 * cut)
    ref = P[(fr > want * 0.08) & (fr < want * 0.3)].mean()
    # smooth, then find where it has fallen 3 dB
    Ps = np.convolve(P, np.ones(9) / 9, mode="same")
    idx = np.where((fr > want * 0.3) & (Ps < ref * 0.5))[0]
    got = fr[idx[0]] if len(idx) else 0
    check(abs(np.log2(got / want)) < 0.45, f"low-pass at {cut:.0%}: corner {got:.0f} Hz (knob says {want:.0f})")

# ---- nothing runs away: full resonance, every type, both ends of Drive ------
worst = 0
for ft in range(4):
    for cut in (0.0, 0.3, 0.6, 0.85, 1.0):
        for drive in (0.0, 1.0):
            x = render(36, 0.6, flt_type=ft, cutoff=cut, reso=1.0, drive=drive, stack=1.0, blend=0.5)
            worst = max(worst, np.abs(x).max())
            if not np.isfinite(x).all(): worst = 9
check(worst < 0.99, f"full resonance, every type, cutoff and drive: peak {20 * np.log10(worst):.1f} dBFS")

# ---- envelope times ---------------------------------------------------------
x = render(72, 1.0, amp_a=0.5, layer=0, blend=1.0)[:, 0]        # sine, attack 50 %
e = env(x)
t90 = np.argmax(e > 0.9 * e[:95].max()) / 100.0
want_a = 0.001 * 8000 ** 0.5
check(0.3 * want_a < t90 < want_a, f"attack at 50 %: 90 % after {t90 * 1000:.0f} ms (ramp {want_a * 1000:.0f} ms)")
x = render(72, 0.3, amp_r=0.5, layer=0, blend=1.0)[:, 0]
e = env(x)
off = int(0.3 * 60 / 110 * 100 * 110 / 60)   # note length in 10 ms frames
tail = e[off:]
t60 = np.argmax(tail < tail[0] * 0.001) / 100.0
want_r = 0.012 * 2000 ** 0.5
check(0.5 * want_r < t60 <= 1.1 * want_r, f"release at 50 %: 60 dB down after {t60 * 1000:.0f} ms (ramp {want_r * 1000:.0f} ms)")

# ---- delay: a dotted eighth at 110 BPM is 409 ms ----------------------------
# (the sine SUB would not do here: it is the clean path and skips the delay)
x = render(84, 0.05, model=7, shape=0.0, amp_r=0.0, dly_mix=1.0, dly_fb=0.5, dly_time=3, bpm=110)
el, er = env(x[:, 0], 441), env(x[:, 1], 441)
def onsets(e):
    on = np.concatenate([[False], e > 0.01 * max(el.max(), er.max())])   # the dry note starts in frame 0
    return [i - 1 for i in range(1, len(on)) if on[i] and not on[i - 1]]
ol, orr = onsets(el), onsets(er)
t1 = ol[1] / 100.0 if len(ol) > 1 else 0          # the dry note starts at 0
check(abs(t1 - 0.4091) < 0.02, f"delay 1/8 dotted at 110 BPM: first repeat after {t1 * 1000:.0f} ms")
i1, i2 = int(0.4091 * 100) + 2, int(2 * 0.4091 * 100) + 2      # inside the first and second repeat
check(el[i1] > 10 * er[i1] and er[i2] > 10 * el[i2],
      f"delay is ping-pong: first repeat left ({20 * np.log10(er[i1] / el[i1] + 1e-9):.0f} dB on the right), "
      f"second right ({20 * np.log10(el[i2] / er[i2] + 1e-9):.0f} dB on the left)")

# ---- reverb: decay follows Size ---------------------------------------------
for size in (0.25, 0.5, 0.75):
    x = render(60, 0.1, layer=7, blend=1.0, amp_r=0.0, rev_mix=1.0, rev_size=size, rev_tone=0.8)
    # the tool renders 1.5 s after the note: enough to read the slope
    e = env(x.mean(axis=1), 441)
    i0 = int(0.25 * 100)
    seg = 20 * np.log10(e[i0:i0 + 100] + 1e-12)
    slope = np.polyfit(np.arange(len(seg)) / 100.0, seg, 1)[0]
    rt = -60.0 / slope if slope < 0 else 99
    want = 0.4 * 60 ** size
    check(0.5 * want < rt < 1.8 * want, f"reverb size {size:.0%}: RT60 {rt:.1f} s (designed {want:.1f})")

# ---- shimmer at its worst: longest tail, full shimmer, a held chord ----------
x = render(48, 12.0, stack=0.6, rev_mix=1.0, rev_size=1.0, shimmer=1.0, rev_tone=1.0, amp_r=0.2)
e = env(x.mean(axis=1), 4410)
check(np.abs(x).max() < 0.98 and np.isfinite(x).all(), f"shimmer, worst case: peak {20 * np.log10(np.abs(x).max()):.1f} dBFS")
half = len(e) * 2 // 3
print(f"     level over the held note: start {20 * np.log10(e[5]):.1f} dB, middle {20 * np.log10(e[len(e) // 2]):.1f} dB, end {20 * np.log10(e[-16]):.1f} dB")

# ---- pump: a quarter-note duck ----------------------------------------------
subprocess.run([TOOL, "render", "0", os.path.join(TMP, "p.wav"), "fx", "pump=1.0", "pump_rate=1", "bpm=120",
                "rev_mix=0", "amp_s=1", "age=0", "flt_env=0", "vel=0"], check=True)
w = wave.open(os.path.join(TMP, "p.wav"), "rb")
x = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(np.float64).reshape(-1, 2)[:, 0] / 32768
e = env(x, 441)[50:250]
ac = np.correlate(e - e.mean(), e - e.mean(), "full")[len(e) - 1:]
lag = np.argmax(ac[30:80]) + 30
check(abs(lag - 50) <= 2, f"pump at 1/4, 120 BPM: repeats every {lag * 10} ms")
check(e.min() < 0.12 * e.max(), f"pump at full depth ducks to {20 * np.log10(e.min() / e.max()):.1f} dB")

# ---- VHS: pitch wobble of the right size ------------------------------------
# note 81 with the sine layer alone (an octave under the key) is 440 Hz
from scipy import signal
def cents_track(x):
    y = signal.sosfiltfilt(signal.butter(4, [300, 600], btype="band", fs=SR, output="sos"), x)
    ph = np.unwrap(np.angle(signal.hilbert(y)))
    f = np.diff(ph) * SR / (2 * np.pi)
    f = signal.sosfiltfilt(signal.butter(2, 30, fs=SR, output="sos"), f)[SR // 2:-SR * 2]
    return 1200 * np.log2(f / 440.0)
c = cents_track(render(81, 6.0, layer=0, blend=1.0, vhs=1.0)[:, 0])
check(25 < c.max() - c.min() < 90, f"VHS at full: pitch wanders {c.min():.0f} .. {c.max():+.0f} cents")
c = cents_track(render(81, 3.0, layer=0, blend=1.0, vhs=0.0)[:, 0])
check(abs(c).max() < 0.5, "VHS at 0 leaves the pitch alone")
c = cents_track(render(81, 6.0, layer=0, blend=1.0, age=1.0)[:, 0])
check(10 < c.max() - c.min() < 90, f"Age at full: pitch wanders {c.min():.0f} .. {c.max():+.0f} cents")
c = cents_track(render(81, 3.0, layer=0, blend=1.0, age=0.0)[:, 0])
check(abs(c).max() < 0.5, "Age at 0 is dead in tune")

# ---- stack: wide up top, centred at the bottom ------------------------------
x = render(69, 2.0, stack=0.0)
corr0 = np.corrcoef(x[SR // 2:, 0], x[SR // 2:, 1])[0, 1]
x = render(69, 2.0, stack=0.8)
corr1 = np.corrcoef(x[SR // 2:SR * 2, 0], x[SR // 2:SR * 2, 1])[0, 1]
check(corr0 > 0.999 and corr1 < 0.7, f"stack: mono at 0 (corr {corr0:.3f}), wide at 80 % (corr {corr1:.2f})")
x = render(31, 2.0, stack=0.8)
corr2 = np.corrcoef(x[SR // 2:SR * 2, 0], x[SR // 2:SR * 2, 1])[0, 1]
check(corr2 > 0.995, f"stack at 80 % on a low G: still in the middle (corr {corr2:.3f})")

# ---- the low end goes back to the middle, the rest does not -------------------
x = render(36, 2.0, stack=0.0, cho_mix=1.0, cho_mode=4, phs_depth=0.8, cutoff=0.8)   # C2 saw, wide effects
lo = signal.sosfilt(signal.butter(4, 100, fs=SR, output="sos"), x[SR // 2:], axis=0)
hi = signal.sosfilt(signal.butter(4, 600, btype="high", fs=SR, output="sos"), x[SR // 2:], axis=0)
cl = np.corrcoef(lo[:, 0], lo[:, 1])[0, 1]; ch = np.corrcoef(hi[:, 0], hi[:, 1])[0, 1]
check(cl > 0.93 and ch < 0.8, f"chorus + phaser on a low note: under 100 Hz stays centred (corr {cl:.2f}), above 600 Hz is wide (corr {ch:.2f})")

# ---- the sine sub stays clean under full drive -------------------------------
x = render(45, 2.0, layer=0, blend=1.0, drive=1.0, cutoff=0.3)[:, 0]     # the sub alone, A1 = 55 Hz
seg = x[SR // 2:SR // 2 + 65536]
S = np.abs(np.fft.rfft(seg * np.hanning(len(seg)))); fr = np.fft.rfftfreq(len(seg), 1 / SR)
fund = (S[np.abs(fr - 55.0) < 4] ** 2).sum()
thd = 10 * np.log10(((S[fr > 80] ** 2).sum() + 1e-20) / fund)
check(thd < -30, f"sine sub at full Drive: harmonics {thd:.0f} dB under the fundamental")

# ---- and it skips the space effects: no echo, no reverb tail ------------------
x = render(57, 0.2, layer=0, blend=1.0, amp_r=0.0, dly_mix=1.0, dly_fb=0.6, rev_mix=1.0, rev_size=0.8)
e = env(x.mean(axis=1), 441)
check(e[40:].max() < 1e-4 * e.max() + 1e-6, "sine sub alone leaves no delay or reverb tail")

print("\nFAILED" if fails else "\nall dsp checks ok")
sys.exit(1 if fails else 0)

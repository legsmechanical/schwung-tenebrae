#!/usr/bin/env python3
"""analyze.py -- numbers for a rendered WAV: level, spectrum, width, movement.

    python3 tools/analyze.py a.wav [b.wav ...]

Not a judge of whether a sound is GOOD. It answers the questions a render can
answer: is it silent, clipped, lopsided, static, mono, or all top / all bottom.
"""
import sys, wave
import numpy as np

def load(path):
    w = wave.open(path, 'rb')
    n = w.getnframes()
    x = np.frombuffer(w.readframes(n), dtype=np.int16).astype(np.float64) / 32768.0
    return x.reshape(-1, 2), w.getframerate()

def db(v):
    return 20 * np.log10(max(v, 1e-9))

def stats(path):
    x, sr = load(path)
    m = x.mean(axis=1)
    peak = np.abs(x).max()
    # active part: frames above -50 dB of peak
    win = 1024
    nfr = len(m) // win
    fr = np.sqrt((m[:nfr * win].reshape(nfr, win) ** 2).mean(axis=1) + 1e-18)
    act = fr > max(fr.max() * 0.003, 1e-6)
    rms = np.sqrt((m[:nfr * win].reshape(nfr, win)[act] ** 2).mean()) if act.any() else 0.0
    # long-term spectrum of the active part
    seg = m[:nfr * win].reshape(nfr, win)[act] if act.any() else m[:win].reshape(1, win)
    spec = (np.abs(np.fft.rfft(seg * np.hanning(win), axis=1)) ** 2).mean(axis=0)
    f = np.fft.rfftfreq(win, 1 / sr)
    cent = (f * spec).sum() / max(spec.sum(), 1e-18)
    bands = [(20, 120), (120, 500), (500, 2000), (2000, 6000), (6000, 20000)]
    tot = spec.sum() + 1e-18
    bd = [10 * np.log10(spec[(f >= a) & (f < b)].sum() / tot + 1e-12) for a, b in bands]
    # stereo
    l, r = x[:, 0], x[:, 1]
    den = np.sqrt((l * l).sum() * (r * r).sum()) + 1e-18
    corr = (l * r).sum() / den
    side = np.sqrt(((l - r) ** 2).mean() + 1e-18) / (np.sqrt(((l + r) ** 2).mean()) + 1e-18)
    # movement: how much the short-term centroid wanders while sounding
    cs = []
    for k in range(0, nfr):
        if not act[k]: continue
        s = np.abs(np.fft.rfft(m[k * win:(k + 1) * win] * np.hanning(win))) ** 2
        cs.append((f * s).sum() / max(s.sum(), 1e-18))
    move = (np.std(cs) / (np.mean(cs) + 1e-9)) if cs else 0.0
    dc = abs(m.mean())
    tail = fr[-int(sr / win * 0.25):].mean() if nfr > 20 else 0
    clip = int((np.abs(x) >= 0.999).sum())
    return dict(peak=db(peak), rms=db(rms), cent=cent, bands=bd, corr=corr, side=db(side), move=move,
                dc=dc, clip=clip, tail=db(tail), secs=len(m) / sr)

if __name__ == '__main__':
    print(f"{'file':28s} {'peak':>6s} {'rms':>6s} {'cent':>6s}  {'sub':>5s} {'low':>5s} {'mid':>5s} {'hi':>5s} {'air':>5s}  {'corr':>5s} {'side':>6s} {'move':>5s} {'tail':>6s} clip")
    for p in sys.argv[1:]:
        s = stats(p)
        b = s['bands']
        name = p.split('/')[-1][:28]
        print(f"{name:28s} {s['peak']:6.1f} {s['rms']:6.1f} {s['cent']:6.0f}  {b[0]:5.1f} {b[1]:5.1f} {b[2]:5.1f} {b[3]:5.1f} {b[4]:5.1f}  {s['corr']:5.2f} {s['side']:6.1f} {s['move']:5.2f} {s['tail']:6.1f} {s['clip']}")

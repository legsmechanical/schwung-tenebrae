#!/usr/bin/env python3
"""spectro.py -- spectrogram contact sheets for a folder of rendered WAVs.

    python3 tools/spectro.py build/audition build/audition/sheet     # sheet_1.png ...

Log-frequency, 40 Hz - 16 kHz, 70 dB range, level drawn underneath. Eight to a
sheet. For looking at envelopes, filter movement, tails and anything that
should not be there; it does not tell you whether a sound is good.
"""
import glob, os, sys, wave
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

def load(p):
    w = wave.open(p, "rb")
    x = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(np.float64) / 32768.0
    return x.reshape(-1, 2), w.getframerate()

def spec(m, sr, n=4096, hop=512):
    win = np.hanning(n)
    frames = [np.abs(np.fft.rfft(m[i:i + n] * win)) for i in range(0, len(m) - n, hop)]
    S = np.array(frames).T
    f = np.fft.rfftfreq(n, 1 / sr)
    edges = np.geomspace(40, 16000, 161)
    out = np.zeros((160, S.shape[1]))
    for b in range(160):
        sel = (f >= edges[b]) & (f < edges[b + 1])
        if sel.any(): out[b] = S[sel].max(axis=0)
        else: out[b] = S[np.argmin(np.abs(f - edges[b]))]
    return 20 * np.log10(out / (n / 4) + 1e-9)

files = sorted(glob.glob(os.path.join(sys.argv[1], "*.wav")))
per = 8
for s in range(0, len(files), per):
    chunk = files[s:s + per]
    fig, axes = plt.subplots(2, 4, figsize=(20, 8))
    for ax, p in zip(axes.flat, chunk):
        x, sr = load(p)
        m = x.mean(axis=1)
        D = spec(m, sr)
        ax.imshow(D, origin="lower", aspect="auto", cmap="magma", vmin=-80, vmax=-10,
                  extent=[0, len(m) / sr, 0, 160])
        ticks = [50, 100, 200, 500, 1000, 2000, 5000, 10000]
        ax.set_yticks([160 * np.log(t / 40) / np.log(400) for t in ticks])
        ax.set_yticklabels([str(t) if t < 1000 else f"{t // 1000}k" for t in ticks], fontsize=7)
        e = np.sqrt(np.convolve(m ** 2, np.ones(1024) / 1024, mode="same"))
        ax2 = ax.twinx()
        ax2.plot(np.arange(len(e)) / sr, 20 * np.log10(e + 1e-6), color="cyan", lw=0.6, alpha=0.8)
        ax2.set_ylim(-70, 0); ax2.set_yticks([])
        ax.set_title(os.path.basename(p)[:-4], fontsize=10)
    for ax in axes.flat[len(chunk):]: ax.axis("off")
    plt.tight_layout()
    out = f"{sys.argv[2]}_{s // per + 1}.png"
    plt.savefig(out, dpi=55)
    plt.close()
    print(out)

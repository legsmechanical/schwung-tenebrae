# Developing Tenebrae

Everything here is for working on the module itself. For playing it, see the
[README](README.md).

## Where things are

```
src/dsp/
  tenebrae_params.h    every parameter, its page and its graphic: the source of truth
  tenebrae_engine.h    voices: oscillator models, stack, layer, drive, filter, envelopes, LFO
  tenebrae_fx.h        tone, chorus, phaser, delay, reverb + shimmer, pump, VHS, level
  tenebrae_presets.h   the factory presets (each lists only what differs from the defaults)
  tenebrae.c           plugin API v2 wrapper: JSON, state, MIDI, idle handling
src/host/              plugin_api_v1.h, move_info.h, copied unchanged from Schwung
tools/                 tenebrae_tool.c (the plugin built for this machine), level_bank.py, spectro.py, ...
tests/                 contract.test.mjs, dsp_checks.py, bank_checks.py
scripts/               build.sh, test.sh, install.sh
```

A preset is named in a saved state by its `uid`, never by its place in the
browser: presets can be reordered freely, but a uid is never changed or
reused. A new preset takes the next unused number.

## Build, test, install

```bash
./scripts/build.sh      # native tool + module.json, then the aarch64 cross-build (Docker)
./scripts/test.sh       # everything that can be checked off the device
./scripts/install.sh    # copies dist/tenebrae to the Move; restarts nothing
```

`install.sh` writes to the stock Schwung tree
(`/data/UserData/schwung/modules/sound_generators/tenebrae`) and does not restart
the host. A new module appears after the next host restart.

### Hearing it without the device

```bash
./build/tenebrae_tool render 12 out.wav         # preset 13 (LD Sunset), its own phrase
./build/tenebrae_tool note out.wav 48 2 model=3 shape=0.6     # one note from the defaults
python3 tools/spectro.py build/audition build/audition/sheet
```

`tenebrae_tool` includes the plugin source and drives it through the same entry
points the host calls, so what it renders is the module.

## What has and has not been checked

Checked, off the device (`./scripts/test.sh`):

- the contract the host is served: five pages of eight, five graphics that
  resolve, no host validator warnings; `module.json` agrees with the plugin;
- the engine against its own knobs: tuning, the filter corner, envelope and
  delay times, reverb decay, the pump, VHS and Age pitch drift, that full
  resonance and full shimmer stay bounded, that the sine sub stays clean;
- the API: read-back of every key, state round trip, exact silence when idle,
  90 s of random parameter writes and notes with no non-finite sample;
- the bank: levels set by measurement, no half-set modulation, no shared
  effects setup, basses centred, pads wide and out of the low end.

**Not checked:**

- The presets were written from published recipes and tuned by measurement
  and spectrogram, and were picked over by ear as a bank rather than one by
  one. Measurement makes a bank even; it cannot make it good.
- **CPU inside the host is unmeasured.** The cost of the code itself was
  measured on a Move (2026-10-12, `tools/bench_move.sh`: the plugin source
  built as a program and run on the device, one core, CPU time against real
  time). The presets playing their phrases (the 128 that existed then) average 2.0 %, the heaviest
  (PD Monolith) 5.8 %. The worst case that can be built, with every voice
  held, full stack, a layer and every effect on:

  | Voices | Lightest model | Heaviest model (Bell) | Worst single block |
  |---|---|---|---|
  | 8 (shipped) | 5.2 % | 8.6 % | 9.8 % |
  | 12 | 6.9 % | 12.1 % | 13.2 % |
  | 16 | 8.6 % | 15.6 % | 16.8 % |

  That is about 1.6 % for the effects and 0.5-0.9 % per voice. Inside the host
  the same code shares a core and its cache with everything else on the
  device, so these are a floor, not a promise.
- Aftertouch, the mod wheel, sustain pedal and pitch bend are implemented and
  untested on hardware.


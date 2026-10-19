# Tenebrae

A macro synth for synthwave, darksynth and darkwave, as a Schwung sound
generator for Ableton Move.

Three pages make the sound, two polish it. Every knob is a macro: one turn
moves several things underneath, and the ranges are cut so that most of the
travel is usable.

**Status: 0.1.0, first build.** Installed and played on one Move, and its CPU cost
measured there outside the host. See
[What has and has not been checked](#what-has-and-has-not-been-checked).

## The five pages

| Page | Row 1 | Row 2 |
|---|---|---|
| **Osc** | Model · Shape · Shape Env · Stack | Layer · Blend · Age · Glide |
| **Filter** | Type · Cutoff · Resonance · Drive | Env Amount · Attack · Decay · Sustain |
| **Amp + LFO** | LFO Target · Rate · Depth · Velocity | Attack · Decay · Sustain · Release |
| **FX Motion** | Chorus Type · Mix · Phaser · Rate | Pump · Division · VHS · Tone |
| **FX Space** | Delay · Time · Feedback · Level | Reverb · Size · Color · Shimmer |

The preset browser is the first page; the host adds its own My Presets and
Module pages after the last.

### Osc

**Model** picks the oscillator; **Shape** is that model's one tone control.

| Model | What it is | Shape |
|---|---|---|
| Saw | the analog poly saw | triangle → saw → saw + square |
| Pulse | pulse / PWM | width, square → thin |
| Sync | hard sync | how far the slave is swept |
| FM | two-operator FM at 1:1 | bite (the FM amount) |
| Bell | inharmonic FM | struck metal → clang |
| Wave | a wavetable | soft, organ, hollow, reed, glass, voice, comb, spike |
| Vox | formants | oo – oh – ah – eh – ee |
| Zap | a sine with a pitch drop | how far it falls: toms, kicks, lasers |

- **Shape Env** lets the filter envelope move Shape, up or down. This is where
  FM plucks, sync sweeps and envelope PWM come from.
- **Stack** runs from one oscillator to seven. To about 10 % it is three
  oscillators a few cents apart; 10–30 % pulls those three wide (a reese);
  above that a second and a third pair come in and it becomes a supersaw. The
  spread is wide on high notes and closes to the centre on low ones, so a bass
  stays mono whatever Stack is doing.
- **Layer** is a second source: a sub, a copy of the model at an interval, or
  noise. **Sine** is the *clean* sub: it skips the drive, the filter, and the
  chorus / phaser / delay / reverb, so the low end stays solid and dry however
  hard the rest is pushed. The square subs are the vintage kind and go through
  everything. **Blend**: 0 off, 50 both at full, 100 the layer alone.
- **Age**: 0 is exact and restarts every oscillator in step. Up, each voice
  jitters and slowly wobbles in pitch on its own, and every note lands with a
  little scatter in pitch, cutoff and envelope time. It adds no noise, and it
  is all per voice: the notes of a chord move against each other. A wobble the
  whole instrument shares is VHS.
- **Glide**: Poly, Mono, or a glide time. A glide time is mono and slides only
  between overlapping notes.

### Filter

A state-variable filter: LP24, LP12, band-pass, high-pass. **Drive** has three
zones on one knob:

- **to 50 %** the mixer overdrives into the filter: warm, thick;
- **50–75 %** adds a hard clip *after* the amp, per note: bite, while chords
  stay clean and a decaying note cleans up as it fades;
- **75 % up** pushes the summed voices into the bus clipper as well: chords
  intermodulate, the way they do through an amp.

The filter envelope is attack / decay / sustain and releases with the amp.

### Amp + LFO

One LFO, with the target on a knob: Pitch (vibrato that arrives late), Shape,
Filter, Amp, Pan, random stepped filter, and **Sync Ramp** — a falling ramp on
the cutoff locked to the tempo, where Rate picks 1/4, 1/8, 1/8T, 1/16 or 1/32.
Depth 0 is off; there is no "none" target to leave a depth pointing at.

Pad pressure and the mod wheel add vibrato; pressure also opens the filter.

### FX

In signal order: Tone → Chorus → Phaser → Delay → Reverb (+ Shimmer) → low
end to centre → Pump → VHS → Level.

- **Chorus**: I, II and I+II are the classic two-rate stereo chorus (0.513 Hz
  and 0.863 Hz, 1.66–5.35 ms); **Ens** is a three-phase string-machine
  ensemble; **Dim** is a subtle widener with no wobble.
- **Pump** ducks everything, tails included, in time with the tempo: a
  sidechain with no kick needed.
- **VHS**: wow, flutter, a falling top end, saturation and a hiss that rides
  the signal.
- **Tone** down is dark and heavy; up adds air and a low cut that climbs to
  about 540 Hz, which is what keeps a pad or an arp out of the bass.
- **Delay** is ping-pong and tape-like, always in time with the tempo.
- **Shimmer** feeds the reverb tail back into itself an octave up.
- After the reverb, the low end goes back to the middle: a crossover at 150 Hz
  gives both sides the same bottom. A chorus or a phaser on a bass widens its
  body and leaves its weight where it was.

## Presets

163, grouped by category in the browser: 26 bass, 26 lead, 30 pad, 18 keys,
24 pluck, 11 brass, 9 voices, 6 stabs, 5 sequences, 8 FX.

Inside each category there are up to four groups, in this order:

- the **staples** — the sounds these genres are built from;
- the ones where an **effect is the sound**: a pump that makes the rhythm, a
  delay that plays the other half of the line, a tail an octave up, a tape
  that has been played too often;
- the **bigger and wilder** ones: acid, talkbox and 808-style basses, hoover
  and guitar-style leads, high-pass and band-pass pads, pipe organ,
  harpsichord, tubular bells, FM brass, an orchestra hit. All of them are
  still things you play a tune or a chord on;
- **horror**: retro and modern horror-soundtrack sounds — slasher ostinatos,
  a flying-saucer signal, Italian prog-horror arps, a cursed music box, a wake harmonium,
  chanting monks, whispering witches, stingers — with only two straight sound
  effects among them.

**VX** is voices and choirs, **ST** stabs and hits, and **SQ** sequences: hold
a chord and the tempo plays it.

| | Staples | An effect is the sound | Bigger and wilder | Horror |
|---|---|---|---|---|
| **BS** 1–26 | Night Drive · Coastline · Tailfin · Solid State · Chrome Teeth · Undertow · Cold Sequence · Body Work | Sidechain · Phase Runner · Tape Sub · Gatecrash | Acid Rain · Slap Circuit · Mouth · Wolf · Low Rider · Octave Machine · Sync Jaw · Power Line · Bit Reaper · Precinct 13 | Night Stalker · Dread Reese · Cold Heartbeat · Grindhouse |
| **LD** 27–52 | Sunset · Prism · Razorwire · Black Chrome · Seance · Phantom | Echo Chamber · Shimmer Solo · VHS Rental · Ping Pong · Laserbeam · Neon Choir | Hoover '91 · Shredder · Anthem · Talk To Me · FM Scream · Fifth Element · Pan Pipes · Screamer · Octavia · Raygun Melody | Saucer Signal · Possession · Prowler · Banshee |
| **PD** 53–82 | Dusk · Glass Avenue · Skyline · Winter Ensemble · Cathedral · Frostline | Aurora · Tidal · Solina Phase · Betamax · Frozen Lake · Gated Heaven · Wide Open · Undersea | Monolith · Sweep Machine · High Wire · Radio Ghost · FM Halo · Sync Tide · Reverse Bloom · Tremolo Sky · Vapor Trail · Prism Scan | Attic Dust · Requiem · The Other Side · Cold Storage · Unease · Hollow House |
| **KY** 83–100 | Velvet · Chapel · Giallo | Tremolo Motel · Music Box · Tape Piano · Cathode | Pipe Dream · Goblin Clav · Tubular · Wurli Burn · Fantasia · Carousel | Haddonfield · Cursed Music Box · Wake Harmonium · Rotted Rhodes · *Deep Tubular* |
| **PL** 101–124 | Neon Arp · Signal · Ricochet · Palm Shadows | Dotted Dream · Raindrops · Cascade · Pulsar · Chorus Harp · Arcade | Jangle '85 · Marimba Nights · Pizzicato · Skyrocket · Koto · Chug · Photon · Vanishing Point | Profondo · Clock Tower · Follower · Bone Marimba · *Droplets* · *Hollow Harp* |
| **BR** 125–135 | Stadium · Off-World |  | Countdown · FM Section · Fanfare · Sync Horns · Velvet Horn · Titan | Overlook · Doom Horn · *Room 237* |
| **VX** 136–144 |  |  | Black Mass · Angel Dust · Robot Heart · Doo-Wop · Siren Song · Mellotape | Nursery Rhyme · Catacomb Monks · Three Mothers |
| **ST** 145–150 |  |  | Orch Hit · Rave Stab · FM Knife · Sync Slam | Jump Scare · Shower Stabs |
| **SQ** 151–155 |  |  | Motorik · Triplet Ghost · Strobe · Mainframe | Pursuit |
| **FX** 156–163 | Laser Toms · Event Horizon | Black Hole · Wormhole · Dead Channel · Thunder Toms |  | Rising Dread · Basement |

Names in *italics* were added one at a time after the duplicate audit: a
reworked preset kept next to the original it was reworked from.

A saved sound remembers its preset by a fixed id, not by its place in the
list, so the bank can be reordered or added to without renaming anything you
have saved.

Several of these want to be *held*: Sidechain, Gatecrash, Tidal, Gated Heaven,
Cathode and Pulsar get their rhythm from the tempo, not from your hands.

Each is described in a comment above it in
[`src/dsp/tenebrae_presets.h`](src/dsp/tenebrae_presets.h).

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

## Credits

Engine and presets: original to this repository, MIT.

TinyK (fleshinachair) was the reference for the *balance* this aims at — hot
filter passband and extra fundamental for weight, a non-oversampled filter and
a hard clip after the amp for grit. No TinyK code or data is used.

`src/host/plugin_api_v1.h` and `src/host/move_info.h` are copied unchanged from
Schwung (Charles Vestal, MIT).

Preset recipes drew on published breakdowns, chiefly Reverb Machine's artist
articles, Syntorial's preset recipes, Attack Magazine and MusicTech tutorials,
and the Juno-60 chorus measurements in pendragon-andyh/Juno60.

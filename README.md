# Tenebrae

A dark, retro synth with an edge, for
[Schwung](https://github.com/charlesvestal/schwung) on Ableton Move.

Tenebrae is for sounds with a shadow over them: basses that snarl, pads that
drift out of tune, leads that scream through a hard clip, plucks that vanish
into long echoes, bells from somewhere they shouldn't be, and a worn tape
wobbling under all of it. Very little sits still: envelopes, LFOs and
tempo-locked effects keep every sound moving. It has six pages of
knobs. Four of them make the sound and two dress it. A seventh page rolls a
new sound. Every knob is a
*macro*: one turn moves several things at once, and each range is cut so that
most of its travel sounds good.

- **8 oscillator models**: saw, pulse, hard sync, FM, bell, wavetable,
  formant voice and a pitch-drop "zap" for toms and lasers.
- **A gritty filter** with three kinds of drive on one knob.
- **A cinematic effects chain**: chorus, phaser, tempo pump, VHS, tape delay
  and a reverb with shimmer.
- **163 presets** and an Init patch, from heavy basses and wide pads to a set of retro and
  modern horror-soundtrack sounds.
- **A Random page** that makes new sounds in ten categories (bass, lead,
  pad and so on) from around forty sound recipes, or varies the sound you
  have. Eight knobs set how far each part may stray, and you can always go
  back to where you were.
- **8 voices**, with mono and glide modes.

## Install

Install Tenebrae from the Schwung module catalog with **schwung-manager**
(`http://move.local:7700`), or download `tenebrae-module.tar.gz` from the
[latest release](https://github.com/legsmechanical/schwung-tenebrae/releases)
and unpack it into `modules/sound_generators/` on the Move.

## Playing it

Load Tenebrae into a track's synth slot like any other Schwung sound
generator.

- **Presets**: the first page is the preset browser. Turn the jog wheel to
  move through the presets, or press Left / Right to step one at a time.
- **Octave**: Up / Down shifts the pads by an octave, up to four either way.
- **Pages**: choose Osc, Filter, Amp, Mod, FX Motion or FX Space. The knobs
  then control that page's settings, in the order of the table below.
- **Random**: the last page. Click in, turn the jog to choose what to make,
  click to make it. See [Random](#random).
- **Saving**: the host's own My Presets page saves and recalls your sounds.

Tenebrae also responds to:

| | |
|---|---|
| **Velocity** | level and brightness; the **Velocity** knob sets how much |
| **Pad pressure** | vibrato and a brighter filter, or what **Pressure Target** picks; the **Pressure** knob sets how much, and 0 turns it off |
| **Mod wheel** | vibrato; the **Wheel** knob sets how much |
| **Sustain pedal** | holds notes |
| **Pitch bend** | ±2 semitones |

## Five sounds from scratch

Each of these starts from any preset. Change only the knobs named.

- **Rolling bass**: Model Saw, Layer Sub Sine, Blend about 30. Glide Mono.
  Cutoff about half, Env Amount up, a short Decay and a low Sustain. Drive to
  about 40.
- **Supersaw pad**: Model Saw, Stack 60 or more. Amp Attack about 60 and
  Release about 65. Chorus I, Reverb up, Tone a little above the middle to keep
  it out of the bass.
- **Arp pluck**: Amp Sustain 0, Env Amount up, a short filter Decay. Delay
  about 30 at 1/8 Dot. Hold a chord and let the delay play the second half of
  the line.
- **Lead**: Glide between 60 and 150 ms. LFO Target Pitch with a little
  Depth: the vibrato comes in after the note starts, as a player's does.
- **Dark and worn**: Age up, VHS up, Tone down.

## The six pages

| Page | Row 1 | Row 2 |
|---|---|---|
| **Osc** | Model · Shape · Shape Env · Stack | Layer · Blend · Age · Glide |
| **Filter** | Type · Cutoff · Resonance · Drive | Env Amount · Attack · Decay · Sustain |
| **Amp** | Attack · Decay · Sustain · Release | Spread |
| **Mod** | LFO Target · Rate · Depth · LFO Delay | Velocity · Pressure · Pressure Target · Wheel |
| **FX Motion** | Chorus Type · Mix · Phaser · Rate | Pump · Division · VHS · Tone |
| **FX Space** | Delay · Time · Feedback · Level | Reverb · Size · Color · Shimmer |

### Osc

**Model** picks the oscillator. **Shape** is that model's main tone control.

| Model | What it is | Shape turns it from… |
|---|---|---|
| Saw | the analog poly saw | triangle → saw → saw + square |
| Pulse | pulse width | square → thin |
| Sync | hard sync | gentle → a screaming sweep |
| FM | two-operator FM | round → biting |
| Bell | inharmonic FM | struck metal → clang |
| Wave | a wavetable | soft, organ, hollow, reed, glass, voice, comb, spike |
| Vox | a formant voice | oo – oh – ah – eh – ee |
| Zap | a sine that drops onto the note | a small drop → a laser |

- **Shape Env** lets the filter envelope move Shape, up or down. FM plucks,
  sync sweeps and swept pulse widths all come from here.
- **Stack** goes from one oscillator to seven. Low settings give three
  oscillators a few cents apart, the middle pulls them wide (a reese), and the
  top brings in two more pairs for a supersaw. Low notes stay in the centre
  while high notes spread wide, so a bass stays solid whatever Stack is doing.
- **Layer** adds a second source: a sub, a copy of the oscillator at an
  interval, or noise.
  - **Sub Sine** is the *clean* sub. It skips the drive, the filter and the
    chorus, phaser, delay and reverb, so the low end stays solid and dry
    however hard you push the rest.
  - **Sub Square** and **Sub -2 Oct** are vintage subs and go through
    everything.
  - **Blend**: 0 is off, 50 is both at full, 100 is the layer alone.
- **Age**: at 0 everything is exact and tight. Turned up, each voice drifts
  and wobbles in pitch on its own, and every note lands a little differently,
  so chords beat slowly against themselves. Turned all the way up, it is a
  tired old machine. Age adds no noise; for hiss and tape wobble, use VHS.
- **Glide**: Poly, Mono, or a glide time. A glide time plays one note at a
  time and slides only between notes that overlap.

### Filter

**Type** is LP24, LP12, band-pass or high-pass. **Drive** has three zones on
one knob:

- **Up to 50**: the oscillators overdrive into the filter. Warm and thick.
- **50 to 75**: a hard clip *after* the amp, on each note. It bites, but
  chords stay clean and a fading note cleans up as it fades.
- **75 and up**: all the voices are clipped together as well, so chords snarl
  the way they do through an amp.

**Env Amount** is how far the envelope moves the cutoff, up or down.
**Attack**, **Decay** and **Sustain** shape it, and it releases with the amp.

### Amp

Row one is the amp envelope. **Spread** fans the notes of a chord across the
stereo field, each new note taking the next place. Like Stack's width it closes
up toward the bottom of the keyboard, so a bass stays in the middle, and a mono
voice is always centred.

### Mod

**LFO Target** chooses what the LFO moves:

| Target | What you hear |
|---|---|
| Pitch | vibrato, which comes in after the note starts |
| Shape | the model's tone moving (pulse-width modulation on Pulse) |
| Filter | a filter sweep |
| Amp | tremolo |
| Pan | auto-pan |
| Filter S&H | the filter jumping to random places |
| Filter Sync Ramp | a falling filter ramp locked to the tempo; **Rate** picks 1/4, 1/8, 1/8T, 1/16 or 1/32 |

**Depth** 0 turns the LFO off. **LFO Delay** holds the LFO back after each
note, up to four seconds, then fades it in: a lead that starts straight and
blooms into vibrato, a pad whose filter starts moving halfway through.

**Velocity** sets how much playing harder changes the level and tone.
**Pressure** sets how much pad pressure (aftertouch) does, and 0 turns it off.
**Pressure Target** picks what it does:

| Target | What pressing does |
|---|---|
| Vib + Filter | adds vibrato and opens the filter |
| Vibrato | adds vibrato only |
| Filter | opens the filter only |
| LFO Depth | turns the LFO up, whatever it is moving |

**Wheel** sets how much vibrato the mod wheel adds, and 0 turns it off.

### FX Motion and FX Space

The sound passes through the effects in this order: Tone, Chorus, Phaser,
Delay, Reverb and Shimmer, Pump, VHS, then Level.

- **Chorus**: I, II and I+II are the classic stereo chorus. **Ensemble** is a
  string-machine ensemble. **Dimension** widens without any wobble.
  **Mix** sets how much.
- **Phaser**: depth, and its **Rate**.
- **Pump** ducks the whole sound in time with the tempo, tails included: a
  sidechain without a kick. **Division** sets how often.
- **VHS**: wow, flutter, a fading top end, a little saturation and hiss.
- **Tone**: down is dark and heavy. Up adds air and cuts the lows, which keeps
  a pad or an arp out of the bass.
- **Delay** is ping-pong and tape-like: the repeats darken as they go.
  **Time** follows the tempo. **Feedback** sets how many repeats.
- **Reverb**, its **Size**, and **Color** from a dark tail to a bright one.
  **Shimmer** feeds the tail back into itself an octave up.
- **Level** is the output volume.

However wide the chorus or phaser makes a bass, its lowest notes stay centred:
the effects widen the body and leave the weight in the middle.

## Random

The last page makes new sounds:

```
   OSC    FILT    AMP     MOD        how far each part may change
 <         Pad              >        what to make: jog left / right
   MOTN   SPACE   WILD    FRESH
```

Click into the page and turn the jog to choose what to make: **Vary
Current**, or a category (Bass, Lead, Pad, Keys, Pluck, Brass, Voice, Stab,
Sequence, FX). **Click** to make one, and click again for another. A category
makes a new sound of that kind. Vary Current makes a relative of the sound
you started from. Play each one as long as you like.

When you press **Back** with a new sound playing, Tenebrae asks **Keep** or
**Revert**. Revert puts back exactly the sound you had before your first
click, however many you made. Leaving the page another way keeps the new sound
playing, and Back on your next visit still offers Revert. Choosing a preset
also ends it.

### The knobs

The eight knobs set how far a new sound may stray. They work whether or not
you have clicked in, and they are saved with the set. Choosing a preset or
making a sound never changes them.

| Knob | Turned down | Middle | Turned up |
|---|---|---|---|
| **Osc**, **Filter**, **Amp**, **Mod**, **Motion**, **Space** | keeps that part of your current sound (all the way down: exactly) | the generator's own amount | that part may go further: wider ranges, bigger moves, more often invented |
| **Fresh** | each part is taken whole from a factory preset | a mix of taken, blended and invented | each part is invented, owing nothing to the presets |
| **Wild** | (starts at zero) | | past the recipes: other oscillator models, filter types, layers and LFO targets, looser limits; Vary may switch on what was off |

Turning a part all the way down locks it, so you can keep your filter and
effects and make a new oscillator and envelope, or the reverse.

### How a sound is made

Each category has a handful of recipes, around forty in all: a reese, an acid
line and a sub among the basses, a supersaw, an ensemble and a choir among the
pads, and so on. A recipe says which oscillators, filters and voice modes
belong to it, the range each defining setting must sit in, and how much
movement, space, grit and width the sound may have in total.

A new sound is built in the order a sound designer works: oscillator, filter,
amp envelope, modulation, then the two effects pages. Each section is either
taken whole from a factory preset (of any category) that fits the recipe, or
blended from two that do, sometimes a little past either one. That way what
holds a section together survives, and the result can still land where no
preset is. Whatever the earlier sections spent of the movement, space, grit
and width totals shapes what the later ones may add.

Tenebrae then **plays the new sound once, out of earshot, and measures it**.
It sets the Level to match the presets, and checks the sound against its
category: it must keep sounding through its phrase, a bass must have real low
end, and a pad must be wide and stay out of the bass. A sound that fails is
replaced by another from the same recipe. This takes a fraction of a second.
The sound you hear first can change once, quietly, when it does.

**Vary Current** keeps what makes the sound the instrument it is: its
oscillator model, voice mode, filter type, and whether it holds or dies away
(unless Wild is up). It nudges everything else, by as much as each knob
allows, and sometimes swaps in another preset's modulation or effects.

A new sound is named for its category and a number, such as *Rand Pad 12*. Once
kept, it is an ordinary sound: edit it on any page and save it with My
Presets.

The page needs Schwung 1.5.0 or later.

## Presets

An **Init** patch first (every control at its default), then 163 presets,
grouped by category: 26 bass (**BS**), 26 lead (**LD**),
30 pad (**PD**), 18 keys (**KY**), 24 pluck (**PL**), 11 brass (**BR**),
9 voices and choirs (**VX**), 6 stabs and hits (**ST**), 5 sequences (**SQ**)
and 8 effects (**FX**).

Within each category the presets come in this order:

- **Staples**: the basses, pads, leads and keys everything else builds on.
- **The effect is the sound**: a pump that makes the rhythm, a delay that
  plays the other half of the line, a tail an octave up, a worn-out tape.
- **Bigger and wilder**: acid, talkbox and 808-style basses, hoover and
  guitar-style leads, band-pass pads, pipe organ, harpsichord, tubular bells,
  FM brass, an orchestra hit. All of them are still for playing tunes and
  chords.
- **Horror**: retro and modern horror-soundtrack sounds. Slasher ostinatos,
  a flying-saucer signal, Italian prog-horror arps, a cursed music box, a wake
  harmonium, chanting monks, whispering witches and stingers.

| | Staples | The effect is the sound | Bigger and wilder | Horror |
|---|---|---|---|---|
| **BS** 2–27 | Night Drive · Coastline · Tailfin · Solid State · Chrome Teeth · Undertow · Cold Sequence · Body Work | Sidechain · Phase Runner · Tape Sub · Gatecrash | Acid Rain · Slap Circuit · Mouth · Wolf · Low Rider · Octave Machine · Sync Jaw · Power Line · Bit Reaper · Precinct 13 | Night Stalker · Dread Reese · Cold Heartbeat · Grindhouse |
| **LD** 28–53 | Sunset · Prism · Razorwire · Black Chrome · Seance · Phantom | Echo Chamber · Shimmer Solo · VHS Rental · Ping Pong · Laserbeam · Neon Choir | Hoover '91 · Shredder · Anthem · Talk To Me · FM Scream · Fifth Element · Pan Pipes · Screamer · Octavia · Raygun Melody | Saucer Signal · Possession · Prowler · Banshee |
| **PD** 54–83 | Dusk · Glass Avenue · Skyline · Winter Ensemble · Cathedral · Frostline | Aurora · Tidal · Solina Phase · Betamax · Frozen Lake · Gated Heaven · Wide Open · Undersea | Monolith · Sweep Machine · High Wire · Radio Ghost · FM Halo · Sync Tide · Reverse Bloom · Tremolo Sky · Vapor Trail · Prism Scan | Attic Dust · Requiem · The Other Side · Cold Storage · Unease · Hollow House |
| **KY** 84–101 | Velvet · Chapel · Giallo | Tremolo Motel · Music Box · Tape Piano · Cathode | Pipe Dream · Goblin Clav · Tubular · Wurli Burn · Fantasia · Carousel | Haddonfield · Cursed Music Box · Wake Harmonium · Rotted Rhodes · Deep Tubular |
| **PL** 102–125 | Neon Arp · Signal · Ricochet · Palm Shadows | Dotted Dream · Raindrops · Cascade · Pulsar · Chorus Harp · Arcade | Jangle '85 · Marimba Nights · Pizzicato · Skyrocket · Koto · Chug · Photon · Vanishing Point | Profondo · Clock Tower · Follower · Bone Marimba · Droplets · Hollow Harp |
| **BR** 126–136 | Stadium · Off-World |  | Countdown · FM Section · Fanfare · Sync Horns · Velvet Horn · Titan | Overlook · Doom Horn · Room 237 |
| **VX** 137–145 |  |  | Black Mass · Angel Dust · Robot Heart · Doo-Wop · Siren Song · Mellotape | Nursery Rhyme · Catacomb Monks · Three Mothers |
| **ST** 146–151 |  |  | Orch Hit · Rave Stab · FM Knife · Sync Slam | Jump Scare · Shower Stabs |
| **SQ** 152–156 |  |  | Motorik · Triplet Ghost · Strobe · Mainframe | Pursuit |
| **FX** 157–164 | Laser Toms · Event Horizon | Black Hole · Wormhole · Dead Channel · Thunder Toms |  | Rising Dread · Basement |

The numbers are each category's place in the browser.

**Some presets want to be held.** Sidechain, Gatecrash, Tidal, Gated Heaven,
Cathode, Pulsar and every **SQ** sequence get their rhythm from the tempo, not
from your hands: hold a chord and let it play. The **FX** drones, such as Event
Horizon, Wormhole and Basement, want a low note or a low fifth held for a
while.

## Good to know

- **Pump, delay, the Sync Ramp LFO and the sequences follow the Set's
  tempo.** While the transport plays they lock to the beat; when it is
  stopped they keep running at the same tempo.
- **A glide setting plays one note at a time.** For chords, set Glide to
  Poly.
- **A heavy bass that keeps its weight**: use Layer Sub Sine. The sub stays
  clean underneath, however much Drive, chorus or reverb is on the rest.
- **A saved sound keeps its own settings.** It remembers which preset it came
  from, but your changes are saved with it, so later updates to the presets
  will not change your sounds.

## Credits

Engine and presets: original to this repository, MIT.

Building, testing and measuring: see [DEVELOPMENT.md](DEVELOPMENT.md).

`src/host/plugin_api_v1.h` and `src/host/move_info.h` are copied unchanged from
Schwung (Charles Vestal, MIT).

Preset recipes drew on published breakdowns, chiefly Reverb Machine's artist
articles, Syntorial's preset recipes, Attack Magazine and MusicTech tutorials,
and the Juno-60 chorus measurements in pendragon-andyh/Juno60.

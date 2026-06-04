# Sonaglio — FM Percussion Synth for Korg Drumlogue

Sonaglio is a **5-engine, ARM NEON-accelerated FM percussion instrument** for the Korg Drumlogue SDK.
It is built around a fixed-voice model with perceptual parameter control, a shared ADR envelope ROM,
dual LFO modulation, and combo instrument routing.

Design priorities (in order):
1. **Punch** — clear transient impact, fast attack definition
2. **Body** — low-mid weight, perceived mass, decay shape
3. **Playability** — small number of musically meaningful controls per engine
4. **Efficiency** — ARM NEON parallelism, static allocation, no heap

---

## Architecture overview

### Engines (5 total)

| Engine | Role | Synthesis method |
|--------|------|-----------------|
| **Kick** | Low-end strike | 2-op FM + pitch sweep |
| **Snare** | Crack + wire + shell | 2-op FM + 3-band bandpass noise |
| **Metal** | Inharmonic ring | 4-op FM cascade + feedback + HP noise |
| **Perc** | Block / wood / tom | 2-op FM, configurable body |
| **Hat** | Hi-hat / cymbal | 6× detuned square oscillators + noise |

All engines process 4 parallel voices via NEON `float32x4_t`. Only lane 0 is active in the
current selector model; the remaining lanes are reserved for future probability-triggered modes.

### Shared infrastructure

- **Envelope ROM** — 128 curated ADR shapes (exponential curve, `attack_ms 0–9`, `decay_ms 45–1850`,
  `release_ms 20–1025`). Sections: tight (0–15), punch (16–31), body (32–47), long body (48–63),
  gated/flam (64–79), open linear (80–95), metallic tail (96–111), experimental (112–127).
- **LFO system** — two audio-rate LFOs (triangle / ramp / chord steps) routed through `lfo_smoother_t`
  for zipper-free transitions. Targets: pitch, FM index, envelope, noise mix, metal gate, cross-phase.
- **MIDI handler** — note-on/off → instrument selector routing; per-note bitmask for combo hits.
- **Euclidean tuning** — 9 modes spread across 4 engines (off, chromatic, minor, diatonic, whole-tone,
  pentatonic, dim7, aug8, tritone).
- **Combo routing** — 15 instrument slots (5 solo + 10 pairs). Pairs use `gap`, `blend`, `scatter`
  to schedule and de-tune the secondary engine.

---

## Parameter model (24 parameters)

### Page 1 — Instrument + macros

| # | Name | Range | Description |
|---|------|-------|-------------|
| 0 | Instrument | 0–14 | Solo or combo instrument selector |
| 1 | Blend | 0–100 | Level balance in combo mode |
| 2 | Gap | 0–100 | Delay between primary and secondary hit |
| 3 | Scatter | 0–100 | Pitch/timing jitter on secondary hit |

### Page 2 — Kick + Snare

| # | Name | Range | Description |
|---|------|-------|-------------|
| 4 | Kick Atk | 0–100 | Kick click hardness and pitch-drop depth |
| 5 | Kick Body | 0–100 | Kick low-end weight and sweep length |
| 6 | Snare Atk | 0–100 | Snare crack, wire energy, top-end brightness |
| 7 | Snare Body | 0–100 | Snare shell weight and tone sustain |

### Page 3 — Metal + Perc

| # | Name | Range | Description |
|---|------|-------|-------------|
| 8 | Metal Atk | 0–100 | Strike brightness, inharmonic spread, bite |
| 9 | Metal Body | 0–100 | Ring density and FM depth |
| 10 | Perc Atk | 0–100 | Transient hardness and edge |
| 11 | Perc Body | 0–100 | Body FM depth and decay weight |

### Page 4 — LFO 1

| # | Name | Range | Description |
|---|------|-------|-------------|
| 12 | LFO1 Shape | 0–8 | Shape combo (Tri+Tri, Rmp+Rmp, Chd+Chd, …) |
| 13 | LFO1 Rate | 0–100 | Rate (maps to 0.05–18 Hz) |
| 14 | LFO1 Target | 0–10 | None, Pitch, ModIdx, Env, LFO2Ph, LFO1Ph, ResFrq, Reson, NoizMx, ResMrph, MtlGate |
| 15 | LFO1 Depth | 0–100 | Modulation depth |

### Page 5 — Euclidean tuning + LFO 2

| # | Name | Range | Description |
|---|------|-------|-------------|
| 16 | EuclTun | 0–8 | Pitch spread across engines (Off … Tritone) |
| 17 | LFO2 Rate | 0–100 | Rate (maps to 0.05–18 Hz) |
| 18 | LFO2 Target | 0–10 | Same target set as LFO1 |
| 19 | LFO2 Depth | 0–100 | Modulation depth |

### Page 6 — Envelope + global shaping

| # | Name | Range | Description |
|---|------|-------|-------------|
| 20 | Env Shape | 0–127 | Envelope ROM index (bit 7 selects Gong character for Metal) |
| 21 | HitShape | 0–100 | Transient hardness: low=round, high=hard front edge |
| 22 | BodyTilt | 0–100 | Low-mid weight: low=lean/short, high=heavy/full |
| 23 | Noise Char | 0–100 | Noise-to-FM blend; adds noise floor to all engines |

**Design rule**: every engine interprets its two controls with the same semantic contract —
`Atk` = attack energy / brightness / crack; `Body` = weight / decay / stability.

---

## Engine details

### Kick

- Carrier frequency range: 20–420 Hz (MIDI-mapped)
- FM topology: single modulator → carrier (2-op)
- **Amp envelope**: `env^2` — exponential body decay
- **Pitch sweep**: `env^4` — short, concentrated at onset (was `env^8`, which
  compressed to a delta function under HitShape)
- **FM click index**: `env^4`; **FM body index**: `env^2`
- Sweep depth: 0.24–2.11 octaves (Attack × Body)
- Sub sine reinforcement gated by `sqrt(env)`

### Snare — per-component independent decay (ctag-inspired)

- Carrier frequency range: 90–650 Hz (MIDI-mapped)
- FM topology: single modulator → carrier (2-op), inharmonic ratio 1.42–2.42
- Each component has its own one-pole exponential decay with an **absolute** 1/e
  time constant, independent of the shared envelope shape:
  - **Crack FM index**: 7–23 ms (Attack) — instantaneous front-edge snap
  - **Tonal shell + sustained FM**: 35–120 ms (Body) — tonal ring
  - **Wire buzz noise**: 50–150 ms (Attack + Body) — sustains longest; the snare signature
- The shared ROM envelope is applied only as a linear master VCA (onset attack +
  ENV_SHAPE length + note-off). No power-law compounding.
- Noise generation: 3-band blended HPF/BPF/high-shelf (900, 2200, 6200 Hz)
- Click transient driven by `index_level²` (a faster clean exponential)

### Metal — algo11 parallel pairs (copych-derived, MIT)

- Carrier frequency range: 500–8000 Hz (MIDI-mapped)
- FM topology: **3 independent carrier+modulator pairs** (DX7 algo 11) via
  `fm_operator.h` — no serial cascade, so each partial rings cleanly
- Two characters: **Cymbal** (ratios 1.0 / 1.483 / 1.932 / 2.546) and **Gong**
  (1.0 / 2.756 / 3.752 / 5.404) — selected via bit 7 of `ENV_SHAPE`
- DX7-style exponential `fm_level` (`0.1 × 161^v − 0.1`) and feedback (`1/2^(7−fb)`)
- **Carrier envelope**: `sqrt(gated_env)` — slow amplitude decay for long metallic ring
- **Modulator envelope**: `env^8` burst + linear ring (LFO-scalable, capped)
- Inharmonic spread: ratio scale 0.82–1.50 (Body-controlled); pair-A carrier is the
  fixed fundamental anchor at ratio 1.0
- Noise: HP above ~2500 Hz + BPF at 4200 Hz, `env^2`-gated strike noise

### Perc

- Carrier frequency range: 45–1200 Hz (MIDI-mapped)
- FM topology: 3-op (carrier + 2 modulators)
- **Amp**: linear (`env`)
- **Strike FM**: `env^8` — short thwack
- **Body FM**: linear (`env`) — sustains through the body (was `env^2`)
- Strike index range: 0.30–3.00; single envelope application (no double-VCA)

### Hat

- Six detuned square-wave oscillators at TR-808/909 metallic intervals
  (306, 512, 551, 743, 826, 900 Hz), MIDI-shifted as a group
- Body controls tone/noise ratio; Attack controls click transient
- No FM computation — cheapest synthesis path in the instrument
- Noise: HPF at 3000 Hz + LPF at 10000 Hz, `env^2`-gated

---

## Envelope shaping

The shared envelope feeds two pre-processed streams to the engines:

- `transient_env = lerp(env, env^4, HitShape) × (1 + 0.18 × HitShape)` — used by Kick, Snare, Hat
- `body_env = lerp(env^2, env, BodyTilt) × (0.82 + 0.28 × BodyTilt)` — used by Metal, Perc

Engines then derive further domain envelopes (`env^2`, `env^8`, `sqrt`) internally. The key design
principle is that **amplitude envelopes use mild or linear powers** while **transient accents
(pitch drop, FM crack, click)** use high powers (`env^8`). This prevents compounding envelope
non-linearities that would make parameter shaping ineffective.

---

## Signal path

```
MIDI note-on
    ↓
fm_perc_synth_note_on() → instrument selector → queue pending triggers
    ↓
fm_perc_synth_process() (per sample):
    ├─ pending trigger dispatch (fire_engine, neon_envelope_trigger)
    ├─ LFO smoother → LFO enhanced → lfo1/lfo2 bipolar
    ├─ neon_envelope_process → envelope.level
    ├─ fm_make_transient_env / fm_make_body_env
    ├─ kick_engine_process  (transient_env)
    ├─ snare_engine_process (transient_env + noise_add)
    ├─ metal_engine_process (body_env + gate)
    ├─ perc_engine_process  (body_env)
    ├─ hat_engine_process   (transient_env + noise_add)
    ├─ separation EQ (one-pole per engine: kick LP 190Hz, snare HP 420Hz,
    │                  metal HP 500Hz, perc BP 220–1400Hz, hat HP 800Hz)
    ├─ weighted mix
    └─ fm_soft_clip(mix × master_gain)  →  scalar output
```

Velocity is latched into each trigger's gain at note-on (so delayed shadow/combo
hits keep the velocity of the note that spawned them). MIDI note-on with velocity 0
is treated as a note-off.

---

## Preset bank (64 presets)

`fm_presets.cc` ships a two-part bank:

- **0–46 — General-MIDI kit** matching the `GmDrumNote` list of
  copych/ESP32-S3_FM_Drum_Synth (notes 35–81). Each preset is a drum-voice
  *timbre*; pitch comes from the played MIDI note. Coverage:

  | GM range | Sonaglio engine |
  |----------|-----------------|
  | Bass drums (35–36) | Kick |
  | Snares / side stick / clap (37–40) | Snare (clap uses Gap multi-strike) |
  | Toms / bongos / congas / timbales / woodblocks / cuica (41–66, 75–79) | Tom→Perc |
  | Hi-hats / tambourine / cabasa / maracas / guiro (42–46, 54, 69–70, 73–74) | Hat |
  | Cymbals / cowbell / agogo / triangle (49–57, 67–68, 80–81) | Metal (Cymbal/Gong) |
  | Whistles (71–72) | Tom→Perc + pitch LFO |

- **47–63 — Sonaglio experimental & combos**: layered K+S/K+M/K+H/T+M/etc.,
  Euclidean-tuned metal, Gong drone, metal wash, glitch perc, flam snare,
  dual-LFO chaos, pitch-drop kick.

Every preset is rendered through the full synth in `test_all_presets_render`
(finite, bounded, audible).

---

## Comparison with reference projects

### ctag-fh-kiel/md-drum-synth

The ctag md-drum-synth provides scalar C++ models used in the Machinedrum/Elektron-inspired
desktop synth. Key FM snare and cymbal models:

| Feature | ctag md-drum-synth | Sonaglio |
|---------|-------------------|----------|
| FM topology | 2-op (snare), 4-pair independent (cymbal) | 2-op (snare), 4-op serial cascade (metal) |
| Envelope | True per-component first-order IIR (`1 - dt/decay`) | Shared ADR ROM + power-law domains |
| Noise | White + HP filter | 3-band blended (snare), HP+BPF (metal) |
| Parallelism | Scalar | ARM NEON 4-wide |
| MIDI pitch | Fixed frequency | MIDI-mapped carrier range |
| Parameter abstraction | Raw synthesis values (f_b, I, d_m…) | Musical macro controls (Atk, Body) |

**Sonaglio advantages**: NEON efficiency for embedded ARM, richer noise processing, LFO modulation,
combo instruments, Euclidean tuning, MIDI pitch tracking.

**ctag advantage**: true independent per-component decay constants (amplitude, FM index, and noise
each decay at their own rate independently — more natural than power-law domains on a shared envelope).

The ctag cymbal uses 4 independent carrier/modulator pairs (each pair rings at its own inharmonic
frequency) which produces cleaner separation between partials than a serial FM cascade. This could
be a useful "Character 2" variation for the Sonaglio Metal engine in a future revision.

**License**: No explicit OSI license found in the ctag repo. Assume All Rights Reserved unless
confirmed otherwise — do not copy code without written permission.

### copych/ESP32-S3_FM_Drum_Synth

6-operator FM synthesis with 17 routing algorithms on ESP32-S3. Not directly applicable to
Drumlogue due to the different hardware architecture and licensing ambiguity, but the algorithm
variety and waveform blending (sine, triangle, square, saw, noise) are interesting references for
future engine expansion.

**License**: No license file visible in the repository — treat as All Rights Reserved.

---

## File structure

| File | Purpose |
|------|---------|
| `unit.cc` | Drumlogue entry point and hardware interface |
| `header.c` | User-facing 24-parameter definitions |
| `fm_perc_synth_process.h` | Main render loop, voice dispatch, LFO integration |
| `engine_mapping.h` | Global envelope projections, NEON helpers |
| `kick_engine.h` | Kick FM engine |
| `snare_engine.h` | Snare FM + noise engine (per-component independent decay) |
| `metal_engine.h` | Metal/cymbal engine — algo11 3×2-op parallel pairs |
| `fm_operator.h` | Portable scalar FM operator (copych-derived, MIT) |
| `perc_engine.h` | Perc FM engine |
| `hat_engine.h` | Square-wave hi-hat engine |
| `envelope_rom.h` | 128-entry ADR envelope table |
| `lfo_enhanced.h` | Audio-rate LFO (triangle / ramp / chord) |
| `lfo_smoothing.h` | Zipper-free parameter smoother for LFO |
| `midi_handler.h` | MIDI note routing and classification |
| `fm_voices.h` | NEON FM primitives, one-pole filters, MIDI→freq |
| `sine_neon.h` | Fast NEON sine approximation |
| `float_math.h` | Fast exp2, log2, sqrt, cubic clip (NEON) |
| `prng.h` | NEON parallel PRNG + probability gate |
| `constants.h` | Global constants, enums, tuning tables |
| `fm_presets.cc/.h` | Factory preset bank |
| `synth.h` | Legacy parameter storage (retained for compatibility) |

---

## Build

```bash
# Drumlogue SDK (ARM cross-compile)
cd platform/drumlogue/Sonaglio
make

# Unit tests (ARM cross-compile + qemu)
CXX=arm-linux-gnueabihf-g++ RUNNER=qemu-arm-static ./run_sonaglio_tests.sh
```

---

## Non-goals

- Physical modeling
- Heap allocation or dynamic voice graphs
- Engine allocation controls in the UI
- General-purpose synth behavior
- Per-engine envelope shape parameters

Sonaglio is intentionally narrow: a percussion instrument with a clear sonic identity and a small,
musical parameter surface.

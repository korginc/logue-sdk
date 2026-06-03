# Sonaglio — FM Percussion Synth for Korg Drumlogue

Sonaglio is a fixed-voice percussion synth for the Korg Drumlogue SDK. It is designed as a **4-voice, probability-triggered, NEON-accelerated FM percussion instrument** with a strong focus on:

- **Punch**: clear transient impact and fast attack definition
- **Body**: low-mid weight, perceived mass, and decay shape
- **Aggression**: controlled brightness, drive, and digital edge
- **Playability**: a small number of musically meaningful controls

The current design intentionally prioritizes **performative control over synthesis complexity**.

---

## Core architecture

The instrument uses four fixed engines:

- Kick
- Snare
- Metal
- Perc

Each engine is assigned a stable role in the instrument. Voice allocation is removed from the active control surface so the design remains predictable and easier to tune.

The resonant engine is still kept in the codebase for now, but it is not part of the active instrument path. It belongs to a possible future derivative project.

---

## Parameter model

The current UI model is built around **24 parameters**:

### Per-voice probability
- Kick probability
- Snare probability
- Metal probability
- Perc probability

### Per-engine controls
Each engine has two controls:
- **Attack / Energy / Brightness**
- **Body / Decay / Stability**

### Global shaping controls
- **HitShape**: transient hardness / front-edge emphasis
- **BodyTilt**: low-mid weight / body support
- **Drive**: global saturation / grit / bus push

### Remaining modulation and structure controls
- LFO1 block
- Euclidean tuning block
- LFO2 block
- Envelope shape selection

This model deliberately avoids exposing low-level FM details directly to the user.

---

## Design philosophy

The instrument is not trying to be a general synth. It is trying to be a **percussion instrument with a clear sound-design language**.

The parameter philosophy is:

> Parameters should describe musical behavior first, and synthesis math second.

This means the mapping layer must translate user controls into perceptual targets such as:

- transient energy
- spectral brightness
- pitch-drop depth
- shell or body weight
- ring length
- noise amount
- drive amount
- stability / instability

---

## Why this design is different

A drum/percussion synth can easily become too technical if the user is forced to think in terms of ratios, modulation indices, and operator structure. Sonaglio instead uses a **macro-control model**:

- `Attack` makes the hit harder or brighter
- `Body` makes the sound fuller or longer
- `HitShape` changes the transient profile globally
- `BodyTilt` biases the instrument toward weight or leanness
- `Drive` pushes the sound into stronger saturation and density

This is intended to make presets easier to author, easier to compare, and easier to judge musically.

---

## Engine-specific intent and envelope character

Each engine has its own envelope shaping philosophy, giving distinct timbral characters without user-editable per-engine parameters. These are assigned by design and tuned per instrument type.

### Kick
- **Amp envelope**: linear (`env^1`) — full-body presence from attack to tail
- **Pitch sweep**: very fast (`env^8`) — brief drop concentrated at onset
- **FM index**: very fast (`env^8`) — click complexity dies immediately
- **Sweep depth range**: 0.10..2.20 octaves — dramatic pitch drop at high Attack/Body settings
- Focus: low-end strike, FM pitch sweep, body weight

### Snare
- **Amp envelope**: linear (`env^1`) — full tail presence
- **Pitch**: very fast (`env^8`) — crack pitch transient only
- **FM index**: very fast (`env^8`) — brightness at onset
- **Noise**: squared (`env^2`) — noise longer than FM but shorter than amp
- Focus: crack, noise wire, shell/body balance

### Metal
- **Amp envelope**: **square-root** (`env^0.5`) — slower than linear, long perceived ring
- **Ring FM depth (op1 carrier)**: sqrt decay — FM modulation stays alive longer
- **Mid cascade (op2→op3)**: squared (`env^2`) — mid-decay FM cluster
- **Attack FM (op4→op3)**: fourth-power + eighth-power (`env^4`, `env^8`) — bright strike burst only
- **Ring index range**: 1.5..4.7 (was 0.30..1.45) — genuine DX7-style FM indices for metallic character
- **Strike index range**: 0.5..5.5 (was 0.35..3.55) — stronger attack burst
- **Character 0**: DX7 cymbal ratios (1.0, 1.483, 1.932, 2.546)
- **Character 1**: Gong ratios (1.0, 2.756, 3.752, 5.404) — selected via bit 7 of `ENV_SHAPE`
- Focus: inharmonic FM ring, metallic density, long tail

### Perc
- **Amp**: linear (`env^1`) — full decay body
- **Strike FM**: eighth-power (`env^8`) — short thwack only
- **Body FM**: fourth-power (`env^4`) — medium sustain cluster
- **Strike index range**: 0.30..3.00 (was 0.35..2.05) — more dramatic FM transient
- Focus: block, wood, tom, and digital FM percussion behavior

The four engines should cover the primary needs of the instrument without needing a larger engine count.

---

## Why the resonant engine is not part of the active path

The resonant engine is conceptually useful, but it does not fit the current project identity as well as the four FM engines. Keeping it active would dilute the parameter budget and weaken the 4-engine performative model.

It is retained as a candidate for a separate future instrument that can share the same 4-voice probability framework but explore a different sonic identity.

---

## Control-path strategy

The codebase is organized so that:

- `header.c` defines the user-facing parameters
- `synth.h` receives and stores parameter changes
- `engine_mapping.h` translates parameters into musical control targets
- the engine files implement the actual DSP behavior

This separation is deliberate and should remain stable.

---

## Why velocity is still undecided

Velocity exists as a possible engine/runtime concept, but it is not yet a core control surface. It should only remain if it is proven to affect the sound in a meaningful way, such as:

- stronger transient drive
- brighter attack at high velocity
- stronger body at high velocity

If it does not clearly improve the instrument, it should be removed to keep the design clean.

---

## Parameter ranges

Most controls are currently unipolar (`0..100`) because the instrument is still being stabilized.

Bipolar controls (`-100..100`) may be useful later for specific cases such as:

- modulation polarity
- spectral skew
- detune/spread direction
- transient emphasis vs suppression
- body bias toward thinner or heavier response

These are intentionally deferred until the current design is stable and validated.

---

## Presets

Preset content is currently being rebuilt around the new parameter language.

The presets should eventually form curated families such as:

- tight and punchy
- heavy and low
- bright and cracky
- metallic and dense
- dry and minimal
- aggressive / driven
- experimental

Preset values should not be interpreted as raw technical numbers; they should be treated as authoring choices for distinct sound identities.

---

## Future project direction

A future derivative project could reuse the same 4-voice probability framework but move into a different synthesis family:

- resonant percussion
- modal percussion
- tonal impact synthesis
- hybrid struck objects

That project would keep the same platform philosophy but use a different engine family.

---

## LFO routing

LFO1 and LFO2 are fully functional modulation sources. They are routed through `lfo_smoother_t` for zipper-free parameter transitions, then converted to the `lfo_enhanced_t` audio path.

**Rate conversion**: the smoother stores normalized 0..1 (matching the 0-100 UI range). Before reaching the audio oscillator the value is converted to cycles/sample: `rate_cps = min_hz/sr + norm × (max_hz − min_hz)/sr`.

**Available targets**: pitch, FM index, amplitude envelope, noise mix, metal gate, LFO2 phase (from LFO1), LFO1 phase (from LFO2).

**Shapes**: triangle, ramp, chord steps (selected by LFO1_SHAPE param via `shape_combo`).

---

## Current priorities

1. Hardware listening test for metal engine improvements (ring_index boost, sqrt envelope)
2. Preset rebuild around new sound families and improved parameter ranges
3. Test suite for monotonic parameter behavior
4. Tune instrument against reference material
5. Decide whether velocity becomes a real control source

---

## Non-goals

- physical modeling
- deep parameter explosion
- engine allocation controls
- heap allocation
- dynamic graphs
- generic synth behavior

Sonaglio is meant to be narrow, fast, and playable.


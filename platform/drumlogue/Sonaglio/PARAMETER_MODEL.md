# Parameter Model — FM Percussion Synth

## Scope

This document defines the control surface for the Drumlogue FM percussion project after the redesign.

The current product identity is:

- **4 fixed engines**: Kick, Snare, Metal, Perc
- **4 fixed voice positions**
- **2 engine parameters per engine**
- **3 reclaimed global parameters**
- **resonant engine retained in code but not called**
- **no voice allocation parameter**

The purpose of this model is to make sound design **predictable, performative, and consistent** under a strict parameter budget.

---

## Design rules

1. Parameters must describe **perceptual behavior**, not internal DSP details.
2. Every engine must interpret its two parameters with the same semantic contract:
   - **Param1 = Attack / Energy / Brightness**
   - **Param2 = Body / Decay / Stability**
3. Global parameters must improve coherence across engines, not compete with them.
4. Parameter mapping is part of the synthesis design, not a post-process.
5. The system must remain static-allocation friendly and NEON-friendly.

---

## Engine layout

The instrument uses four fixed voices:

| Voice | Engine |
|------|--------|
| 0 | Kick |
| 1 | Snare |
| 2 | Metal |
| 3 | Perc |

Voice reassignment is removed. This simplifies sound design and frees one parameter for more useful control.

---

## Reclaimed global parameters

The redesign reuses three freed parameter slots as global controls.

### 1) HitShape
Controls transient character.

Range: `0.0 .. 1.0`

Meaning:
- lower values: rounder attack, less click
- higher values: harder strike, more edge, stronger front transient

Internal uses:
- attack exponent
- click weighting
- transient overshoot
- filter opening during attack

---

### 2) BodyTilt
Controls low-mid weight and sustained body.

Range: `0.0 .. 1.0`

Meaning:
- lower values: lean, short, dry
- higher values: heavier, fuller, more low-mid support

Internal uses:
- pitch drop depth
- body FM index
- tail emphasis
- low-mid energy retention

---

### 3) Drive
Controls nonlinear aggression.

Range: `0.0 .. 1.0`

Meaning:
- lower values: cleaner, more polite
- higher values: denser, harsher, more compressed and aggressive

Internal uses:
- per-engine saturation
- bus saturation
- transient compression
- metallic density boosting

---

## Shared envelope domains

All engines should interpret the main envelope in three internal domains.

```c
attack_env = env^4..env^8   // controlled by HitShape
body_env   = env^2
tail_env   = env
```

The exact exponent for attack can vary with HitShape, but the semantic contract must remain stable.

---

## Macro targets

Each engine maps Param1 / Param2 plus the three global controls into the same internal target vocabulary.

### Shared targets
- excitation_gain
- attack_brightness
- attack_click
- pitch_drop_depth
- fm_index_attack
- fm_index_body
- noise_amount
- decay_scale
- stability
- dynamic_filter_open
- drive_amount

### Engine-specific extras
- Metal: inharmonic_spread, ring_density
- Perc: ratio_bias, variation_bias

---

## Engine mapping summary

### Kick
Goal: punch, weight, front edge.

- Param1 increases hit hardness and brightness.
- Param2 increases low-end body and decay support.

### Snare
Goal: crack, shell body, sizzle.

- Param1 increases crack, noise, and top-end impact.
- Param2 increases shell/body and tone length.

### Metal
Goal: metallic attack, ring, density.

- Param1 increases strike hardness, inharmonic spread, and bite.
- Param2 increases ring support and resonant density.

### Perc
Goal: digital percussion, wood/slap/tom-like impact.

- Param1 increases transient hardness and edge.
- Param2 increases body, pitch drop, and ring support.

---

## Parameter equations

The implementation uses nonlinear mappings, not linear slider-to-value conversion.

Recommended primitives:

```c
x2 = x * x
x4 = x2 * x2
mix = a + (b - a) * x
```

Nonlinear control is required because perceptual sensitivity is not linear.

---

## Global behavior contract

### HitShape
Must make all engines feel more percussive when raised.

### BodyTilt
Must make all engines feel fuller when raised.

### Drive
Must increase perceived density and punch without breaking stability.

---

## Consistency requirements

The parameter model is valid only if the following remain true:

- Increasing Param1 should increase attack energy or brightness.
- Increasing Param2 should increase body or decay support.
- Increasing HitShape should make the transient harder.
- Increasing BodyTilt should make the sound heavier.
- Increasing Drive should increase density and aggression.
- The same preset family must produce comparable behavior across engines.

---

## Out of scope for this project

- Full physical modeling
- Dynamic allocation of voices to engines
- Additional user parameters for each engine
- Complex filter banks
- Heap allocation or runtime graph rebuilding

---

## Future split project

The resonant engine is retained in code, but not called in the current product.

It is reserved for a future second project that shares:
- the 4-voice probability framework
- the Drumlogue runtime model
- the NEON-friendly implementation style

That future project can focus on resonant, tonal, or modal percussion.

# Sonaglio — Engine Redesign Progress (2026-06)

## Session summary

This session focused on diagnosing and fixing compounding envelope non-linearity
in the snare and metal engines, completing a test suite rewrite (16/16 passing),
and assessing reference projects for potential engine additions.

---

## Root cause: compounding power-law envelopes

Both the snare and metal engines received `transient_env` or `body_env`, which
are already shaped by the global `HitShape` / `BodyTilt` projections:

```c
transient_env = lerp(env, env^4, HitShape) × (1 + 0.18 × HitShape)
```

At `HitShape = 0.5`, `env = 0.7`: `transient_env ≈ 0.595`.

Then engines applied further power laws on top:
- Snare `amp_env = envelope²` → `0.595² = 0.354`
- Snare `noise_env = env⁴` → `0.354² = 0.125` (essentially gone before wire buzz develops)
- Snare click (`env⁸` on top of `env²`) → `0.016` (inaudible at any but the very first samples)

This made body and sustain parameters effectively deaf — the envelope had already
died before the character had time to develop. Parameter sweeps of Attack and Body
produced almost no audible difference in the mid-to-late envelope.

---

## Snare engine fixes

**File**: `snare_engine.h`

### `snare_engine_process` changes

| Domain | Was | Now | Rationale |
|--------|-----|-----|-----------|
| `amp_env` | `envelope²` | `envelope` (linear) | Shell needs body decay, not instant cutoff |
| `noise_env` | `env⁴` | `env²` | Wire buzz must sustain through the hit body |
| FM `body_index` modulation | `env²` × body_index | `amp_env` × body_index | Tracks linear shell amplitude |
| `low_shell` coefficient | `0.38` | `0.32` | Compensates for louder linear amp_env |

### `snare_engine_update` range changes

| Parameter | Base | Coeff(attack) | Coeff(body) | Notes |
|-----------|------|------|------|-------|
| `noise_mix` | `0.10` | `+0.40×atk` | `-0.15×body` | More audible wire buzz |
| `noise_gain_base` | `0.030` | — | `+0.50×mix` | Wire character more present |
| `click_gain` | `0.40` | `+1.30×atk` | — | Accent, not dominant |
| `tone_gain_base` | `0.65` | — | `+0.85×body` | Reduced to balance louder linear amp |

### Output restructure

Removed double-noise construction (noise added twice via separate paths).
Clean split into three parallel contributors:
- `tone_out` = FM tone × linear amp_env + low_shell reinforcement
- `wire_out` = bandpass noise × env² wire gain + env⁸ click
- `output` = tone_out + wire_out → cubic clip with short env⁸ drive

---

## Metal engine fixes

**File**: `metal_engine.h`

### Root causes

1. `ring_index = 10..16.5` applied identically at all four cascade stages (Op4→Op3,
   Op3→Op2, Op2→Op1). With serial FM at full depth every stage hashes — the carrier
   has no clean fundamental ring underneath complex upper partials.

2. `feedback_gain` reaching 0.90 (Op4 self-feedback). Above ~0.60, the DX7 feedback
   operator produces square-wave buzz rather than metallic ring.

3. Noise BPF at 7600 Hz aligned with carrier range top — too much high-frequency
   strike noise overlapping the FM spectrum.

### `metal_engine_recompute` changes

| Parameter | Was | Now | Notes |
|-----------|-----|-----|-------|
| `ring_index` range | `10..16.5` | `7..13` | Richly metallic but not hashed |
| `feedback_gain` cap | `0.90` | `0.60` | Metallic richness, not buzz |
| `output_gain` base | `0.34` | `0.40` | Compensates for reduced ring_index |

### `metal_engine_process` changes

| Location | Was | Now | Notes |
|----------|-----|-----|-------|
| Op2→Op1 modulation | `ring_index × gated_env` | `ring_index × 0.50 × gated_env` | Cleaner carrier fundamental |
| Noise BPF cutoff | `7600 Hz` | `4200 Hz` | Better cymbal/metallic noise character |

---

## Test suite rewrite

**File**: `test_sonaglio.cpp` — 16 tests, all passing.

Tests validated:
- `test_mapping_helpers` — HitShape and BodyTilt monotonicity, fm_make_drive_gain, soft_clip
- `test_probability_gate` — 0%, 50%, 100% probability gates
- `test_preset_loading` — all 26 presets validate finite, within-range params
- `test_envelope_smoke` — trigger/process/release cycle
- `test_nonlinear_envelope_decays` — decay coefficient validity
- `test_engine_smoke` — finite output + parameter sensitivity, all 4 engines
- `test_engine_output_bounds` — all engines ±5.0 at extreme params
- `test_snare_noise_sustain` — nonzero wire buzz energy at env=0.35
- `test_snare_attack_sensitivity` — high attack >1.05× energy vs low attack at onset
- `test_metal_parameter_bounds` — feedback_gain ∈ [0, 0.60], ring_index ∈ [7, 13]
- `test_metal_ring_sustains` — nonzero metal energy at env=0.35 (sqrt envelope working)
- `test_metal_body_increases_ring` — body param produces different output over 64 samples
- `test_metal_gong_character` — cymbal ≠ gong character (different spectral content)
- `test_note_routing` — smoke test, no crash
- `test_synth_smoke` — full synth produces nonzero output
- `test_noise_char_effect` — PARAM_NOISE_CHAR=0 vs 100 with INST_SNARE differs over 96 samples

Fixed test bugs:
- Removed stale `PARAM_DRIVE` references (replaced by `PARAM_NOISE_CHAR` in current model)
- Preset 0 is KICK, which doesn't take `noise_add`; forced `INST_SNARE` in noise_char test

---

## engine_mapping.h additions

Added `fm_make_drive_gain` utility:
```c
// Maps 0..1 drive to amplitude multiplier 1.0..3.5 for pre-clip saturation.
// Not wired to a UI parameter. Available for preset-time use.
ENGINE_MAPPING_INLINE float32x4_t fm_make_drive_gain(float32x4_t drive) {
    return vaddq_f32(vdupq_n_f32(1.0f), vmulq_n_f32(fm_vclamp01(drive), 2.5f));
}
```

---

## Reference project analysis

### ctag-fh-kiel/md-drum-synth

Key FM models reviewed: `FmSnareModel`, `FmCymbalModel`.

**Snare**: true per-component first-order IIR decay — amplitude, FM index, and noise
each decay at their own independent rate (`1 - dt/decay_constant`). This avoids
the compounding problem entirely.

**Cymbal**: 4 independent carrier/modulator pairs (each at its own inharmonic frequency),
combined at the output. Cleaner partial separation vs Sonaglio's serial cascade.

Techniques not yet in Sonaglio that could improve engines:
- True independent per-component decay constants (amplitude, index, noise)
- 4-pair independent cymbal model as "Character 2" for the Metal engine

**License**: No explicit OSI license found. Must be treated as All Rights Reserved.
Do not copy code without written permission from ctag-fh-kiel.

### copych/ESP32-S3_FM_Drum_Synth

6-operator FM synthesis with 17 routing algorithms (DX-style) on ESP32-S3.

Interesting techniques: waveform blending (sine, triangle, square, saw, noise per operator).
Not directly applicable due to different hardware architecture (ESP32-S3 vs ARM Cortex-A7).

**License**: No license file visible. Must be treated as All Rights Reserved.
Do not copy code without written permission from copych.

### Summary: can code be reused?

Both reference repositories lack explicit OSI-compatible licenses. Neither ctag nor
copych code may be incorporated without written permission from the respective authors.

However, **ideas** and **techniques** described in publications, academic papers, or
general DSP references are not subject to copyright. The following improvements are
independently implementable from first principles:
- Per-component independent decay (general FM synthesis design)
- Multi-pair inharmonic oscillators for cymbal (documented in DX7 literature, Roads, etc.)
- Self-feedback FM operator (standard Yamaha DX algorithm)

---

## Current engine quality assessment

### Snare — improved

After the envelope fixes:
- Wire buzz now sustains through the hit body (env² domain)
- Shell FM tracks amplitude linearly — body parameter is audible
- Click is an accent (env⁸) not the dominant element
- Attack and Body sweeps produce clearly different timbres

Remaining limitation:
- Still a single shared envelope, not independent per-component decay
- The wire buzz cannot decay independently from the FM shell
- At very long envelope ROM shapes, the tone and noise taper together (less natural)

### Metal — improved

After the cascade and feedback fixes:
- ring_index 7..13 gives richly metallic FM without broadband hash
- feedback_gain capped at 0.60 — ring character without square-wave saturation
- Op2→Op1 at 50% ring_index gives a cleaner carrier fundamental
- Cymbal (ratios 1.0/1.483/1.932/2.546) and Gong (1.0/2.756/3.752/5.404) characters
  are now meaningfully different via bit 7 of ENV_SHAPE

Remaining limitation:
- Serial cascade still mixes partials at the phase level — vs. 4-pair parallel cymbal
  which allows independent partial decay
- At high Body (dense FM), the serial cascade can still shade toward noise-like

### Kick — not modified (good)

Already working well per user assessment. Linear amp envelope, env⁸ pitch/FM transients.

### Perc — not modified (good)

Already working well. Linear amp, env⁸ strike, env⁴ body FM.

### Hat — not modified

6-detuned square oscillators with HPF noise. Simplest engine. No FM computation.
Not a priority for redesign.

---

## Remaining known issues

1. **No per-component independent decay** — shared envelope affects all components
   simultaneously. Implementing true independent decay (like ctag snare) would require
   per-engine per-component envelope state — larger struct, more CPU, but much more
   natural sounding.

2. **PARAM_DRIVE not wired** — `fm_make_drive_gain` exists in engine_mapping.h but
   there is no UI parameter for it. The global "drive" role is currently filled by
   `PARAM_NOISE_CHAR` (noise-to-FM blend) which is different in character. A true
   pre-clip amplitude multiplier could be wired to an unused or repurposed slot.

3. **Hat engine is minimal** — no FM computation; square oscillators only. At high
   frequency settings can sound harsh. A 4-pair inharmonic model (from DX cymbal
   literature) would give it more ring and partial definition.

4. **Metal at high Body approaching noise** — serial 4-op cascade at ring_index=13
   with dense ratios creates near-noise texture. Consider a second character mode
   using parallel pairs instead of serial cascade.

5. **No velocity scaling** — current instrument ignores MIDI velocity. Simple 0..1
   gain scaling on velocity would improve playability significantly.

---

## Files changed in this session

| File | Change |
|------|--------|
| `snare_engine.h` | Linear amp_env, env² wire buzz, restructured output |
| `metal_engine.h` | ring_index 7..13, feedback cap 0.60, 50% Op2→Op1, BPF 4200 Hz |
| `engine_mapping.h` | Added `fm_make_drive_gain` utility |
| `test_sonaglio.cpp` | Complete rewrite — 16 tests, 16/16 passing |
| `README.md` | Complete rewrite — accurate architecture, parameter tables, engine details |
| `PARAMETER_MODEL.md` | Removed — content merged into README.md |

---

## Next steps

### Short term (before next HW test)
- HW test snare: verify wire buzz is now audible at moderate Attack settings
- HW test metal: verify ringing character with cymbal vs. gong distinction
- HW test global HitShape/BodyTilt: confirm they feel like distinct controls

### Medium term
- Add per-component decay constants to snare (separate noise decay from shell decay)
- Add a parallel 4-pair cymbal character mode to Metal engine
- Wire velocity to output gain (simple, high impact)

### Long term
- Investigate Hat engine FM upgrade (inharmonic pairs, independent Copyright-free design)
- Consider adding a 6th engine (resonant/modal) using the existing NEON framework

# Sonaglio — Progress and Design Roadmap

## Current status

The project has moved from a 5-engine / allocation-driven layout to a **fixed 4-engine FM percussion instrument**.

What is already in place:

- 4 fixed engines: Kick, Snare, Metal, Perc
- New parameter contract in `header.c`
- `synth.h` control-layer rewrite
- `engine_mapping.h` for parameter projection
- rewritten engine files for the current 4-engine model
- preset data rebuilt around the new parameter language
- resonant engine retained in code, but not active in the main instrument path

The current design is now much closer to a stable instrument identity rather than a collection of loosely related synth modes.

---

## What is still being stabilized

### 1. Final runtime wiring
The control model is defined, but the last pass of runtime validation is still needed to confirm that:

- `HitShape` affects transient behavior in the render path
- `BodyTilt` affects body and low-mid weight in the render path
- `Drive` affects saturation / bus push in the render path

### 2. Engine behavior review
The engine rewrites are in place, but they still need listening and test-based confirmation to verify:

- punch consistency
- body consistency
- output balance between engines
- how far each engine can reach within the current parameter range

### 3. Preset tuning
The presets are structurally correct, but they are still provisional.
They should be tuned after the core behavior is confirmed, not before.

### 4. Test suite refresh
The old tests were tied to the previous architecture and must be replaced with tests that validate the new behavior model.

---

## Important design choices already made

### Fixed engine identity
The project now assumes:

- Kick = low-end strike
- Snare = crack + shell + noise
- Metal = inharmonic metallic energy
- Perc = flexible percussion / block / tom / wood / digital percussion

### No voice allocation in the UI
The instrument no longer spends parameter budget on engine allocation.
This makes the sound design more direct and easier to preset.

### Global shaping controls
The three reclaimed global controls are now part of the instrument identity:

- `HitShape`
- `BodyTilt`
- `Drive`

These are currently kept in `params[]` and should remain there.

### Resonant engine policy
The resonant engine remains in the codebase for possible future use, but it is not part of the active instrument.

---

## Open design questions

### Velocity
Velocity exists as a possible runtime factor, but it is still undecided whether it should remain.

Possible uses:
- transient drive
- attack brightness
- body emphasis
- saturation amount

If it does not produce a clearly audible benefit, it should be removed.

### Parameter polarity
Most parameters are currently unipolar (`0..100`).

Possible later extension:
- bipolar values (`-100..100`) for specific controls that benefit from a two-sided meaning

Potential candidates:
- modulation polarity
- spectral tilt
- detune spread direction
- transient emphasis vs suppression
- body bias toward thin or heavy behavior

This should wait until the current version is stable.

### Reference tuning
Reference-based tuning is useful, but only after the engine behavior is stable.
Potential references include:
- Nord Drum / Nord Drum 2
- DX7-style percussion behavior
- Model:Cycles-style digital percussion behavior

These references are best used as listening targets, not as direct architecture templates.

---

## Future implementation ideas

### Short term
- verify the new render path
- confirm the final parameter wiring
- remove any leftover legacy references
- tighten preset families
- add tests for monotonic parameter behavior
- add tests for output bounds and stability

### Medium term
- tune engine response curves
- refine preset families by sound character
- decide whether velocity becomes a real sound-control source
- decide whether any controls should become bipolar

### Long term
- possible second project using the same 4-voice probability framework
- resonant / modal / tonal percussion engine family
- alternate instrument identity with shared platform basics

---

## What should not be expanded too early

The following should remain intentionally limited until the instrument is stable:

- number of user parameters
- number of engine families
- parameter polarity changes
- advanced routing controls
- experimental modulation features

The instrument should first prove that the new 4-engine model is strong, coherent, and playable.

---

## Next steps

1. Final runtime validation
2. Listening and tuning pass
3. Preset refinement
4. Test rewrite and execution
5. Decide on velocity and future bipolar controls
6. Optional reference-matching pass

---

## Definition of done for the current stage

The current stage is complete when:

- the instrument produces stable audio
- the new parameter model feels consistent across engines
- presets are meaningfully distinct
- the three global controls clearly affect sound
- the test suite confirms the expected control behavior

# Sonaglio — HW Test Follow-up Progress

## Current Status

First hardware smoke test passed:

- the unit loads after adding the preset source file to `config.mk`
- sound is produced
- parameter/UI path is alive enough for first evaluation

Main HW findings:

- output level is too low
- many presets are too short/tight
- envelope indices above ~95 are more audible
- sound shaping/filtering is not yet enough to judge the instrument properly
- engine design needs a second review after envelope/gain fixes

## Immediate Fixes in This Patch

### 1. Output Gain / Selector Mix

The selector model now uses only lane 0, but the previous output stage still behaved like a reduced four-voice sum:

- low master gain
- very small per-engine mix weights

Patch:

- `master_gain` increased from `0.25f` to `0.85f`
- per-engine output trims changed to stronger nominal values:
  - Kick: `0.95`
  - Snare: `0.90`
  - Metal: `0.78`
  - Perc/Tom: `0.88`

This should make the instrument much easier to monitor on hardware.

### 2. Delayed Trigger Envelope Retrigger

The selector model uses delayed combo/shadow hits through `pending[]`.

Previously delayed triggers could fire while the shared envelope was already decaying or nearly off.

Patch:

- `process_pending_triggers()` now retriggers the shared envelope when an event fires.

This should make `Gap`, combo modes, and shadow hits more audible.

### 3. Envelope ROM Reshape

The previous ROM had many very short low-index envelopes, and the first HW test confirmed they were nearly inaudible.

Patch:

- `envelope_rom.h` now contains a curated Sonaglio v3 table.
- The lower indices are still tight, but no longer microscopic.
- Indices 80–95 remain open/linear and are now explicitly useful for HW testing.
- Reverse/pad-like slow-attack regions have been replaced by percussive long/metal/experimental shapes.

New ranges:

- `0–15`: tight click/body
- `16–31`: punch drum
- `32–47`: body drum
- `48–63`: long body
- `64–79`: gated/flam
- `80–95`: open linear
- `96–111`: metallic tail
- `112–127`: experimental long

### 4. Test Preset Bank

The preset file is now a hardware-test bank:

- `test1` … `test26`

The goal is not musical factory presets yet. The goal is to evaluate:

- single engines
- combos
- longer envelopes
- gap/scatter behavior
- LFO safety
- drive/output level
- metal/perc tails

### 5. Preset Source Ownership

`load_fm_preset()` is now expected to come from `fm_presets.cc`.

The duplicate inline implementation was removed from `fm_perc_synth_process.h` to avoid duplicate-definition risk now that `fm_presets.cc` is compiled.

## Build Note

`config.mk` must include the preset source file.

Example:

```make
CXXSRC = unit.cc fm_presets.cc
```

or equivalent, depending on the current project layout.

## Files Updated

- `fm_perc_synth_process.h`
- `envelope_rom.h`
- `fm_presets.cc`
- `fm_presets.h`

## Next Hardware Test

Test this patch before deep engine redesign.

Recommended checks:

1. Confirm compile still succeeds with `fm_presets.cc` included.
2. Confirm all presets `test1` … `test26` are visible/loadable.
3. Check whether `test1`–`test8` are now clearly audible.
4. Compare short envelopes around `test24` against longer envelopes around `test25`.
5. Check combo audibility:
   - `test9` K+S
   - `test10` K+T
   - `test11` K+M
   - `test12` S+T
   - `test13` S+M
   - `test14` T+M
6. Check whether output is now strong but not clipping.
7. Check if delayed/shadow hits are now audible.

## After This Test

If the output/envelopes are now usable, continue with engine review in this order:

1. Kick
   - low-end strength
   - attack click
   - body stability
2. Snare
   - noise filtering
   - crack/body balance
   - tail audibility
3. Perc/Tom
   - body tuning
   - block/tom distinction
4. Metal
   - brightness filtering
   - tail control
   - avoiding hash

Do not redesign all engines until the envelope and output-gain layer has been validated on hardware.

---

# Sonaglio — Engine and LFO Improvements (2026-05-23)

## Summary

Three categories of issues addressed in this patch:

1. **LFO routing completely broken** — fixed
2. **Metal engine not metallic / not ringing** — fixed
3. **Body parameters too subtle** — widened ranges

---

## Fix 1: LFO routing

### Root cause A — smoother setters never called

`fm_perc_synth_update_params` never called `lfo_smoother_set_rate/depth/target` for any of the 6 LFO parameters. The smoother stayed frozen at `target=LFO_TARGET_NONE`, `depth=0`, causing zero modulation regardless of UI values.

**Fix**: Added 6 `lfo_smoother_set_*` calls at the end of `fm_perc_synth_update_params`:
```cpp
const uint32x4_t all_lanes = vdupq_n_u32(0xFFFFFFFFu);
lfo_smoother_set_rate(&synth->lfo_smooth, 0, p[PARAM_LFO1_RATE]   * 0.01f, all_lanes);
lfo_smoother_set_depth(&synth->lfo_smooth, 0, p[PARAM_LFO1_DEPTH] * 0.01f, all_lanes);
lfo_smoother_set_target(&synth->lfo_smooth, 0, (uint8_t)p[PARAM_LFO1_TARGET], all_lanes);
// same for LFO2
```

### Root cause B — rate unit mismatch

`lfo_smooth.current_rate` stores normalized 0..1 (matching UI 0-100 scaled by 0.01). But `lfo.rate1` expects cycles/sample. Direct assignment `synth->lfo.rate1 = synth->lfo_smooth.current_rate1` wrote 0.01..0.5 as Hz-equivalent, producing ~0.5–24 Hz at 48 kHz but from the wrong base — near-zero modulation rate.

**Fix**: Replaced direct assignment with unit conversion in `fm_perc_synth_process`:
```cpp
const float32x4_t r_min  = vdupq_n_f32(LFO_ENHANCED_RATE_MIN_HZ / LFO_ENHANCED_SAMPLE_RATE);
const float32x4_t r_span = vdupq_n_f32((LFO_ENHANCED_RATE_MAX_HZ - LFO_ENHANCED_RATE_MIN_HZ)
                                        / LFO_ENHANCED_SAMPLE_RATE);
synth->lfo.rate1 = vaddq_f32(r_min, vmulq_f32(synth->lfo_smooth.current_rate1, r_span));
```
Now rate spans 0.05 Hz..18 Hz mapped to cycles/sample correctly.

---

## Fix 2: Metal engine — make it ring and sound metallic

### Problem

The metal engine FM indices were too low for real metallic character:
- `ring_index`: 0.30..1.45 (DX7 cymbal needs 2.0–5.0)
- `strike_index`: 0.35..3.55 (acceptable, but could be stronger)
- Amplitude decayed with linear envelope — very fast decay, no ring

### Changes to `metal_engine_recompute`

| Parameter | Old range | New range | Notes |
|---|---|---|---|
| `ring_index` | 0.30..1.45 | 1.5..4.7 | DX7-style metallic FM depth |
| `strike_index` | 0.35..3.55 | 0.5..5.5 | Stronger attack burst |
| `ring_gain` base | 0.42 | 0.55 | Higher base amplitude |
| `output_gain` | 0.86..0.74 | 0.68..0.54 | Compensates for louder FM |

### Changes to `metal_engine_process`

Added **square-root envelope** (`env^0.5`) for amplitude and op1 carrier FM depth:
```cpp
// NEON sqrt: x * rsqrt(x) with one Newton-Raphson iteration
float32x4_t safe = vmaxq_f32(gated_env, vdupq_n_f32(1e-8f));
float32x4_t r = vrsqrteq_f32(safe);
r = vmulq_f32(vrsqrtsq_f32(vmulq_f32(safe, r), r), r);
float32x4_t env_sqrt = vmulq_f32(safe, r);
```

`env_sqrt` decays much more slowly than linear at low levels, giving:
- Longer perceived ring on the carrier (op1 modulation uses `ring_index × env_sqrt`)
- Longer amplitude tail (`fm_output × env_sqrt × ring_gain`)

The deeper cascade operators still use `env2` / `env4` / `env8` for natural fast-decaying attack character.

---

## Fix 3: Body parameter ranges widened

### Kick — sweep_depth

| Parameter | Old range | New range |
|---|---|---|
| `sweep_depth` | 0.18..1.63 oct | 0.10..2.20 oct |

More dramatic pitch drop when Attack and/or inv_body are high.

### Perc — strike_index

| Parameter | Old range | New range |
|---|---|---|
| `strike_index` | 0.35..2.05 | 0.30..3.00 |

Stronger FM transient at high Attack settings.

---

## Per-engine envelope character summary

Each engine now has a distinct envelope shaping philosophy:

| Engine | Amp domain | Pitch domain | Index domain | Notes |
|---|---|---|---|---|
| Kick | `env^1` | `env^8` | `env^8` | Punchy linear body, instant pitch drop |
| Snare | `env^1` | `env^8` | `env^8` | Noise on `env^2` for longer tail |
| Metal | `env^0.5` (sqrt) | — | mix | Longer ring, sustained FM via sqrt |
| Perc | `env^1` | `env^8` | `env^8` / `env^4` | Fast strike, medium body FM |

---

## Files modified

- `fm_perc_synth_process.h` — LFO smoother setters + rate unit conversion
- `metal_engine.h` — ring_index/strike_index ranges, sqrt envelope
- `kick_engine.h` — wider sweep_depth range
- `perc_engine.h` — wider strike_index range
- `README.md`, `PROGRESS.md` — documentation updated

---

## Fix 4: Envelope tail clamp relaxed + metal preset retuning

### Envelope clamp

`get_envelope()` clamped **all** decays to 480ms and releases to 320ms — a
hardware-test-phase safety measure. This capped the metallic ROM range
(indices 96-127, designed for 350-1850ms tails) far below its intent, so the
metal engine's new sqrt amplitude envelope had nothing long to ring into.

**Fix**: clamp raised to decay ≤ 1200ms, release ≤ 700ms. Attack stays ≤ 6ms
(fast onsets preserved). Non-metal presets all use env_shape < 36 (decay
< 480ms) so they are unaffected; only metallic/long ranges benefit.

### Metal preset retuning (the four pure-metal presets, instrument = METAL)

Predictions based on the new ring_index (1.5..4.7) and sqrt ring envelope:

| Preset | env_shape | metal A/B | Intent |
|---|---|---|---|
| MetalClang | 8 → 99 (metallic ~515ms) | 95/65 → 88/60 | Bright cymbal clang, real ring |
| MetalWash  | 70 → 106 (metallic ~900ms) | 80/90 | Sustained shimmering wash |
| GongHit    | 90 → 238 (Gong char + idx110 ~1120ms) | 100/95 → 50/95 | Dark, dense, long gong |
| BellRing   | 110 (now unclamped ~1120ms) | 100/85 → 75/85 | Bright cymbal-bell shimmer |

- `GongHit` had lost its Gong character (bit 7); restored via env_shape 238
  (`128 + 110`). The `(uint8_t)` cast in `update_params` preserves the bit even
  though `params[]` is `int8_t`.
- Brightness (Attack) reduced on the bright presets because the higher base FM
  indices now make full Attack harsher than before.
- INDEX LFO depth on MetalClang trimmed 40 → 30 since the base ring_index is
  now much higher.

Other presets left intact: combos (KM/SM/TM) share one envelope between a short
percussive engine and metal, so a long metallic env would smear the percussive
layer. Those stay short by design.

### Note on LFO now being live

Because LFO routing was previously silent, all preset LFO depths were untested
no-ops. They are now audible. The existing depths were reviewed and judged
musically reasonable for their intent (e.g. Shaker's fast NOISE_MIX tremolo,
Industrial's METAL_GATE chop), so they were left as-is pending hardware
listening. Adjust after HW feedback if any feel too strong.


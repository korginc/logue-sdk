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


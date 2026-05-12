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


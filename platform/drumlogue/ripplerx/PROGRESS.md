# Project Status Tracker

## Phase 1 to 9: [COMPLETED]
- Core DSP, sample loading, structural physical modeling, and OS lifecycles finished.

---

## Phase 10: Bug-Fix Session — Root Cause of Hardware Silence [COMPLETED]

A full code audit was performed after confirming zero audio output on hardware.
Four confirmed bugs were found and fixed.

### BUG 1 — CRITICAL (Primary cause of silence): Init() leaves all DSP params at zero

**Root cause:** `Init()` calls `Reset()` which does `memset(&state, 0, ...)`.
This zeroes every field of `SynthState`, including inside all four `VoiceState` structs:

| Field zeroed | Effect |
|---|---|
| `mallet_stiffness = 0.0f` | Mallet exciter outputs exactly 0.0 — zero energy enters the waveguide |
| `feedback_gain = 0.0f` | No signal circulates in the delay loop — resonator is dead |
| `lowpass_coeff = 0.0f` | The 1-pole loss filter holds its state at 0 permanently — silences feedback |

With all three zeroed, the mallet produces no impulse, and even if any energy leaked
in, the feedback loop would be dead. The result is complete silence, every note.

**Fix:** Call `LoadPreset(0)` at the end of `Init()`, after the sample-function
pointers are stored. This runs `setParameter` for every slot, setting
`mallet_stiffness = 0.5f`, `feedback_gain ≈ 0.87f`, `lowpass_coeff ≈ 0.5f`, etc.

### BUG 2 — Chamberlin SVF tuning formula uses wrong divisor

**Root cause:** `filter.h` `set_coeffs()` computes:
```cpp
f = 2.0f * fastercosfullf(0.25f - (safe_cutoff / srate));
```
`fastercosfullf` maps input [0, 1] to a full 360° cycle, so:
`cos(2π(0.25 − x)) = sin(2πx)` — this gives `f = 2·sin(2π·cutoff/srate)`.

The correct Chamberlin formula is `f = 2·sin(π·cutoff/srate)`, requiring:
```cpp
f = 2.0f * fastercosfullf(0.25f - (safe_cutoff / (2.0f * srate)));
```
With the old formula the filter operated at double the intended cutoff frequency.
At 10 kHz the computed `f` was 1.93 instead of the correct 1.22. While the SVF
remains stable in practice (the 0.45·srate clamp keeps `f < 2`), frequency
scaling was wrong for the entire LowCut parameter range.

**Fix:** Changed divisor from `srate` to `2.0f * srate` in `set_coeffs()`.

### BUG 3 — Reset() does not restore mix_ab default

**Root cause:** `Reset()` restores `master_gain = 1.0f` and `master_drive = 1.0f`
after the memset, but not `mix_ab`. The `SynthState` struct declares `mix_ab = 0.5f`
as its default, but memset overwrites that to 0. With `mix_ab = 0`, the voice
output formula `outA*(1−mix_ab) + outB*mix_ab` becomes `outA*1 + outB*0`, so
Resonator B is completely dropped from the mix regardless of the HitPos parameter.

**Fix:** Added `state.mix_ab = 0.5f` restoration inside `Reset()`.

### BUG 4 — Stale `read_pos` variable in process_waveguide()

**Root cause:** `process_waveguide()` computed `read_pos` (same formula as `read_idx`)
and never used it. Dead code — harmless but misleading.

**Fix:** Removed the unused variable.

---

## Phase 11: Independent Resonator B Control — Partls-selector strategy [COMPLETED]

The **Partls** parameter (index 8, range 0–6) now serves a dual role:
- Values **0–4**: set the active partials count ("4", "8", "16", "32", "64")
  and display with an A/B suffix so the user sees which resonator is targeted.
- Value **5**: switch edit context to **Resonator A** for subsequent parameter changes.
- Value **6**: switch edit context to **Resonator B** for subsequent parameter changes.

**Dkay**, **Mterl**, and **Inharm** now route to `resA` or `resB` independently,
determined by `m_is_resonator_a`. **Model** is also per-resonator (`m_model_a` /
`m_model_b`), updating `phase_mult` for each independently.

**Preset loading** always initialises both resonators symmetrically:
`LoadPreset` forces ResA context for the main parameter loop, then explicitly
mirrors Dkay/Mterl/Inharm to ResB, then restores the user's edit context.

**Reset** resets `m_is_resonator_a = true` so a cold start or Suspend always
leaves the engine in a clean, deterministic ResA-first state.

### Phase 11 bug-fix review [COMPLETED]
The following bugs were found and fixed after the initial Partls-selector commit:

- **Compile error**: `True` → `true` (C++ boolean literal).
- **Compile error**: Missing `}` closing the `if (index == k_paramModel)` block in
  `getParameterValue`, leaving `return m_params[index]` unreachable.
- **Compile error**: Missing `;` after `model_names_a[]` initialiser in
  `getParameterStrValue`.
- **Bug**: `getParameterStrValue` for Model used `model_names_a` for both
  resonators — now uses `model_names_b` correctly for ResB.
- **Bug**: `getParameterStrValue` for Partls values 5 and 6 fell through to "---"
  — now shows "-> ResA" / "-> ResB".
- **Bug**: `getParameterStrValue` for Partls used `m_active_partials` as the
  array index instead of the function's `value` argument — fixed.
- **Bug**: `getParameterStrValue` for Bank, NzFltr, and Program indexed into their
  arrays with stored state variables instead of the `value` argument — fixed.
  The function argument is always the value being *browsed*, not the stored state.
- **Bug**: `k_paramLowCut` handler lost its `/ 1000.0f` divisor for the resonance
  Q, passing 707–4000 to `set_coeffs` instead of 0.707–4.0. SVF was near-
  unstable. Fixed.

---

## Phase 12: Unused Parameters (currently do nothing — future work)

* **`Tone` (Index 12):** Unmapped. Could become a tilt-EQ shelf inside the
  feedback loop, or a master high-shelf for brightness control.
* **`Partls` (Index 9):** Currently only gates ResB on/off (threshold = 2).
  Better usage: repurpose as A/B coupling depth — how much ResB's output
  feeds back into ResA's exciter input.
* **`VlMllRes` & `VlMllStf` (Indices 6 & 7):** Unmapped velocity modifiers.
  Real drums sound sharper at higher velocity. Map these to scale
  `mallet_stiffness` and the noise envelope rate by `current_velocity`.
* **`NzFltr` & `NzFltFrq` (Indices 21 & 22):** Master SVF exists but noise
  has no dedicated filter. Add a second `FastSVF` inside `ExciterState`
  that shapes the noise before it enters the delay line.
* **`MlltRes` (Index 4):** Parsed but not forwarded to the exciter.
  Map to an additional `mallet_lp` pole to vary the strike brightness.
* **`TubRad` (Index 17):** Unmapped. Could scale `lowpass_coeff` (wider tube
  = less high-frequency loss) or `ap_coeff` (tube geometry affects dispersion).

---

## Phase 13: Missing Features & Improvements (TODO)

* **Dynamic Energy Squelch (CPU):** Track per-voice output RMS. When the
  waveguide decays below −80 dB, set `is_active = false` immediately rather
  than waiting for the release envelope. Prevents zombie voices consuming CPU
  for the full envelope release time after the signal is inaudible.
* **Dedicated Noise SVF:** Instantiate a second `FastSVF` inside `ExciterState`
  to filter white noise before it enters the delay line. Wire `NzFltr` and
  `NzFltFrq` parameters to its coefficients.
* **Model B Parsing:** The Model UI parameter passes 0–17. Currently only
  Model A is extracted (`value % 9`). Extract Model B (`value / 9`) and apply
  its topology to `resB` independently.
* **Pitch Bend:** Wire `unit_pitch_bend()` into `NoteOn` delay-length math.
  A ±semitone bend would require multiplying `delay_length` by `2^(bend/12)`.
* **True Stereo Master Filter:** `master_filter` is a single mono `FastSVF`.
  Add a second instance to `SynthState` for the right channel so future stereo
  effects (chorus, panning) pass through a properly independent filter.
* **Voice 0 skipped on first note:** `NoteOn` increments `next_voice_idx`
  before assigning, so the first ever note goes to voice 1. Minor; reorder to
  assign before incrementing or just note it as a cosmetic issue.

---

## Phase 1: Core DSP Structures [COMPLETED]
- [x] Define flat, cache-friendly C++ structures (`Waveguide`, `Exciter`, `Voice`).
- [x] Establish memory bounds (4096-sample delay line for safe sub-bass).

## Phase 2: API & UI Binding [COMPLETED]
- [x] Connect `header.c` parameter indices to the engine.
- [x] Write `setParameter` translation logic.
- [x] Fix `MlltStif` buffer overflow bug in UI header.

## Phase 3: The Audio Processing Loop [COMPLETED]
- [x] Write the per-sample DSP loop.
- [x] Implement linear interpolation for accurate pitch tuning.

## Phase 4: Architectural Refactor & Sample Management [COMPLETED]
- [x] JIT sample loading inside `NoteOn` safely implemented.
- [x] Hardware "Hello World" successfully compiled and tested on Drumlogue!

## Phase 5: Envelopes & Exciters [COMPLETED]
- [x] `envelope.h` and `noise.h` implemented and tested on hardware.
- [x] Noise mix and PCM sample triggering confirmed working.

## Phase 6: Filters & Master FX [COMPLETED]
- [x] `filter.h`: Implement a 2-pole Chamberlin State Variable Filter (SVF).
- [x] Tie `header.c` LowCut and Resonance parameters to the Master SVF.
- [x] Integrate SVF incrementally into the audio loop.

## Phase 7: Waveguide Models & Tables [COMPLETED]
- [x] `tables.h`: Defined fast-math lookup tables for MIDI-to-Delay-Length conversion.
- [x] Implemented branchless Tube physics (Inverting Feedback via `phase_mult`).
- [x] Implemented Membrane physics (Inharmonic irrational detuning of Resonator B).
- [x] Linked UI `Model` parameter to physical topologies.

## Phase 8: Preset Design & Acoustic Tuning [PENDING]
- [ ] Reverse-engineer waveguide parameters for legacy 28 presets.
- [ ] Derive coefficients for new instruments: Timpani, Djambé, Taiko, Marching Snare, Tam Tam, Koto.

## Phase 9: UI Polish & Missing SDK Hooks [COMPLETED]
- [x] Preset Management: `LoadPreset()`, `getPresetIndex()`, `getPresetName()`.
- [x] State Reporting: `getParameterValue()` for OLED display sync.
- [x] Parameter Linkage: all core `header.c` knobs wired in `setParameter`.
- [x] Release Phase Logic: `NoteOff`, `GateOff`, `AllNoteOff`, master envelope VCA.
- [x] Free Parameter Decision: Gain slot → overdrive drive multiplier (1×–21×).

## Phase 10: Bug-Fix Session [COMPLETED]
- [x] Fix Init/Reset silence: `LoadPreset(0)` called at end of `Init()`.
- [x] Fix Chamberlin SVF formula: divisor corrected to `2*srate`.
- [x] Fix `Reset()` not restoring `mix_ab = 0.5f`.
- [x] Remove dead `read_pos` variable from `process_waveguide()`.
- [x] Document mono-filter intentional L-copy pattern with TODO for true stereo.

## Phase 11: Independent Resonator B Control — Partls-selector [COMPLETED]
- [x] Partls 0–4 = partial count; 5 = select ResA edit; 6 = select ResB edit.
- [x] Dkay, Mterl, Inharm route to resA or resB based on m_is_resonator_a.
- [x] Model is per-resonator (m_model_a / m_model_b), phase_mult updated independently.
- [x] LoadPreset: forces ResA context, mirrors Dkay/Mterl/Inharm to ResB, restores context.
- [x] Reset: resets m_is_resonator_a = true for deterministic cold start.
- [x] Fix: True → true (compile error).
- [x] Fix: Missing } in getParameterValue (compile error / unreachable return).
- [x] Fix: Missing ; after model_names_a[] init (compile error).
- [x] Fix: Model B showing model_names_a instead of model_names_b.
- [x] Fix: Partls values 5/6 showing "---" — now "-> ResA" / "-> ResB".
- [x] Fix: getParameterStrValue used state vars instead of value arg (Bank, NzFltr, Program, Partls).
- [x] Fix: k_paramLowCut dropped /1000.0f for Q — SVF near-unstable. Restored.

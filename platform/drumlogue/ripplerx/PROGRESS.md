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

### BUG 2 — Chamberlin SVF formula catastrophically wrong (filter always near-Nyquist)

**Root cause:** `filter.h` `set_coeffs()` used `fastercosfullf`, which takes
**radians** (it is implemented as `fastersinfullf(x + π/2)`), not a [0,1]-normalised
fraction as the comment claimed. The formula:
```cpp
f = 2.0f * fastercosfullf(0.25f - (safe_cutoff / (2.0f * srate)));
```
actually computes `2·cos(0.25_rad − cutoff/(2·srate))`. Because 0.25 radians
is only slightly less than the cosine peak, the argument stays near 0 for all
audio frequencies and `f` is always in [1.91, 1.97] — regardless of cutoff.

The Chamberlin SVF stability limit is `f < 2`. With f ≈ 1.91, the filter has
an eigenvalue of ≈ 4.85, causing a ×5 blow-up every 3 samples. Both the
**master_filter** (LowCut) and the **noise_filter** (NzFltr/NzFltFrq) diverge
to NaN within ~60 samples of receiving any nonzero audio. The brickwall limiter
(`fmaxf(-0.99, fminf(0.99, NaN)) = 0.99`) silently masked the failure so the
synth appeared to work — every note immediately hard-clipped to ±0.99.

A previous partial fix changed the divisor from `srate` to `2*srate` (based on
the incorrect assumption about `fastercosfullf`'s input domain). That fix had
no meaningful effect: both versions produce f near 1.91 for all audio
frequencies and are equally unstable.

**Fix:** Replace the broken formula with `sinf()`, which is unambiguous and
only runs in the UI thread (setParameter) so its cost is negligible:
```cpp
f = 2.0f * sinf(M_PI * safe_cutoff / srate);
```
Correct f values for reference: 10 Hz → 0.0013, 1 kHz → 0.131,
5 kHz → 0.643, 10 kHz → 1.218, 20 kHz (clamped) → 1.975. All < 2, stable.

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

## Phase 12: Unused Parameters

### Implemented [COMPLETED]

* **`VlMllStf` (Index 7):** In `NoteOn`, overrides `mallet_stiffness` on the
  triggered voice: `stiffness = base_stiff + (VlMllStf/100) * velocity`.
  Harder hits produce a stiffer (brighter) mallet strike per-note without
  affecting concurrently ringing voices.
* **`VlMllRes` (Index 6):** In `NoteOn`, overrides `noise_env.attack_rate` on
  the triggered voice: positive values shorten the attack at high velocity
  so accent hits have a sharper transient.  Formula:
  `attack = base_attack + (VlMllRes/100) * velocity * 0.5`.
* **`MlltRes` (Index 4):** Wired as a second 1-pole LP cascaded after the
  existing `mallet_stiffness` LP.  High value → mallet energy passes through
  quickly (bright), low value → extra roll-off (dark/woody body).
  Both poles' state variables (`mallet_lp` and `mallet_lp2`) are reset to 0
  on each `NoteOn` to prevent clicks from polyphonic overlap.

### Previously Pending — now completed in Phase 13

* ~~**`Tone` (Index 12):** Unmapped.~~ → Tilt EQ (Phase 13).
* ~~**`Partls` values 0–4:** Only gated ResB on/off.~~ → Bidirectional coupling depth (Phase 13).
* ~~**`NzFltr` & `NzFltFrq` (Indices 21 & 22):** No dedicated noise filter.~~ → `FastSVF noise_filter` in ExciterState (Phase 13).
* ~~**`TubRad` (Index 17):** Unmapped.~~ → Combined with Mterl for `lowpass_coeff` (Phase 13).

---

## Phase 14: Code-Audit Bug-Fix Session [COMPLETED]

A second full audit was performed after the first round of UTs passed.
Six issues were found and fixed.

### FIX 1 — Critical: Squelch prematurely kills voice during delay-line transit

**Root cause:** The old squelch checked `fabsf(voice_out) < 0.0001f` while
`is_releasing` was set. For note 60 (C4) the delay-line round-trip takes ~183
samples (~3.8 ms). Any GateOff within that window produced silence because
`voice_out` was genuinely zero (the wave hadn't reflected yet) and the voice
was killed before a single audible sample emerged.

**Fix:** Replaced amplitude-threshold squelch with a **damper-pedal envelope**
approach. `master_env` is configured in `NoteOn` with `sustain_level=1.0` and
`decay_rate=0`, so it holds at **1.0×** during gate-on with no audible effect.
On GateOff/NoteOff it fades to 0 at the `k_paramRel` rate. The voice is marked
inactive only when `master_env.state == ENV_IDLE` — time-based, not
amplitude-based.

**Side-effect fix:** `k_paramRel` now audibly controls the physical-tail fade.
Previously the Release knob had no effect because `master_env` output was
commented out.

### FIX 2 — High: `Reset()` did not clear filter states `z1`, `ap_x1`, `ap_y1`

**Root cause:** `Reset()` zeroed `buffer[]` and `write_ptr` but left the 1-pole
LP state (`z1`) and allpass states (`ap_x1`, `ap_y1`) intact. A `Reset()` called
mid-play (OS pattern change) left non-zero filter memory, causing a click or DC
transient at the start of the next note.

**Fix:** Added explicit zeroing of all six filter-state fields inside the
`Reset()` per-voice loop.

### FIX 3 — Medium: FastSVF resonance lower-clamp mismatch

**Root cause:** `set_coeffs` clamped `resonance` to a minimum of `1.0`. The UI
`Resnc` bottom is `707` → `0.707` after `/1000`. The Butterworth flat-response
Q (0.707) was silently raised to Q=1.0 and the bottom half of the Resnc knob
travel had no effect.

**Fix:** Changed the lower clamp from `1.0f` to `0.5f`. The UI minimum 0.707
now passes through unchanged, giving a true Butterworth response at the
minimum knob position.

### FIX 4 — Low: Duplicate `is_active`/`is_releasing` assignment in `NoteOn`

**Root cause:** `is_active = true; is_releasing = false;` appeared twice —
once before and once after the sample-loading block that was inserted between
two existing copies of the same assignment.

**Fix:** Removed the redundant second copy.

### FIX 5 — Low: `m_params[25]` over-allocated by one element

**Root cause:** Declared with 25 slots despite only 24 parameters (0–23).
Index 24 was never read or written.

**Fix:** Corrected to `m_params[24]`.

### FIX 6 — Low: `k_paramMterl` guard missing lower-bound check

**Root cause:** `if (value <= 30)` accepted values below −10, though the inner
`fmaxf` clamped them safely. The asymmetric guard was inconsistent with the
`header.c` range.

**Fix:** Changed to `if (value >= -10 && value <= 30)`.

### Test fixes (test_hw_debug.cpp)

- **T9 hardcoded voice index:** `voices[1].resA` → `voices[s.state.next_voice_idx].resA`.
- **T9 missing from runner:** `test_delay_roundtrip()` added to `main()`.

---

## Phase 13: Parameter Activation & SVF Fix [COMPLETED]

Four parameters stored in `m_params` but previously producing no DSP effect
have been wired up, and the underlying SVF formula bug that made them unusable
has been corrected.

### SVF formula fix (filter.h)
`fastercosfullf` takes **radians**, not a [0,1]-normalised fraction (see BUG 2
above). The formula was producing f ≈ 1.91–1.97 for all audio frequencies,
making both the master filter and the noise filter catastrophically unstable
(eigenvalue ≈ 4.85, ×5 blow-up per 3 samples → NaN within 60 samples, silently
masked by the brickwall limiter). Fixed with `f = 2·sinf(π·fc/srate)`.

### Dedicated Noise SVF (NzFltr / NzFltFrq)
Added `FastSVF noise_filter` to `ExciterState` (inside `ENABLE_PHASE_6_FILTERS`
guard). `k_paramNzFltr` sets its mode (0=LP, 1=BP, 2=HP). `k_paramNzFltFrq`
sets its cutoff (clamped 20–20000 Hz). Applied to the noise burst in
`process_exciter()` before the noise scales by `noise_decay_coeff`.
Defaults on Reset(): LP mode, 12 kHz cutoff.

### TubRad + Mterl combined (k_paramTubRad / k_paramMterl)
The two parameters now share a `case k_paramMterl: case k_paramTubRad:` block.
Either change recalculates `lowpass_coeff` from both stored values:
- Mterl (−10 to +30) → base material brightness (0.01 = dull wood, 1.0 = lossless metal)
- TubRad (0 to 20) → pulls the coefficient towards 1.0 (wider tube = brighter)

### Partls as bidirectional coupling depth (k_paramPartls values 0–4)
UI values 0–4 (previously only gating ResB on/off) now also set cross-coupling
depth (0.0–1.0). ResA receives `exciter + resB_out_prev × depth × 0.5`;
ResB receives `exciter + outA × depth × 0.5`. The `m_active_partials >= 16`
CPU gate for ResB is preserved. `resB_out_prev` is reset on NoteOn and in the
`else` branch to prevent artefacts when ResB is inactive.

### Tone tilt EQ (k_paramTone, −10 to +30)
Per-voice 1-pole LP (coefficient 0.3, cutoff ≈ 2.7 kHz) extracts the low
component. Negative Tone blends toward LP (darker); positive Tone boosts the HP
component (brighter, up to 2× high-shelf gain at Tone=30). Applied after the
velocity scale, before the master envelope/limiter. `tone_lp` state is reset on
NoteOn.

### Unit tests (T10–T15)
- **T10** (structural): verifies NzFltr and NzFltFrq route to `noise_filter.mode`
  and `noise_filter.f` on all 4 voices. Structural (not audio) because sustained
  noise into the feedback resonator overflows float before a round-trip completes.
- **T11**: verifies TubRad raises `lowpass_coeff` above the Mterl-only value but
  stays below 1.0 (damping still present).
- **T12**: verifies both Partls=0 (ResA only) and Partls=4 (dual + coupling)
  produce sound.
- **T13**: uses pre-limiter `ut_voice_out` tap to verify Tone=30 gives a higher
  peak than Tone=−10.
- **T14**: verifies that `noise_filter.lp` and `.bp` are exactly 0.0 after both
  `Reset()` and `NoteOn()` even after noise injection has driven those states to
  NaN (confirmed by T14a: lp = −nan before the fix was applied).
- **T15**: verifies that `Partls=5` (select ResA) and `Partls=6` (select ResB)
  do not overwrite `m_coupling_depth`, which would silently enable full coupling
  whenever the user entered editor mode.

### Post-activation bug review (code review pass)
Two additional bugs found and fixed after the initial Phase 13 commit:

**BUG: noise_filter SVF state (lp/bp/hp) not cleared on Reset() or NoteOn()**
- `FastSVF.set_coeffs()` only updates `f` and `q`; it never zeroes `lp`, `bp`,
  or `hp`. Reset() called set_coeffs() and nothing else. NoteOn() didn't touch
  the noise filter state at all.
- Result: on the very first note the states start at 0 (in-class init), but
  every subsequent note reuses stale (and eventually NaN) filter memory, causing
  an audible click/pop on retrigger.
- Fix: both Reset() and NoteOn() now explicitly zero `lp`, `bp`, and `hp` inside
  the `ENABLE_PHASE_6_FILTERS` guard.

**BUG: Partls=5/6 (editor-select mode) silently set coupling depth to 1.0**
- The processBlock coupling formula read `m_params[k_paramPartls]` directly and
  clamped it: `fmaxf(0, fminf(4, value)) / 4`. When the user set Partls=5 or 6
  to enter the resonator-edit menu, `m_params` stored 5/6 and the clamp produced
  4/4 = 1.0 — full coupling — regardless of what the user had previously set.
- Fix: added `float m_coupling_depth` (private member, updated only when
  Partls < 5) and changed processBlock to use `m_coupling_depth` directly.

---

## Phase 15: Dynamic Squelch, Pitch Bend, Tone Cache & fasterpowf Fix [COMPLETED]

### Dynamic Energy Squelch

Per-voice 1-pole RMS follower (α = 0.01, τ ≈ 2 ms at 48 kHz) tracks `voice_out`
**before** the master-envelope fade. When a releasing voice's RMS drops below
`0.0001f` (≈ −80 dB) the voice is immediately set `is_active = false`, freeing
the CPU slot without waiting for the full release envelope to expire.

A 1000-sample guard (~20 ms) prevents the squelch from firing during the
delay-line round-trip, where `voice_out` is genuinely zero even though the
waveguide has energy in transit. Data fields added to `VoiceState`:
- `float rms_env = 0.0f` — RMS envelope follower state
- `float base_delay_A/B = 0.0f` — pre-bend root delay lengths (see Pitch Bend)

### Pitch Bend (MIDI 0–16383, ±2 semitone range)

`PitchBend(uint16_t bend)` maps centre=8192 → 0 semitones, full-up=16383 →
+2 st, full-down=0 → −2 st. The delay multiplier is `2^(−semitones/12)` (note
negative: higher pitch = shorter delay).

`base_delay_A/B` are stored at NoteOn time (before applying any held bend) so
that `PitchBend()` can always re-derive from the root pitch without accumulating
error across successive bend messages.

**BUG: `fasterpowf(2.0f, 0.0f)` ≈ 0.9714, not 1.0**
`fasterpow2f(p)` = `(uint32_t)(8388608 * (p + 126.94269504))` interpreted as
float. The constant 126.94269504 is slightly below 127, so at p=0 the result
decodes to ≈0.9714 instead of 1.0. Every centre-bend would silently detune the
voice downward by ~50 cents. Fixed by using an exact special-case for bend=8192:
```cpp
if (bend == 8192) {
    m_pitch_bend_mult = 1.0f;
} else {
    float semitones = ((float)bend - 8192.0f) * (2.0f / 8192.0f);
    m_pitch_bend_mult = powf(2.0f, -semitones / 12.0f);
}
```
`powf` is used in place of `fasterpowf` throughout PitchBend because this
function is called from the MIDI thread (not the audio loop) and accuracy
matters more than speed here.

`unit.cc` stub wired: `unit_pitch_bend` now calls `s_synth.PitchBend(bend)`.

Buffer-overflow guard: all delay_length assignments in NoteOn and PitchBend are
clamped to `[2, DELAY_BUFFER_SIZE−2]` to prevent out-of-bounds reads on low
notes bent downward (e.g. MIDI note 0 − 2 st → delay ≈ 6585 > 4096).

### Tone Parameter — audio-thread race condition fix

Reading `m_params[k_paramTone]` in `processBlock` on every sample was a data
race with the UI thread calling `setParameter`. Fixed by adding `float tone`
to `SynthState` (updated only in `setParameter`) and hoisting
`const float tone_val = state.tone` once per block before the voice loop.

### Unit tests T16–T18

- **T16a**: low-`feedback_gain` voice is killed within a bounded frame count
  after GateOff (energy squelch fires when RMS < −80 dB).
- **T16b**: a sustaining voice (no GateOff) stays active for 200 frames.
- **T17a/b**: PitchBend up/down proportionally shortens/lengthens delay_length.
- **T17c**: PitchBend(8192) (centre) restores delay_length to within 0.1
  samples of the pre-bend value (exact, because bend == 8192 → mult = 1.0f).
- **T18a**: a note struck while bend is held at max-up inherits the shorter
  delay immediately.
- **T18b**: centre-bend after a bent NoteOn restores the root pitch delay
  (base_delay_A stored correctly in NoteOn).

All 45 tests pass.

### Remaining future work

* **True Stereo Master Filter:** `master_filter` is a single mono `FastSVF`.
  Add a second instance to `SynthState` for the right channel so future stereo
  effects (chorus, panning) pass through a properly independent filter.
* **Voice 0 skipped on first note:** `NoteOn` increments `next_voice_idx`
  before assigning, so the first ever note goes to voice 1. Minor cosmetic issue.

---

## Phase 16: Physical Model Review & DSP Correctness Pass [COMPLETED]

Full audit of the digital waveguide physical model against known physical
acoustics theory. Eight improvements applied; 45/45 tests pass.

### 1. Coupling symmetry fix (correctness)

The old coupling fed ResA with ResB's previous-sample output (1-sample delay)
but fed ResB with ResA's current-sample output (zero delay). This asymmetry made
the two resonators physically non-reciprocal — ResB always "heard" ResA one
sample ahead — creating a subtle formant artefact at high coupling depths.

Fix: added `float resA_out_prev` to `VoiceState`; both resonators now use the
previous sample's output of the other, making the coupling symmetric and
physically correct.

### 2. Membrane inharmonicity ratio (correctness)

`resB.delay_length = base_delay * 0.68f` gave a second mode ratio of 1/0.68 ≈
1.47. Real circular membranes have overtone ratios determined by zeros of the
Bessel function J_mn. The dominant second mode (1,1) has ratio ≈ 1.5926 →
1/1.5926 ≈ **0.628**.

Fix: changed multiplier from `0.68f` to `0.628f`.

### 3. Filter order in the waveguide loop (correctness)

Old order: LP → AP → write. The AP (dispersion) operated on an already
frequency-attenuated signal, reducing its audible inharmonicity at high
frequencies.

Physical order: AP models in-medium wave propagation (stiffness); LP models
boundary absorption (reflection loss). Correct order: **AP → LP → write**.

Fix: swapped the two filter stages in `process_waveguide()`. The feedback write
now uses `filtered_out` (LP output) rather than `ap_out` (AP output).

### 4. `while` → `if` for delay-line read pointer (performance)

`delay_length` is clamped to [2, 4094], so `read_idx ≥ −4094`. One addition
of `DELAY_BUFFER_SIZE` (4096) always puts it in range. The `while` loop body
can execute at most once; using `while` added an unnecessary backward branch in
the innermost hot loop.

### 5. Linear interpolation Horner form (performance)

`(a * (1-f)) + (b * f)` → `a + f * (b - a)`: one multiply + two adds instead
of two multiplies + one add. Saves one multiply per sample per active resonator.

### 6. Mallet LP gate — denormal prevention (performance)

The two cascaded mallet-shaping LP filters ran every sample for the full voice
lifetime even though their state decays to ≈ sub-denormal within ~250 samples.
Added a gate (`mallet_lp2 > 1e-6f`) that skips both LP updates and the
`* 15.0f` add once the mallet has fully settled. Eliminates wasted CPU and
prevents denormal stalls on non-FTZ hardware.

### 7. `rms_env` renamed `mag_env` (naming accuracy)

The envelope follower smooths `|x|` (mean absolute value), not `x²` (which
would be RMS). Renamed to `mag_env` throughout (`dsp_core.h`,
`synth_engine.h`). Behaviour is identical; only the name reflects what the
code actually computes.

### 8. Loop filter pitch compensation (tuning accuracy)

The 1-pole LP and allpass both add group delay at the fundamental frequency ω₀,
making the actual loop period longer than `delay_length` samples and the pitch
correspondingly flat. For the default preset (lowpass_coeff = 0.604), the error
was ≈ 35 cents flat at C4.

Using the DC-limit approximation (valid for all MIDI notes at 48 kHz since
ω₀ ≪ 1 rad/sample): τ_LP ≈ (1 + pole)/(1 − pole) and τ_AP ≈ (1 + c)/(1 − c).

Fix: after computing the nominal delay from the lookup table, NoteOn subtracts
(τ_LP + τ_AP) from each resonator's `delay_length` before storing it as the
base for PitchBend. The corrected delay for C4 changes from 189.8 to 186.1
samples — 3.7 samples shorter — bringing pitch error from −35 cents to < 1 cent.

### Pre-existing compile errors also fixed

Two `static constexpr` declarations were missing their `float` type keyword
(introduced in a user commit). The `ProgramIndex` enum had enum values with
spaces in their names and no commas — both invalid C++. Fixed: added `float`,
replaced spaces with camelCase, added commas, renamed the count marker from
`k_ProgramIndex` to `k_NumPrograms` to avoid shadowing confusion.

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

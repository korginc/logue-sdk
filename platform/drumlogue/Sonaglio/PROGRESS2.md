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

6-operator FM synthesis with 18 routing algorithms (DX-style) on ESP32-S3.
Each instrument is a `FmDrumPatch` (6 operators + algorithm select); patches defined in
`FmPatch.cpp` map GM drum notes to complete operator configurations.

Architecture highlights:
- `FmOperator` class: normalized [0,1) phase, `fm_level = 0.1 × 161^v − 0.1` (DX7-style
  exponential depth), self-feedback via `feedback_ = 1/2^(7-fb)`, 5 waveforms
- `MOD_RANGE = 16.0f`: modulator output scaled into phase, matching DX7 FM depth convention
- Algo 11 (3 independent carrier+modulator pairs) used for cymbal/metallic tones
- Patch-based design: all 6 operators configured per-instrument

**License**: **MIT License** (Copyright 2025 copych) — confirmed from `LICENSE` file at repo root.
Code may be incorporated with attribution. _(Earlier assessment of "All Rights Reserved" was wrong.)_

**Ported to Sonaglio**: `fm_operator.h` is a clean C-struct port of `FmOperator` class,
stripped of ESP32/Arduino/STL dependencies, using Sonaglio's `float_math.h` helpers.
`metal_engine.h` was rewritten using algo11 from this codebase. Attribution is preserved in
the `fm_operator.h` file header.

### Summary: can code be reused?

- **copych**: MIT License confirmed — code incorporated with attribution ✓
- **ctag-fh-kiel/md-drum-synth**: No explicit OSI license — treated as All Rights Reserved;
  techniques may be independently implemented from DSP first principles only.

Improvements implemented from copych (MIT, with attribution):
- `fm_operator.h`: full port of copych `FmOperator` (normalized phase, DX7 fm_level, feedback,
  5 waveforms, `FMO_MOD_RANGE = 16.0f`)
- `metal_engine.h`: algo11 (3×2-op parallel pairs) eliminating the serial-cascade hash problem

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

### Metal — rewritten (algo11 parallel pairs)

Complete rewrite of `metal_engine.h` using copych algo11 (3 independent carrier+modulator
pairs) via the new `fm_operator.h`:
- Serial cascade eliminated — each carrier ring is clean with its own modulator only
- DX7-style fm_level and feedback via `fm_operator.h` (MIT-attributed copych port)
- Cymbal (DX7 inharmonic ratios) and Gong (tam-tam ratios) characters via `char_select`
- Body parameter spreads inharmonic ratios (ratio_scale 0.82..1.50)
- sqrt(env) carrier envelope for long metallic ring tail
- env8 + linear modulator envelope for fast transient burst + sustained ring FM
- Strike noise: HP + BPF blend gated by env2 (dies with transient)
- Output: 1/√3 normalised sum of 3 carriers

Remaining limitation:
- Still one shared env decay — each partial's modulator depth cannot decay independently
- Carriers at fixed ratios; no pitch glide between pairs

### Kick — not modified (good)

Already working well per user assessment. Linear amp envelope, env⁸ pitch/FM transients.

### Perc — not modified (good)

Already working well. Linear amp, env⁸ strike, env⁴ body FM.

### Hat — not modified

6-detuned square oscillators with HPF noise. Simplest engine. No FM computation.
Not a priority for redesign.

---

## GM drum note coverage analysis

The copych project targets all 47 GM drum notes (MIDI ch10, notes 35–81). Sonaglio
currently has 5 engines and 26 named-but-unnamed hardware evaluation presets.

### Engine-to-GM mapping

| GM notes | Sound | Coverage with existing engines |
|----------|-------|-------------------------------|
| 35, 36 | Bass Drum | INST_KICK ✓ |
| 37 | Side Stick | INST_SNARE preset (low body, short decay) — approximation |
| 38, 40 | Snare | INST_SNARE ✓ |
| 39 | Hand Clap | **No engine** — multi-strike sequencer required (see below) |
| 41, 43, 45, 47, 48, 50 | Toms (6 pitches) | INST_TOM at different note pitches — preset per pitch |
| 42, 44 | Closed / Pedal Hi-Hat | INST_HAT, short decay preset |
| 46 | Open Hi-Hat | INST_HAT, long decay preset |
| 49, 55, 57 | Crash / Splash Cymbal | INST_METAL Cymbal char, medium decay |
| 51, 53, 59 | Ride Cymbal / Ride Bell | INST_METAL Cymbal char, long / very-short decay |
| 52 | Chinese Cymbal | INST_METAL Gong char |
| 54, 69, 70 | Tambourine / Cabasa / Maracas | INST_HAT noise-dominant preset (short, HPF-heavy) |
| 56 | Cowbell | INST_METAL — cymbal carrier ratios ≈ 1.0/1.483 match 540/800 Hz interval; short decay |
| 58 | Vibraslap | INST_SNARE or INST_TOM approximation |
| 60, 61 | Hi/Lo Bongo | INST_TOM high-frequency presets |
| 62, 63, 64 | Conga (mute/open/low) | INST_TOM mid-frequency presets |
| 65, 66 | Hi/Lo Timbale | INST_TOM mid-frequency, brighter attack |
| 67, 68 | Agogo | INST_METAL short decay, high note |
| 71, 72 | Whistle | INST_TOM high freq, low body (pure tone) |
| 73, 74 | Guiro | INST_HAT with long envelope and low noise mix — approximate |
| 75, 76, 77 | Claves / Woodblock | INST_TOM very short decay, specific freq preset |
| 78, 79 | Cuica | No close equivalent; INST_TOM modulated approximation only |
| 80, 81 | Triangle (mute/open) | INST_METAL long decay (open) / gated (mute) |

**Result: ~38–40 of 47 GM notes are coverable with preset tuning of the 5 existing engines.**
The remaining 7–9 are approximations of varying quality; only one requires a new engine.

### One new engine needed: Clap

The **Hand Clap** (GM 39) and the ctag `FmClapModel` both require a multi-strike envelope:
3 discrete hits spaced ~12 ms apart, each decaying before the next. This is a timing /
envelope-sequencing feature — not a different synthesis algorithm. It cannot be emulated
with a single continuous envelope over any parameter combination.

**Implementation plan (independent of copych/ctag, implemented from first principles):**
- Add `clap_engine_t` using the existing `fm_op_t` from `fm_operator.h` (MIT-attributed)
- Multi-strike: 3 pre-accumulated envelope peaks at t=0, t+12ms, t+24ms (simple additive
  approach: sum of 3 short exponential decays offset by sample count)
- FM core: 2-op model same as Snare/Kick; noise HPF layer same as Hat
- New `INST_CLAP` instrument selector constant in constants.h
- One new case in `fm_perc_synth_process.h` dispatch

This is the only addition that requires new code beyond presets.

---

## Remaining known issues

1. **No per-component independent decay** — shared envelope affects all components
   simultaneously. True independent decay (like ctag snare) would require per-engine
   per-component envelope state — larger struct, more CPU, but more natural sound.

2. **PARAM_DRIVE not wired** — `fm_make_drive_gain` exists in engine_mapping.h but has
   no UI parameter. Currently `PARAM_NOISE_CHAR` fills the "drive" role (noise-to-FM
   blend), which is a different character. A true pre-clip amplitude multiplier could be
   wired to an unused or repurposed slot.

3. **Hat engine is minimal** — 6 detuned square oscs + HPF noise; no FM. At high
   frequency can sound harsh. A 4-pair inharmonic FM model (DX cymbal literature)
   would add ring and partial definition.

4. ~~**Metal at high Body → noise**~~ **resolved** — algo11 parallel pairs fix this.

5. **No velocity scaling** — instrument ignores MIDI velocity. Simple 0..1 gain on
   velocity would improve playability significantly.

6. **Presets are HW evaluation stubs** — all 26 presets are named test1..test26 and
   tuned for parameter range verification, not musical use. A full musical preset bank
   is needed before the unit is usable as an instrument.

---

## Files changed in this session

| File | Change |
|------|--------|
| `snare_engine.h` | Linear amp_env, env² wire buzz, restructured output |
| `metal_engine.h` | **Complete rewrite** — algo11 3×2-op parallel pairs via fm_operator.h |
| `fm_operator.h` | **New** — C-struct port of copych FmOperator (MIT), normalized phase, DX7 fm_level/feedback |
| `hat_engine.h` | Removed duplicate global `sample_rate_`/`inverse_sample_rate_`; use `INV_SAMPLE_RATE` |
| `fm_perc_synth_process.h` | Fixed `metal.brightness` field ref (field removed in rewrite) |
| `engine_mapping.h` | Added `fm_make_drive_gain` utility |
| `test_sonaglio.cpp` | Updated `test_metal_parameter_bounds` for new `fm_op_t` struct; 16/16 passing |
| `README.md` | Complete rewrite — accurate architecture, parameter tables, engine details |
| `PARAMETER_MODEL.md` | Removed — content merged into README.md |

---

## TODO

### Before next HW test
- HW test snare: verify wire buzz is audible at moderate Attack settings
- HW test metal: verify clean ring with algo11; confirm cymbal vs. gong distinction
- HW test HitShape/BodyTilt as distinct controls

### Preset bank expansion (no new engines needed)
Replace the 26 test stubs with a named musical preset bank covering the GM drum set.
The mapping table above is the guide. Target ~40 presets:
- 2 × Kick (standard, punchy)
- 3 × Snare (standard, tight, rimshot-approximate)
- 6 × Tom (floor low/high, mid low/high, hi low/high)
- 3 × Hi-hat (closed, pedal, open)
- 5 × Cymbal (crash, ride, ride bell, splash, chinese)
- Cowbell, Triangle (open/mute), Claves, Woodblock (×2), Agogo (×2)
- Bongo ×2, Conga ×3, Timbale ×2
- Noise perc: Tambourine, Cabasa, Maracas
- Misc approximations: Vibraslap, Guiro, Cuica, Whistle

### New engine: Clap
Add `clap_engine.h` with multi-strike envelope (3 peaks at 0 / 12 ms / 24 ms).
FM core from `fm_operator.h`; noise HPF layer. New `INST_CLAP` in constants.h.
Add dispatch case in `fm_perc_synth_process.h`.

### Medium term engine improvements
- Per-component decay for snare (separate noise decay from shell decay)
- Wire velocity to output gain
- Hat engine: replace square oscs with 4-pair inharmonic FM (DX cymbal literature)

### Long term
- Investigate Hat engine FM upgrade (copyright-free, from DX7 / Roads literature)
- Consider modal/resonator engine for metalophone-type sounds (bars, bells)

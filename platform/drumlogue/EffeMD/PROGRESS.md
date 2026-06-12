# EffeMD — Implementation Progress

Status snapshot for the next agent/developer. Read `README.md` first for the
architecture; this file tracks what was done, what is verified, and what is
still open.

**Status: builds and passes all unit tests.**
- `make CROSS_COMPILE=arm-linux-gnueabihf-` → `build/effemd.drmlgunit`
  (ARMv7 NEON shared object, all `unit_*` entry points exported). Verified
  with binutils; **not yet tested on drumlogue hardware.**
- `./run_effemd_tests.sh` → all tests pass natively (x86) and cross-compiled
  for ARMv7+NEON under `qemu-arm` (which exercises the NEON cymbal path).

---

## Steps done

### 1. Code review of the initial import (all findings fixed)

The first import compiled nothing; the integration layer was a sketch.
Defects found and fixed:

**Build system**
- `config.mk`: missing line-continuation backslash after `TRXClaves.cpp`
  (orphaned the last two sources); the SDK Makefile only builds `.cc`
  C++ sources, so all `*.cpp` models were never compiled → renamed to `.cc`.
- `Makefile` (local copy): `--param max-inline-insns-single=9999999999`
  overflows newer GCC's int parameter → reduced to 99999999.

**Broken/missing code in the integration layer**
- `fm_perc_synth_process.h`: `synth->models[synth->models]`,
  `synth->models.Init()` (non-compiling placeholders);
  `fm_perc_synth_note_on/off` and `fm_perc_synth_has_pending` were called
  from `synth.h` but did not exist; includes of `envelope_rom.h` /
  `fm_presets.h` which were never imported from Sonaglio → full rewrite
  (see §2).
- `synth.h`: idle gate read `synth_.envelope.stage` (no such member here);
  `getParameterValue()` returned the model's DSP float instead of the raw
  UI value the runtime expects; `getParameterStrValue()` returned a pointer
  to a stack buffer (dangling) → fixed with a `mutable` member buffer.
- `DrumModel.h`: missing closing quote in `#include "constants.h`;
  pure-virtual `RenderControls()`/`getPresetIndex()` that no model
  implements (would have made every model abstract) → removed.
- `FmTomModel.h`: `float A_f, d_f, start_phase / 2.0f;` (syntax error).
- `FmClapModel.h`: `bool active` missing semicolon.
- `FmCymbalModel.h`: constructor init-order warning (x_prev/y_prev).
- 6 models called `fasterfullsinf()` — the function in `float_math.h` is
  `fastersinfullf()` (typo, nothing compiled).
- `TRXSnareDrum.cc` included non-existent `fm_presets.h`; duplicate `break`.
- `TRXHiHat` used `std::default_random_engine`/`std::random_device`
  (no `<random>` include, not RT-safe) and `static float phase[6]` inside
  `generateMetallicNoise()` (state shared across instances, not reset).
- `header.c`: `Count` parameter had default 50 outside its 1..6 range.

**Real DSP bugs caught by the new unit tests**
- `float_math.h fastertanhf()`: the rational approximation was only valid
  for x ≥ 0; for negative inputs it diverged (≈ −1.6 at x = −2), so every
  "soft clip" stage **amplified** negative half-waves (TRXBassDrum peaked at
  9.7 with factory preset 0). Replaced with an odd-symmetric Padé 3/2
  approximant clamped to [−1, 1].
  ⚠ Sonaglio carries its own copy of `float_math.h` with the same latent bug.
- `e_expff(−x)` rounds to exactly 1.0 for x ≲ 6e−5 (float granularity of
  `1.0 + x/1024`), so per-sample `env *= e_expff(-1/(sr·decay))` froze for
  decay ≳ 0.3 s (TRX models rang forever / peaked > 300 after param edits).
  Fixed by computing decay multipliers once per Trigger with libm `expf()`
  (control-rate, accurate, and cheaper per sample). Applied to TRXBassDrum,
  TRXSnareDrum, TRXClaves, TRXHiHat (incl. hoisting the per-sample filter
  coefficient `e_expff` calls).
- `FmRimshotModel`: `K_Mix` mapped to 0..10 instead of 0..1 (10× gain).
- `float_math.h` had NEON helpers (`fast_div_neon`, `neon_sqrtq_f32`)
  outside the `__arm__` guard → guarded, file now compiles on x86 hosts.

### 2. Integration with the drumlogue/Sonaglio ecology (rewritten)

- `fm_perc_synth_process.h`: new scalar integration layer —
  instrument selector + model registry, `note_on` (velocity gain 0.3+0.7·v/127,
  Euclidean offset, pitch ratio, trigger), `note_off` (no-op: one-shots),
  parameter routing (`K_Instrument`/`K_Euclidean_Tuning` global, everything
  else to the active model), per-sample process with master gain 1.3 +
  soft clip `x/(1+|x|)` (Sonaglio's output curve), idle detection
  (50 ms below −80 dBFS) replacing Sonaglio's envelope-stage gate.
- `synth.h`: rewritten control layer; NEON stereo block write (`vst2q_f32`)
  with scalar tail and `memset` idle path; correct raw-value
  `getParameterValue`; safe string buffer.
- Removed unused/broken Sonaglio leftovers from EffeMD:
  `engine_mapping.h`, `fm_voices.h`, `midi_handler.h`, `lfo_enhanced.h`,
  `lfo_smoothing.h`, `smoothing.h`, `prng.h` (copies live on in
  `../Sonaglio/` if needed again). `fm_engine_to_euclid_lane()` moved into
  `constants.h` (scalar) with corrected lanes: TRXBassDrum→Kick lane,
  TRXSnareDrum→Snare lane, FmRimshot→Perc lane.

### 3. FM operator swap (Mutable Instruments → Sonaglio)

- `FmKickModel`, `FmSnareModel` no longer reference `plaits::fm::Operator`
  (which was never imported); they now use `fm_op_t` from `fm_operator.h`.
- Added `fmo_render_raw()` to `fm_operator.h`: advances phase (with
  modulation + DX7 feedback) and returns the raw waveform, so the models can
  keep the original EFM convention of applying `I · mod_env` externally as a
  normalized-phase deviation (divide by `FMO_MOD_RANGE` to compensate the
  operator's internal ×16 scaling).
- Kick feedback: UI mapping changed from `value·0.16` (0..16) to
  `value·0.07` (0..7) to match the operator's DX7 feedback domain.
- FM code verified: operator math reviewed against copych's original
  (phase convention, feedback `2^(fb−7)`, exp depth curve) and covered by
  unit tests (audibility, boundedness, decay, determinism, pitch response).

### 4. Processing parity (md-drum-synth `main.cpp` audioCallback vs Sonaglio `Render()`)

- md-drum-synth: trigger-on-demand + `models[selected]->Process()` per
  sample, mono duplicated to stereo → preserved exactly, minus the GUI/FFT.
- From Sonaglio's `Render()`: velocity latching, master gain + soft clip,
  idle/early-out, NoteOn(vel 0)→NoteOff, GateOn at root note 36.
- Not carried over (by design, documented): combo/blend routing, pending
  trigger queue (Gap/Scatter), shared envelope ROM, dual LFO system.

### 5. Euclidean tuning

- `EuclTun` parameter (page 6) with Sonaglio's 9 modes / `EUCLID_OFFSETS`
  table; offset selected per instrument class lane and added to the MIDI
  note, then applied as a pitch ratio to all model oscillators via
  `DrumModel::setPitchRatio()`. MIDI note transposition (root 36) works for
  all 11 models as a side effect.

### 6. Unit tests

- `test_effemd.cc` + `run_effemd_tests.sh` (pattern from Sonaglio's runner):
  102 checks — per-model render sanity / determinism / pitch / parameters,
  Euclidean table, instrument switching, velocity, presets, idle behavior.
  Host-native and ARM-qemu both green.

### 7. ARM NEON v7 refactoring

- `FmCymbalModel::Process()`: 4 mod/carrier pairs vectorized
  (`neon_sin`, vectorized phase wrap, `vpadd` horizontal sum), scalar
  fallback kept for host builds.
- `synth.h Render()`: 4-sample chunks, stereo interleave via `vst2q_f32`,
  idle `memset` path.
- Everything ARMv7-safe (`vpadd` instead of `vaddvq`, no `vsqrtq_f32`).

---

## TODO / roadmap

- [ ] **Hardware test on a real drumlogue** (load `effemd.drmlgunit`, check
      CPU headroom, UI strings, preset switching, parameter feel). The
      `O:%.3f`/`x` parameter display refresh quirk noted in `synth.h` needs
      on-device verification.
- [ ] **Per-model UI ranges**: all 24 params share generic 0..100 ranges;
      consider per-instrument `k_unit_param_type_*` tuning and better names.
- [ ] **More presets**: only 2 (`MD Init`, `MD Alt`, from md-drum-synth's
      defaults); `num_presets` in `header.c` must match `NUM_OF_PRESETS`.
- [ ] **K_Reserved parameter** (page 6 slot 3): candidate uses — LFO from
      Sonaglio's `lfo_enhanced.h`, drive amount, or per-hit scatter.
- [ ] **NEON round 2** (measure first on hardware): TRXHiHat 6-square stack,
      FmCowbell/FmRimshot dual carriers, block-level model processing
      (Process(float* buf, n) interface) to amortize virtual dispatch.
- [ ] **neon_sin accuracy**: cymbal NEON path uses a 2-term polynomial sine
      (~1e-3 error) vs `fastersinfullf` scalar; sounds fine for inharmonic
      content but A/B on hardware would be good.
- [ ] **Backport fixes to Sonaglio**: `fastertanhf` negative-input bug and
      the unguarded NEON block live in Sonaglio's `float_math.h` too.
- [ ] Consider note-off choke for `FmCymbal` with sustain > 0 (currently
      rings until the next trigger, as in md-drum-synth).

## Instructions for the next agent

1. Build: `make CROSS_COMPILE=arm-linux-gnueabihf-` in this directory (or
   the SDK docker builder). Artifact: `build/effemd.drmlgunit`.
2. Always run `./run_effemd_tests.sh` after touching any model or the
   integration layer; add a test alongside any bug fix (the 6 DSP bugs above
   were all caught by tests, not by reading code).
3. Keep `fm_perc_synth_process.h` and the models free of unguarded NEON —
   host-testability is the project's main safety net until hardware testing
   is routine. Device-only code belongs in `synth.h`/`unit.cc` or behind
   `#if defined(__ARM_NEON)`.
4. The 24-parameter contract (`header.c` ⇄ `fm_param_index_t` in
   `constants.h`) is fixed; if you change one side, change the other.
5. When adding a model: derive from `DrumModel`, add to `engine_id_t`,
   `instruments_strings`, the registry in `fm_perc_synth_init()`, a lane in
   `fm_engine_to_euclid_lane()`, and the model list in `test_effemd.cc`.

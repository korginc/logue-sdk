# EffeMD — Machinedrum-style FM Percussion Synth for Korg Drumlogue

EffeMD is a **percussive FM synth** for the Korg drumlogue, built around the
eleven EFM (Elektron FM) drum models from
[ctag-fh-kiel/md-drum-synth](https://github.com/ctag-fh-kiel/md-drum-synth),
integrated into the drumlogue runtime using the infrastructure developed for
the [Sonaglio](../Sonaglio) project, which it supersedes.

Where Sonaglio used 4 perceptually-controlled engines with combo routing,
EffeMD exposes the original Machinedrum-inspired models directly: one
instrument selector, the model's own EFM parameters on the UI pages, plus
velocity, MIDI-note transposition and Sonaglio's Euclidean tuning system.

---

## History

1. **md-drum-synth** (CTAG, FH Kiel) — a cross-platform desktop FM drum
   synthesizer inspired by the Elektron Machinedrum / EFM engine, developed
   as an agentic-coding case study (see [Citation](#citation)). It provides
   the 11 drum models (7 FM models + 4 TRX-style models) with ImGui controls
   and a Mutable Instruments (`plaits::fm`) operator core.
2. **Sonaglio** — an ARM NEON FM percussion unit for drumlogue with a scalar
   FM operator (adapted from copych's ESP32 FM drum synth), Euclidean tuning,
   velocity handling and a host-testable DSP layer.
3. **EffeMD** — this project: the md-drum-synth models ported to the
   drumlogue, with the Mutable Instruments operator replaced by the Sonaglio
   FM operator (`fm_operator.h`), desktop-only code removed, real-time-safe
   noise sources, and Sonaglio-style integration (velocity, Euclidean tuning,
   idle gating, NEON in the render path).

---

## Architecture overview

### Layers

```
unit.cc            drumlogue SDK callbacks (entry points)
  └── synth.h      control layer: parameters, presets, MIDI, block render
        └── fm_perc_synth_process.h
                   integration layer: instrument selector, velocity,
                   Euclidean tuning, soft clip, idle detection (scalar)
              └── DrumModel.h + 11 models (scalar, host-testable)
                    └── fm_operator.h  (Sonaglio scalar FM operator)
                    └── float_math.h   (fast scalar math approximations)
                    └── sine_neon.h    (NEON sine, used by FmCymbalModel)
```

Design rules (inherited from Sonaglio):
- no heap allocation, all state static in `fm_perc_synth_t`
- deterministic, real-time-safe processing (no `rand()`, no `std::random_*`)
- the whole DSP layer below `synth.h` is free of NEON intrinsics (or guards
  them behind `__ARM_NEON`) so it compiles and unit-tests natively on a host

### Instruments (11 total)

| # | Model | Synthesis method |
|---|-------|------------------|
| 0 | **FmKick** | 2-op FM, DX7-style modulator feedback, pitch sweep, ratio mode |
| 1 | **FmSnare** | 2-op FM + white noise + one-pole HPF |
| 2 | **FmTom** | 2-op FM with fixed modulator feedback + pitch sweep |
| 3 | **FmClap** | 2-op FM retriggered N times (clap_count/interval) + HPF |
| 4 | **FmRimshot** | 1 modulator driving 2 carriers (body + rim), mix + HPF |
| 5 | **FmCowbell** | 1 modulator driving 2 carriers at the classic 1:1.48 ratio |
| 6 | **FmCymbal** | 4 modulator/carrier pairs at inharmonic ratios (NEON-vectorized) |
| 7 | **TRXBassDrum** | Sine + pitch ramp + harmonics (tanh) + noise burst + clip |
| 8 | **TRXSnareDrum** | 2 detuned sines + snap noise + filtered noise + clip |
| 9 | **TRXClaves** | 2 detuned sines, exponential decay + clip |
| 10 | **TRXHiHat** | 6 detuned squares + white noise, LPF/HPF, gap crossfade |

Every model derives from `DrumModel` (Init / Trigger / Process /
loadPreset / setParameter / getParameter) and owns its DSP parameters.

### FM operator

`fm_operator.h` is the Sonaglio scalar operator (adapted from
[copych/ESP32-S3_FM_Drum_Synth](https://github.com/copych/ESP32-S3_FM_Drum_Synth),
MIT license): normalized [0,1) phase, DX7-style feedback
(`1 / 2^(7-fb)`), exponential FM depth curve, fast sine from `float_math.h`.

The md-drum-synth models originally rendered through the Mutable Instruments
`plaits::fm::Operator`. FmKick and FmSnare now use `fm_op_t` instead,
preserving the original EFM math: the modulator output is applied to the
carrier as a *normalized-phase* deviation `I * mod_env * mod_out`
(`fmo_render_raw()` + `1/FMO_MOD_RANGE` scaling). The remaining models use
direct phase-accumulator FM with the shared `WrapPhase`/`ExpDecay` helpers,
exactly as in md-drum-synth.

### Parameter contract

24 fixed parameters (6 pages × 4), see `header.c` and `fm_param_index_t` in
`constants.h`. Page 1 holds the **Instrument** selector; the last page holds
**EuclTun**. All other parameters are routed to the *selected* model, which
maps the 0..100 UI value onto its own DSP range and ignores parameters it
does not implement (those display as `x`, implemented ones as `O:<value>`).

### Note handling, velocity, Euclidean tuning

On note-on the integration layer:
1. latches velocity into a per-trigger gain (`0.3 + 0.7·vel/127`, Sonaglio convention),
2. looks up the instrument's engine class lane (Kick/Snare/Metal/Perc) and adds
   the `EUCLID_OFFSETS[mode][lane]` semitone offset to the MIDI note —
   exactly as in Sonaglio's `sonaglio_euclid_offset_for_engine()`,
3. converts `(note − 36 + offset)` semitones into a pitch ratio applied to all
   of the model's oscillator frequencies (`DrumModel::setPitchRatio`),
4. triggers the model.

Euclidean modes (`EuclTun`): Off, Clstr E(4,4), Minor E(4,6), Diatn E(4,7),
Whole E(4,8), Penta E(4,10), Dim7 E(4,12), Aug8 E(4,16), Trit E(4,24).
With a Euclidean mode active, assigning different instrument classes to
sequencer steps spreads their pitches across the selected interval set.

### Output stage and idle gating

Per sample: `model → velocity gain → master gain (1.3) → soft clip
x/(1+|x|)`. After 50 ms below −80 dBFS the synth flags itself idle and
`Render()` zero-fills whole blocks without running any DSP
(the equivalent of Sonaglio's envelope-stage NEON idle gate).

### ARM NEON (v7)

- `synth.h Render()`: mono samples are generated 4 at a time and interleaved
  to stereo with a single `vst2q_f32`; idle blocks short-circuit to `memset`.
- `FmCymbalModel`: the 4 modulator/carrier pairs run in parallel NEON lanes
  using `neon_sin()` (polynomial sine from `sine_neon.h`) with vectorized
  phase wrapping — the heaviest model reduced from 8 scalar sines/sample to
  2 NEON evaluations. A scalar fallback remains for host builds.
- All NEON usage is ARMv7-safe (no `vaddvq`/`vsqrtq` AArch64-isms).

---

## Building

Standard logue-sdk drumlogue build (docker):

```sh
cd platform/drumlogue/EffeMD
make            # via the SDK docker builder, or:
make CROSS_COMPILE=arm-linux-gnueabihf-   # with a local ARM toolchain
```

Produces `build/effemd.drmlgunit` — copy to the drumlogue's `Units/Synths`
directory via USB mass storage.

## Testing

The DSP layer is host-testable:

```sh
./run_effemd_tests.sh                       # native build + run
# Optional: exercise the NEON paths under qemu
CXX=arm-linux-gnueabihf-g++ RUNNER=qemu-arm \
  EXTRA_CXXFLAGS="-march=armv7-a -mfpu=neon -mfloat-abi=hard -static" \
  ./run_effemd_tests.sh
```

Tests cover: render sanity (finite/audible/bounded/decaying) for all 11
models, retrigger determinism, pitch-ratio response, parameter set/get,
Euclidean table properties, instrument switching, velocity scaling, preset
loading and idle detection. See `PROGRESS.md` for status and roadmap.

---

## References

- EFM (Elektron FM) synthesis: [Elektronauts — MD voices diagram & EFM paper](https://www.elektronauts.com/t/md-voices-diagram/173460/16)
- [ctag-fh-kiel/md-drum-synth](https://github.com/ctag-fh-kiel/md-drum-synth) — original models and EFM implementation
- [copych/ESP32-S3_FM_Drum_Synth](https://github.com/copych/ESP32-S3_FM_Drum_Synth) — scalar FM operator (MIT), via Sonaglio
- [Sonaglio](../Sonaglio) — drumlogue integration infrastructure, Euclidean tuning design
- [Korg logue-sdk](https://github.com/korginc/logue-sdk) — drumlogue unit SDK

## Citation

EffeMD's drum models are ported from md-drum-synth. If you use this software
or reference its development, please cite the original work:

```bibtex
@inproceedings{Manzke2025AgenticCoding,
  author    = {Robert Manzke},
  title     = {Agentic Coding with AI: A case study to develop a program for testing synthetic drum models},
  booktitle = {Proceedings of the AmiEs 2025 International Symposium on Ambient Intelligence and Embedded Systems},
  year      = {2025},
  address   = {Hamburg, Germany},
  url       = {https://international-symposium.org/amies_2025/proceedings_2025/Manzke_AmiEs_2025_Paper.pdf},
  note      = {PDF available online}
}
```

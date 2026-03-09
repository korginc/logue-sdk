# README.md - Updated with Resonant Mode & Voice Allocation

## Project Overview

A **4-voice FM percussion synthesizer** for KORG drumlogue with **5 synthesis engines** and **intelligent voice allocation**. Now featuring a resonant synthesis mode based on Lazzarini's summation formulae (2017, Section 4.10.3), allowing one voice to be dynamically assigned to resonant synthesis while maintaining the original 4-voice structure.

## Key Features

- **5 Synthesis Engines**: Kick, Snare, Metal, Perc, and Resonant
- **Voice Allocation**: Single-parameter control for which voice uses resonant synthesis
- **Resonant Modes**: Low-pass, Band-pass, High-pass, Notch, Peak
- **LFO Targets**: Expanded to 8 targets including resonant parameters
- **Envelope ROM**: 128 ADR curves optimized for percussion
- **NEON Optimization**: Fully vectorized for ARMv7

## Parameter Page Layout (v2.0)

```
Page 1: Voice Probabilities (NEW)
┌─────────────┬─────────────┬─────────────┬─────────────┐
│  Voice1Prob │  Voice2Prob │  Voice3Prob │  Voice4Prob │
│   (0-100%)  │   (0-100%)  │   (0-100%)  │   (0-100%)  │
└─────────────┴─────────────┴─────────────┴─────────────┘

Page 2: Kick + Snare Parameters
┌─────────────┬─────────────┬─────────────┬─────────────┐
│  KSweep     │  KDecay     │  SNoise     │  SBody      │
│   (0-100%)  │   (0-100%)  │   (0-100%)  │   (0-100%)  │
└─────────────┴─────────────┴─────────────┴─────────────┘

Page 3: Metal + Perc Parameters
┌─────────────┬─────────────┬─────────────┬─────────────┐
│  MInharm    │  MBrght     │  PRatio     │  PVar       │
│   (0-100%)  │   (0-100%)  │   (0-100%)  │   (0-100%)  │
└─────────────┴─────────────┴─────────────┴─────────────┘

Page 4: LFO1
┌─────────────┬─────────────┬─────────────┬─────────────┐
│  L1Shape    │  L1Rate     │  L1Dest     │  L1Depth    │
│   (0-8)     │   (0-100%)  │   (0-7)     │  (-100-100) │
└─────────────┴─────────────┴─────────────┴─────────────┘

Page 5: LFO2
┌─────────────┬─────────────┬─────────────┬─────────────┐
│  L2Shape    │  L2Rate     │  L2Dest     │  L2Depth    │
│   (0-8)     │   (0-100%)  │   (0-7)     │  (-100-100) │
└─────────────┴─────────────┴─────────────┴─────────────┘

Page 6: Envelope + Voice + Resonant
┌─────────────┬─────────────┬─────────────┬─────────────┐
│  EnvShape   │  VoiceAlloc │  ResMode    │  ResMorph   │
│   (0-127)   │   (0-11)    │   (0-4)     │   (0-100%)  │
└─────────────┴─────────────┴─────────────┴─────────────┘
```

## Voice Allocation (Param 21)

Single parameter controls which instrument gets resonant treatment:

| Value | Display | Voice 0 | Voice 1 | Voice 2 | Voice 3 |
|-------|---------|---------|---------|---------|---------|
| 0 | "K-S-M-P" | Kick | Snare | Metal | Perc |
| 1 | "K-S-M-R" | Kick | Snare | Metal | **Resonant** |
| 2 | "K-S-R-P" | Kick | Snare | **Resonant** | Perc |
| 3 | "K-R-M-P" | Kick | **Resonant** | Metal | Perc |
| 4 | "R-S-M-P" | **Resonant** | Snare | Metal | Perc |

**Design Philosophy**: Only one voice can be resonant at a time, keeping the UI intuitive while adding significant sonic variety.

## Resonant Synthesis Engine

Based on Lazzarini's summation formulae (2017, Section 4.10.3), the resonant engine combines single-sided and double-sided responses:

### Mathematical Foundation

```
Single-sided (low-pass):  s(t) = sin(ωt) / (1 - 2a cos(θ) + a²)
Double-sided (band-pass): s(t) = (1 - a²) sin(ωt) / (1 - 2a cos(θ) + a²)

Where:
- ω = 2πf₀ (fundamental frequency)
- θ = 2πf_c (resonance center frequency)
- a controls resonance (0 ≤ a < 1)
```

### Resonant Modes (Param 22)

| Value | Mode | Description |
|-------|------|-------------|
| 0 | LowPass | Single-sided response, low-pass character |
| 1 | BandPass | Mixed response, original resonant synthesis |
| 2 | HighPass | Derived high-pass response |
| 3 | Notch | Notch filter character |
| 4 | Peak | Boosted band-pass with resonance |

### Resonant Frequency (Param 23)

Maps 0-100% to **50 Hz - 8000 Hz** center frequency for the resonant peak.

## LFO Targets (Expanded to 0-7)

| Value | Target | Description |
|-------|--------|-------------|
| 0 | None | LFO disabled |
| 1 | Pitch | Modulate oscillator frequency |
| 2 | Modulation Index | Modulate FM amount |
| 3 | Envelope | Modulate envelope shape/level |
| 4 | LFO2 Phase | LFO1 modulates LFO2's phase |
| 5 | LFO1 Phase | LFO2 modulates LFO1's phase |
| **6** | **Res Freq** | **NEW: Modulate resonant center frequency** |
| **7** | **Resonance** | **NEW: Modulate resonance amount** |

## Architecture (Updated with Resonant Mode)

```
┌─────────────────────────────────────────────────────────────┐
│                    MIDI Trigger Input                       │
└─────────────────────────┬───────────────────────────────────┘
                          ▼
┌─────────────────────────────────────────────────────────────┐
│              Probability Gate (4 parallel PRNGs)            │
│         Page 1: Kick/Snare/Metal/Perc probabilities         │
└─────────────────────────┬───────────────────────────────────┘
                          ▼
┌─────────────────────────────────────────────────────────────┐
│               NEON Parallel Voice Processing                │
│         (4 voices with engine masks for efficiency)         │
└─────────────────────────┬───────────────────────────────────┘
                          ▼
┌─────────────────────────────────────────────────────────────────────┐
│                        ENGINE ARRAY (5 engines)                       │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐       │
│  │  KICK   │ │  SNARE  │ │  METAL  │ │  PERC   │ │RESONANT│       │
│  │2 params │ │2 params │ │2 params │ │2 params │ │3 params │       │
│  └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘       │
└─────────┼──────────┼──────────┼──────────┼───────────┼───────────────┘
          ▼          ▼          ▼          ▼           ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      VOICE ALLOCATION MATRIX                         │
│            (Param 21 determines mapping to voices)                   │
│         Voice 0 → Engine A, Voice 1 → Engine B, etc.                 │
└─────────────────────────────────┬───────────────────────────────────┘
                                  ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    ENVELOPE ROM (Page 6)                             │
│              128 predefined ADR curves, selected by param 20         │
└─────────────────────────────────┬───────────────────────────────────┘
                                  ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    LFO MODULATION MATRIX (Pages 4-5)                 │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │ LFO1: Shape(0-8) + Rate + Target(0-7) + Depth(-100-100)    │   │
│  │ LFO2: Shape(0-8) + Rate + Target(0-7) + Depth(-100-100)    │   │
│  └─────────────────────────────────────────────────────────────┘   │
│         ↓            ↓            ↓            ↓                    │
│    Pitch Mod    Index Mod     Env Mod     Res Freq/Res              │
└─────────────────────────────────┬───────────────────────────────────┘
                                  ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      AUDIO OUTPUT (Stereo)                           │
│                     Mixed 4-voice sum                                │
└─────────────────────────────────────────────────────────────────────┘
```

## NEON Optimization Strategy

All engines process 4 voices in parallel using NEON intrinsics:

```c
// Process 4 voices simultaneously
float32x4_t phases = vld1q_f32(&voice[0].phase);
float32x4_t freqs = vld1q_f32(&voice[0].freq);
float32x4_t outputs = neon_sin(phases);
```

The voice allocation system uses **engine masks** for efficient parallel processing:

```c
// Each engine processes only the voices assigned to it
float32x4_t kick_out = kick_engine_process(&synth->kick, env,
                                           synth->engine_mask[ENGINE_MODE_KICK]);
```

## Literature References

1. **Lazzarini, V. (2017).** *Computer Music Instruments: Foundations, Design and Development*. Springer.
   - **Section 4.10.3**: Resonant synthesis using summation formulae
   - **Key insight**: Single-sided and double-sided combinations model resonant filters

2. **Chowning, J. (1973).** "The Synthesis of Complex Audio Spectra by Means of Frequency Modulation." *Journal of the Audio Engineering Society*.
   - **Key insight**: FM percussion fundamentals

3. **Kirby, T. & Sandler, M. (2020).** "Advanced Fourier Decomposition for Realistic Drum Synthesis." *DAFx Conference*.
   - **Key insight**: RDFT-based drum analysis and synthesis

## File Structure

```
your_project/
├── header.c              # Parameter definitions (updated with voice allocation)
├── unit.cc               # SDK glue code
├── synth.h               # Integration class
├── config.mk             # NEON compiler flags
├── fm_perc_synth.h       # Main synth with voice allocation
├── fm_presets.h          # 12 presets (8 original + 4 resonant)
├── resonant_synthesis.h  # NEW: Resonant engine
├── kick_engine.h         # Kick engine
├── snare_engine.h        # Snare engine
├── metal_engine.h        # Metal engine
├── perc_engine.h         # Perc engine
├── lfo_enhanced.h        # LFO system with 8 targets
├── envelope_rom.h        # 128 envelope shapes
├── prng.h                # NEON PRNG
├── sine_neon.h           # NEON sine approximation
├── smoothing.h           # Parameter smoothing
├── fm_voices.h           # Voice structures
├── midi_handler.h        # MIDI handling
├── constants.h           # Central constants
├── tests.h               # Unit tests
└── benchmark.h           # Performance benchmarks
```

## Memory Usage Estimate

| Component | Size |
|-----------|------|
| FM Engines (5) | ~2.5 KB |
| LFO System | ~0.5 KB |
| Envelope ROM | ~1 KB |
| Parameter storage | ~0.5 KB |
| **Total State** | **~4.5 KB** |
| Code Size | ~10-12 KB |
| Stack Usage | ~1 KB |

**Total**: Well within drumlogue's limits (< 64 KB)

--

# Testing
## The test suite now covers:
1. Voice Allocation - No duplicates, resonant appears at most once
2. Probability - Statistical accuracy of PRNG
3. Morph Parameter - Correct range mapping per mode
4. Engine Ranges - Parameter validation
5. Integration - Full system coordination

## The benchmark suite measures:
1. Division Operations - Comparing direct vs reciprocal methods
2. Engine Performance - Cycle estimates vs targets
3. Memory Usage - ROM/RAM estimates
4. Allocation Overhead - Confirming negligible per-sample cost

### Run all tests
./run_unit_tests.sh all

### Run specific test
./run_unit_tests.sh alloc      # Voice allocation tests only
./run_unit_tests.sh prob       # Probability tests only
./run_unit_tests.sh morph      # Morph parameter tests only

### Run all benchmarks
./run_benchmarks.sh all

### Run specific benchmark
./run_benchmarks.sh division   # Division operation benchmarks
./run_benchmarks.sh engines    # Engine performance estimates
./run_benchmarks.sh memory     # Memory usage estimates

### Using make targets
make test           # Run all tests
make bench          # Run all benchmarks
make test-alloc     # Run allocation tests only
make bench-division # Run division benchmarks only
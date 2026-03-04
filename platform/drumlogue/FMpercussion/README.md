Excellent refinements! You're absolutely right about the parameter budget. Let me update both documents with these efficient design choices:

# README.md - Percussion FM Synth for KORG drumlogue

## Project Overview

A **4-voice FM percussion synthesizer** with **efficient parameter mapping** - 24 parameters across 6 pages, with combinatorial controls creating complex interactions.

## Parameter Page Layout (Final - Optimized)

```
Page 1: Trigger Probability (per voice)
┌─────────────┬─────────────┬─────────────┬─────────────┐
│  ProbKick   │  ProbSnare  │  ProbMetal  │  ProbPerc   │
│   (0-100%)  │   (0-100%)  │   (0-100%)  │   (0-100%)  │
└─────────────┴─────────────┴─────────────┴─────────────┘

Page 2: Kick + Snare Parameters
┌─────────────┬─────────────┬─────────────┬─────────────┐
│  KickParam1 │  KickParam2 │ SnareParam1 │ SnareParam2 │
│   (0-100%)  │   (0-100%)  │   (0-100%)  │   (0-100%)  │
└─────────────┴─────────────┴─────────────┴─────────────┘

Page 3: Metal + Perc Parameters
┌─────────────┬─────────────┬─────────────┬─────────────┐
│ MetalParam1 │ MetalParam2 │ PercParam1  │ PercParam2  │
│   (0-100%)  │   (0-100%)  │   (0-100%)  │   (0-100%)  │
└─────────────┴─────────────┴─────────────┴─────────────┘

Page 4: LFO1 Configuration
┌─────────────┬─────────────┬─────────────┬─────────────┐
│   LFO1      │   LFO1      │   LFO1      │   LFO1      │
│   Shape     │   Rate      │   Target    │   Depth     │
│   (0-8)     │   (0-100%)  │   (0-5)     │  (-100-100) │
└─────────────┴─────────────┴─────────────┴─────────────┘

Page 5: LFO2 Configuration
┌─────────────┬─────────────┬─────────────┬─────────────┐
│   LFO2      │   LFO2      │   LFO2      │   LFO2      │
│   Shape     │   Rate      │   Target    │   Depth     │
│   (0-8)     │   (0-100%)  │   (0-5)     │  (-100-100) │
└─────────────┴─────────────┴─────────────┴─────────────┘

Page 6: Global Envelope + Future
┌─────────────┬─────────────┬─────────────┬─────────────┐
│  Envelope   │   (unused)  │   (unused)  │   (unused)  │
│   Shape     │             │             │             │
│   (0-127)   │             │             │             │
└─────────────┴─────────────┴─────────────┴─────────────┘
```

## Per-Instrument Parameter Mapping (2 params each)

With pitch already covered by LFO modulation, we focus on timbral shaping:

| Instrument | Param1 | Param2 | Description |
|------------|--------|--------|-------------|
| **Kick** | Sweep Depth | Decay Shape | Pitch sweep amount + decay curve |
| **Snare** | Noise Mix | Body Resonance | Noise level + filter/body tone |
| **Metal** | Inharmonicity | Brightness | Ratio spread + high-frequency content |
| **Perc** | Ratio Center | Tonal Variation | Base ratio + variation amount |

## Envelope Shape ROM (0-127)

Single parameter selects from 128 predefined ADR curves, optimized for percussion:

| Range | Attack | Decay | Release | Character |
|-------|--------|-------|---------|-----------|
| 0-15 | 0-1 ms | 50-100 ms | 20-50 ms | **Tight & dry** - for fast patterns |
| 16-31 | 1-5 ms | 100-200 ms | 50-100 ms | **Punchy** - standard drums |
| 32-47 | 5-10 ms | 200-300 ms | 100-150 ms | **Fat** - bigger sounds |
| 48-63 | 10-20 ms | 300-400 ms | 150-200 ms | **Boomy** - resonant |
| 64-79 | 0-1 ms | 50-100 ms | 100-200 ms | **Gated** - quick decay, long release |
| 80-95 | 1-5 ms | 100-200 ms | 200-300 ms | **Ambient** - washier |
| 96-111 | 5-10 ms | 200-300 ms | 300-400 ms | **Reverse-like** - slower attack |
| 112-127 | 10-20 ms | 300-400 ms | 400-500 ms | **Pad-like** - sustained percussion |

## Architecture (Updated)

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
│         (4 voices × up to 4 operators = 16 ops parallel)    │
└─────────────────────────┬───────────────────────────────────┘
                          ▼
┌──────────────────────────────────────────────────────────────┐
│                        FM Engine Array                       │
│  ┌───────────┐ ┌──────────┐ ┌────────────┐ ┌──────────┐      │
│  │  KICK     │ │  SNARE   │ │  METAL     │ │  PERC    │      │
│  │2 params:  │ │2 params: │ │2 params:   │ │2 params: │      │
│  │Sweep/Decay│ │Noise/Body│ │Inharm/Brght│ │Ratio/Var │      │
│  └─────┬─────┘ └────┬─────┘ └────┬───────┘ └────┬─────┘      │
└─────────┼───────────┼────────────┼──────────────┼────────────┘
          ▼           ▼            ▼              ▼
┌──────────────────────────────────────────────────────────────┐
│              Envelope ROM (Page 6)                           │
│      128 predefined ADR curves, selected by single param     │
└─────────────────────────┬────────────────────────────────────┘
                          ▼
┌──────────────────────────────────────────────────────────────┐
│              LFO Modulation Matrix (Pages 4-5)               │
│  ┌─────────────────────────────────────────────────────┐     │
│  │ LFO1: Shape(0-8) + Rate + Mask + Depth              │     │
│  │ LFO2: Shape(0-8) + Rate + Mask + Depth              │     │
│  └─────────────────────────────────────────────────────┘     │
│         ↓              ↓              ↓                      │
│    Pitch Mod     Index Mod     Env Mod (future)              │
└─────────────────────────┬────────────────────────────────────┘
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                      Audio Output (Stereo)                  │
│                     Mixed 4-voice sum                       │
└─────────────────────────────────────────────────────────────┘
```

## Is 2 Parameters per FM Engine Enough for Decent Sound Design?

**YES - Absolutely.** Here's why:

### Kick Drum (2 parameters)
- **Sweep Depth** (0-100%): Controls how much pitch drops (0 = static, 100% = 3 octave drop)
- **Decay Shape** (0-100%): Controls exponential curve (0 = slow decay, 100 = fast "clicky" decay)

This covers the two most important dimensions: pitch contour and amplitude envelope. Classic 808/909 kicks are often just these two variables.

### Snare (2 parameters)
- **Noise Mix** (0-100%): Balance between tonal body and sizzle
- **Body Resonance** (0-100%): Tunes the filter/body frequency

This captures the key snare dimensions: noise content and tonal center.

### Metal (2 parameters)
- **Inharmonicity** (0-100%): Spreads the modulator ratios from harmonic to clangorous
- **Brightness** (0-100%): Controls high-frequency content via filter/mod index

Cymbals are defined by inharmonic content and brightness - perfect.

### Perc (2 parameters)
- **Ratio Center** (0-100%): Maps to modulator ratio (1.0 - 4.0)
- **Tonal Variation** (0-100%): Adds secondary modulation for complexity

Toms and percussion need pitch (covered by LFO) and tonal character - covered.

## Design Validation: Parameter Coverage

| Sound Dimension | How Covered |
|-----------------|-------------|
| Pitch | LFO modulation |
| Pitch Envelope | Kick Param1 (sweep) |
| Amplitude Envelope | Envelope ROM (Page 6) |
| Tonal Brightness | Metal Param2, Snare Param2 |
| Noise Content | Snare Param1 |
| Harmonic/Inharmonic | Metal Param1, Perc Param1-2 |
| Modulation | LFO1/2 Pages 4-5 |

---

### Parameter Mapping Summary

| Page | Parameter | Type | Range | Purpose |
|------|-----------|------|-------|---------|
| 1-1 | ProbKick | Percent | 0-100 | Kick trigger probability |
| 1-2 | ProbSnare | Percent | 0-100 | Snare trigger probability |
| 1-3 | ProbMetal | Percent | 0-100 | Metal trigger probability |
| 1-4 | ProbPerc | Percent | 0-100 | Perc trigger probability |
| 2-1 | KickParam1 | Percent | 0-100 | Kick sweep depth |
| 2-2 | KickParam2 | Percent | 0-100 | Kick decay shape |
| 2-3 | SnareParam1 | Percent | 0-100 | Snare noise mix |
| 2-4 | SnareParam2 | Percent | 0-100 | Snare body resonance |
| 3-1 | MetalParam1 | Percent | 0-100 | Metal inharmonicity |
| 3-2 | MetalParam2 | Percent | 0-100 | Metal brightness |
| 3-3 | PercParam1 | Percent | 0-100 | Perc ratio center |
| 3-4 | PercParam2 | Percent | 0-100 | Perc variation |
| 4-1 | LFO1Shape | Strings | 0-8 | LFO1 waveform combo |
| 4-2 | LFO1Rate | Percent | 0-100 | LFO1 speed |
| 4-3 | LFO1Mask | Strings | 0-14 | LFO1 voice destination |
| 4-4 | LFO1Depth | Percent | 0-100 | LFO1 modulation amount |
| 5-1 | LFO2Shape | Strings | 0-8 | LFO2 waveform combo |
| 5-2 | LFO2Rate | Percent | 0-100 | LFO2 speed |
| 5-3 | LFO2Mask | Strings | 0-14 | LFO2 voice destination |
| 5-4 | LFO2Depth | Percent | 0-100 | LFO2 modulation amount |
| 6-1 | EnvShape | Scaled | 0-127 | Envelope ROM selector |
| 6-2 | (unused) | - | - | Future expansion |
| 6-3 | (unused) | - | - | Future expansion |
| 6-4 | (unused) | - | - | Future expansion |

### LFO Shape Encoding (0-8) - Confirmed

| Value | LFO1 Shape | LFO2 Shape |
|-------|------------|------------|
| 0 | Triangle | Triangle |
| 1 | Ramp | Ramp |
| 2 | Chord | Chord |
| 3 | Triangle | Ramp |
| 4 | Triangle | Chord |
| 5 | Ramp | Triangle |
| 6 | Ramp | Chord |
| 7 | Chord | Triangle |
| 8 | Chord | Ramp |

### LFO Target Encoding (0-5)

| Value | Target | Description |
|-------|--------|-------------|
| 0 | None | LFO not assigned (disabled) |
| 1 | Pitch | Modulate oscillator frequency (± depth) |
| 2 | Modulation Index | Modulate FM amount (± depth) |
| 3 | Envelope | Modulate envelope shape/level |
| 4 | LFO2 Phase | LFO1 modulates LFO2's phase (LFO1 only) |
| 5 | LFO1 Phase | LFO2 modulates LFO1's phase (LFO2 only) |

### Phase Independence

```c
// LFO1 and LFO2 start with 90° phase offset to prevent cancellation
lfo1_phase = 0.0f;
lfo2_phase = 0.25f;  // 90° offset (0.25 of full cycle)
```

### Modulation Depth (Bipolar)

Depth parameter range: -100 to +100
- Positive values: Normal modulation direction
- Negative values: Inverted modulation (e.g., pitch goes down instead of up)
- 0: No modulation (even if target assigned)

### Chord Mode Intervals - Root-3rd-5th (Confirmed)
- Step 0: 0 semitones (Root)
- Step 1: +4 semitones (Major 3rd)
- Step 2: +7 semitones (Perfect 5th)

### Voice Mask Encoding (0-14)

| Value | Kick | Snare | Metal | Perc | Display |
|-------|------|-------|-------|------|---------|
| 0 | ✓ | - | - | - | "Kick" |
| 1 | - | ✓ | - | - | "Snare" |
| 2 | - | - | ✓ | - | "Metal" |
| 3 | - | - | - | ✓ | "Perc" |
| 4 | ✓ | ✓ | - | - | "K+S" |
| 5 | ✓ | - | ✓ | - | "K+M" |
| 6 | ✓ | - | - | ✓ | "K+P" |
| 7 | - | ✓ | ✓ | - | "S+M" |
| 8 | - | ✓ | - | ✓ | "S+P" |
| 9 | - | - | ✓ | ✓ | "M+P" |
| 10 | ✓ | ✓ | ✓ | - | "K+S+M" |
| 11 | ✓ | ✓ | - | ✓ | "K+S+P" |
| 12 | ✓ | - | ✓ | ✓ | "K+M+P" |
| 13 | - | ✓ | ✓ | ✓ | "S+M+P" |
| 14 | ✓ | ✓ | ✓ | ✓ | "All" |

# Integration Checklist
## Files to include in your project folder:
```
your_project/
├── header.c              # Provided (parameter definitions)
├── unit.cc               # Provided (SDK glue code)
├── synth.h               # NEW (integration class above)
├── config.mk             # Updated (compiler flags)
├── fm_perc_synth.h       # Your main synth header
├── fm_presets.h          # Your presets
├── kick_engine.h         # Kick engine
├── snare_engine.h        # Snare engine
├── metal_engine.h        # Metal engine
├── perc_engine.h         # Perc engine
├── lfo_enhanced.h        # LFO system
├── envelope_rom.h        # Envelope ROM
├── prng.h                # PRNG
├── sine_neon.h           # Sine oscillator
├── smoothing.h           # Parameter smoothing
├── fm_voices.h           # Voice structures
├── midi_handler.h        # MIDI handling
├── profile.h             # Profiling (optional)
└── benchmark.h           # Benchmarks (optional)
```

# Key Integration Points
1. synth.h now includes your fm_perc_synth.h and implements all required callbacks
2. Render() calls fm_perc_synth_process() for each sample
3. NoteOn/Off calls your synth's note handlers
4. setParameter() updates your synth's parameter array
5. LoadPreset() loads from your fm_presets.h
6. NEON flags added to config.mk for optimization


# Memory Usage Estimate
- FM Synth State: ~2KB
- Code Size: ~8-10KB
- Stack Usage: ~1KB
- Total: Well within drumlogue's limits (< 64KB)



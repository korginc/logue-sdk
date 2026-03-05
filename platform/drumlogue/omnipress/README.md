# README.md - OmniPress Master Compressor for KORG drumlogue

## Overview

**OmniPress** is a character master bus compressor for the KORG drumlogue, inspired by the **Eventide Omnipressor** and **Empirical Labs EL8 Distressor**. It features **three distinct compression modes** in a single unit, taking advantage of the drumlogue's 4-channel master effect input for external sidechain processing.

### Key Features

| Feature | Description |
|---------|-------------|
| **3 Compression Modes** | Standard, Distressor, Multiband |
| **External Sidechain** | 4-channel input for ducking/pumping |
| **Drive/Wavefolder** | 5 distortion modes from soft clip to sub-octave |
| **NEON Optimization** | Fully vectorized for ARM Cortex-A7 |
| **12 User Parameters** | Spread across 3 control pages |
| **24dB/oct Crossover** | Linkwitz-Riley filters for multiband mode |

---

## Compression Modes

### Mode 0: Standard Compressor
A versatile, clean compressor with all the essentials:
- **Threshold**: -60 to 0 dB
- **Ratio**: 1:1 to 20:1
- **Attack**: 0.1 to 100 ms
- **Release**: 10 to 2000 ms
- **Soft/Hard Knee** selectable
- **Peak/RMS detection** with blend

### Mode 1: Distressor Mode
Emulates the Empirical Labs EL8 Distressor with its unique character:

| Feature | Implementation |
|---------|---------------|
| **8 Ratios** | 1:1 (warm), 2:1, 3:1, 4:1, 6:1, 10:1 (opto), 20:1, NUKE |
| **Distortion Modes** | Dist 2 (2nd harmonic), Dist 3 (3rd harmonic), Both |
| **Opto Mode** | Extended release times up to 20 seconds |
| **NUKE Mode** | Brick-wall limiting with 40dB+ reduction |
| **1:1 Warm Mode** | Harmonic enhancement without compression |

### Mode 2: Multiband Compressor
3-band compression with independent controls:

| Band | Frequency Range | Independent Controls |
|------|----------------|---------------------|
| **Low** | 20 - 250 Hz | Threshold, Ratio, Makeup |
| **Mid** | 250 - 2500 Hz | Threshold, Ratio, Makeup |
| **High** | 2500 - 20 kHz | Threshold, Ratio, Makeup |
| **All** | Full range | Global adjustments |

Features:
- **Linkwitz-Riley 24dB/oct** crossovers (phase neutral)
- **Solo/Mute** per band
- **Independent attack/release** per band
- **Gain reduction meters** per band (future)

---

## Drive/Wavefolder (5 Modes)

The drive stage offers 5 distinct character modes, controllable via the DRIVE parameter (0-100%):

| Mode | Name | Description | Character |
|------|------|-------------|-----------|
| **0** | Soft Clip | Tanh approximation | Tube-like saturation |
| **1** | Hard Clip | Brick-wall limiter | Digital distortion |
| **2** | Triangle Folder | Wavefolding | Synth-like aggression |
| **3** | Sine Folder | Sinusoidal folding | Smooth, complex harmonics |
| **4** | Sub-Octave | Zero-crossing square wave | Gritty, synth bass |

The drive amount controls both the input gain to the waveshaper and the dry/wet blend for parallel processing.

---

## Parameter Reference

### Page 1: Core Dynamics

| Param | Name | Range | Description |
|-------|------|-------|-------------|
| 0 | THRESH | -60.0 to 0.0 dB | Compression threshold |
| 1 | RATIO | 1.0 to 20.0 | Compression ratio |
| 2 | ATTACK | 0.1 to 100.0 ms | Attack time |
| 3 | RELEASE | 10 to 2000 ms | Release time |

### Page 2: Character & Output

| Param | Name | Range | Description |
|-------|------|-------|-------------|
| 4 | MAKEUP | 0.0 to 24.0 dB | Output gain |
| 5 | DRIVE | 0 to 100% | Drive/wavefolder amount |
| 6 | MIX | -100 to +100 | Dry/Wet balance |
| 7 | SC HPF | 20 to 500 Hz | Sidechain high-pass filter |

### Page 3: Mode Selection

| Param | Name | Range | Description |
|-------|------|-------|-------------|
| 8 | COMP MODE | 0-2 | 0=Standard, 1=Distressor, 2=Multiband |
| 9 | BAND SEL | 0-3 | 0=Low, 1=Mid, 2=High, 3=All (for multiband) |
| 10 | L THRESH | -60.0 to 0.0 dB | Selected band threshold |
| 11 | L RATIO | 1.0 to 20.0 | Selected band ratio |

### Page 4: Distressor & Future

| Param | Name | Range | Description |
|-------|------|-------|-------------|
| 12 | DSTR MODE | 0-3 | 0=None, 1=Dist2, 2=Dist3, 3=Both |
| 13-15 | (Future) | - | Reserved for expansion |

---

## Signal Flow Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                         INPUT (4-channel)                         │
│              [Main L, Main R, Sidechain L, Sidechain R]           │
└─────────────────────────┬───────────────────────────────────────┘
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                      SIDECHAIN SELECT                             │
│              External (SC L/R) or Internal (Main L/R)             │
└─────────────────────────┬───────────────────────────────────────┘
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                      SIDECHAIN HPF (20-500 Hz)                    │
│                    12dB/oct Bessel filter                         │
└─────────────────────────┬───────────────────────────────────────┘
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                     ENVELOPE DETECTOR                             │
│              Peak / RMS / Blend with attack/release               │
└─────────────────────────┬───────────────────────────────────────┘
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                        MODE SELECTOR                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐           │
│  │  STANDARD    │  │  DISTRESSOR  │  │  MULTIBAND   │           │
│  │  • Ratio     │  │  • 8 Ratios  │  │  • Crossover │           │
│  │  • Knee      │  │  • Opto      │  │  • 3 Bands   │           │
│  │  • Smoothing │  │  • Harmonics │  │  • Indep Comp│           │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘           │
└─────────┼─────────────────┼─────────────────┼─────────────────────┘
          ▼                 ▼                 ▼
    Gain Reduction    Gain Reduction    Band Gains
          └─────────────┬─────────────────┘
                        ▼
┌─────────────────────────────────────────────────────────────────┐
│                     DRIVE / WAVEFOLDER (5 modes)                  │
│          Soft Clip │ Hard Clip │ Triangle │ Sine │ SubOctave      │
└─────────────────────────┬───────────────────────────────────────┘
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                      DRY/WET MIX (Parallel)                       │
│                   Blend processed with original                    │
└─────────────────────────┬───────────────────────────────────────┘
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                       MAKEUP GAIN (0-24 dB)                       │
└─────────────────────────┬───────────────────────────────────────┘
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                         OUTPUT (Stereo)                            │
│                      Compressed and character-rich                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## NEON Optimization Strategy

All processing is vectorized to process **4 samples simultaneously**:

```c
// Load 4 stereo frames with sidechain
float32x4x4_t interleaved = vld4q_f32(in_p);
float32x4_t main_l = interleaved.val[0];  // L0, L1, L2, L3
float32x4_t main_r = interleaved.val[1];  // R0, R1, R2, R3
float32x4_t sc_l   = interleaved.val[2];  // SC L0, SC L1, SC L2, SC L3
float32x4_t sc_r   = interleaved.val[3];  // SC R0, SC R1, SC R2, SC R3

// Process all 4 samples in parallel
float32x4_t envelope = envelope_detect(&envelope_, main_l, sidechain);
float32x4_t gain_db = gain_computer_process(&gain_comp_, envelope_db, thresh_db_, ratio_);
```

Performance target: **< 200 cycles per sample** (< 2% CPU on 1GHz ARM Cortex-A7)

---

## Parameter String Display

| Parameter | Values Displayed |
|-----------|------------------|
| COMP MODE | "Standard", "Distressor", "Multiband" |
| BAND SEL | "Low", "Mid", "High", "All" |
| DSTR MODE | "None", "Dist 2", "Dist 3", "Both" |
| MIX | "DRY" (-100), "BAL" (0), "WET" (+100) |
| RATIO | Shows as "4.0:1", "20:1", "NUKE", "Opto" |

---

## Future Expansion

The architecture supports easy addition of:

1. **More band parameters** (Attack, Release per band)
2. **Knee control** (0-100% softness)
3. **Detection mode** (Peak/RMS/Blend)
4. **Stereo link** adjustment
5. **Lookahead** (up to 10ms)
6. **Sidechain listen** mode
7. **8 factory presets** for common use cases

---

## Technical Specifications

| Specification | Value |
|---------------|-------|
| Sample Rate | 48 kHz fixed |
| Input Channels | 4 (Main L/R + Sidechain L/R) |
| Output Channels | 2 (Stereo) |
| Parameters | 12 (expandable to 24) |
| CPU Target | < 2% @ 1GHz |
| Memory | ~4 KB |
| Crossover | Linkwitz-Riley 24dB/oct |
| Drive Modes | 5 types |

---

## Credits & Inspiration

- **Eventide Omnipressor** (1970s) - Reverse compression concept
- **Empirical Labs EL8 Distressor** - Ratio modes and harmonic distortion
- **SSL Console** - Multiband architecture
- **SHARC Audio Elements** - DSP building block patterns

---

**OmniPress** brings studio-grade dynamics processing to the KORG drumlogue, with three compressors in one and enough character to satisfy any genre.
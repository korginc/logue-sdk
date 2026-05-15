# NeonLabirinto – Labyrinthine Resonant Reverb for drumlogue

**NeonLabirinto** is a character-rich, highly optimized Feedback Delay Network (FDN) reverb for the KORG drumlogue. Moving far beyond transparent room simulation, it transforms ordinary sounds into immersive, evolving textures using physical material resonances, chaotic spatial routing, and exotic microtonal shimmering.

## Core Architecture

- **8-Channel FDN** mixed via a **Fully Vectorized Fast Walsh-Hadamard Transform (FWHT)**. This O(N log N) algorithm distributes energy perfectly across all 8 channels using zero multiplications, executing in a fraction of a microsecond.
- **Vectorized Interleaved Delay Line**: Stores all 8 FDN channels in a single time-aligned frame to enable `vld4q_f32` gathering—resulting in 3× faster delay line reads.
- **Active Partial Counting (APC)**: A CPU-saving algorithm that continuously monitors the decay envelope. When the reverb tail drops below -100 dBFS, the heavy FDN calculations are instantly bypassed while preserving dry signal flow.
- **Tape-Style Interpolated Pre-Delay**: A 1-pole slew limiter wraps the pre-delay read head. Adjusting the pre-delay acts like physically speeding up or slowing down magnetic tape, bending the pitch seamlessly without zipper noise or clicks, while linear interpolation provides natural tape-head high-frequency damping.

## The DSP Features

### 1. Material Body Resonance (Double Filters)
Instead of standard 1-pole high-frequency damping, NeonLabirinto utilizes true 2nd-order Direct-Form II Transposed Biquad filters inside the feedback loop to emulate the physical body resonance of different acoustic materials. To prevent SIMD comb-filtering artifacts, these operate in mathematically perfect scalar loops:
* **Wood** (Preset 0 *foresta*): Warm, highly-damped low-mid resonance.
* **Stone** (Preset 1 *tempio*): Dark, heavy, and highly reflective.
* **Metal** (Preset 2 *labirinto*): Glassy, inharmonic ringing with high-frequency retention.
* **Crystal** (Preset 3 *esotico*): Bright allpass-like shimmer; medium-Q bandpass for microtonal shimmer character.
* **Noise** (Preset 4 *stellare*): Internal noise generator acting as the acoustic resonator source.

The material filter is selected automatically when loading a preset; it cannot be changed independently of preset selection.

### 2. Coloured Noise Injection
When the filter mode is set to **Noise** (*stellare* preset), the reverb acts as an acoustic resonator for an internal pseudo-random noise generator. The noise color sweeps smoothly from deep Brown, through Pink and Grey (notched), up to harsh Violet. Use **DFSN** (diffusion) to shape the noise density and **DAMP** to control the noise injection gain.

### 3. Cochrane 18-EDO Microtonal Shimmer (PILL = 4)
Instead of relying on synthetic ring modulation, the *Stellare* and *Esotico* presets implement a genuine **Cochrane Shimmer** (PILL = 4). The 8 delay lines are subjected to deep, independent Doppler pitch-shifts locked to an 18-EDO (Equal Division of the Octave) microtonal scale. When these echoes collide in the Hadamard matrix, they generate massive, non-Western acoustic beating and dense harmonic interference. **SHMR** controls the shimmer frequency (0–100 maps to the pitch-shift frequency range).

## Parameter Guide

NeonLabirinto has **12 parameters** across 3 pages.

### Page 1: Main Controls

| ID | Name | Range | Description |
|----|------|-------|-------------|
| 0 | Preset | 0–3 | Loads a factory preset (see Presets section) |
| 1 | MIX | 0–100.0% | Wet/dry blend (x0.1 precision) |
| 2 | TIME | 0.1–10.0 s | Mid-frequency RT60 decay time |
| 3 | LOW | 0.1–10.0 s | Low-frequency RT60 (bass tail length) |

### Page 2: Advanced Controls

| ID | Name | Range | Description |
|----|------|-------|-------------|
| 4 | HIGH | 0.1–10.0 s | High-frequency RT60 (treble tail length) |
| 5 | DAMP | 200–10000 Hz | Damping crossover — frequencies above this decay faster |
| 6 | WIDE | 0–200% | Stereo width of the reverb tail |
| 7 | DFSN | 0–100.0% | Diffusion / complexity — how much the delay lines interleave |

### Page 3: Routing & Shimmer

| ID | Name | Range | Description |
|----|------|-------|-------------|
| 8 | PILL | 0–4 | Routing macro: 0=sparse(2ch), 1=ping-pong(4ch), 2=stone(6ch), 3=full(8ch), 4=shimmer |
| 9 | SHMR | 0–100 | Shimmer frequency — controls microtonal Doppler pitch-shift in PILL=4 mode |
| 10 | PDLY | 0–200 ms | Slew-limited pre-delay (tape-style interpolation avoids zipper noise) |
| 11 | VIBR | 0.1–3.0 Hz | LFO speed for random diffusion matrix modulation |

## Factory Presets

| # | Name | Filter | PILL | Character |
|---|------|--------|------|-----------|
| 0 | foresta | Wood | 3 (full) | Warm, mellow room; short decay, moderate diffusion |
| 1 | tempio | Stone | 2 (stone) | Dark, heavy; long lows, tight highs, wide stereo |
| 2 | labirinto | Metal | 1 (ping-pong) | Glassy, ping-pong stereo bouncing; moderate length |
| 3 | esotico | Crystal | 4 (shimmer) | Microtonal shimmer; bright, exotic, non-Western character |
| 4 | stellare | Noise | 4 (shimmer) | Long, spacey; noise-seeded reverb with deep shimmer tail |

> **Note:** `num_presets` is 4 in `header.c` (the SDK preset-recall slot count); the 5th preset (*stellare*) is accessible by setting Preset = 4 at runtime.

## Technical & Build Notes

- **Scalar vs. Vector Segregation:** While 90% of the DSP (delay reading/writing, mixdown, modulation) runs in parallel via ARM NEON intrinsics, Infinite Impulse Response (IIR) states like the material biquads and noise filters are calculated via optimized scalar loops to prevent NEON comb-filtering artifacts.
- **Branchless DSP:** Buffer wrapping and phase accumulations utilize float/bitwise arithmetic rather than `while` loops, completely eliminating Cortex-A7 branch-prediction pipeline stalls.
- **Denormal Safety:** The engine forces `Flush-to-Zero` and `Default-NaN` in the ARM FPSCR to guarantee CPU usage never spikes when the reverb tail decays into subnormal values.

## Building for drumlogue

Place `NeonAdvancedLabirinto.h`, `unit.cc`, and `header.c` in your SDK project. Ensure `float_math.h` is available. Compile with `-O3 -mcpu=cortex-a7 -mfpu=neon-vfpv4` (or appropriate for drumlogue’s ARM processor).
# Drumlogue Master Compressor: "OmniPress"

## Overview
This project implements a character master bus compressor for the KORG drumlogue, heavily inspired by the Empirical Labs EL8 Distressor and Eventide Omnipressor. It is designed as a `masterfx` unit taking advantage of the drumlogue's 4-channel master effect input (Main L/R and Sidechain L/R) to allow for sidechain-pumped master bus compression.

## DSP Architecture

### 1. Audio I/O
The unit processes a 48kHz audio stream. The input buffer geometry consists of 4 interlaced channels per frame: `[main_left, main_right, sidechain_left, sidechain_right]`. The output geometry is 2 channels: `[out_left, out_right]`. 

### 2. Signal Flow
1. **Sidechain Filter**: A high-pass filter on the sidechain/detector input to prevent heavy low-end (kick drums) from overly triggering the compressor.
2. **Envelope Detector**: Calculates the level of the control signal (either Main L/R or external Sidechain). Configurable for Peak or RMS detection.
3. **Gain Computer**: Translates the detected envelope into a gain reduction value based on Threshold, Ratio, and a configurable soft/hard knee.
4. **VCA (Voltage Controlled Amplifier)**: Applies the computed gain reduction smoothly using Attack and Release time constants.
5. **Saturation / Wavefolder (Character Stage)**: Applies non-linear distortion to emulate the "Dist 2" and "Dist 3" harmonic distortion modes of hardware compressors.
6. **Mix Stage**: Parallel compression blend (Dry/Wet) and Make-up Gain.

### 3. Parameter Mapping (UI)
The drumlogue supports up to 24 parameters per unit, spread across pages of 4.

* **Page 1 (Core Dynamics)**
    * `THRESH`: Threshold (dB)
    * `RATIO`: Compression Ratio (e.g., 1:1 to limit/infinity)
    * `ATTACK`: Attack time (ms)
    * `RELEASE`: Release time (ms)
* **Page 2 (Character & Output)**
    * `MAKEUP`: Make-up gain (dB)
    * `DRIVE`: Saturation / Wavefolder drive amount
    * `MIX`: Dry/Wet balance
    * `SC-HPF`: Sidechain High-Pass Filter cutoff (Hz)

---

## Feasibility Study: Overdrive & Wavefolder Option

Adding a distortion stage is highly desirable for a Distressor-style unit, but it must fit within the CPU limits of the drumlogue's ARM Cortex-A7 processor.

### Overdrive (Harmonic Distortion)
To emulate the "Dist 2" (tube/2nd order) and "Dist 3" (tape/3rd order) modes:
* **Method**: Soft clipping using polynomial approximations or fast math approximations.
* **Math**: A standard soft-clipper like hyperbolic tangent $y = \tanh(x)$ is computationally expensive. We will instead use a fast polynomial approximation or a piecewise function:
    $$f(x) = x - \frac{x^3}{3}$$
* **Feasibility**: Highly feasible. Using ARM NEON intrinsics like `vmla_f32` (vector multiply-accumulate), we can compute soft clipping for stereo channels simultaneously with very low CPU overhead.

### Wavefolding
To push the unit into more extreme, Omnipressor/industrial territory:
* **Method**: Folding the wave back on itself when it exceeds a threshold, rather than clipping it.
* **Math**: A sine-based folder $y = \sin(x \cdot \text{drive})$ or a simpler triangle folder:
    $$y = 4 \left| \left( \frac{x}{4} + 0.25 \right) - \left\lfloor \frac{x}{4} + 0.75 \right\rfloor \right| - 1$$
* **Feasibility**: Feasible, but requires careful management of aliasing. Wavefolding generates high-order harmonics that will alias at 48kHz. An oversampling framework (even 2x) would sound much better, but might exceed the strict time limits of the `unit_render` callback. 
* **Recommendation**: Implement a fast anti-aliased soft clipper (Overdrive) first. If CPU profiling shows enough headroom, implement a simple triangle wavefolder without oversampling as a "Lo-Fi Industrial" mode.

## SIMD Optimization Strategy
All per-sample DSP must be vectorized. The main `Process()` loop steps 4 samples at a time using `vld1q_f32()`.
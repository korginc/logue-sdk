# RipplerX-Waveguide (Drumlogue Bare-Metal DSP)

## Overview
This project is a polyphonic Physical Modeling synthesizer designed natively for the Korg Drumlogue. It abandons desktop-VST object-oriented paradigms in favor of a strictly **Data-Oriented Design**. It uses fixed memory allocations, branchless mathematics, and ARM NEON SIMD optimization to perfectly respect the Drumlogue's ~20-microsecond RTOS audio deadline.

## Core Acoustic Logic (Karplus-Strong / Digital Waveguide)
Instead of additive sine-wave synthesis (which is mathematically expensive and prone to instability), this engine uses Digital Waveguides.
The physics of a string, tube, or membrane are simulated by injecting a short burst of energy (The Exciter) into a circular buffer (The Delay Line). The sound loops continuously, passing through a Loss Filter on every cycle. The Delay Length determines the pitch, and the Loss Filter determines the physical material.

## Signal Flow Architecture
```text
MIDI Note / Gate
       │
       ▼
┌────────────────────────────────────────────────────────┐
│ 1. EXCITER STAGE (The Strike)                          │
│   ├─► Noise Generator (Filtered burst)                 │
│   ├─► Mallet (Mathematical impulse)                    │
│   └─► PCM Sample (Safely bound-checked via OS)         │
└──────┬─────────────────────────────────────────────────┘
       │
       ├─────────────────────────┐ (A/B Split)
       │                         │
┌──────▼──────────────┐   ┌──────▼──────────────┐
│ 2A. RESONATOR A     │   │ 2B. RESONATOR B     │
│   ├─► Delay Line    │   │   ├─► Delay Line    │
│   ├─► Loss Filter   │   │   ├─► Loss Filter   │
│   └─► Fractional Hz │   │   └─► Fractional Hz │
└──────┬──────────────┘   └──────┬──────────────┘
       │                         │
       └───────────┬─────────────┘
                   │
┌──────────────────▼─────────────────────────────────────┐
│ 3. MASTER SHAPING                                      │
│   ├─► A/B Mixer                                        │
│   ├─► Brickwall Limiter (RTOS NaNs safety net)         │
└──────────────────┬─────────────────────────────────────┘
                   ▼
            TO DRUMLOGUE OS
```
                --------

## 2. Architectural Breakdown of the Missing Modules
Here is how we will build those missing pieces without losing our ultra-fast, bare-metal performance.

### A. Constants & Tables (constants.h / tables.h)
- Instead of calculating complex math on the fly, we use pre-calculated arrays (tables) stored in the Drumlogue's flash memory.

- Pitch-to-Frequency Table: Instead of calling powf() to convert MIDI notes to Hertz, we will have a simple const float mtof_table[128] array. Looking up a value in an array takes 1 CPU cycle.

- Constants: We will define c_sampleRate = 48000.0f, c_maxVoices = 4, and our UI boundary constants here so the whole project uses a single source of truth.

### B. The Envelope Generator (envelope.h)
- We need a lightweight Envelope to shape the Noise burst and act as a VCA (Voltage Controlled Amplifier) if the user wants the sound to choke immediately on GateOff.

- The Structure: A simple struct holding attack_rate, decay_rate, and current_level.

- The Logic: We use an exponential decay multiplier (e.g., current_level *= decay_rate). It requires zero if/else branches during the release phase, making it incredibly fast.

### C. The Filter (filter.h)
- We will use a Chamberlin State Variable Filter (SVF). It is highly efficient and provides Lowpass, Highpass, and Bandpass outputs simultaneously from the exact same calculation.

- We will use this to filter the Noise Exciter (so you can have low "thumps" or high "clicks" to strike the resonator).

- We will also put one at the very end of the signal chain as a Master Filter (mapped to header.c LowCut knob).

### D. The Waveguide Models (models.h)
- In old RipplerX VST, "Models" meant loading an array of 64 sine-wave ratios. In our new Waveguide engine, "Models" dictate the physical topology of the delay lines:

- String / Marimba: 1 Delay Line + Lowpass Filter + Dispersion (Allpass).

- Open Tube (Flute): 1 Delay Line + Inverting Feedback (multiplies by -1.0 every loop).

- Closed Tube (Clarinet): 1 Delay Line + Non-Inverting Feedback.

- Membrane (Drumhead): 2 Detuned Delay Lines running in parallel to simulate the chaotic 2D vibration of a drum skin.

- We will use c_parameterModel UI knob to simply swap out a few coefficients and routing paths inside process_waveguide().

### The Exciter Stage (Mallet & Noise)
To excite the passive waveguide resonators, the engine uses lightweight mathematical impulse generators.
- **The Mallet:** A heavily damped, single-cycle pulse simulating a physical strike.
- **The Noise:** A fast PRNG noise burst shaped by an AR (Attack-Release) envelope to simulate breath, bow friction, or snare wires.
Both are implemented as flat, inline C++ structs to minimize function-call overhead.

### Preset & UI Translation
The synthesizer reuses the legacy preset parameter arrays (Bells, Marimba, etc.). The 0-1000 UI ranges are caught by the `setParameter` function and mathematically translated into physical Waveguide coefficients (Feedback $g$, Filter Cutoff $H(z)$, Dispersion) in the control thread. This allows legacy UI configurations to seamlessly drive the new acoustic engine without modification.
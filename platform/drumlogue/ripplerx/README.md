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
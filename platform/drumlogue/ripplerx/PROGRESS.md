# Project Status Tracker

## Phase 1: Core DSP Structures [COMPLETED]
- [x] Define flat, cache-friendly C++ structures (`Waveguide`, `Exciter`, `Voice`).
- [x] Establish memory bounds (4096-sample delay line for safe sub-bass).

## Phase 2: API & UI Binding [COMPLETED]
- [x] Connect `header.c` parameter indices to the engine.
- [x] Write `setParameter` translation logic (Pre-calculate all filter coefficients in the UI thread).
- [x] Implement the dynamic Sample Loading bounds check to prevent RTOS crashes.

## Phase 3: The Audio Processing Loop [COMPLETED]
- [x] Write the per-sample DSP loop (Fractional Delay -> Filter -> Feedback -> Write).
- [x] Implement linear interpolation for accurate pitch tuning.
- [x] Write the master block processing loop (Summation & Exciter routing).

## Phase 4: The Hardware "Hello World" [NEXT]
- [ ] Compile the minimal viable engine (Oscillator + VCA).
- [ ] Deploy to Drumlogue and verify CPU load and stability.
- [ ] Ensure sequencer and pitch wheel behave correctly.

## Phase 5: Envelopes & Exciters [PENDING]
- [ ] `envelope.h`: Implement a fast, branchless ADSR/AR struct.
- [ ] `noise.h`: Add a white/pink noise generator for breath, bow, and snare wire exciters.

## Phase 6: Filters & Master FX [PENDING]
- [ ] `filter.h`: Implement a 2-pole State Variable Filter (SVF) for the noise exciter and global output.
- [ ] Tie `header.c` Cutoff and Resonance parameters to the SVF.

## Phase 7: Waveguide Models & Tables [PENDING]
- [ ] `constants.h` & `tables.h`: Define fast-math lookup tables and tuning constants.
- [ ] `models.h`: Define the structural differences between "String", "Tube", and "Membrane" (e.g., swapping loop filters or adding secondary delay lines).
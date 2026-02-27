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

## Phase 3: The Audio Processing Loop [COMPLETED]
## Phase 4: The Hardware "Hello World" [READY FOR TEST]
- [x] `unit.cc` wrapper implemented with complete OS lifecycle compliance (`Init`, `Reset`, etc.).
- [ ] Compile the minimal viable engine.
- [ ] Deploy to Drumlogue and verify CPU load and stability.

## Phase 5: Envelopes & Exciters [READY FOR TEST]
- [x] `envelope.h`: Implement a fast, branchless AR/ADSR struct.
- [x] `noise.h`: Add a fast PRNG (Xorshift32) noise generator.
- [x] **Incremental Integration:** Modules added to `dsp_core.h` and `dsp_process.h` behind `#ifdef ENABLE_PHASE_5_EXCITERS` for safe debugging.

## Phase 6: Filters & Master FX [IN PROGRESS]
- [x] `filter.h`: Implement a 2-pole Chamberlin State Variable Filter (SVF).
- [x] Tie `header.c` Cutoff and Resonance to the Master SVF.
- [x] Integrate SVF incrementally into the audio loop.

## Phase 7: Waveguide Models & Tables [PENDING]
- [ ] `constants.h` & `tables.h`: Define fast-math lookup tables and tuning constants.
- [ ] `models.h`: Define the structural differences between "String", "Tube", and "Membrane" (e.g., swapping loop filters or adding secondary delay lines).

## Phase 8: Preset Design & Acoustic Tuning [PENDING]
- [ ] Reverse-engineer waveguide parameters for original 28 presets.
- [ ] Derive exact physics coefficients for new instruments: Timpani, Djambé, Taiko Drum, Marching Snare, Tam Tam, Koto.



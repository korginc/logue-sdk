# Project Status Tracker

## Phase 1: Core DSP Structures [COMPLETED]
- [x] Define flat, cache-friendly C++ structures (`Waveguide`, `Exciter`, `Voice`).
- [x] Establish memory bounds (4096-sample delay line for safe sub-bass).

## Phase 2: API & UI Binding [COMPLETED]
- [x] Connect `header.c` parameter indices to the engine.
- [x] Write `setParameter` translation logic (Pre-calculate all filter coefficients in the UI thread).

## Phase 3: The Audio Processing Loop [COMPLETED]
- [x] Write the per-sample DSP loop (Fractional Delay -> Filter -> Feedback -> Write).
- [x] Implement linear interpolation for accurate pitch tuning.
- [x] Write the master block processing loop (Summation & Exciter routing).

## Phase 4: Architectural Refactor & Sample Management [COMPLETED]

### Summary
The initial implementation of sample loading was found to be unreliable, leading to silence. The architecture has been refactored based on the working 'resonator' reference project to improve stability and robustness.

### Changes & Fixes
- **[FIXED]** Replaced the fragile, state-based sample loading mechanism with a "just-in-time" model. The `NoteOn` function now directly fetches sample data from the OS when a note is triggered.
- **[REMOVED]** Deleted the `loadConfigureSample` and `clearSampleState` functions, which were triggered by the UI, in favor of the more robust `NoteOn`-based loading.
- **[COMPLETED]** Added defensive checks to the `Init` and `NoteOn` functions to ensure all OS-provided sample API pointers are valid before use, preventing potential crashes.
- **[FIXED]** Corrected a parameter index mismatch between `header.c` and `synth_engine.h` that prevented sample bank and number changes from being recognized by the engine, causing sample loading to fail.

## Phase 5: Sound Generation & UI Integration [IN PROGRESS]

### Next Steps (To-Do)
- **[ ] Synthesizer First:** The synth engine should produce sound without samples, using the "Mallet" and "Noise" exciters.
    - **[ ] Enable Synthesis:** Uncomment the `#define ENABLE_PHASE_5_EXCITERS` in `dsp_core.h` to activate the existing noise generator.
    - **[ ] Implement Mallet:** Implement the mathematical "Mallet" exciter described in `README.md`.
    - **[ ] Implement Mixer:** Add a parameter to select or mix between Sample, Noise, and Mallet exciters.

- **[ ] Full UI Integration:**
    - **[COMPLETED]** Implement `unit_get_param_value`: In `unit.cc`, fully implement this function to report the current state of all parameters back to the UI.
    - **[COMPLETED]** Implement `unit_get_param_str_value`: In `unit.cc`, implement this to display the correct names for string-based parameters like the current sample, bank, and exciter type.
    - **[ ] Test & Verify:** Thoroughly test that all UI controls map correctly to the engine and that the screen accurately reflects the state of the synth.

## Phase 6: Filters & Master FX [IN PROGRESS]
- [x] `filter.h`: Implement a 2-pole Chamberlin State Variable Filter (SVF).
- [ ] Tie `header.c` Cutoff and Resonance to the Master SVF.
- [ ] Integrate SVF incrementally into the audio loop.

## Phase 7: Waveguide Models & Tables [PENDING]
- [ ] `constants.h` & `tables.h`: Define fast-math lookup tables and tuning constants.
- [ ] `models.h`: Define the structural differences between "String", "Tube", and "Membrane" (e.g., swapping loop filters or adding secondary delay lines).

## Phase 8: Preset Design & Acoustic Tuning [PENDING]
- [ ] Reverse-engineer waveguide parameters for original 28 presets.
- [ ] Derive exact physics coefficients for new instruments: Timpani, Djambé, Taiko Drum, Marching Snare, Tam Tam, Koto.

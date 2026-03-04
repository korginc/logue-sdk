# Project Status Tracker

## Phase 1: Core DSP Structures [COMPLETED]
- [x] Define flat, cache-friendly C++ structures (`Waveguide`, `Exciter`, `Voice`).
- [x] Establish memory bounds (4096-sample delay line for safe sub-bass).

## Phase 2: API & UI Binding [COMPLETED]
- [x] Connect `header.c` parameter indices to the engine.
- [x] Write `setParameter` translation logic.
- [x] Fix `MlltStif` buffer overflow bug in UI header.

## Phase 3: The Audio Processing Loop [COMPLETED]
- [x] Write the per-sample DSP loop.
- [x] Implement linear interpolation for accurate pitch tuning.

## Phase 4: Architectural Refactor & Sample Management [COMPLETED]
- [x] JIT sample loading inside `NoteOn` safely implemented.
- [x] Hardware "Hello World" successfully compiled and tested on Drumlogue!

## Phase 5: Envelopes & Exciters [COMPLETED]
- [x] `envelope.h` and `noise.h` implemented and tested on hardware.
- [x] Noise mix and PCM sample triggering confirmed working.

## Phase 6: Filters & Master FX [COMPLETED]
- [x] `filter.h`: Implement a 2-pole Chamberlin State Variable Filter (SVF).
- [x] Tie `header.c` LowCut and Resonance parameters to the Master SVF.
- [x] Integrate SVF incrementally into the audio loop.

## Phase 7: Waveguide Models & Tables [COMPLETED]
- [x] `tables.h`: Defined fast-math lookup tables for MIDI-to-Delay-Length conversion.
- [x] Implemented branchless Tube physics (Inverting Feedback via `phase_mult`).
- [x] Implemented Membrane physics (Inharmonic irrational detuning of Resonator B).
- [x] Linked UI `Model` parameter to physical topologies.

## Phase 8: Preset Design & Acoustic Tuning [PENDING]
- [ ] Reverse-engineer waveguide parameters for legacy 28 presets.
- [ ] Derive coefficients for new instruments: Timpani, Djambé, Taiko, Marching Snare, Tam Tam, Koto.

## Phase 9: UI Polish & Missing SDK Hooks [IN PROGRESS]
- [x] **Preset Management:** Implemented `LoadPreset(uint8_t idx)`, `getPresetIndex()`, and `getPresetName(uint8_t idx)` to properly display patch names on the Drumlogue OLED.
- [x] **State Reporting:** Implemented `getParameterValue(uint8_t id)` to track UI state across menu pages.
- [x] **Parameter Linkage:** Mapped all core `header.c` knobs inside `setParameter`. (Mallet and Models pending Phase 7/8).
- [x] **Release Phase Logic:** Implemented `NoteOff`, `GateOff`, and `AllNoteOff` callbacks. Applied Master Envelope as a VCA in the audio loop to properly choke ringing delay lines.
- [x] **The "Free Parameter" Decision:** Decide what to map to the open slot at Index 2 (Gain). (Options: Master Resonance, A/B Mix, or Limiter Drive). Resonance mapped instead of noiser filter Q and empty parameter is gain with limiter drive
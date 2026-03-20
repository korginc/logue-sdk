# ScrutaAstri - Project Status

## Current Situation
* Repository initialized and batch Python script created to ingest `AKWF` wavetables.
* Core DSP framework fully coded, integrating authentic hardware routing with newly developed morphing filters.
* Ready for hardware compilation and UI testing.

## Discoveries & Architecture Pivots
* **The `ino` File Audit:** Discovered that the original hardware uses a "Crush Sandwich" (Filter -> Bitcrusher -> Filter), linear Hz detuning for Osc 2, and uses LFO 3 exclusively as a VCA Tremolo.
* **Audio-Rate Modulation:** Implemented a system to use the active oscillator wavetable to dynamically destabilize the Sherman filter emulation at 48kHz, avoiding the C++ `setParameter` control-rate trap.

## Phase 1 & 2: Core DSP [COMPLETED]
- [x] Implement `WavetableOscillator` with linear interpolation.
- [x] Upgrade `FastLFO` with 6 shapes (including new ADSR-style `LFO_EXP_DECAY`) and exponential 0.01Hz to 1000Hz mapping.
- [x] Implement `MorphingFilter` (Clean -> Moog Saturation -> Sherman Asymmetry).
- [x] Build the Stargazer audio routing block in `synth.h`.
- [x] Write Python batch processor for wavetable header generation.

## Next Steps (Session 3: Hardware Tuning)
- [ ] **Compile and Flash:** Build the `.drumlgunit` and flash it to the Drumlogue.
- [ ] **Calibrate the Morph Boundaries:** Test the `CLEAN_MOOG_BORDER` (33) and `MOOG_SHE_BORDER` (66) on the hardware encoders to ensure the distortion ramps musically.
- [ ] **Verify LFO Curves:** Ensure the exponential mapping of the LFO rate knobs feels intuitive when sweeping from slow drones to audio-rate FM.
- [ ] **Check CPU Load:** The `MorphingFilter` has non-linear integrators. Verify the Drumlogue CPU can handle two of them running simultaneously alongside the wavetable interpolators without stuttering.

## Future Improvements / TODO
* **Stereo Spreading:** The Stargazer is strictly mono. We could easily offset the phase of LFO 2 for the Right channel to create a massive, swirling stereo drone.
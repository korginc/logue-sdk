# ScrutaAstri - Project Status

## Current Situation
* Core DSP framework, including authentic hardware routing and morphing filters, is fully coded and mathematically stabilized.
* Modulation matrix successfully expanded utilizing an Active Partial Counting (APC) block to evaluate complex routing targets every 4 frames, significantly reducing CPU cycles.
* Zero-crossing deferred frequency updates implemented to eliminate subharmonic popping artifacts.
* Bidirectional oscillator playback (reverse wavetables) logic integrated and exposed to the hardware UI via expanded preset ranges.
* Ready for hardware compilation and custom wavetable ingestion.

## Discoveries & Architecture Pivots
* **The `ino` File Audit:** Discovered that the original hardware uses a "Crush Sandwich" (Filter -> Bitcrusher -> Filter), linear Hz detuning for Osc 2, and uses LFO 3 exclusively as a VCA Tremolo.
* **Audio-Rate Modulation:** Implemented a system to use the active oscillator wavetable to dynamically destabilize the Sherman filter emulation at 48kHz, avoiding the C++ `setParameter` control-rate trap.
* **Filter Stability & Arithmetic Economy:** Standard linear filter topologies exploded under extreme audio-rate modulation. Resolved by clamping negative frequencies, restoring the Euler-forward Chamberlin SVF, and replacing expensive `std::tanh` calls with a fast cubic polynomial (`x - x^3/3`) to implement Moog/Sherman saturation directly inside the integrators.
* **Polivoks Chaos:** The `PolivoksFilter` requires explicit non-linear saturation in its resonance feedback loop and integrators to safely replicate the KR140UD12 op-amp overdrive without generating NaN crashes.
* **Bidirectional Playback Logic:** Duplicating wavetable arrays in memory is inefficient. Reversing playback is handled purely via negative phase increments triggered by preset index ranges (24-47: Osc 1 Rev, 48-71: Osc 2 Rev, 72-95: Both Rev).
* **Wavetable Format Protocol:** Lossy formats like MP3 introduce phase smearing and pre-echo, ruining audio-rate phase interpolation. All custom timbres must be ingested as uncompressed `.wav` files (PCM).

## Phase 1 & 2: Core DSP [COMPLETED]
- [x] Implement `WavetableOscillator` with linear interpolation.
- [x] Upgrade `FastLFO` with 6 shapes (including new ADSR-style `LFO_EXP_DECAY`) and exponential 0.01Hz to 1000Hz mapping utilizing fast approximations.
- [x] Implement `MorphingFilter` (Clean -> Moog Saturation -> Sherman Asymmetry).
- [x] Build the Stargazer audio routing block in `synth.h`.
- [x] Write Python batch processor for wavetable header generation.

## Phase 3: Debugging & Modulation Matrix [COMPLETED]
- [x] **Fix Parameter Parsing:** Correct the `k_paramF2Reso` assignment and `k_paramO2Detune` unipolar math.
- [x] **Fix Filter 1 Routing:** Consolidate the dynamic asymmetry logic to precede a single `filter1.process()` call per frame.
- [x] **Zero-Crossing Updates:** Implement deferred frequency updates for `k_paramO2SubOct` transitions to prevent pops.
- [x] **Modulation Matrix:** Expand the `mod_target = m_params[k_paramProgram] % 24` switch utilizing Active Partial Counting.
- [x] **Filter Hardening:** Implement bounds clamping and fast non-linear integrators in both SVF and Polivoks filters.
- [x] **Reverse Wavetables UI:** Update `header.c` `Program` max limit to `95` to expose reverse playback modes.

## Hardware Bugs Fixed

| Bug | Root cause | Fix |
|-----|-----------|-----|
| **CMOS knob dead zone 73–90%** | APC block overwrote `filter1.drive` every 4 samples with a resonance-derived value, discarding the CMOS parameter | Added `m_cmos_filter_drive` persistent member; APC now sums `m_cmos_filter_drive + m_f1_drive_base + m_drv1_mod_multiplier×5` |
| **LFO always sweeping filter** | `fasterpow2f(l1_val * 3.0f)` applied unconditionally to filter cutoff regardless of preset's LFO target | Replaced with gated `m_f1_lfo_mult` / `m_f2_lfo_mult` (default 1.0); only set ≠1.0 when the active preset targets F1/F2 cutoff |

## Phase 4: Drone / Noise Synth Engines (presets 95–96) [IN PROGRESS]

Two new engine types are reserved at the top of the preset range (95 = crystal drone, 96 = metal drone).
The character of each is inspired by the two reverb modes from the companion `reverb_labirinto` project:
esotico (crystal, diffuse harmonic cloud) and labirinto (metal, sharp metallic ring).

### Current implementation (drone_engine.h — parallel resonator bank, NOT self-oscillating)

A `DroneEngine` struct with 4 parallel biquad bandpass resonators driven by continuous LCG white noise.
Result: *colored noise* — the resonators filter the noise but do not sustain a pitched tone.
Root cause: no feedback from output back to input; the biquad resonates but decays when the noise floor
is not at the resonant frequency. This does **not** produce a self-oscillating drone.

### Design alternatives under evaluation

**Option A — Biquad sinusoidal oscillator bank (self-oscillating, recommended for purity)**

Replace the noise-driven resonators with second-order digital resonators (biquad oscillators):

```
y[n] = r × (2·cos(ω₀)·y[n-1] − y[n-2])
```

With `r = 0.9999` the poles lie fractionally inside the unit circle. The oscillator is self-sustaining,
produces a pure sinusoid at ω₀, and requires no external input. Four such oscillators at the chosen
partial ratios produce a drone chord.

- Crystal (preset 95): ratios {1.0, 1.5, 2.0, 3.15}, r = 0.99994 → smooth harmonic cloud
- Metal (preset 96): ratios {1.0, 1.6732, 2.4281, 3.5}, r = 0.99990 → metallic bell partials
- Memory: ~48 bytes per drone (trivial)
- Pitch: recompute cos(ω₀) on note change; no delay buffer
- Amplitude stability: normalize every 512 samples to prevent slow unit-circle drift
- Wavetable oscillators: bypassed; downstream filter chain (MorphingFilter, PolivoksFilter, CMOS tanh)
  still applies and provides timbral shaping
- Pros: true self-oscillation, exact pitch, near-zero memory
- Cons: source is pure sinusoids — timbral variety only from filter settings, not wavetable choice

**Option B — Wavetable oscillators feeding internal delay lines (comb/allpass resonator)**

Keep osc1/osc2 as the excitation source. Insert a feedback comb filter or allpass chain between the
oscillator mix and filter1:

```
Comb:    y[n] = x[n] + g · y[n − D]         (D = sr / base_hz, g ≈ 0.97)
Allpass: y[n] = −g·x[n] + x[n−D] + g·y[n−D]
```

The oscillator runs at low amplitude (~0.05–0.10) to excite the resonator; the comb amplifies the
resonant frequency and builds into a sustained drone. The chosen wavetable colours the timbre.

- Crystal (preset 95): allpass chain (2 sections), 15% dry + 85% resonator
- Metal (preset 96): feedback comb + one-pole LPF in feedback loop, 8% dry + 92% resonator
- Memory: ~3 KB per drone (D_max ≈ 1468 samples for MIDI 24+)
- LFO modulation of wavetable, oscillator frequency, filter sweep all remain meaningful
- Pros: preserves wavetable identity; LFO routing fully used; natural pitch tracking
- Cons: needs oscillator input to sustain (not purely self-oscillating); 6 KB extra RAM

### Decision pending hardware test

The final choice between A and B depends on the target character:
- Option A for a pure drone/pad that works from silence, with maximum filter-sculpting potential
- Option B for an evolving drone that retains wavetable-instrument personality
- Not mutually exclusive: Option A source could also feed the comb for additional body

## Next Steps
- [ ] **Choose and implement drone engine** (Option A or B — see section above)
- [ ] **Hardware test drone presets 95/96** after new implementation
- [ ] **Calibrate output gains** for drone engines to match level of wavetable presets
- [ ] **Ingest New Wavetables:** Extract uncompressed `.wav` cycles for the target timbres (low hum of distant prop plane, AUM singing, trombone, hurdy-gurdy). For evolving sounds, slice multiple sequential cycles.
- [ ] **Generate Headers:** Run the Python batch processor to convert the new `.wav` files into C-arrays and update `NUM_WAVETABLES` with new samples.
- [x] **Compile and Flash:** Build the `.drumlgunit` and flash it to the Drumlogue. DONE: works almost flawlessly
- [ ] **Calibrate the Morph Boundaries:** Test the `CLEAN_MOOG_BORDER` (33) and `MOOG_SHE_BORDER` (66) on the hardware encoders to ensure the distortion ramps musically.
- [ ] **Verify LFO Curves:** Ensure the exponential mapping of the LFO rate knobs feels intuitive when sweeping from slow drones to audio-rate FM.
- [ ] **Verify audio stepping:** Some parameters when change are doing a step in sound. While it's not crucial, a smooth transition would be preferable.
- [ ] **LFO rate modulation:** Let's try the new features first and then we can create a new modulation target when LFO rate is addressed by preset choice.
- [ ] **Check CPU Load:** Verify the Drumlogue ARM processor maintains thread execution time under the new load.

## Future Improvements / TODO
* **Stereo Spreading:** The Stargazer is strictly mono. Offset the phase of LFO 2 for the Right channel to create a massive, swirling stereo drone.
# Project Status Tracker
# Project Status Tracker

## Phase 1 to 9: [COMPLETED]
- Core DSP, sample loading, structural physical modeling, and OS lifecycles finished.

## Phase 10: Final Feature Polish & Restoration (TODO) [IN PROGRESS]
- [x] Fix `current_frame` bug causing DC-offset freeze on empty samples.
- [x] Remap Gain to `CoarsPtch` slot for 20x heavy clipping overdrive.
- [x] Expose `Model` and `Partials` to the OLED via `getParameterStrValue`.
- [ ] Parse `Model B` logic from the Model parameter (`value / 9`).
- [ ] Connect `Tube Radius` to adjust the waveguide's lowpass filter or dispersion.
- [ ] Restore the original 28 VST preset names and exact coefficients.


## Phase 11. Unused Parameters (Currently doing nothing)

Looking at your `header.c` and `synth_engine.h`, the following parameters are parsed but not actually affecting the audio thread yet:

* **`Tone` (Index 12):** Currently unmapped. *Improvement:* We can map this to a simple tilt-EQ inside the feedback loop, or a Master High-Shelf filter, allowing users to make the drum sound "darker" or "brighter" independently of the material.
* **`Partls` (Index 9):** In the old VST, this counted sine waves. In a Waveguide, partials don't exist in the same way. *Improvement:* We can repurpose this as "Complexity" or "Coupling"—controlling how aggressively Resonator B interferes with Resonator A.
* **`VlMllRes` & `VlMllStf` (Indices 6 & 7):** These are velocity modifiers. *Improvement:* A real drum sounds sharper when hit harder. We need to multiply the `MlltStif` and `MlltRes` by the incoming MIDI velocity so the drum responds dynamically to sequencer accents.
* **`NzFltr` & `NzFltFrq` (Indices 21 & 22):** You have a master SVF, but the noise exciter needs its own dedicated filter so you can simulate the low "thump" of a kick drum beater or the high "hiss" of snare wires.

---

## Phase 12. Missing Features & Improvements

To make this engine truly robust and CPU-efficient, we should add these features to our Phase 10 roadmap:

* **Dynamic Energy Squelch (CPU Optimization):** To save extra CPU cycles dynamically during playback, we shouldn't just rely on the release envelope to turn voices off. We should implement an auto-sleep function. By calculating the RMS energy of the waveguide, we can instantly set `is_active = false` the millisecond the physical vibration drops below the audible noise floor (e.g., `-80dB`).
* **Dedicated Noise SVF:** Instantiating a second `FastSVF` inside `ExciterState` that strictly filters the white noise *before* it hits the delay line.
* **Model B Parsing:** Your UI passes a single 0-17 integer for "Model." We are currently only extracting Model A (`value % 9`). We need to extract Model B (`value / 9`) and apply its specific topology to `resB`.
* **Pitch Bend:** The Korg SDK provides `unit_pitch_bend`. We need to wire this into the `NoteOn` delay length calculation so the drum pitch can be warped in real-time.

-----------------------------------------------------------

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
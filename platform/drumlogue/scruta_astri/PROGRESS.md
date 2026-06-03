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

## Phase 4: Drone / Noise Synth Engines (presets 95–96) [EXPERIMENTAL — NOT PRODUCTION READY]

> **Status:** The compact FDN engine is implemented and partially functional, but self-oscillation
> is **not reliably guaranteed** in all configurations. The root cause is an architectural conflict
> with the existing APC modulation system (see Known Issues below). Leaving as experimental until
> a dedicated drone APC mode is designed. Presets 95/96 are usable but behaviour is unpredictable
> depending on LFO settings.

Two new engine types are reserved at the top of the preset range (95 = crystal drone, 96 = metal drone).
The character of each is inspired by the two reverb modes from the companion `reverb_labirinto` project:
esotico (crystal, diffuse harmonic cloud) and labirinto (metal, sharp metallic ring).

### Current implementation (drone_engine.h — parallel resonator bank, NOT self-oscillating)

A `DroneEngine` struct with 4 parallel biquad bandpass resonators driven by continuous LCG white noise.
Result: *colored noise* — the resonators filter the noise but do not sustain a pitched tone.
Root cause: no feedback from output back to input; the biquad resonates but decays when the noise floor
is not at the resonant frequency. This does **not** produce a self-oscillating drone.

### Why the reverb sounded harsh, not organ-like

The NeonAdvancedLabirinto reverb (esotico/labirinto modes) produces its character through four
mechanisms working in combination — none of which a simple sinusoidal oscillator bank reproduces:

1. **Hadamard cross-coupling**: all 8 delay lines feed into all others via a Hadamard matrix.
   The intermodulation between lines generates dense, chaotic spectral content. Independent parallel
   oscillators have no coupling; the chaos disappears.
2. **Saturation in the feedback loop**: soft clip runs on every feedback cycle, injecting new harmonics
   on every pass. Each cycle the spectrum changes. Static oscillators generate static spectra.
3. **Inharmonic delay line lengths**: the 8 delay lines are tuned to room-acoustic prime-ish lengths,
   not to a musical fundamental + harmonics. The resonances are intentionally non-musical, creating
   dense, noisy buildup rather than a pitched chord.
4. **Allpass diffusion (crystal/esotico)**: the allpass cascade smears phases across the spectrum,
   spreading energy broadly and producing an airy, chaotic shimmer, not discrete pitches.

A pure biquad oscillator bank would sound like a clean organ. The FDN character requires at minimum:
cross-coupling + saturation + inharmonic resonances.

### Design alternatives under evaluation (revised — four options)

**Option A — Biquad sinusoidal oscillator bank**

Replace noise-driven resonators with second-order digital resonators:
```
y[n] = r × (2·cos(ω₀)·y[n-1] − y[n-2]),   r ≈ 0.9999
```
Self-sustaining pure sinusoid at ω₀. Four oscillators at chosen partial ratios produce a drone chord.

- Crystal: ratios {1.0, 1.5, 2.0, 3.15}, r = 0.99994
- Metal: ratios {1.0, 1.6732, 2.4281, 3.5}, r = 0.99990
- Memory: ~48 bytes per drone
- Pitch: recompute cos(ω₀) on note change only
- Stability: renormalize amplitude every 512 samples against unit-circle drift
- **Character: clean, organ/bell-like partials. Does NOT reproduce FDN harshness.**
- Pros: true self-oscillation, near-zero memory, exact pitch
- Cons: pure sinusoids — downstream filter chain is the only timbral variety. Wavetable bypassed.

**Option B — Wavetable oscillators feeding internal delay lines (comb / allpass resonator)**

Keep osc1/osc2 as excitation. Insert feedback comb/allpass between oscillator mix and filter1:
```
Comb:    y[n] = x[n] + g · y[n−D]            (D = sr/base_hz, g ≈ 0.97)
Allpass: y[n] = −g·x[n] + x[n−D] + g·y[n−D]
```
Oscillator runs at low amplitude (~0.05) to excite; comb builds resonance at the oscillator pitch.

- Crystal: 2-section allpass chain, 15% dry + 85% resonator output
- Metal: feedback comb + one-pole LPF in loop, 8% dry + 92% resonator
- Memory: ~3 KB per drone (delay buffer, D_max ≈ 1468 samples at MIDI 24)
- **Character: sustained resonant amplification of whatever the wavetable provides. Richer than pure
  sinusoids but does NOT reproduce cross-coupling or saturation chaos.**
- Pros: wavetable timbral identity preserved; LFO modulation fully used
- Cons: needs oscillator input to sustain; 6 KB RAM; still lacks FDN cross-coupling

**Option C — Compact self-oscillating FDN (reproduces the reverb character)**

A miniature Feedback Delay Network with the same ingredients that create the reverb's harsh texture:
- N = 4 short delay lines (20–80 samples each → resonant frequencies 600 Hz–2400 Hz at 48 kHz)
- 4×4 Hadamard mixing (scale 0.5) with cross-coupling on every sample
- Soft clip (cubic saturation) applied inside the feedback loop
- Feedback gain g ≈ 0.97–0.99 → self-sustaining, never fully decays
- Tiny noise injection (~0.001) to excite and prevent pure silence after long silence
- MIDI note does NOT directly set delay lengths (range mismatch); instead:
  - The note sets a post-FDN resonant filter (bandpass/LPF) tuned to base_hz
  - This pulls the specific harmonic of the FDN output closest to the desired pitch

Signal flow: noise_injection → FDN(short delays, Hadamard, saturation) → resonant_filter(MIDI) → out

Memory: 4 × 80 samples × 4 bytes = 1.28 KB per drone (trivial).
**Character: genuine cross-coupled, saturating, inharmonic noise texture shaped by MIDI filter —
closest reproduction of the reverb's harsh/noisy drone character.**

Pros: reproduces the actual FDN sound; self-oscillating without input; tiny memory
Cons: pitch is "suggested" by the filter, not exact (the FDN buzzes at its own resonances);
      requires careful stability management (feedback × Hadamard energy ≤ 1); more complex to implement

**Option D — Capture FDN output as new wavetables**

Offline workflow: run the FDN simulation in Python/C++ at a fixed fundamental pitch (e.g. 440 Hz,
delay lengths = sr/440 ≈ 109 samples per line), let it reach steady state, capture N consecutive
output cycles. Slice into individual wavetable cycles (256 samples each), normalize, and add to
wavetables.h as a new set of "FDN character" wavetable entries.

Capture multiple FDN states:
- Esotico (crystal) at different feedback gains: ~8–16 cycles = 8–16 new wavetable entries
- Labirinto (metal) at different saturation levels: ~8–16 cycles = 8–16 new wavetable entries

These wavetables then live inside the existing system, accessible via O1Wave/O2Wave knobs.
The LFO wavetable-sweep mode (existing feature) can scan through them slowly → timbral evolution
that approximates the FDN's dynamic character as a static-snapshot slideshow.

Presets 95/96 would simply set O1Wave to the first FDN-character wavetable index and use the
LFO sweep to animate through the FDN states.

Signal flow: FDN_wavetable → osc1 → (existing filter chain, LFO modulation, CMOS)
Memory: only wavetable ROM, no extra RAM. CPU: same as any other wavetable preset.
**Character: frozen snapshot of the FDN texture, with LFO animation giving slow evolution.
The full shaping chain (filters, CMOS, bitcrusher) applies on top.**

Pros: zero runtime cost; full parameter routing preserved; works with all existing LFO modulation;
      FDN character captured accurately offline without real-time stability concerns
Cons: temporal evolution of the FDN is approximated (discrete snapshots, not continuous chaos);
      requires an offline generation step (Python simulation); the FDN is inherently non-periodic,
      so capturing a clean single cycle requires careful phase alignment

### Parameter routing analysis per option

Current parameter assignments and how they map in each drone mode:

| Parameter | Normal wavetable use | Option A (biquad osc) | Option B (wavetable→comb) | Option C (FDN) | Option D (captured WT) |
|-----------|---------------------|----------------------|--------------------------|----------------|----------------------|
| O1Wave | Wavetable index | Unused → remap to partial mix ratio | Shapes comb excitation timbre | Unused → remap to FDN delay tuning | FDN wavetable index (the point) |
| O2Wave | Wavetable index | Unused → remap to saturation amount | Second comb layer timbre | Unused | Second FDN capture set |
| O2Detune | Hz detune | Unused → remap to partial detuning (chorus-like) | Comb resonance drift | FDN cross-coupling amount | WT index offset (LFO target) |
| O2SubOct | Sub-octave ratio | Could set octave of partials | Comb feedback overtone | FDN delay length scale | Playback octave |
| O2Mix | Osc2 blend | Unused → remap to dry/resonator ratio | Dry/comb mix | Noise injection level | Osc2 FDN blend |
| F1Cutoff | Filter cutoff | Post-processing (useful) | Post-processing (useful) | MIDI-pitch filter (critical for pitch) | Post-processing (useful) |
| F1Reso | Filter Q | Post-processing (useful) | Post-processing (useful) | Sharpness of pitch selection | Post-processing (useful) |
| F2Cutoff | Filter cutoff | Post-processing (useful) | Post-processing (useful) | Post-processing (useful) | Post-processing (useful) |
| CMOS | Distortion amount | Shapes sinusoidal partials interestingly | Extra saturation after comb | Stacks on top of FDN saturation | Shapes the captured texture |
| SRRed/BitRed | Bitcrusher | Could create digital granularity | Interesting on comb output | Very interesting on FDN | Adds lo-fi FDN character |
| LFO1-3 | Modulation | Only downstream filter/VCA | Full routing preserved | Noise inject modulation | Full routing preserved (esp. WT sweep) |

Key observations on parameter routing:
- Options A and C waste O1Wave/O2Wave (no wavetable selected). These should be remapped to
  engine-specific controls rather than left silent. Without remapping, 2 of 6 parameter pages
  become dead knobs for drone presets.
- Option D is the only one where O1Wave/O2Wave remain musically meaningful in the same way.
- F1Cutoff/F1Reso become the **primary pitch-sculpting tool** in Option C (the FDN post-filter
  determines what the user perceives as the drone's pitch). In all other options they are secondary.
- CMOS distortion stacks differently: in Option C it layers with FDN's internal saturation,
  potentially creating extreme grit. In Options A/D it is the primary distortion source.

### Combinatorial considerations

These options are not mutually exclusive:

- **C + D**: Run the compact FDN in real-time (Option C) for preset 95, and also generate offline
  FDN wavetables (Option D) that users can load as regular wavetable presets. Best of both worlds.
- **A + comb**: Use a sinusoidal oscillator bank (Option A) as the source feeding into a feedback
  comb (borrowing from Option B). The comb adds harmonic density and some FDN-like behavior.
- **D as first step**: Option D requires only Python work (no ARM code changes), so it can be
  done first as a low-risk experiment to test whether the captured FDN character is musically
  satisfying before committing to a real-time FDN implementation.

### Decision — Option C selected: Compact self-oscillating FDN

Design requirements resolved:
1. Exact MIDI pitch **not required** — post-FDN filter provides perceived pitch suggestion via F1Cutoff/F1Reso.
2. Live self-generative chaos **preferred** over static snapshots. A proper chaotic attractor evolves
   continuously; snapshot animation (Option D) is static in both the live-FDN and snapshot cases.
   Option D remains a future extension once Option C is working.
3. **All 24 knobs** should remain functional — achievable by remapping the 5 wavetable-specific
   parameters to FDN controls (see full routing table below).
4. Option D (captured wavetables) is **dependent** on Option C being implemented first. Once Option C
   runs on hardware the Python simulator can capture steady-state cycles via convert_wavetables.py.

#### Chaos mechanism

The compact FDN is a nonlinear dynamical system. Depending on feedback gain `g`:
- `g` low → decays to zero (stable)
- `g` moderate → periodic limit cycle (repeating waveform, sounds static)
- `g` above bifurcation threshold → **chaotic strange attractor** (trajectory never repeats, bounded
  by saturation — genuine self-generative evolution without parameter changes)
- `g` too high → large-amplitude saturation explosion (the reverb "screech")

The Hadamard cross-coupling is what enables chaos: the 4 delay lines mutually destabilize each other,
preventing convergence to a simple periodic orbit. The tanh saturation bounds the amplitude.

`O2Detune` (remapped in drone mode) becomes the **chaos level knob**:
- Left (-100): periodic / stable (predictable tone)
- Centre (0): calibrated chaotic attractor (self-generative, evolving, bounded)
- Right (+100): aggressive chaos / high-amplitude saturation (loud, dense noise)

#### FDN hardware architecture

```
                    ┌──────────────────────────────────────────────┐
noise_inject ──────►│  4×4 Hadamard mix (fast: 8 adds, 4 muls)    │
                    │  tanh saturation (drive = O2SubOct)          │
             ┌──────┤  4 delay lines (per-line one-pole filter)    ├──────┐
             │      └──────────────────────────────────────────────┘      │
             │ feedback g (controlled by O2Detune)                        │
             └────────────────────────────────────────────────────────────┘
                                                 │ output sum
                                                 ▼
                                        F1 filter (perceived pitch, MIDI Note → F1Cutoff)
                                                 │
                                        F2 filter, SRR, BRR, CMOS tanh VCA
```

Delay line specifications:
- Crystal (preset 95): 4 lines at lengths {61, 79, 83, 97} samples → resonant peaks at {787, 608, 578, 495} Hz
  Per-line one-pole LPF (smooth, diffuse character). Near-harmonic ratio set.
- Metal (preset 96): 4 lines at lengths {29, 37, 43, 53} samples → resonant peaks at {1655, 1297, 1116, 906} Hz
  Per-line one-pole BPF (emphasises upper mids). TR-808 metallic ratio set.

Memory per drone: 4 × 128 samples × 4 bytes = 2 KB. Total for both: ~4 KB.

Fast 4×4 Hadamard (no matrix multiply needed):
```
a = d[0]+d[1];  b = d[0]-d[1];  c = d[2]+d[3];  e = d[2]-d[3]
h[0]=a+c; h[1]=b+e; h[2]=a-c; h[3]=b-e;   each × 0.5
```

#### Full 24-parameter routing in drone mode

| # | Parameter (hardware name) | Normal wavetable use | Drone mode remapping |
|---|--------------------------|---------------------|---------------------|
| 0 | Prgrm | Preset / mod target | Stays (95 = crystal FDN, 96 = metal FDN) |
| 1 | Note | MIDI pitch of oscillators | Base frequency for post-FDN F1 filter (perceived pitch) |
| 2 | O1Wave | Wavetable index osc1 | FDN delay line base length → spectral register (dark ↔ bright) |
| 3 | O2Wave | Wavetable index osc2 | FDN delay line ratio set index (different inharmonicity patterns) |
| 4 | O2Dtun | Hz detune osc2 | **FDN feedback gain g** = chaos level (stable ↔ chaotic ↔ aggressive) |
| 5 | O2Sub | Sub-octave ratio | FDN saturation hardness (0=soft tanh, 1=cubic, 2=asymm, 3=fold, 4=hard) |
| 6 | O2Mix | Osc2 blend level | Noise injection level (0=pure self-oscillation, 100=high noise excitation) |
| 7 | Volume | Master volume | Stays |
| 8 | F1Cut | Filter 1 cutoff | Post-FDN spectral emphasis / perceived pitch shaping (critical in drone mode) |
| 9 | F1Res | Filter 1 Q | Pitch selectivity (high Q = more pitched; low Q = broader, noisier) |
| 10 | F2Cut | Filter 2 cutoff | Second spectral shaper |
| 11 | F2Res | Filter 2 Q | Second filter Q |
| 12 | L1Wave | LFO1 waveform | Stays (modulates drone parameters via mod_target) |
| 13 | L1Rate | LFO1 rate | Stays |
| 14 | L1Dpth | LFO1 depth | Stays |
| 15 | L2Wave | LFO2 waveform | Stays |
| 16 | L2Rate | LFO2 rate | Stays |
| 17 | L2Dpth | LFO2 depth | Stays |
| 18 | L3Wave | LFO3 waveform | Stays |
| 19 | L3Rate | LFO3 rate | Stays |
| 20 | L3Dpth | LFO3 depth | Stays |
| 21 | SRRed | Sample rate reduction | Stays — very interesting on FDN output (lo-fi chaos) |
| 22 | BitRed | Bit reduction | Stays — bit-crushed FDN creates very different textures |
| 23 | CMOS | Distortion | Stays — stacks on top of FDN internal saturation |

Interesting LFO modulation targets in drone mode (via mod_target = preset % 24):
- `k_paramF1Cutoff` (8): slowly sweeps the perceived pitch → pitch-evolving drone
- `k_paramO2Detune` (4): rhythmically modulates chaos level → stuttering / breathing chaos
- `k_paramOsc2Mix` (6): varies noise injection → density tremolo
- `k_paramCMOSDist` (23): distortion tremolo on FDN output → gating / pumping
- `k_paramBitRed` (22): bit-depth variation on chaotic signal → digital granularity

#### Known Issues (why this is experimental)

1. **APC mod_target conflict** — `mod_target = preset % 24` maps drone presets to APC slots
   designed for wavetable use:
   - Crystal (95 % 24 = 23 = `k_paramBitRed`): the APC block dynamically modulates the bit
     crusher depth via LFO2. This creates audible digital artifacts unrelated to the FDN.
   - Metal (96 % 24 = 0 = `k_paramProgram`): the APC block applies `m_volume_mod_multiplier =
     l3_val * 0.1f`, meaning **LFO3 directly amplitude-modulates the drone output**. Whether
     the drone is audible at all depends on LFO3's current phase and depth settings.

2. **Self-oscillation depends on LFO3 depth** — for preset 96, if LFO3 is active and at depth > 0,
   the volume can be periodically cut. This makes self-oscillation appear unreliable even though
   the FDN is internally oscillating.

3. **Fix required** — the processBlock APC switch needs a dedicated drone branch that bypasses
   the wavetable-oriented mod_target logic when `use_drone == true`. Until then, drone presets
   are sensitive to LFO configuration and behave unpredictably.

#### Option D as future follow-up

Once Option C is working on hardware: run the FDN simulation in Python at a fixed pitch (e.g. 440 Hz,
delay lengths scaled so D ≈ 109 samples per line at 48 kHz), let it reach the chaotic attractor,
capture 16–32 consecutive output cycles (256 samples each). These become additional wavetable entries
in wavetables.h. The existing LFO wavetable-sweep preset mode then animates through FDN states.
This requires extending convert_wavetables.py to accept raw float arrays in addition to .wav files.

## Next Steps
- [x] **Implement Option C: Compact self-oscillating FDN** — implemented in `drone_engine.h`.
  Hardware-calibrated: g=2.10, drive floor=1.20, noise_amp=0.0040. Self-oscillates reliably
  in isolation but behaviour is currently coupled to LFO3 via APC (see Known Issues).
- [ ] **Fix APC conflict for drone mode** — add `if (use_drone) { /* drone-specific APC */ }`
  branch in processBlock to decouple drone output from wavetable mod_target logic. Until then
  presets 95/96 are experimental.
- [ ] **Calibrate output gains** for drone engines to match level of wavetable presets.
- [ ] **(Future) Option D: FDN wavetable capture** — extend convert_wavetables.py, capture FDN
  steady-state cycles offline, add as new wavetable entries.
- [ ] **Ingest New Wavetables:** Extract uncompressed `.wav` cycles for target timbres (low hum of
  distant prop plane, AUM singing, trombone, hurdy-gurdy).
- [ ] **Generate Headers:** Run Python batch processor, update NUM_WAVETABLES.
- [x] **Compile and Flash:** Build `.drumlgunit` and flash to Drumlogue. DONE.
- [ ] **Calibrate Morph Boundaries:** Test CLEAN_MOOG_BORDER (33) and MOOG_SHERMAN_BORDER (66).
- [ ] **Verify LFO Curves:** Exponential LFO rate mapping should feel intuitive in hardware.
- [ ] **Verify audio stepping:** Smooth parameter transitions (not critical but preferred).
- [ ] **LFO rate modulation:** Design modulation target for LFO rate once other features are tested.
- [ ] **Check CPU Load:** Verify ARM Cortex-A7 thread execution time under FDN load.

## Future Improvements / TODO
* **Stereo Spreading:** The Stargazer is strictly mono. Offset phase of LFO 2 for Right channel.
* **Option D — FDN wavetable capture:** See Next Steps above.

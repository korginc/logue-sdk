# ScrutaAstri
A Korg drumlogue port of the Moffenzeef Stargazer drone synthesizer, supercharged with audio-rate modulations and morphing filters.

## Overview
ScrutaAstri (Italian for "Stargazer") transforms the Korg drumlogue into a continuous, evolving drone machine. It replicates the core architecture of the original hardware—utilizing wavetable synthesis, dual resonant filters, and aggressive LoFi distortion—while extending its capabilities with Drumlogue-exclusive features like audio-rate LFOs and Sherman-style filter asymmetry.



## The Authentic Signal Path
By analyzing the original Teensy Audio patches from the hardware's source code, ScrutaAstri replicates the exact "Crush Sandwich" routing of the Stargazer:
1. **Linear Detuned Oscillators:** Osc 1 and Osc 2 are mixed. Osc 2 detunes linearly (+/- 5Hz) for constant-speed phase beating.
2. **Filter 1 (Pre-Crush):** Lowpass SVF, modulated by LFO 1.
3. **The Crush Sandwich:** Sample Rate Reduction (SRR) and Bit Rate Reduction (BRR) process the filtered signal.
4. **Filter 2 (Post-Crush):** Lowpass SVF, modulated by LFO 2.
5. **Master VCA:** LFO 3 acts as a Tremolo (DC offset + LFO) on the final output.
6. **CMOS Distortion:** Emulated with an extreme mathematical soft-clipper.

## Drumlogue-Exclusive Enhancements
* **The Morphing Filter:** The `CMOSDist` parameter does more than add gain. It morphs the filters smoothly from Clean SVF (0-33) -> Moog-style symmetrical saturation (34-65) -> Sherman Filterbank asymmetrical chaos (66-100).
* **Audio-Rate Wavetable Asymmetry:** In the Sherman territory, the raw shape of Wavetable 1 is injected into Filter 2 at audio rates (48kHz), physically ripping the filter's DC offset apart based on the audio source.
* **Exponential LFOs:** LFO rates are exponentially mapped from glacial 100-second cycles (0.01Hz) up to audio-rate FM screams (1000Hz).
* **Percussive Shapes:** A new `LFO_EXP_DECAY` shape provides ADSR-like percussive strikes for the Master VCA.
* **Wavetable Morphing:** LFO 3 secretly sweeps the Osc 1 wavetable index across the 90-wave array for evolving textures.

## The Drone / Sequencer Concept
Unlike typical drumlogue synthesizers, ScrutaAstri operates in **Infinite Sustain** mode. The audio thread continuously calculates and outputs the drone. The step sequencer is primarily used for **Motion Sequencing**—automating LFO rates, filters, and distortion parameters per step while the drone screams continuously in the background.
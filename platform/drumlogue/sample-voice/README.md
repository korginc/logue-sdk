# SmplVox — drumlogue sample-playback drum voice

A `synth` unit for the **drumlogue** that is deliberately built around the
features that make the drumlogue different from every other logue-SDK target.

## What makes it drumlogue-specific

| Capability | How this unit uses it |
|---|---|
| **On-board PCM sample banks** | The drumlogue runtime descriptor exposes `get_num_sample_banks()`, `get_num_samples_for_bank()` and `get_sample()`. SmplVox browses those banks live, shows each sample's real name in the `SAMPLE` parameter, and resamples the selected `sample_wrapper_t` PCM data. No other logue platform lets custom code read the device's sample memory. |
| **Linux / Cortex-A7 runtime** | Units are ordinary shared objects, so the voice uses the C++ standard library (`<cmath>`, `std::round`, `std::tanh`) and float math freely instead of bare-metal MCU constraints. |
| **Gate-based drum triggering** | Hits arrive from the step sequencer via `unit_gate_on(velocity)` / `unit_gate_off()`, with per-step velocity. Keyboard `note_on` also works and sets playback pitch. |

## Signal flow

```
sample bank → linear-interp resampler → amp decay env × velocity
            → bit-depth reduction → drive/saturation → equal-power pan → out
```

The pitch-sweep envelope adds a decaying semitone offset to the resampler speed
for classic drum "punch".

## Parameters

| Page | Param | Notes |
|---|---|---|
| 1 | `BANK` | Sample bank index (display: `BK n`). |
| 1 | `SAMPLE` | Sample within the bank; shows the on-device sample name. |
| 1 | `PITCH` / `FINE` | Coarse (semi) and fine (cents) tuning of playback. |
| 2 | `DECAY` | Amplitude decay (~2 ms … 4 s). |
| 2 | `P.SWEEP` / `P.TIME` | Pitch-envelope depth (±48 semi) and time (~2 ms … 1 s). |
| 2 | `START` | Sample start offset (% of length). |
| 3 | `REVERSE` | Reverse playback. |
| 3 | `CRUSH` | Bit-depth reduction (16 → ~2 bits). |
| 3 | `DRIVE` | `tanh` saturation. |
| 3 | `PAN` | Equal-power pan. |

## Build

Same flow as the other drumlogue projects — use the Docker build environment
(`docker/run_interactive.sh`) and run `make` from this directory. Output is
`sample_voice.drmlgunit`.

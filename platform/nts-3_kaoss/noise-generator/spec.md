# Noise Generator Plugin Specification (NTS-3 Kaoss Pad)

## Overview
The Noise Generator is a "Generic FX" unit for the KORG NTS-3 Kaoss Pad Kit. It acts as a performance-oriented noise source that generates different spectral types of noise with real-time stereo panning and level control.

## Project Architecture
To successfully compile for the NTS-3, the project must follow this structure within the `logue-sdk/platform/nts-3_kaoss/` directory:

### Mandatory Files
- **`config.mk`**: Project configuration.
    - Sets `PROJECT := noise_generator`
    - Sets `PROJECT_TYPE := genericfx`
    - Lists source files (`UCSRC`, `UCXXSRC`).
- **`header.c`**: Unit metadata.
    - Specifies Developer ID and Unit ID.
    - Defines the `num_params` and the `params[]` descriptor array.
    - Defines `default_mappings` for X/Y/Depth hardware controls.
- **`unit.cc`**: SDK Interface.
    - Implements required callbacks: `unit_init`, `unit_render`, `unit_set_param_value`, and `unit_touch_event`.
    - Manages the lifecycle of the effect instance.
- **`effect.h`**: Modular DSP Logic.
    - Inherits from the `Processor` base class.
    - Handles internal parameter states and the actual audio buffer processing loop.

### Directory Dependencies
This unit depends on relative paths to the SDK's shared resources. It must be located at:
`logue-sdk/platform/nts-3_kaoss/noise-generator/`
So that it can access:
- `../common/`: Shared headers like `unit_genericfx.h`.
- `../ld/`: Linker scripts required for the ARM binary.

### Code Interface Requirements
- **Includes**: Must include `unit_genericfx.h` for NTS-3 generic effect definitions.
- **API Version**: Must utilize `UNIT_API_VERSION` provided by the SDK headers.
- **Sample Rate**: Hard-coded to 48000 Hz for the NTS-3 hardware.

## Functional Requirements
The plugin must provide the following features:
- **Real-time Noise Generation**: Continuous generation of White, Pink, and Brown noise.
- **Kaoss-Style Activation**: Audio is generated only when the user is touching the pad or when the effect is active and "Hold" is enabled.
- **Stereo Panning**: Control over the stereo balance of the generated noise.
- **Type Selection**: Seemless switching between spectral noise characteristics.
- **Volume Control**: Mastery over the output level of the generator.

## Parameter Specifications

| Parameter | Control Mapping | Range | Description |
| :--- | :--- | :--- | :--- |
| **LEVEL** | `DEPTH` Knob | 0 - 1023 | Controls the overall gain/volume of the noise. |
| **PAN** | **X-Axis** | 0 - 1023 | Controls the stereo position (Left to Right). |
| **TYPE** | **Y-Axis** | 0 - 2 | Selects noise type: 0=White, 1=Pink, 2=Brown. |

### Control Behaviors
- **Muting**: The effect should output silence when the touch pad is not being touched (unless the hardware's HOLD function is active).
- **Panning Law**: Implement a constant-power panning law to ensure the perceived volume remains consistent as the noise moves across the stereo field.

## Technical Implementation Details

### Noise Algorithms
- **White Noise**: Utilize the SDK's `osc_white()` function for uniform energy distribution across the frequency spectrum.
- **Pink Noise**: Implement an approximation filter (e.g., Paul Kellet's economy 3-pole filter) to achieve consistent power per octave (-3dB/octave slope).
- **Brown Noise**: Implement a leaky integrator (integral of white noise) to achieve a deeper, bass-heavy sound with power decreasing at -6dB/octave.

### Components
- `header.c`: Defines the unit metadata, target platform (NTS-3), and default parameter mappings.
- `effect.h`: The core C++ logic containing the `NoiseGenerator` class, noise algorithms, and state management.
- `unit.cc`: The boilerplate interface connecting the SDK callbacks (`unit_render`, `unit_touch_event`, etc.) to the `NoiseGenerator` class.
- `config.mk`: Project configuration for the build system.

## Data Flow & Signal Chain

The plugin follows a specific event and audio data flow handled by the `logue-sdk` runtime:

### 1. Control Input Flow (Events)
Users interact with the physical hardware, which flows through the software as follows:
- **Physical Interaction**: User touches the X/Y pad or adjusts the `DEPTH` knob.
- **Mapping (Firmware Level)**: The NTS-3 firmware looks at the `default_mappings` in `header.c` to determine which internal parameter ID corresponds to the physical control.
- **SDK Callback**: The firmware executes the `unit_set_param_value(id, value)` callback in `unit.cc`.
- **Processor Update**: `unit.cc` passes the new value to the `NoiseGenerator::setParameter(index, value)` method.
- **State Change**: The `NoiseGenerator` class updates its internal `Params` structure, which is then used by the next audio processing cycle.

### 2. Touch Interaction Flow
- **Direct Events**: Physical touch events are also sent via `unit_touch_event(id, phase, x, y)`.
- **Gating**: `unit.cc` calls `NoiseGenerator::touchEvent`, which sets the `is_touched` flag. This flag acts as a high-speed gate for the audio output.

### 3. Audio Processing Flow (Signal Chain)
The host requests audio rendering at a specific sample rate (48kHz):
- **Host Request**: The NTS-3 runtime calls `unit_render(in, out, frames)` in `unit.cc`.
- **DSP Loop**: `unit.cc` triggers `NoiseGenerator::process(in, out, frames)`:
    1. **Check State**: Verification of `is_active` and `is_touched` flags. If either is false, the buffer is zeroed (silence).
    2. **Noise Generation**: Sources white noise using `osc_white()`.
    3. **Filtering**: If `TYPE` is Pink or Brown, the white noise is passed through the respective recursive filters.
    4. **Gain & Pan**: The signal is multiplied by the `LEVEL` parameter and split into Left and Right channels using a constant-power panning law based on the `PAN` offset.
    5. **Buffering**: The final stereo samples are written into the interleaved `out` buffer.

## Build and Developer Guide

### Prerequisites
- KORG `logue-sdk` environment.
- Docker or Podman (for cross-compilation via `run_cmd.sh`).

### Build Process
From the root of the `logue-sdk` repository:
```bash
./docker/run_cmd.sh build nts-3_kaoss/noise-generator
```

### Installation
1. Locate the generated `noise_generator.nts3unit` in the project folder.
2. Use the KORG NTS-3 Librarian or `logue-cli` to upload the unit to the hardware.

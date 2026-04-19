# Auto Tune Plugin Specification (NTS-3 Kaoss Pad)

## Overview
The Auto Tune Plugin is a "Generic FX" unit for the KORG NTS-3 Kaoss Pad Kit. It processes incoming audio (typically vocals or monophonic instruments) and corrects its pitch to a specified musical scale and key. It allows for everything from subtle pitch correction to the "Hard-Tune" robotic effect.

## Project Architecture
To build this module, the project must follow the standard NTS-3 structure within `logue-sdk/platform/nts-3_kaoss/auto_tune/`:

### Mandatory Files
- **`config.mk`**: Sets project name (`PROJECT := auto_tune`), type (`genericfx`), and source lists.
- **`header.c`**: Contains the `unit_header_t` with Developer/Unit IDs and parameter descriptors.
- **`unit.cc`**: Implements the C-style callback bridge (`unit_init`, `unit_render`, etc.) to the C++ logic.
- **`effect.h`**: Modular DSP logic where all the audio processing and pitch analysis happens.

### Code Interface Requirements
- **Sample Rate**: Hard-coded to 48000 Hz.
- **Buffer Size**: Processes audio in blocks (typically 32-64 frames).
- **API**: Must include `unit_genericfx.h`.


## Functional Requirements
- **Real-time Pitch Detection**: Analyze the periodic waveform of the input audio to determine the current fundamental frequency ($f_{in}$).
- **Key & Scale Quantization**: Map the detected pitch to the nearest note in a user-selected musical key and scale.
- **Pitch Shifting**: Resample the audio in real-time to shift the pitch from the input frequency ($f_{in}$) to the target frequency ($f_{target}$).
- **Adjustable Retune Speed**: Control how quickly the pitch moves towards the target (Soft vs. Hard tuning).
- **Dry/Wet Mixing**: Blend the original signal with the tuned signal.

## Parameter Specifications

| Parameter | Control Mapping | Range | Description |
| :--- | :--- | :--- | :--- |
| **ROOT** | **X-Axis** | 0 - 11 | Sets the key/root note (0=C, 1=C#, ..., 11=B). |
| **SCALE** | **Y-Axis** | 0 - 5 | Selects scale: 0=Chrom, 1=Major, 2=Minor, 3=Dorian, 4=Mj Pen, 5=Mi Pen. |
| **SPEED** | **DEPTH** Knob | 0 - 1023 | Correction speed. 0 = Instant (Robot), 1023 = Slow/Natural. |
| **MIX** | Menu / Param 4 | 0 - 100% | Dry/Wet balance of the effect. |

## Technical Implementation Roadmap

### 1. Pitch Detection (The Input)
For programmer, the **AMDF (Average Magnitude Difference Function)** algorithm is recommended due to its computational efficiency on the NTS-3's ARM Cortex-M4 processor.

**How it works:**
1. Maintain a circular buffer of the last 1024 input samples.
2. For various "lags" (delay amounts $\tau$), calculate the sum of absolute differences between the current signal and the delayed signal:
   $AMDF(\tau) = \sum_{n=0}^{N-1} |x(n) - x(n-\tau)|$
3. The lag $\tau$ that produces the **lowest** value is the estimated period of the waveform.
4. $Frequency = \frac{SampleRate}{\tau}$.

**Beginner Tip:** You don't need to check every lag. To detect between 80Hz and 1000Hz, check lags ($\tau$) between 48 and 600 samples. Only perform this heavy calculation every 256 or 512 samples to save CPU; do not run it on every single sample.

### 2. Pitch Quantization (The Logic)
Once you have the detected frequency, convert it to a MIDI-style fractional note value:
$Note = 12 \times \log_2(\frac{Frequency}{440}) + 69$

1. Use the `quantizeToScale` logic (provided in the `note_api.h` common header) to find the target note in the selected scale.
2. Smooth the transition from the current pitch to the target pitch using a **Linear Smoother** to prevent clicks:
   `current_note = (alpha * target_note) + ((1.0f - alpha) * current_note);`
   The `SPEED` parameter should directly control the `alpha` value (smaller alpha = slower, more natural tuning).

### 3. Pitch Shifting (The Output)
To shift the pitch, you must read from a buffer at a different rate than you write to it.

1. **Write**: Store incoming samples into a 2048-sample circular buffer.
2. **Read**: Use a fractional read pointer (a `float`).
   - If the target is an octave higher, the read pointer moves twice as fast ($2.0 \times$).
   - If the target is lower, it moves slower ($< 1.0 \times$).
3. **Linear Interpolation**: To read at index `10.4`, you must mix index `10` and `11`:
   `sample = (buffer[10] * 0.6f) + (buffer[11] * 0.4f);`
4. **Windowed Overlap-Add (Optional but Recommended)**: Instead of a straight circular buffer, use two overlapping "windows" (taps). When one tap gets too far from the write pointer, fade into the other tap. This is the most complex part but prevents audible "clicks" when the read pointer loops.

## Component Implementation Details

### `header.c`
Define the parameter mapping so the hardware knows how to route the X/Y pads:
```c
.default_mappings = {
  .x_param_id = 0, // ROOT
  .y_param_id = 1, // SCALE
  .depth_param_id = 2, // SPEED
}
```

### `effect.h`
Create an `AutoTune` class that inherits from `Processor`.
- **`process()`**: The main loop. It should detect pitch every ~10ms (not every sample to save CPU), calculate the shift ratio, and then apply that ratio to the incoming stereo samples.
- **State Management**: Keep track of `current_phi` (read pointer phase) and `target_ratio`.

## Build Instructions
1. Ensure the `logue-sdk` environment is set up.
2. Place the code in `platform/nts-3_kaoss/auto_tune/`.
3. Run the build command:
```bash
./docker/run_cmd.sh build nts-3_kaoss/auto_tune
```
4. Load the generated `auto_tune.nts3unit` onto your NTS-3 using the Librarian.

---

### Tips for Success
- **Mono Detection**: Detect pitch only on the Left channel to save 50% CPU. Apply the same pitch shift to both Left and Right.
- **Hysteresis**: Add a small "threshold" for pitch detection so that background noise doesn't cause the auto-tuner to jump around randomly.
- **Reference**: Look at `platform/nts-3_kaoss/examples/x-to-note/note_api.h` for pre-written scale quantization code.
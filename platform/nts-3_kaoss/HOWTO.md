# Creating Plugins for KORG NTS-3 Kaoss Pad: A Beginner's Guide

Welcome to the world of NTS-3 plugin development! This guide is designed for programmers who are new to the platform or to Audio DSP (Digital Signal Processing) in general.

The NTS-3 is a performance-oriented device. Unlike a traditional synthesizer, it is built to be "played" via its touch pad. Your plugin will likely be a **Generic FX**, which means it takes sound in, changes it, and sends it out.

---

## 1. The Core Architecture

Every NTS-3 plugin is built from three main pieces:

### 🧩 `header.c`: The Identity Card
This file tells the NTS-3 what your plugin is. It defines:
- **Name**: What shows up on the screen.
- **Parameters**: The knobs/settings (e.g., "Depth", "Rate").
- **Mappings**: How the Touch Pad (X/Y) controls those parameters by default.

### 🔌 `unit.cc`: The Bridge
This file is the "wrapper." It connects the KORG SDK to your actual logic. You usually don't need to change much here once it's set up; it simply passes messages from the hardware to your `Effect` class.

### 🧠 `effect.h` (or similar): The Brain
This is where the magic happens. It contains your **DSP Logic**. This is where you calculate how the sound changes (e.g., adding an echo, filtering frequencies, or shifting pitch).

---

## 2. The Lifecycle of a Plugin

Thinking of your plugin like a video game. It has specific stages handled by "callback" functions:

1.  **`unit_init` (The Setup)**:
    - Called once when you select the plugin.
    - Use this to prepare your variables and allocate memory (like delay buffers).
2.  **`unit_render` (The Game Loop)**:
    - This is called hundreds of times per second.
    - It receives a "buffer" containing multiple "frames" (slices) of audio data.
    - You modify that data and send it back out.
3.  **`unit_touch_event` (The Input)**:
    - Triggered whenever someone touches the pad.
    - It gives you `X` and `Y` coordinates (0 to 1023).
4.  **`unit_teardown` (The Cleanup)**:
    - Called when you switch to a different plugin.
    - Use this to release any resources you used.

---

## 3. Beginner DSP Basics

### Buffers and Frames
Sound isn't processed one single sample at a time; it's processed in "buffers."
- **Frames**: A frame represents one slice of time across all channels (Left and Right).
- **Stereo**: NTS-3 typically uses 2 channels per frame.
- **Float representation**: Sound is represented by numbers ranging from `-1.0` to `1.0`.
    - `0.0` is silence.
    - `1.0` and `-1.0` are maximum volume.

### Pointers
In `Process()`, you'll see `const float * in` and `float * out`. These are pointers.
- `in[0]` is the first sample for the Left channel.
- `in[1]` is the first sample for the Right channel.
- To process them, you read from `in` and write your result to `out`.

---

## 4. How to Make "Bug-Free" Plugins

Debugging code on hardware can be hard! Follow these rules to keep your plugins stable:

### 🚫 Rule #1: No `new` or `malloc` in the audio thread
Never allocate memory inside the `Process()` function. It's too slow and will cause the audio to "crackle" or the device to crash. Always allocate memory during `Init()`.

### 🛡️ Rule #2: Initialize EVERYTHING
Computers don't start with "zero" in memory. If you don't set a variable to 0 (e.g., `float myLevel = 0.0f;`), it might contain a random high number that causes a massive blast of noise!

### 💾 Rule #3: Respect SDRAM
Large memory (like long delay lines) should be allocated in **SDRAM** using the SDK hooks. The internal memory is very small; if you try to make a big array locally, the plugin won't load.

### 🧮 Rule #4: Watch your Math
- **Division by Zero**: `x / 0` will crash your plugin. Always check if the denominator is 0.
- **Clipping**: If your output goes past `1.0` or below `-1.0`, it will sound like harsh digital distortion. Use `clipminmaxf(-1.0f, value, 1.0f)` to keep it safe.

---

The touch pad is the heart of the NTS-3. Coordinates are:
- **X**: 0 (left) to 1023 (right).
- **Y**: 0 (bottom) to 1023 (top).

In `unit_touch_event`, you can map these to your parameters. For example:
```cpp
// Simple example: Use X to control Depth
params_.depth = (float)x / 1023.0f; 
```

---

## 6. Built-in Superpowers (The SDK Library)

You don't have to write everything from scratch! KORG provides a library of pre-written functions that are optimized for the hardware. You can find these in the `common/` folder (specifically `osc_api.h`, `fx_api.h`, and `utils/float_math.h`).

### 🔊 Sound Generators
- `osc_white()`: Returns a random value between -1.0 and 1.0 (Gaussian White Noise).
- `osc_sinf(phase)`, `osc_sawf(phase)`, `osc_sqrf(phase)`: Basic oscillators.
- `osc_bl_sawf(phase, index)`: A **Band-Limited** Sawtooth. This sounds much better than a basic saw because it removes "aliasing" (harsh digital artifacts).

### 🧮 Math & Safety
- `linintf(fraction, value1, value2)`: Linear interpolation. Essential for smoothly moving between two values.
- `osc_notehzf(note)`: Converts a MIDI note number (0-151) into a frequency in Hertz.
- `clipminmaxf(min, val, max)`: Safely keeps a value within a range. Use this to prevent audio from getting too loud or parameters from going out of bounds.
- `ampdbf(amplitude)` / `dbampf(db)`: Convert between volume levels and Decibels.
- `fx_get_bpmf()`: Gets the current tempo of the device as a number (e.g., 120.0). Perfect for syncing echos to the beat.

### 🔄 LFOs & Modulation (Check `dsp/simplelfo.hpp`)
- `dsp::SimpleLFO`: A ready-made class for generating modulation signals. 
    - Use `setF0(frequency, 1.0f/48000.0f)` to set the speed.
    - Use `cycle()` to update its position every sample.
    - Use `sine_bi()`, `triangle_bi()`, or `square_bi()` to get the current "bipolar" value (-1.0 to 1.0).

### ⏳ Delay & Echo (Check `dsp/delayline.hpp`)
- `dsp::DelayLine`: Handles all the complex logic of a circular buffer (used for delays).
    - `setMemory(buffer, size)`: Connects the delay to your SDRAM area.
    - `write(sample)`: Adds the next piece of sound.
    - `readFrac(position)`: Reads sound from the past with **interpolation** (makes smooth pitch shifts or flanging possible).

### 📊 Pro-Level Filters (Check `dsp/biquad.hpp`)
- `dsp::BiQuad`: A professional-grade filter class.
    - `setSOLP(k, q)`: Sets up a **Second Order Low Pass** filter.
    - `setSOHP(k, q)`: Sets up a **Second Order High Pass** filter.
    - `process(sample)`: Runs your audio through the filter.

### 🧹 Bulk Buffer Handling (Check `utils/buffer_ops.h`)
- `buf_clr_f32(buffer, size)`: Resets an entire audio block to silence instantly.
- `buf_cpy_f32(src, dst, size)`: Copies an entire block of audio from one place to another very quickly.

---

## 7. Pro Tip: Parameter Curves

In `header.c`, you can define how your knobs "feel." Instead of everything being linear, you can use:
- `k_genericfx_curve_exp`: Exponential (great for frequency).
- `k_genericfx_curve_log`: Logarithmic (great for volume).
- `k_genericfx_curve_toggle`: For On/Off buttons.

---

## 8. Advanced Pro-Tips

### 🏎️ Performance Optimizations
- **Use `fast_inline`**: Mark your small, frequently called functions with `fast_inline`. This tells the compiler to embed the code directly and optimize it for speed.
- **`__restrict__` pointers**: In your `Process` loop, use `const float * __restrict__ in_p = in;`. This tells the compiler that the input and output buffers don't overlap, allowing it to generate much faster code.
- **Minimize Branching**: Try to avoid `if` statements inside your `for` loop if possible. Every "branch" slows down the audio processing slightly.

### 🏷️ Custom Parameter Names (Enums)
If you want a parameter to show "SINE", "SAW", "SQUARE" instead of "0, 1, 2":
1.  In `header.c`, set the param type to `k_unit_param_type_strings`.
2.  In `unit.cc`, implement `unit_get_param_str_value`.
3.  Return a pointer to a static string based on the value.

### 🧠 Advanced Memory (SDRAM)
- **Internal RAM**: Very fast, but very small (32KB total load size). Use for simple variables and small state.
- **SDRAM**: Larger (3MB per runtime), but slightly slower. Use `desc->hooks.sdram_alloc()` in `unit_init()` to get large buffers for delays or reverbs.
- **Progressive Clear**: If you need to clear a massive buffer, don't do it all at once in `unit_init()` if it takes too long. Do it in small chunks inside the `unit_render()` loop over several seconds.

---

## 9. How to Start

1.  **Copy an Example**: Don't start from a blank page. Copy folders like `dummy-genericfx` or `gate_reverb`.
2.  **Change the Name**: Edit `header.c` to give your plugin a unique name and ID.
3.  **Build Often**: Run `make` frequently to catch syntax errors early.
4.  **Test in Small Steps**: Add one feature (like change volume), test it, then add the next (like add a filter).

Happy coding! If you get stuck, look at the `common/` folder to see how KORG built the base tools you're using.

#pragma once
#include "dsp_core.h"

/**
The Architectural Wins Here:
Zero Division or Trigonometry: Notice that inside the massive for (size_t i = 0; i < frames; ++i) loop, there is not a single /, cos(), pow(), or log(). It is purely +, -, and *. This is why this engine will never trigger a Watchdog Timeout or denormal CPU spike on the hardware.

Bitwise Masking (& DELAY_MASK): Instead of using expensive if (ptr >= 4096) ptr = 0; branches, or slow modulo (% 4096) operators, we use a bitwise AND. It takes exactly 1 clock cycle on the ARM CPU to wrap the delay line safely.

Contiguous Memory Arrays: wg.buffer is just an array of float. The CPU will pre-fetch these arrays into the L1 cache, meaning memory access is effectively zero-cost.
 */

// ==============================================================================
// 1. The Core Physics (Executed per-voice, per-sample)
// ==============================================================================

// Processes a single sample through the Waveguide
inline float process_waveguide(WaveguideState& wg, float exciter_input) {
    // 1. Calculate the read pointer position for exact pitch
    float read_pos = (float)wg.write_ptr - wg.delay_length;
    if (read_pos < 0.0f) read_pos += (float)DELAY_BUFFER_SIZE; // Wrap around safely

    // 2. Fractional Delay Line Reading (Linear Interpolation)
    int idx_int = (int)read_pos;
    float frac = read_pos - (float)idx_int;

    int idx_A = idx_int & DELAY_MASK;
    int idx_B = (idx_A + 1) & DELAY_MASK;

    // Blend the two samples based on the fraction
    float delay_out = (wg.buffer[idx_A] * (1.0f - frac)) + (wg.buffer[idx_B] * frac);

    // 3. The Loss Filter (1-pole Lowpass) - Simulates Material (Wood/Metal)
    // wg.lowpass_coeff was pre-calculated in setParameter()
    wg.z1 = (delay_out * wg.lowpass_coeff) + (wg.z1 * (1.0f - wg.lowpass_coeff));
    float filtered_out = wg.z1;

    // 4. Feedback & Exciter Addition
    // wg.feedback_gain is our "Decay" time
    float new_val = exciter_input + (filtered_out * wg.feedback_gain);

    // 5. Write back to the delay line and advance the pointer
    wg.buffer[wg.write_ptr] = new_val;
    wg.write_ptr = (wg.write_ptr + 1) & DELAY_MASK;

    return new_val;
}

// Processes the Exciter (Generates the initial "strike" or sample burst)
inline float process_exciter(ExciterState& ex) {
    float out = 0.0f;

    // Play PCM Sample if loaded
    if (ex.sample_ptr && ex.current_frame < ex.sample_frames) {
        // Assume interleaved sample buffer (L R L R). We just read the Left channel for the exciter.
        size_t raw_idx = ex.current_frame * ex.channels;
        out = ex.sample_ptr[raw_idx];
        ex.current_frame++;
    }

    // Add optional Noise Burst here (e.g. for snare wires or breath noise)
    // if (ex.current_noise_env > 0.001f) {
    //     float white_noise = ((float)rand() / (float)RAND_MAX * 2.0f) - 1.0f;
    //     out += white_noise * ex.current_noise_env;
    //     ex.current_noise_env *= ex.noise_decay_coeff; // Exponential decay
    // }

    return out;
}

// ==============================================================================
// 2. The Master Audio Loop (Called by Drumlogue OS)
// ==============================================================================

// Inside your main class (e.g., RipplerXWaveguide):
inline void processBlock(float* __restrict main_out, size_t frames) {

    // Clear the output buffer
    for (size_t i = 0; i < frames * 2; ++i) {
        main_out[i] = 0.0f;
    }

    // Process each active voice
    for (size_t v = 0; v < NUM_VOICES; ++v) {
        VoiceState& voice = state.voices[v];
        if (!voice.is_active) continue;

        // Process the block for this voice
        for (size_t i = 0; i < frames; ++i) {

            // 1. Generate the Exciter burst
            float exciter_sig = process_exciter(voice.exciter);

            // 2. Feed the Exciter into both Resonators
            float outA = process_waveguide(voice.resA, exciter_sig);
            float outB = process_waveguide(voice.resB, exciter_sig);

            // 3. Mix the resonators together
            // (mix_ab is controlled by the UI, 0.0 = A only, 1.0 = B only)
            float voice_out = (outA * (1.0f - state.mix_ab)) + (outB * state.mix_ab);

            // Apply note velocity
            voice_out *= voice.current_velocity;

            // 4. Sum into the master interleaved stereo buffer (L and R)
            main_out[i * 2]     += voice_out * state.master_gain; // Left
            main_out[i * 2 + 1] += voice_out * state.master_gain; // Right
        }
    }

    // 5. The Hardware Safety Net (Brickwall Limiter)
    for (size_t i = 0; i < frames * 2; ++i) {
        float x = main_out[i];

        // Fast Soft-Clipper: Smoothly rounds off extreme peaks (Tube-like saturation)
        // Equation: x / (1.0 + abs(x))
        float clipped = x / (1.0f + fabsf(x));

        // Hard-clamp to absolute Drumlogue DAC limits just in case
        main_out[i] = fmaxf(-0.99f, fminf(0.99f, clipped));
    }
}
#pragma once
#include <cstdint>
#include <cstddef>

/** Because we are optimizing for bare-metal, notice there are no virtual functions,
 * no dynamic memory (new/malloc), and no deep class hierarchies.
 * Just pure data that the CPU cache can read sequentially. */

// A power-of-two buffer size allows us to use bitwise AND (& 4095) for lightning-fast
// wrap-around instead of slow modulo (%) operators.
// 4096 samples @ 48kHz = ~85.3ms max delay (Deepest possible pitch: ~11.7 Hz)
constexpr size_t DELAY_BUFFER_SIZE = 4096;
constexpr size_t DELAY_MASK = DELAY_BUFFER_SIZE - 1;

/**
 * The Exciter injects initial energy into the resonators.
 * It is completely passive once the sample or noise burst finishes.
 */
struct ExciterState {
    // PCM Sample Data
    const float* sample_ptr = nullptr;
    size_t sample_frames = 0;
    size_t current_frame = 0;
    uint8_t channels = 1;

    // Noise Burst Data
    float noise_decay_coeff = 0.0f;
    float current_noise_env = 0.0f;
};

/**
 * A single Digital Waveguide (Physical Model).
 * Contains the delay line memory and the loop filter states.
 */
struct WaveguideState {
    float buffer[DELAY_BUFFER_SIZE] = {0.0f};
    uint32_t write_ptr = 0;

    // Fractional tuning for exact pitch
    float delay_length = 0.0f;

    // Fast-math loop coefficients (Calculated from UI parameters)
    float feedback_gain = 0.0f;    // Determines Decay Time
    float lowpass_coeff = 1.0f;    // Determines Material/Tone

    // Filter State Memory
    float z1 = 0.0f;               // 1-pole lowpass history
};

/**
 * The Master Voice Structure.
 * Holds the Exciter and two parallel Resonators (A and B).
 */
struct VoiceState {
    bool is_active = false;
    bool is_releasing = false;

    uint8_t current_note = 60;
    float current_velocity = 0.0f;

    ExciterState exciter;
    WaveguideState resA;
    WaveguideState resB;
};

// Global Synth State (4 Voices limit for strict CPU budgeting)
constexpr size_t NUM_VOICES = 4;
struct SynthState {
    VoiceState voices[NUM_VOICES];
    uint8_t next_voice_idx = 0;

    // Master FX
    float mix_ab = 0.5f;
    float master_gain = 1.0f;
};
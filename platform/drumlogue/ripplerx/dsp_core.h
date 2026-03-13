#pragma once
#include <cstdint>
#include <cstddef>


// =================================================================
// UNCOMMENT THIS LINE TO ACTIVATE PHASE 5 (NOISE & ENVELOPES)
#define ENABLE_PHASE_5_EXCITERS 1
// =================================================================

#ifdef ENABLE_PHASE_5_EXCITERS
#include "noise.h"
#include "envelope.h"
#endif

// =================================================================
// UNCOMMENT THIS LINE TO ACTIVATE PHASE 6 (MASTER FILTER)
#define ENABLE_PHASE_6_FILTERS 1
// =================================================================

#ifdef ENABLE_PHASE_6_FILTERS
#include "filter.h"
#endif

// =================================================================
// UNCOMMENT THIS LINE TO ACTIVATE PHASE 7 (MODELS & TABLES)
#define ENABLE_PHASE_7_MODELS 1
// =================================================================

#ifdef ENABLE_PHASE_7_MODELS
#include "tables.h"
#endif



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

#ifdef ENABLE_PHASE_5_EXCITERS
    FastNoise noise_gen;
    FastEnvelope noise_env;
    FastEnvelope master_env; // Optional: To choke the whole voice on GateOff
#endif

    // Noise Burst Data
    float noise_decay_coeff = 0.0f;
    float current_noise_env = 0.0f;

    float mallet_lp = 0.0f;
    float mallet_lp2 = 0.0f;       // Second LP pole state (MlltRes)
    float mallet_stiffness = 0.5f;
    float mallet_res_coeff = 0.5f; // Second LP pole coefficient (MlltRes)

#ifdef ENABLE_PHASE_6_FILTERS
    FastSVF noise_filter; // Dedicated per-voice noise shaping SVF (NzFltr / NzFltFrq)
#endif
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
    float ap_coeff = 0.0f; // Allpass coefficient (-0.99 to 0.99)
    float ap_x1 = 0.0f;    // Allpass delayed input
    float ap_y1 = 0.0f;    // Allpass delayed output

    // Filter State Memory
    float z1 = 0.0f;               // 1-pole lowpass history

#ifdef ENABLE_PHASE_7_MODELS
    // Physics Topology Multiplier (+1.0f for String, -1.0f for Tube)
    float phase_mult = 1.0f;
#endif
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

    // Coupling and Tone memory
    float resB_out_prev = 0.0f; // ResB output from previous sample (bidirectional coupling)
    float tone_lp = 0.0f;       // 1-pole LP state for the Tone tilt EQ

    // Pitch Bend: pre-bend delay lengths so PitchBend() can reapply the multiplier
    // without accumulating error across successive bend messages.
    float base_delay_A = 0.0f;
    float base_delay_B = 0.0f;

    // Dynamic Energy Squelch: 1-pole absolute-value envelope follower.
    // Smoothing: α=0.01 → τ ≈ 100 samples ≈ 2 ms at 48 kHz.
    float rms_env = 0.0f;
};

// Global Synth State (4 Voices limit for strict CPU budgeting)
constexpr size_t NUM_VOICES = 4;
struct SynthState {
    VoiceState voices[NUM_VOICES];
    uint8_t next_voice_idx = 0;

    // Master FX
    float mix_ab = 0.5f;
    float master_gain = 1.0f;
    float master_drive = 1.0f;
    float tone = 0.0f;        // Tilt EQ amount, cached from k_paramTone [-10, 30]

#ifdef ENABLE_PHASE_6_FILTERS
    FastSVF master_filter;
#endif
};
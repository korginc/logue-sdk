#pragma once

/**
 * @file constants.h
 * @brief Central constants for FM Percussion Synth
 *
 * Version 2.0 - Complete and fixed with all necessary constants
 *
 * This file serves as the single source of truth for all
 * magic numbers used throughout the FM Percussion Synth project.
 *
 * Includes:
 * - NEON optimization constants
 * - Audio and MIDI conversion constants
 * - Parameter indexes (24 parameters across 6 pages)
 * - Voice allocation constants (12 valid combinations)
 * - Resonant synthesis constants (modes, morph ranges)
 * - LFO target constants (0-7 including resonant)
 * - FM engine parameter ranges
 * - Envelope ROM constants
 */

#include <cstdint>
#include <cmath>

// ============================================================================
// NEON Optimization Constants
// ============================================================================
constexpr int NEON_LANES = 4;           // 4x32-bit floats per NEON register
constexpr int VECTOR_ALIGN = 16;         // 16-byte alignment for NEON
constexpr int CACHE_LINE_SIZE = 64;      // ARM Cortex-A9 cache line size

// ============================================================================
// Audio System Constants
// ============================================================================
constexpr float SAMPLE_RATE = 48000.0f;      // Fixed sample rate for drumlogue
constexpr float INV_SAMPLE_RATE = 1.0f / SAMPLE_RATE;
constexpr float NYQUIST_FREQ = SAMPLE_RATE / 2.0f;

// ============================================================================
// MIDI Note to Frequency Conversion
// ============================================================================
constexpr float A4_FREQ = 440.0f;            // A4 reference frequency
constexpr int A4_MIDI = 69;                   // A4 MIDI note number
constexpr float SEMITONE_RATIO = 1.0594630943592953f;  // 2^(1/12)

// Pre-calculated interval ratios for common intervals
constexpr float INTERVAL_UNISON    = 1.0f;               // 0 semitones
constexpr float INTERVAL_MINOR_2ND = 1.059463094f;       // 1 semitone
constexpr float INTERVAL_MAJOR_2ND = 1.122462048f;       // 2 semitones
constexpr float INTERVAL_MINOR_3RD = 1.189207115f;       // 3 semitones
constexpr float INTERVAL_MAJOR_3RD = 1.259921050f;       // 4 semitones (2^(4/12))
constexpr float INTERVAL_PERFECT_4TH = 1.334839854f;     // 5 semitones
constexpr float INTERVAL_TRITONE    = 1.414213562f;      // 6 semitones
constexpr float INTERVAL_PERFECT_5TH = 1.498307077f;     // 7 semitones (2^(7/12))
constexpr float INTERVAL_MINOR_6TH = 1.587401052f;       // 8 semitones
constexpr float INTERVAL_MAJOR_6TH = 1.681792830f;       // 9 semitones
constexpr float INTERVAL_MINOR_7TH = 1.781797436f;       // 10 semitones
constexpr float INTERVAL_MAJOR_7TH = 1.887748625f;       // 11 semitones
constexpr float INTERVAL_OCTAVE    = 2.0f;               // 12 semitones

// ============================================================================
// Parameter Indexes - 24 parameters across 6 pages
// ============================================================================

// Page 1: Voice Probabilities (4 params)
constexpr uint8_t PARAM_VOICE1_PROB = 0;     // Voice 0 probability (0-100%)
constexpr uint8_t PARAM_VOICE2_PROB = 1;     // Voice 1 probability (0-100%)
constexpr uint8_t PARAM_VOICE3_PROB = 2;     // Voice 2 probability (0-100%)
constexpr uint8_t PARAM_VOICE4_PROB = 3;     // Voice 3 probability (0-100%)

// Page 2: Kick + Snare Parameters (4 params)
constexpr uint8_t PARAM_KICK_SWEEP = 4;      // Kick sweep depth (0-100%)
constexpr uint8_t PARAM_KICK_DECAY = 5;      // Kick decay shape (0-100%)
constexpr uint8_t PARAM_SNARE_NOISE = 6;     // Snare noise mix (0-100%)
constexpr uint8_t PARAM_SNARE_BODY = 7;      // Snare body resonance (0-100%)

// Page 3: Metal + Perc Parameters (4 params)
constexpr uint8_t PARAM_METAL_INHARM = 8;    // Metal inharmonicity (0-100%)
constexpr uint8_t PARAM_METAL_BRIGHT = 9;    // Metal brightness (0-100%)
constexpr uint8_t PARAM_PERC_RATIO = 10;     // Perc ratio center (0-100%)
constexpr uint8_t PARAM_PERC_VAR = 11;       // Perc variation (0-100%)

// Page 4: LFO1 Configuration (4 params)
constexpr uint8_t PARAM_LFO1_SHAPE = 12;     // LFO1 shape combo (0-8)
constexpr uint8_t PARAM_LFO1_RATE = 13;      // LFO1 rate (0-100%)
constexpr uint8_t PARAM_LFO1_TARGET = 14;    // LFO1 target (0-7)
constexpr uint8_t PARAM_LFO1_DEPTH = 15;     // LFO1 depth (0-100%, stored as 0-200 for bipolar)

// Page 5: LFO2 Configuration (4 params)
constexpr uint8_t PARAM_LFO2_SHAPE = 16;     // LFO2 shape combo (0-8)
constexpr uint8_t PARAM_LFO2_RATE = 17;      // LFO2 rate (0-100%)
constexpr uint8_t PARAM_LFO2_TARGET = 18;    // LFO2 target (0-7)
constexpr uint8_t PARAM_LFO2_DEPTH = 19;     // LFO2 depth (0-100%, stored as 0-200 for bipolar)

// Page 6: Envelope + Voice + Resonant (4 params)
constexpr uint8_t PARAM_ENV_SHAPE = 20;      // Envelope shape (0-127)
constexpr uint8_t PARAM_VOICE_ALLOC = 21;    // Voice allocation (0-11)
constexpr uint8_t PARAM_RES_MODE = 22;       // Resonant mode (0-4)
constexpr uint8_t PARAM_RES_MORPH = 23;      // Resonant morph (0-100%)

// ============================================================================
// LFO Target Constants
// ============================================================================
constexpr int LFO_TARGET_NONE = 0;
constexpr int LFO_TARGET_PITCH = 1;
constexpr int LFO_TARGET_INDEX = 2;
constexpr int LFO_TARGET_ENV = 3;
constexpr int LFO_TARGET_LFO2_PHASE = 4;
constexpr int LFO_TARGET_LFO1_PHASE = 5;
constexpr int LFO_TARGET_RES_FREQ = 6;
constexpr int LFO_TARGET_RESONANCE = 7;
constexpr int LFO_TARGET_COUNT = 8;

// ============================================================================
// LFO Shape Constants
// ============================================================================
constexpr int LFO_SHAPE_TRIANGLE = 0;
constexpr int LFO_SHAPE_RAMP = 1;
constexpr int LFO_SHAPE_CHORD = 2;
constexpr int LFO_SHAPE_COMBO_COUNT = 9;       // 3×3 = 9 combinations

// Phase offset to prevent cancellation (90° = 0.25 cycle)
constexpr float LFO_PHASE_OFFSET = 0.25f;

// LFO rate range
constexpr float LFO_RATE_MIN = 0.1f;           // 0.1 Hz
constexpr float LFO_RATE_MAX = 10.0f;          // 10 Hz

// ============================================================================
// Voice Allocation Constants
// ============================================================================
constexpr int VOICE_ALLOC_COUNT = 12;        // 12 valid allocations
constexpr int VOICE_ALLOC_MIN = 0;
constexpr int VOICE_ALLOC_MAX = 11;

// ============================================================================
// Engine Mode Constants
// ============================================================================
constexpr int ENGINE_KICK = 0;
constexpr int ENGINE_SNARE = 1;
constexpr int ENGINE_METAL = 2;
constexpr int ENGINE_PERC = 3;
constexpr int ENGINE_RESONANT = 4;
constexpr int ENGINE_COUNT = 5;  // Total number of engines

// ============================================================================
// Resonant Synthesis Constants
// ============================================================================

// Resonant mode types (0-4)
constexpr int RES_MODE_LOWPASS = 0;             // Single-sided low-pass
constexpr int RES_MODE_BANDPASS = 1;            // Double-sided band-pass
constexpr int RES_MODE_HIGHPASS = 2;            // Derived high-pass
constexpr int RES_MODE_NOTCH = 3;                // Notch filter
constexpr int RES_MODE_PEAK = 4;                 // Peak filter
constexpr int RES_MODE_COUNT = 5;                 // Total resonant modes

// Resonant parameter ranges
constexpr float RES_FC_MIN = 50.0f;               // Minimum center frequency (Hz)
constexpr float RES_FC_MAX = 8000.0f;             // Maximum center frequency (Hz)
constexpr float RES_FC_DEFAULT = 1000.0f;         // Default center frequency

constexpr float RES_RESONANCE_MIN = 0.1f;         // Minimum resonance (a parameter)
constexpr float RES_RESONANCE_MAX = 0.9f;         // Maximum resonance (a parameter)
constexpr float RES_RESONANCE_DEFAULT = 0.5f;

constexpr float RES_GAIN_MIN = 1.0f;              // Minimum gain for peak mode
constexpr float RES_GAIN_MAX = 4.0f;              // Maximum gain for peak mode
constexpr float RES_GAIN_DEFAULT = 1.0f;

// Safety epsilon to prevent division by zero
constexpr float RESONANT_DENOM_EPSILON = 0.0001f;


// Phase offset to prevent cancellation (90° = 0.25 cycle)
constexpr int LFO_PHASE_OFFSET = 0.25f;
constexpr int LFO_SHAPE_TRIANGLE = 0;
constexpr int LFO_SHAPE_RAMP = 1;
constexpr int LFO_SHAPE_CHORD = 2;
// LFO Depth range (bipolar)
constexpr int LFO_DEPTH_MIN = -100;
constexpr int LFO_DEPTH_MAX = 100;
constexpr int LFO_DEPTH_DEFAULT = 50;  // +50% modulation

// LFO Rate range (unipolar)
constexpr int LFO_RATE_MIN = 0;
constexpr int LFO_RATE_MAX = 100;
constexpr int LFO_RATE_DEFAULT = 30;
// ============================================================================
// Envelope ROM Constants
// ============================================================================
constexpr int ENV_SHAPE_MIN = 0;
constexpr int ENV_SHAPE_MAX = 127;
constexpr int ENV_SHAPE_DEFAULT = 40;
constexpr int ENV_ROM_SIZE = 128;                // 128 predefined envelope shapes

// Envelope stages
constexpr int ENV_STAGE_ATTACK = 0;
constexpr int ENV_STAGE_DECAY = 1;
constexpr int ENV_STAGE_RELEASE = 2;
constexpr int ENV_STATE_OFF = 3;

// Envelope curve types
constexpr int ENV_CURVE_LINEAR = 0;
constexpr int ENV_CURVE_EXPONENTIAL = 1;

// ============================================================================
// PRNG (Pseudo-Random Number Generator) Constants
// ============================================================================
constexpr uint32_t PRNG_DEFAULT_SEED = 0x9E3779B9;  // Golden ratio constant

// ============================================================================
// Parameter Smoothing Constants
// ============================================================================
constexpr int SMOOTH_FRAMES = 48;                  // 1ms ramp at 48kHz
constexpr float SMOOTH_COEFF_MIN = 0.001f;
constexpr float SMOOTH_COEFF_MAX = 0.5f;

// ============================================================================
// Delay Line Constants (for Percussion Spatializer)
// ============================================================================
constexpr int DELAY_MAX_MS = 500;                  // Maximum delay in milliseconds
constexpr int DELAY_MAX_SAMPLES = DELAY_MAX_MS * 48;  // 24000 samples @48kHz
constexpr int DELAY_MASK = DELAY_MAX_SAMPLES - 1;

// ============================================================================
// LFO Table Constants (for Percussion Spatializer)
// ============================================================================
constexpr int LFO_TABLE_SIZE = 256;                // Size of LFO wavetable
constexpr int LFO_TABLE_MASK = LFO_TABLE_SIZE - 1;

// ============================================================================
// Filter Constants (for filters.h)
// ============================================================================
constexpr float FILTER_Q_BESSEL = 0.5f;            // Bessel response
constexpr float FILTER_Q_BUTTERWORTH = 0.707f;     // Butterworth response
constexpr float FILTER_Q_LINKWITZ_RILEY = 0.5f;    // Linkwitz-Riley (cascaded)

// ============================================================================
// Utility Functions (compile-time)
// ============================================================================

/**
 * Compile-time calculation of 2^(n/12) for any n
 * Can be used to generate interval ratios at compile time
 */
constexpr float interval_ratio(int semitones) {
    return powf(2.0f, semitones / 12.0f);
}

/**
 * Convert dB to linear amplitude
 */
constexpr float db_to_linear(float db) {
    return powf(10.0f, db / 20.0f);
}

/**
 * Convert linear amplitude to dB
 */
constexpr float linear_to_db(float linear) {
    return 20.0f * log10f(linear);
}

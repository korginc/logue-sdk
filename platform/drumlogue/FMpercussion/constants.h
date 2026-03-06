#pragma once

/**
 * @file constants.h
 * @brief Central constants for FM Percussion Synth
 *
 * Single source of truth for all magic numbers
 * Used by lfo_enhanced.h, fm_voices.h, and other modules
 */

#include <cstdint>
#include <cmath>

// ============================================================================
// NEON Constants
// ============================================================================
constexpr int NEON_LANES = 4;
constexpr int VECTOR_ALIGN = 16;

// ============================================================================
// Audio Constants
// ============================================================================
constexpr float SAMPLE_RATE = 48000.0f;
constexpr float INV_SAMPLE_RATE = 1.0f / SAMPLE_RATE;

// ============================================================================
// MIDI Note to Frequency Conversion
// ============================================================================
constexpr float A4_FREQ = 440.0f;
constexpr int A4_MIDI = 69;

// ============================================================================
// Musical Interval Ratios (2^(n/12))
// ============================================================================
// Semitone ratios - defined once in a central location
constexpr float SEMITONE_RATIO = 1.0594630943592953f;  // 2^(1/12)

// Pre-calculated interval ratios for common intervals
constexpr float INTERVAL_UNISON   = 1.0f;              // 0 semitones
constexpr float INTERVAL_MINOR_2ND = 1.059463094f;     // 1 semitone
constexpr float INTERVAL_MAJOR_2ND = 1.122462048f;     // 2 semitones
constexpr float INTERVAL_MINOR_3RD = 1.189207115f;     // 3 semitones
constexpr float INTERVAL_MAJOR_3RD = 1.259921050f;     // 4 semitones  (2^(4/12))
constexpr float INTERVAL_PERFECT_4TH = 1.334839854f;   // 5 semitones
constexpr float INTERVAL_TRITONE    = 1.414213562f;    // 6 semitones
constexpr float INTERVAL_PERFECT_5TH = 1.498307077f;   // 7 semitones  (2^(7/12))
constexpr float INTERVAL_MINOR_6TH = 1.587401052f;     // 8 semitones
constexpr float INTERVAL_MAJOR_6TH = 1.681792830f;     // 9 semitones
constexpr float INTERVAL_MINOR_7TH = 1.781797436f;     // 10 semitones
constexpr float INTERVAL_MAJOR_7TH = 1.887748625f;     // 11 semitones
constexpr float INTERVAL_OCTAVE    = 2.0f;             // 12 semitones

// ============================================================================
// LFO Constants
// ============================================================================
constexpr float LFO_PHASE_OFFSET = 0.25f;  // 90° offset for LFO2

// LFO target definitions
constexpr int LFO_TARGET_NONE = 0;
constexpr int LFO_TARGET_PITCH = 1;
constexpr int LFO_TARGET_INDEX = 2;
constexpr int LFO_TARGET_ENV = 3;
constexpr int LFO_TARGET_LFO2_PHASE = 4;
constexpr int LFO_TARGET_LFO1_PHASE = 5;

// LFO shape definitions
constexpr int LFO_SHAPE_TRIANGLE = 0;
constexpr int LFO_SHAPE_RAMP = 1;
constexpr int LFO_SHAPE_CHORD = 2;

// ============================================================================
// Envelope Constants
// ============================================================================
constexpr int ENV_STAGE_ATTACK = 0;
constexpr int ENV_STAGE_DECAY = 1;
constexpr int ENV_STAGE_RELEASE = 2;
constexpr int ENV_STATE_OFF = 3;

constexpr int ENV_CURVE_LINEAR = 0;
constexpr int ENV_CURVE_EXPONENTIAL = 1;

// ============================================================================
// FM Engine Constants
// ============================================================================
// Kick engine
constexpr float KICK_CARRIER_BASE = 60.0f;
constexpr float KICK_MOD_RATIO_MIN = 1.0f;
constexpr float KICK_MOD_RATIO_MAX = 3.0f;
constexpr float KICK_SWEEP_OCTAVES = 3.0f;

// Snare engine
constexpr float SNARE_CARRIER_BASE = 200.0f;
constexpr float SNARE_MOD_RATIO_MIN = 1.5f;
constexpr float SNARE_MOD_RATIO_MAX = 2.5f;

// Metal engine
constexpr float METAL_RATIO1 = 1.0f;
constexpr float METAL_RATIO2 = 1.4f;
constexpr float METAL_RATIO3 = 1.7f;
constexpr float METAL_RATIO4 = 2.3f;

// Perc engine
constexpr float PERC_RATIO_MIN = 1.0f;
constexpr float PERC_RATIO_MAX = 3.0f;
constexpr float PERC_VARIATION_MAX = 1.5f;

// ============================================================================
// Compressor Constants
// ============================================================================
constexpr float KNEE_WIDTH_DEFAULT = 6.0f;
constexpr float SC_HPF_Q = 0.5f;

// ============================================================================
// Parameter Ranges (from header.c)
// ============================================================================
constexpr int THRESH_MIN = -600;
constexpr int THRESH_MAX = 0;
constexpr int RATIO_MIN = 10;
constexpr int RATIO_MAX = 200;
constexpr int ATTACK_MIN = 1;
constexpr int ATTACK_MAX = 1000;
constexpr int RELEASE_MIN = 10;
constexpr int RELEASE_MAX = 2000;

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
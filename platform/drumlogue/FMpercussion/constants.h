/**
 * @file constants.h
 * @brief Central constants for FM Percussion Synth
 *
 * Updated with resonant synthesis constants
 */

#pragma once

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
constexpr float SEMITONE_RATIO = 1.0594630943592953f;

// ============================================================================
// Musical Interval Ratios
// ============================================================================
constexpr float INTERVAL_UNISON   = 1.0f;
constexpr float INTERVAL_MINOR_2ND = 1.059463094f;
constexpr float INTERVAL_MAJOR_2ND = 1.122462048f;
constexpr float INTERVAL_MINOR_3RD = 1.189207115f;
constexpr float INTERVAL_MAJOR_3RD = 1.259921050f;  // 2^(4/12)
constexpr float INTERVAL_PERFECT_4TH = 1.334839854f;
constexpr float INTERVAL_TRITONE    = 1.414213562f;
constexpr float INTERVAL_PERFECT_5TH = 1.498307077f;  // 2^(7/12)
constexpr float INTERVAL_MINOR_6TH = 1.587401052f;
constexpr float INTERVAL_MAJOR_6TH = 1.681792830f;
constexpr float INTERVAL_MINOR_7TH = 1.781797436f;
constexpr float INTERVAL_MAJOR_7TH = 1.887748625f;
constexpr float INTERVAL_OCTAVE    = 2.0f;

// ============================================================================
// LFO Constants
// ============================================================================
constexpr float LFO_PHASE_OFFSET = 0.25f;
constexpr int LFO_TARGET_NONE = 0;
constexpr int LFO_TARGET_PITCH = 1;
constexpr int LFO_TARGET_INDEX = 2;
constexpr int LFO_TARGET_ENV = 3;
constexpr int LFO_TARGET_LFO2_PHASE = 4;
constexpr int LFO_TARGET_LFO1_PHASE = 5;

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
// FM Engine Constants (Existing)
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
// NEW: Resonant Synthesis Constants
// ============================================================================
constexpr int RESONANT_MODE_COUNT = 5;  // Low-pass, Band-pass, High-pass, Notch, Peak

// Resonant parameter ranges
constexpr float RESONANT_RESONANCE_MIN = 0.0f;
constexpr float RESONANT_RESONANCE_MAX = 0.99f;  // a parameter must be < 1
constexpr float RESONANT_RESONANCE_DEFAULT = 0.5f;

constexpr float RESONANT_CENTER_FREQ_MIN = 50.0f;
constexpr float RESONANT_CENTER_FREQ_MAX = 8000.0f;
constexpr float RESONANT_CENTER_FREQ_DEFAULT = 1000.0f;

constexpr float RESONANT_MIX_MIN = 0.0f;
constexpr float RESONANT_MIX_MAX = 1.0f;
constexpr float RESONANT_MIX_DEFAULT = 0.5f;  // Blend between single/double-sided

// For division protection (to avoid /0)
constexpr float RESONANT_DENOM_EPSILON = 0.0001f;

// ============================================================================
// Parameter Indexes
// ============================================================================
constexpr uint8_t PARAM_PROB_KICK = 0;
constexpr uint8_t PARAM_PROB_SNARE = 1;
constexpr uint8_t PARAM_PROB_METAL = 2;
constexpr uint8_t PARAM_PROB_PERC = 3;

constexpr uint8_t PARAM_KICK_SWEEP = 4;
constexpr uint8_t PARAM_KICK_DECAY = 5;
constexpr uint8_t PARAM_SNARE_NOISE = 6;
constexpr uint8_t PARAM_SNARE_BODY = 7;

constexpr uint8_t PARAM_METAL_INHARM = 8;
constexpr uint8_t PARAM_METAL_BRIGHT = 9;
constexpr uint8_t PARAM_PERC_RATIO = 10;
constexpr uint8_t PARAM_PERC_VAR = 11;

constexpr uint8_t PARAM_LFO1_SHAPE = 12;
constexpr uint8_t PARAM_LFO1_RATE = 13;
constexpr uint8_t PARAM_LFO1_TARGET = 14;
constexpr uint8_t PARAM_LFO1_DEPTH = 15;

constexpr uint8_t PARAM_LFO2_SHAPE = 16;
constexpr uint8_t PARAM_LFO2_RATE = 17;
constexpr uint8_t PARAM_LFO2_TARGET = 18;
constexpr uint8_t PARAM_LFO2_DEPTH = 19;

constexpr uint8_t PARAM_ENV_SHAPE = 20;

// NEW: Resonant mode selection (use param 21)
constexpr uint8_t PARAM_RESONANT_MODE = 21;
constexpr uint8_t PARAM_RESONANT_RESONANCE = 22;
constexpr uint8_t PARAM_RESONANT_CENTER = 23;
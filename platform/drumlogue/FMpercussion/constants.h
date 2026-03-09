#pragma once

/**
 * @file constants.h
 * @brief Central constants for FM Percussion Synth
 *
 * Version 2.0 - Added morph and voice allocation constants
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
constexpr float SEMITONE_RATIO = 1.0594630943592953f;

// ============================================================================
// Parameter Indexes (Updated for v2.0)
// ============================================================================
// Page 1: Voice Probabilities
constexpr uint8_t PARAM_VOICE1_PROB = 0;
constexpr uint8_t PARAM_VOICE2_PROB = 1;
constexpr uint8_t PARAM_VOICE3_PROB = 2;
constexpr uint8_t PARAM_VOICE4_PROB = 3;

// Page 2: Kick + Snare
constexpr uint8_t PARAM_KICK_SWEEP = 4;
constexpr uint8_t PARAM_KICK_DECAY = 5;
constexpr uint8_t PARAM_SNARE_NOISE = 6;
constexpr uint8_t PARAM_SNARE_BODY = 7;

// Page 3: Metal + Perc
constexpr uint8_t PARAM_METAL_INHARM = 8;
constexpr uint8_t PARAM_METAL_BRIGHT = 9;
constexpr uint8_t PARAM_PERC_RATIO = 10;
constexpr uint8_t PARAM_PERC_VAR = 11;

// Page 4: LFO1
constexpr uint8_t PARAM_LFO1_SHAPE = 12;
constexpr uint8_t PARAM_LFO1_RATE = 13;
constexpr uint8_t PARAM_LFO1_TARGET = 14;
constexpr uint8_t PARAM_LFO1_DEPTH = 15;

// Page 5: LFO2
constexpr uint8_t PARAM_LFO2_SHAPE = 16;
constexpr uint8_t PARAM_LFO2_RATE = 17;
constexpr uint8_t PARAM_LFO2_TARGET = 18;
constexpr uint8_t PARAM_LFO2_DEPTH = 19;

// Page 6: Envelope + Voice + Resonant
constexpr uint8_t PARAM_ENV_SHAPE = 20;
constexpr uint8_t PARAM_VOICE_ALLOC = 21;  // 0-11
constexpr uint8_t PARAM_RES_MODE = 22;     // 0-4
constexpr uint8_t PARAM_RES_MORPH = 23;    // 0-100%

// ============================================================================
// Voice Allocation Constants
// ============================================================================
constexpr int VOICE_ALLOC_COUNT = 12;       // 12 valid allocations
constexpr int VOICE_ALLOC_MIN = 0;
constexpr int VOICE_ALLOC_MAX = 11;

// ============================================================================
// Resonant Mode Constants
// ============================================================================
constexpr int RES_MODE_LOWPASS = 0;
constexpr int RES_MODE_BANDPASS = 1;
constexpr int RES_MODE_HIGHPASS = 2;
constexpr int RES_MODE_NOTCH = 3;
constexpr int RES_MODE_PEAK = 4;
constexpr int RES_MODE_COUNT = 5;

// ============================================================================
// Resonant Morph Ranges
// ============================================================================
constexpr float RES_FC_MIN = 50.0f;
constexpr float RES_FC_MAX = 8000.0f;
constexpr float RES_RESONANCE_MIN = 0.1f;
constexpr float RES_RESONANCE_MAX = 0.9f;
constexpr float RES_GAIN_MIN = 1.0f;
constexpr float RES_GAIN_MAX = 4.0f;

// ============================================================================
// LFO Target Constants (Updated)
// ============================================================================
constexpr int LFO_TARGET_NONE = 0;
constexpr int LFO_TARGET_PITCH = 1;
constexpr int LFO_TARGET_INDEX = 2;
constexpr int LFO_TARGET_ENV = 3;
constexpr int LFO_TARGET_LFO2_PHASE = 4;
constexpr int LFO_TARGET_LFO1_PHASE = 5;
constexpr int LFO_TARGET_RES_FREQ = 6;   // NEW
constexpr int LFO_TARGET_RESONANCE = 7;   // NEW
constexpr int LFO_TARGET_COUNT = 8;

// ============================================================================
// Envelope ROM Constants
// ============================================================================
constexpr int ENV_SHAPE_MIN = 0;
constexpr int ENV_SHAPE_MAX = 127;
constexpr int ENV_SHAPE_DEFAULT = 40;

// ============================================================================
// FM Engine Constants (Existing)
// ============================================================================
constexpr float KICK_SWEEP_MIN = 0.0f;
constexpr float KICK_SWEEP_MAX = 1.0f;
constexpr float KICK_DECAY_MIN = 0.0f;
constexpr float KICK_DECAY_MAX = 1.0f;

constexpr float SNARE_NOISE_MIN = 0.0f;
constexpr float SNARE_NOISE_MAX = 1.0f;
constexpr float SNARE_BODY_MIN = 0.0f;
constexpr float SNARE_BODY_MAX = 1.0f;

constexpr float METAL_INHARM_MIN = 0.0f;
constexpr float METAL_INHARM_MAX = 1.0f;
constexpr float METAL_BRIGHT_MIN = 0.0f;
constexpr float METAL_BRIGHT_MAX = 1.0f;

constexpr float PERC_RATIO_MIN = 1.0f;
constexpr float PERC_RATIO_MAX = 4.0f;
constexpr float PERC_VAR_MIN = 0.0f;
constexpr float PERC_VAR_MAX = 1.0f;
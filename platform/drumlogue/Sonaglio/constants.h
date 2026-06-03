#pragma once

/**
 * @file constants.h
 * @brief Active constants for Sonaglio
 *
 * Trimmed to the fixed 4-engine instrument model.
 * Legacy aliases are kept where useful to ease incremental cleanup.
 */

#ifdef __cplusplus
#include <cstdint>
#include <cmath>
#else
#include <stdint.h>
#include <math.h>
#define constexpr static const
#endif

#ifndef fast_inline
#define fast_inline inline __attribute__((always_inline))
#endif

// ============================================================================
// Core audio constants
// ============================================================================
constexpr int   NEON_LANES      = 4;
constexpr int   VECTOR_ALIGN    = 16;
constexpr int   CACHE_LINE_SIZE = 64;
constexpr float SAMPLE_RATE     = 48000.0f;
constexpr float INV_SAMPLE_RATE = 1.0f / SAMPLE_RATE;
constexpr float NYQUIST_FREQ    = SAMPLE_RATE * 0.5f;

// ============================================================================
// MIDI / tuning constants
// ============================================================================
constexpr float A4_FREQ  = 440.0f;
constexpr int   A4_MIDI  = 69;
constexpr float SEMITONE_RATIO   = 1.0594630943592953f;
constexpr float LFO_PHASE_OFFSET = 0.25f;


// ============================================================================
// LFO targets
// ============================================================================
enum lfo_target : uint8_t  {
    LFO_TARGET_NONE       = 0,
    LFO_TARGET_PITCH      = 1,
    LFO_TARGET_INDEX      = 2,
    LFO_TARGET_ENV        = 3,
    LFO_TARGET_LFO2_PHASE = 4,
    LFO_TARGET_LFO1_PHASE = 5,
    LFO_TARGET_RES_FREQ   = 6,
    LFO_TARGET_RESONANCE  = 7,
    LFO_TARGET_NOISE_MIX  = 8,
    LFO_TARGET_RES_MORPH  = 9,
    LFO_TARGET_METAL_GATE = 10,
    LFO_TARGET_COUNT      = 11
};

// ============================================================================
// LFO shapes
// ============================================================================
constexpr int LFO_SHAPE_TRIANGLE    = 0;
constexpr int LFO_SHAPE_RAMP        = 1;
constexpr int LFO_SHAPE_CHORD       = 2;
constexpr int LFO_SHAPE_COMBO_COUNT = 9;

// ============================================================================
// Euclidean tuning
// ============================================================================
enum euclidean_mode : int {
    EUCLID_MODE_OFF   = 0,
    EUCLID_MODE_CLSTR = 1,
    EUCLID_MODE_MINOR = 2,
    EUCLID_MODE_DIATN = 3,
    EUCLID_MODE_WHOLE = 4,
    EUCLID_MODE_PENTA = 5,
    EUCLID_MODE_DIM7  = 6,
    EUCLID_MODE_AUG8  = 7,
    EUCLID_MODE_TRIT  = 8,
    EUCLID_MODE_COUNT = 9
};

// ============================================================================
// Envelope ROM / states
// ============================================================================
constexpr int ENV_SHAPE_MIN         = 0;
constexpr int ENV_SHAPE_MAX         = 127;
constexpr int ENV_SHAPE_DEFAULT     = 40;
constexpr int ENV_ROM_SIZE          = 128;
constexpr int ENV_STAGE_ATTACK      = 0;
constexpr int ENV_STAGE_DECAY       = 1;
constexpr int ENV_STAGE_RELEASE     = 2;
constexpr int ENV_STATE_OFF         = 3;
constexpr int ENV_CURVE_LINEAR      = 0;
constexpr int ENV_CURVE_EXPONENTIAL = 1;

// ============================================================================
// PRNG / presets / smoothing
// ============================================================================
constexpr uint32_t PRNG_DEFAULT_SEED = 0x9E3779B9u;
constexpr uint32_t RAND_DEFAULT_SEED = 0x12345678u;
constexpr int NUM_OF_PRESETS = 26;
constexpr int NAME_LENGTH    = 12;
constexpr int SMOOTH_FRAMES  = 48;

// ============================================================================
// Legacy resonant constants retained for future work
// ============================================================================
enum res_mode : uint8_t {
    RES_MODE_LOWPASS  = 0,
    RES_MODE_BANDPASS = 1,
    RES_MODE_HIGHPASS = 2,
    RES_MODE_NOTCH    = 3,
    RES_MODE_PEAK     = 4,
    RES_MODE_COUNT    = 5
};
constexpr float RES_FC_MIN = 50.0f;
constexpr float RES_FC_MAX = 8000.0f;
constexpr float RESONANT_DENOM_EPSILON = 1e-6f;


// ============================================================================
// Euclidean tuning offsets
// ============================================================================
// offsets[mode][engine lane] = semitones above root.
// Engine mapping used by selector mode:
//   lane 0 = Kick
//   lane 1 = Snare
//   lane 2 = Metal
//   lane 3 = Tom/Perc
static const float EUCLID_OFFSETS[EUCLID_MODE_COUNT][4] = {
    { 0.f,  0.f,  0.f,  0.f},  // 0: Off
    { 0.f,  1.f,  2.f,  3.f},  // 1: E(4,4)
    { 0.f,  1.f,  3.f,  4.f},  // 2: E(4,6)
    { 0.f,  1.f,  3.f,  5.f},  // 3: E(4,7)
    { 0.f,  2.f,  4.f,  6.f},  // 4: E(4,8)
    { 0.f,  2.f,  5.f,  7.f},  // 5: E(4,10)
    { 0.f,  3.f,  6.f,  9.f},  // 6: E(4,12)
    { 0.f,  4.f,  8.f, 12.f},  // 7: E(4,16)
    { 0.f,  6.f, 12.f, 18.f},  // 8: E(4,24)
};

// ============================================================================
// Instrument selector
// ============================================================================
typedef enum {
    INST_KICK = 0,
    INST_SNARE,
    INST_TOM,
    INST_METAL,
    INST_HAT,
    INST_KS,
    INST_KT,
    INST_KM,
    INST_KH,
    INST_ST,
    INST_SM,
    INST_SH,
    INST_TM,
    INST_TH,
    INST_MH,
    INST_COUNT
} sonaglio_instrument_t;

// ============================================================================
// String tables
// ============================================================================
static const char* instruments_strings[INST_COUNT] = {
    "Kick",
    "Snare",
    "Tom",
    "Metal",
    "Hat",
    "K+S",
    "K+T",
    "K+M",
    "K+H",
    "S+T",
    "S+M",
    "S+H",
    "T+M",
    "T+H",
    "M+H",
};

static const char* lfo_shape_strings[LFO_SHAPE_COMBO_COUNT] = {
    "Tri+Tri", "Rmp+Rmp", "Chd+Chd",
    "Tri+Rmp", "Tri+Chd", "Rmp+Tri",
    "Rmp+Chd", "Chd+Tri", "Chd+Rmp"
};

static const char* lfo_target_strings[LFO_TARGET_COUNT] = {
    "None", "Pitch", "ModIdx", "Env",
    "LFO2Ph", "LFO1Ph", "ResFrq", "Reson",
    "NoizMx", "ResMrph", "MtlGate"
};

static const char* euclidean_mode_strings[EUCLID_MODE_COUNT] = {
    "Off",    // 0: disabled — all voices same pitch
    "Clstr",  // 1: E(4,4)  = [0, 1, 2, 3]  chromatic cluster
    "Minor",  // 2: E(4,6)  = [0, 1, 3, 4]  minor 3rd pairs
    "Diatn",  // 3: E(4,7)  = [0, 1, 3, 5]  diatonic cluster
    "Whole",  // 4: E(4,8)  = [0, 2, 4, 6]  whole tone
    "Penta",  // 5: E(4,10) = [0, 2, 5, 7]  pentatonic / 5th
    "Dim7",   // 6: E(4,12) = [0, 3, 6, 9]  diminished 7th
    "Aug8",   // 7: E(4,16) = [0, 4, 8, 12] augmented + octave
    "Trit"    // 8: E(4,24) = [0, 6, 12, 18] tritone spread
};


// ============================================================================
// Utility helpers
// ============================================================================
static inline float interval_ratio(int semitones) {
    return powf(2.0f, semitones / 12.0f);
}

static inline float db_to_linear(float db) {
    return powf(10.0f, db / 20.0f);
}

static inline float linear_to_db(float linear) {
    return 20.0f * log10f(linear);
}
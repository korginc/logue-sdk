#pragma once

/**
 * @file constants.h
 * @brief Active constants for EffeMD
 *
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
constexpr float PI              = 3.14159265f;
constexpr float TWO_PI          = 2.0f * PI;

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
// PRNG / presets / smoothing
// ============================================================================
constexpr uint32_t PRNG_DEFAULT_SEED = 0x9E3779B9u;
constexpr uint32_t RAND_DEFAULT_SEED = 0x12345678u;
constexpr int NAME_LENGTH    = 12;
constexpr int SMOOTH_FRAMES  = 48;



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
// Parameter indices matching the fixed 24-parameter synth contract.
// ============================================================================
typedef enum {
  // page 1
  K_Instrument,           // id
  K_Base_Frequency,         // f_b - fbA - f_bA - Pitch
  K_Modulation_Frequency,   // f_m
  K_Modulation_Index,       // I - ratio_index - I_A
  // page 2
  K_Modulation_Decay,       // d_m - rampDecay
  K_Modulation_Feedback,    // b_m - ramp
  K_Decay_A,                // d_b - d_b1 - d1 - d_bA - decay
  K_UseRatio,               // if true, Modulation_Index is interpreted as a ratio of Base_Frequency rather than an absolute frequency deviation
  // page 3
  K_Mix,                    // Ab1 - A_A - tone
  K_HPF,                    // f_hp - fhp
  K_LPF,                    // f_lp
  K_Sustain,                // sustain
  // page 4
  K_Frequency_Sweep,        // A_f - snap
  K_Sweep_Decay,            // d_f - bump
  K_Noise_Level,            // Abrus - noise - metal
  K_Noise_Decay,            // dbrus
  // page 5
  K_Modulation_Frequency_B, // f_bB
  K_Decay_B,                // d_b2 - d2 - d_bB
  K_Modulation_Index_B,     // I_B
  K_Phase,                  // start_phase
  // page 6
  K_Gap,                    // clap_interval - start - interval - gap - tune - mod_env_sync
  K_Count,                  // clap_count - peak - clip
  reserved,               // new, to be decided - LFO?
  K_Euclidean_Tuning,       // new, to be decided

  PARAM_TOTAL = 24        // fixed see header.c, list must match!!
} fm_param_index_t;

// ============================================================================
// Instrument
// ============================================================================

typedef enum {
    ID_FmKickModel,
    ID_FmSnareModel,
    ID_FmTomModel,
    ID_FmClapModel,
    ID_FmRimshotModel,
    ID_FmCowbellModel,
    ID_FmCymbalModel,
    ID_TRXBassDrum,
    ID_TRXSnareDrum,
    ID_TRXClaves,
    ID_TRXHiHat,
    INST_COUNT,
} engine_id_t;

// ============================================================================
// String tables
// ============================================================================
static const char* instruments_strings[INST_COUNT] = {
"FmKick",
"FmSnare",
"FmTom",
"FmClap",
"FmRimshot",
"FmCowbell",
"FmCymbal",
"TRXBassDrum",
"TRXSnareDrum",
"TRXClaves",
"TRXHiHat",
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
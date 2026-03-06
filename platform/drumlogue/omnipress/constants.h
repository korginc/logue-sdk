#pragma once
/**
 * @file constants.h
 * @brief Central constants for OmniPress Master Compressor
 *
 * Single source of truth for all magic numbers
 * Version 1.0
 */

#include <cstdint>

// ============================================================================
// DSP Constants
// ============================================================================

// Sample rate (fixed for drumlogue)
constexpr float SAMPLE_RATE = 48000.0f;
constexpr float INV_SAMPLE_RATE = 1.0f / SAMPLE_RATE;

// Smoothing constants
constexpr int SMOOTH_FRAMES = 48;        // 1ms at 48kHz
constexpr float SMOOTH_COEFF_MIN = 0.001f;
constexpr float SMOOTH_COEFF_MAX = 0.5f;

// Filter constants
constexpr float FILTER_Q_BESSEL = 0.5f;      // Bessel response
constexpr float FILTER_Q_BUTTERWORTH = 0.707f; // Butterworth response
constexpr float FILTER_Q_LINKWITZ_RILEY = 0.5f; // Linkwitz-Riley (cascaded)

// ============================================================================
// Parameter Ranges (as per header.c)
// ============================================================================

// Page 1: Core Dynamics
constexpr int THRESH_MIN = -600;      // -60.0 dB
constexpr int THRESH_MAX = 0;          // 0 dB
constexpr int THRESH_DEFAULT = -200;   // -20.0 dB

constexpr int RATIO_MIN = 10;          // 1.0:1
constexpr int RATIO_MAX = 200;         // 20.0:1
constexpr int RATIO_DEFAULT = 40;      // 4.0:1

constexpr int ATTACK_MIN = 1;           // 0.1 ms
constexpr int ATTACK_MAX = 1000;        // 100.0 ms
constexpr int ATTACK_DEFAULT = 150;     // 15.0 ms

constexpr int RELEASE_MIN = 10;         // 10 ms
constexpr int RELEASE_MAX = 2000;       // 2000 ms
constexpr int RELEASE_DEFAULT = 200;    // 200 ms

// Page 2: Character & Output
constexpr int MAKEUP_MIN = 0;           // 0 dB
constexpr int MAKEUP_MAX = 240;         // 24.0 dB
constexpr int MAKEUP_DEFAULT = 0;       // 0 dB

constexpr int DRIVE_MIN = 0;            // 0%
constexpr int DRIVE_MAX = 100;          // 100%
constexpr int DRIVE_DEFAULT = 0;        // 0%

constexpr int MIX_MIN = -100;           // 100% dry
constexpr int MIX_MAX = 100;            // 100% wet
constexpr int MIX_DEFAULT = 100;        // 100% wet

constexpr int SC_HPF_MIN = 20;          // 20 Hz
constexpr int SC_HPF_MAX = 500;         // 500 Hz
constexpr int SC_HPF_DEFAULT = 20;      // 20 Hz

// Page 3: Mode Selection
constexpr int COMP_MODE_MIN = 0;
constexpr int COMP_MODE_MAX = 2;
constexpr int COMP_MODE_DEFAULT = 0;    // Standard mode

constexpr int BAND_SEL_MIN = 0;
constexpr int BAND_SEL_MAX = 3;
constexpr int BAND_SEL_DEFAULT = 0;     // Low band

constexpr int BAND_THRESH_MIN = -600;   // -60.0 dB
constexpr int BAND_THRESH_MAX = 0;      // 0 dB
constexpr int BAND_THRESH_DEFAULT = -200; // -20.0 dB

constexpr int BAND_RATIO_MIN = 10;      // 1.0:1
constexpr int BAND_RATIO_MAX = 200;     // 20.0:1
constexpr int BAND_RATIO_DEFAULT = 40;  // 4.0:1

// Page 4: Distressor Mode
constexpr int DSTR_MODE_MIN = 0;
constexpr int DSTR_MODE_MAX = 3;
constexpr int DSTR_MODE_DEFAULT = 0;    // No harmonics

// ============================================================================
// Compression Mode IDs
// ============================================================================

constexpr uint8_t COMP_MODE_STANDARD = 0;
constexpr uint8_t COMP_MODE_DISTRESSOR = 1;
constexpr uint8_t COMP_MODE_MULTIBAND = 2;

// ============================================================================
// Band Selection IDs
// ============================================================================

constexpr uint8_t BAND_LOW = 0;
constexpr uint8_t BAND_MID = 1;
constexpr uint8_t BAND_HIGH = 2;
constexpr uint8_t BAND_ALL = 3;

// ============================================================================
// Distressor Mode IDs
// ============================================================================

constexpr uint8_t DSTR_HARMONICS_NONE = 0;
constexpr uint8_t DSTR_HARMONICS_2ND = 1;   // Dist 2
constexpr uint8_t DSTR_HARMONICS_3RD = 2;   // Dist 3
constexpr uint8_t DSTR_HARMONICS_BOTH = 3;  // Both

// ============================================================================
// Drive/Wavefolder Mode IDs
// ============================================================================

constexpr uint8_t DRIVE_MODE_SOFT_CLIP = 0;
constexpr uint8_t DRIVE_MODE_HARD_CLIP = 1;
constexpr uint8_t DRIVE_MODE_TRIANGLE = 2;
constexpr uint8_t DRIVE_MODE_SINE = 3;
constexpr uint8_t DRIVE_MODE_SUBOCTAVE = 4;

// ============================================================================
// Detection Mode IDs
// ============================================================================

constexpr uint8_t DETECT_MODE_PEAK = 0;
constexpr uint8_t DETECT_MODE_RMS = 1;
constexpr uint8_t DETECT_MODE_BLEND = 2;

// ============================================================================
// Knee Type IDs
// ============================================================================

constexpr uint8_t KNEE_HARD = 0;
constexpr uint8_t KNEE_SOFT = 1;
constexpr uint8_t KNEE_MEDIUM = 2;

// ============================================================================
// Distressor Ratio IDs
// ============================================================================

constexpr uint8_t DIST_RATIO_1_1 = 0;    // Warm mode
constexpr uint8_t DIST_RATIO_2_1 = 1;
constexpr uint8_t DIST_RATIO_3_1 = 2;
constexpr uint8_t DIST_RATIO_4_1 = 3;
constexpr uint8_t DIST_RATIO_6_1 = 4;
constexpr uint8_t DIST_RATIO_10_1 = 5;   // Opto mode
constexpr uint8_t DIST_RATIO_20_1 = 6;
constexpr uint8_t DIST_RATIO_NUKE = 7;   // Brick-wall

// ============================================================================
// Multiband Crossover Frequencies
// ============================================================================

constexpr float XOVER_LOW_FREQ_DEFAULT = 250.0f;   // Low/Mid crossover
constexpr float XOVER_HIGH_FREQ_DEFAULT = 2500.0f; // Mid/High crossover
constexpr float XOVER_MIN_FREQ = 20.0f;
constexpr float XOVER_MAX_FREQ = 20000.0f;

// ============================================================================
// Envelope Detector Constants
// ============================================================================

constexpr float ENV_ATTACK_DEFAULT_MS = 10.0f;
constexpr float ENV_RELEASE_DEFAULT_MS = 100.0f;
constexpr float ENV_HOLD_MS = 10.0f;                // Peak hold time
constexpr float ENV_RMS_WINDOW_MS = 50.0f;          // RMS window size

// ============================================================================
// Gain Computer Constants
// ============================================================================

constexpr float KNEE_WIDTH_DEFAULT = 6.0f;          // 6dB soft knee
constexpr float KNEE_WIDTH_MIN = 0.0f;
constexpr float KNEE_WIDTH_MAX = 20.0f;

// ============================================================================
// Sidechain HPF Constants
// ============================================================================

constexpr float SC_HPF_Q = 0.5f;                    // Bessel response

// ============================================================================
// Makeup Gain Limits
// ============================================================================

constexpr float MAKEUP_GAIN_MAX_LINEAR = 15.85f;    // 24 dB in linear
constexpr float MAKEUP_GAIN_MIN_LINEAR = 1.0f;      // 0 dB

// ============================================================================
// NEON Vector Constants
// ============================================================================

constexpr int NEON_LANES = 4;                       // 4 floats per vector
constexpr int VECTOR_ALIGN = 16;                    // 16-byte alignment

// ============================================================================
// DSP State Constants
// ============================================================================

constexpr float EPSILON = 1e-12f;                   // Prevent denormals
constexpr float DB_COEFF = 8.65617f;                 // 20 / ln(10)
constexpr float INV_DB_COEFF = 0.115129f;            // ln(10) / 20

// ============================================================================
// Parameter Indexes (for code clarity)
// ============================================================================

// Page 1
constexpr uint8_t PARAM_THRESH = 0;
constexpr uint8_t PARAM_RATIO = 1;
constexpr uint8_t PARAM_ATTACK = 2;
constexpr uint8_t PARAM_RELEASE = 3;

// Page 2
constexpr uint8_t PARAM_MAKEUP = 4;
constexpr uint8_t PARAM_DRIVE = 5;
constexpr uint8_t PARAM_MIX = 6;
constexpr uint8_t PARAM_SC_HPF = 7;

// Page 3
constexpr uint8_t PARAM_COMP_MODE = 8;
constexpr uint8_t PARAM_BAND_SEL = 9;
constexpr uint8_t PARAM_BAND_THRESH = 10;
constexpr uint8_t PARAM_BAND_RATIO = 11;

// Page 4
constexpr uint8_t PARAM_DSTR_MODE = 12;
// ... 13-23 reserved
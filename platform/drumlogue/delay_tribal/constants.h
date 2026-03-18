#pragma once
/**
 * @file constants.h
 * @brief Central constants for the delay_tribal effect.
 *
 * Single source of truth for all magic numbers.
 */

// Constants for NEON vectorization
constexpr int NEON_LANES = 4;
constexpr int MAX_CLONES = 16;
constexpr int CLONE_GROUPS = MAX_CLONES / NEON_LANES;
constexpr int CROSSFADE_SAMPLES = 480;  // 10ms @ 48kHz

// Optimization constants
constexpr int CACHE_LINE_SIZE = 64;  // ARM Cortex-A9 cache line
constexpr int PREFETCH_DISTANCE = 4;  // Prefetch 4 samples ahead

// Delay line configuration (48kHz sample rate)
constexpr int DELAY_MAX_MS = 500;
constexpr int DELAY_MAX_SAMPLES = DELAY_MAX_MS * 48;
constexpr int DELAY_MASK = DELAY_MAX_SAMPLES - 1;

// LFO configuration
constexpr int LFO_TABLE_SIZE = 256;
constexpr float LFO_MIN_RATE = 0.1f;
constexpr float LFO_MAX_RATE = 10.0f;

// Filter configuration - TRIBAL MODE
constexpr float TRIBAL_MIN_FREQ = 80.0f;
constexpr float TRIBAL_MAX_FREQ = 800.0f;
constexpr float TRIBAL_DEFAULT_FREQ = 200.0f;
constexpr float TRIBAL_DEFAULT_Q = 2.0f;

// Filter configuration - MILITARY MODE
constexpr float MILITARY_MIN_FREQ = 1000.0f;
constexpr float MILITARY_MAX_FREQ = 8000.0f;
constexpr float MILITARY_DEFAULT_FREQ = 1000.0f;
constexpr float MILITARY_DEFAULT_Q = 0.707f;

// Filter configuration - ANGEL MODE
constexpr float ANGEL_MIN_LOW_CUT = 200.0f;
constexpr float ANGEL_MAX_LOW_CUT = 1000.0f;
constexpr float ANGEL_MIN_HIGH_CUT = 2000.0f;
constexpr float ANGEL_MAX_HIGH_CUT = 6000.0f;
constexpr float ANGEL_DEFAULT_LOW_CUT = 500.0f;
constexpr float ANGEL_DEFAULT_HIGH_CUT = 4000.0f;
constexpr float ANGEL_DEFAULT_Q = 1.0f;

// Filter quality factors
constexpr float FILTER_Q_BUTTERWORTH = 0.707f;
constexpr float FILTER_Q_BESSEL = 0.5f;

// Parameter ranges (as per header.c)
constexpr int PARAM_CLONES_MIN = 0;
constexpr int PARAM_CLONES_MAX = 2;
constexpr int PARAM_MODE_MIN = 0;
constexpr int PARAM_MODE_MAX = 2;
constexpr int PARAM_DEPTH_MIN = 0;
constexpr int PARAM_DEPTH_MAX = 100;
constexpr int PARAM_RATE_MIN = 0;
constexpr int PARAM_RATE_MAX = 100;
constexpr int PARAM_SPREAD_MIN = 0;
constexpr int PARAM_SPREAD_MAX = 100;
constexpr int PARAM_MIX_MIN = 0;
constexpr int PARAM_MIX_MAX = 100;
constexpr int PARAM_WOBBLE_MIN = 0;
constexpr int PARAM_WOBBLE_MAX = 100;
constexpr int PARAM_SOFTEN_MIN = 0;
constexpr int PARAM_SOFTEN_MAX = 100;

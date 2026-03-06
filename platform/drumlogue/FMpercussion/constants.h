#pragma once
/**
 * @file constants.h
 * @brief Central constants for FM drum synth
 *
 * Single source of truth for all magic numbers
 * Version 1.0
 */

#define SMOOTH_FRAMES 48  // 1ms at 48kHz

// LFO shapes (0-8 combinations)
#define LFO_SHAPE_TRIANGLE 0
#define LFO_SHAPE_RAMP     1
#define LFO_SHAPE_CHORD    2

// LFO targets (0-5)
#define LFO_TARGET_NONE        0
#define LFO_TARGET_PITCH       1
#define LFO_TARGET_INDEX       2
#define LFO_TARGET_ENV         3
#define LFO_TARGET_LFO2_PHASE  4
#define LFO_TARGET_LFO1_PHASE  5

// Phase offset to prevent cancellation (90° = 0.25 cycle)
#define LFO_PHASE_OFFSET 0.25f

// LFO Depth range (bipolar)
constexpr int LFO_DEPTH_MIN = -100;
constexpr int LFO_DEPTH_MAX = 100;
constexpr int LFO_DEPTH_DEFAULT = 50;  // +50% modulation

// LFO Rate range (unipolar)
constexpr int LFO_RATE_MIN = 0;
constexpr int LFO_RATE_MAX = 100;
constexpr int LFO_RATE_DEFAULT = 30;
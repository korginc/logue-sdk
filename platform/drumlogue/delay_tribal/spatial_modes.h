#pragma once
/**
 * @file spatial_modes.h
 * @brief Spatial mode definitions for the delay_tribal effect.
 *
 * Modes selected via parameter ID 1: 0=Tribal, 1=Military, 2=Angel.
 */

typedef enum {
    MODE_TRIBAL   = 0,
    MODE_MILITARY = 1,
    MODE_ANGEL    = 2,
    MODE_COUNT    = 3,
} spatial_mode_t;

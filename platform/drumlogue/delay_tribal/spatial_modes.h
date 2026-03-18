#pragma once
/**
 * @file spatial_modes.h
 * @brief Spatial mode definitions for the delay_tribal effect.
 *
 * Modes are selected via parameter ID 1 (Mode): 0=Tribal, 1=Military, 2=Angel.
 */

typedef enum {
    SPATIAL_MODE_TRIBAL   = 0,
    SPATIAL_MODE_MILITARY = 1,
    SPATIAL_MODE_ANGEL    = 2,
    SPATIAL_MODE_COUNT    = 3,
} spatial_mode_t;

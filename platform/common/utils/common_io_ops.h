/**
 * @file    common_io_ops.h
 * @brief   Common operations for input and output
 *
 * @addtogroup utils Utils
 * @{
 *
 * @addtogroup utils_io_ops Input/Output Operations
 * @{
 *
 */

#ifndef __common_io_ops_h
#define __common_io_ops_h

#include "common_float_math.h"

enum { kLeftChannel = 0, kRightChannel = 1 };

// Get a sample from an interlaced buffer with an arbitrary number of channels.
// For example, a stereo input would have two channels, with left being channel 0 and right being channel 1
template<typename t>
static inline __attribute__((optimize("Ofast"),always_inline))
t get_interlaced_sample(t * buffer, const uint32_t index, const uint32_t channel, const uint32_t numChannels)
{
    return buffer[(index * numChannels) + channel];
}

// Write a sample to an interlaced buffer with an arbitrary number of channels.
template<typename t>
static inline __attribute__((optimize("Ofast"),always_inline))
void write_to_interlaced_buffer(t * buffer, t sample, const uint32_t index, const uint32_t channel, const uint32_t numChannels)
{
    buffer[(index * numChannels) + channel] = sample;
}

#endif // __common_io_ops_h
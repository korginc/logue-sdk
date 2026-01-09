/**
 * @file    io_ops.h
 * @brief   Common operations for input and output
 *
 * @addtogroup utils Utils
 * @{
 *
 * @addtogroup utils_mk2_io_ops Input/Output Operations
 * @{
 *
 */

#ifndef __io_ops_h
#define __io_ops_h

#include "utils/common_io_ops.h"
#include "utils/float_simd.h"
#include "runtime.h"

static inline __attribute__((optimize("Ofast"),always_inline))
float32x2_t get_interlaced_samplef32x2(float * buffer, const uint32_t index, const uint32_t channel, const uint32_t numChannels)
{
    return f32x2_ld(&buffer[(index * numChannels) + channel]);
}

static inline __attribute__((optimize("Ofast"),always_inline))
float32x4_t get_interlaced_samplef32x4(float * buffer, const uint32_t index, const uint32_t channel, const uint32_t numChannels)
{
    return f32x4_ld(&buffer[(index * numChannels) + channel]);
}

static inline __attribute__((optimize("Ofast"),always_inline))
void write_to_interlaced_bufferf32x2(float * buffer, float32x2_t sample, const uint32_t index, const uint32_t channel, const uint32_t numChannels)
{
    f32x2_str(&buffer[(index * numChannels) + channel], sample);
}

static inline __attribute__((optimize("Ofast"),always_inline))
void write_to_interlaced_bufferf32x4(float * buffer, float32x4_t sample, const uint32_t index, const uint32_t channel, const uint32_t numChannels)
{
    f32x4_str(&buffer[(index * numChannels) + channel], sample);
}

static inline __attribute__((optimize("Ofast"),always_inline))
void write_oscillator_output_x4(float * buffer, float32x4_t sample, const uint32_t offset, const uint32_t stride, const uint32_t index)
{
    f32x4_str(&buffer[offset + (index * stride)], sample);
}

static inline __attribute__((optimize("Ofast"),always_inline))
void write_oscillator_output_x2(float * buffer, float32x2_t sample, const uint32_t offset, const uint32_t stride, const uint32_t index, const uint32_t channel)
{
    f32x2_str(&buffer[offset + ((index * stride) + channel)], sample);
}

static inline __attribute__((optimize("Ofast"),always_inline))
void write_oscillator_output_x1(float * buffer, float sample, const uint32_t offset, const uint32_t stride, const uint32_t index, const uint32_t channel)
{
    buffer[offset + (index * stride) + (channel % 4)] = sample;
}
#endif // __io_ops_h
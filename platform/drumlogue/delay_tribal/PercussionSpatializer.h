#pragma once

/**
 * Percussion Spatializer - Unified NEON Engine
 * Includes: Constants, Filters, Spatial Modes, Presets, and Core Delay
 */

#include <atomic>
#include <cstdint>
#include <cmath>
#include <arm_neon.h>
#include "unit.h"

// --- CONSTANTS & MACROS ---
#define fast_inline __attribute__((always_inline)) inline

constexpr int NEON_LANES = 4;
constexpr int VECTOR_ALIGN = 16;
constexpr int MAX_CLONES = 16;
constexpr int CLONE_GROUPS = MAX_CLONES / NEON_LANES;

constexpr int DELAY_MAX_MS = 500;
constexpr int DELAY_MAX_SAMPLES = DELAY_MAX_MS * 48000 / 1000;
constexpr int DELAY_MASK = DELAY_MAX_SAMPLES - 1;
constexpr float SAMPLE_RATE = 48000.0f;

constexpr float PI = 3.14159265359f;
constexpr float TWO_PI = 6.28318530718f;
constexpr float HALF_PI = 1.57079632679f;

// --- ENUMS & STRUCTS ---
typedef enum {
    MODE_TRIBAL = 0,
    MODE_MILITARY = 1,
    MODE_ANGEL = 2
} spatial_mode_t;

typedef enum {
    FILTER_LOWPASS,
    FILTER_HIGHPASS,
    FILTER_BANDPASS,
    FILTER_ALLPASS
} filter_type_t;

// NEON-Aligned Data Structures
struct clone_group_t {
    float32x4_t delay_offsets;
    float32x4_t left_gains;
    float32x4_t right_gains;
    float32x4_t mod_phases;
    uint32x4_t phase_flags;
} __attribute__((aligned(16)));

struct interleaved_frame_t {
    float32x4_t left[NEON_LANES];
    float32x4_t right[NEON_LANES];
} __attribute__((aligned(16)));

struct biquad_coeffs_t {
    float32x4_t b0, b1, b2, a1, a2;
};

struct biquad_state_t {
    float32x4_t z1, z2;
};

struct mode_filters_t {
    biquad_coeffs_t pre_filter;
    biquad_state_t pre_state[CLONE_GROUPS];
    filter_type_t pre_type;
};

// --- CORE CLASS ---
class PercussionSpatializer {
public:
    PercussionSpatializer() : write_ptr_(0), clone_count_(4), current_mode_(MODE_TRIBAL) {
        // Strict aligned allocation for NEON vld4 / vst4 operations
        delay_line_ = (interleaved_frame_t*)memalign(16, DELAY_MAX_SAMPLES * sizeof(interleaved_frame_t));
        phase_inc_ = vdupq_n_f32(0.0f);
    }

    ~PercussionSpatializer() {
        if (delay_line_) free(delay_line_);
    }

    int8_t Init(const unit_runtime_desc_t* desc) {
        if (desc->samplerate != 48000) return k_unit_err_samplerate;
        Reset();
        return k_unit_err_none;
    }

    void Reset() {
        if (delay_line_) {
            float32x4_t zero = vdupq_n_f32(0.0f);
            for (int i = 0; i < DELAY_MAX_SAMPLES; i++) {
                for(int j=0; j<NEON_LANES; j++) {
                    delay_line_[i].left[j] = zero;
                    delay_line_[i].right[j] = zero;
                }
            }
        }
        write_ptr_ = 0;
        initFilters();
    }

    // Fast polynomial approximations
    fast_inline float32x4_t fast_sin_neon(float32x4_t x) {
        float32x4_t x2 = vmulq_f32(x, x);
        float32x4_t x3 = vmulq_f32(x, x2);
        float32x4_t x5 = vmulq_f32(x3, x2);
        float32x4_t res = vmlsq_f32(x, x3, vdupq_n_f32(1.0f/6.0f));
        return vmlaq_f32(res, x5, vdupq_n_f32(1.0f/120.0f));
    }

    fast_inline float32x4_t fast_cos_neon(float32x4_t x) {
        return fast_sin_neon(vaddq_f32(x, vdupq_n_f32(HALF_PI)));
    }

    fast_inline float32x4_t process_biquad(float32x4_t in, biquad_state_t& state, const biquad_coeffs_t& coeff) {
        float32x4_t y = vmlaq_f32(state.z1, in, coeff.b0);
        state.z1 = vmlaq_f32(state.z2, in, coeff.b1);
        state.z1 = vmlsq_f32(state.z1, y, coeff.a1);
        state.z2 = vmulq_f32(in, coeff.b2);
        state.z2 = vmlsq_f32(state.z2, y, coeff.a2);
        return y;
    }

    void Process(const float* in, float* out, uint32_t frames) {
        // Horizontal sum helper for the final mixdown
        auto hsum = [](float32x4_t v) -> float {
            float32x4_t t1 = vpaddq_f32(v, v);
            float32x4_t t2 = vpaddq_f32(t1, t1);
            return vgetq_lane_f32(t2, 0);
        };

        for (uint32_t i = 0; i < frames; ++i) {
            float inL = in[i * 2];
            float inR = in[i * 2 + 1];

            float32x4_t vInL = vdupq_n_f32(inL);
            float32x4_t vInR = vdupq_n_f32(inR);

            float outL_accum = 0.0f;
            float outR_accum = 0.0f;

            uint32_t active_groups = (clone_count_ + 3) / 4;

            for (uint32_t g = 0; g < active_groups; ++g) {
                // 1. Read Delayed Clones (Interleaved vld4 emulation via array access)
                uint32_t read_pos = (write_ptr_ - 4800 + DELAY_MAX_SAMPLES) & DELAY_MASK; // Example 100ms base delay

                float32x4_t dL = delay_line_[read_pos].left[g];
                float32x4_t dR = delay_line_[read_pos].right[g];

                // 2. Spatial Process (Tribal example: Circular)
                if (current_mode_ == MODE_TRIBAL) {
                    float32x4_t angles = vdupq_n_f32((float)g * HALF_PI); // Simplified spreading
                    float32x4_t sin_v = fast_sin_neon(angles);
                    float32x4_t cos_v = fast_cos_neon(angles);

                    dL = vmulq_f32(dL, cos_v);
                    dR = vmulq_f32(dR, sin_v);
                }

                // 3. Filtering
                dL = process_biquad(dL, mode_filters_.pre_state[g], mode_filters_.pre_filter);
                dR = process_biquad(dR, mode_filters_.pre_state[g], mode_filters_.pre_filter);

                // 4. Accumulate to output
                outL_accum += hsum(dL);
                outR_accum += hsum(dR);

                // 5. Write Input to Delay Line
                delay_line_[write_ptr_].left[g] = vInL;
                delay_line_[write_ptr_].right[g] = vInR;
            }

            // Mix
            out[i * 2]     = (outL_accum * mix_) + (inL * (1.0f - mix_));
            out[i * 2 + 1] = (outR_accum * mix_) + (inR * (1.0f - mix_));

            write_ptr_ = (write_ptr_ + 1) & DELAY_MASK;
        }
    }

    void setParameter(uint8_t id, int32_t value) {
        float norm = value / 1023.0f;
        switch(id) {
            case 0: mix_ = norm; break;
            case 1: spread_ = norm; break;
            case 2: rate_ = norm * 10.0f; break;
            case 3: depth_ = norm; break;
            case 4: clone_count_ = 4 + (value % 3) * 4; break; // 4, 8, 12 clones
            case 5: current_mode_ = (spatial_mode_t)(value % 3); break;
        }
    }

private:
    interleaved_frame_t* delay_line_;
    uint32_t write_ptr_;

    uint32_t clone_count_;
    spatial_mode_t current_mode_;
    float mix_ = 0.5f;
    float spread_ = 0.5f;
    float rate_ = 1.0f;
    float depth_ = 0.5f;

    mode_filters_t mode_filters_;
    float32x4_t phase_inc_;

    void initFilters() {
        // Initialize a dummy passthrough / simple lowpass for safety
        for (int i = 0; i < CLONE_GROUPS; i++) {
            mode_filters_.pre_state[i].z1 = vdupq_n_f32(0.0f);
            mode_filters_.pre_state[i].z2 = vdupq_n_f32(0.0f);
        }
        mode_filters_.pre_filter.b0 = vdupq_n_f32(1.0f);
        mode_filters_.pre_filter.b1 = vdupq_n_f32(0.0f);
        mode_filters_.pre_filter.b2 = vdupq_n_f32(0.0f);
        mode_filters_.pre_filter.a1 = vdupq_n_f32(0.0f);
        mode_filters_.pre_filter.a2 = vdupq_n_f32(0.0f);
    }
};
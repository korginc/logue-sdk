#pragma once

/**
 * @file PercussionSpatializer.h
 * @brief Percussion Spatializer Core Processor
 *
 * NEON-optimized multi-clone generator for drumlogue
 *
 * FIXED:
 * - Added null pointer checks for all allocations
 * - Correct ARMv7 horizontal sum using vpadd_f32
 * - Graceful degradation on allocation failure
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <arm_neon.h>
#include "unit.h"
#include "constants.h"
#include "spatial_modes.h"
#include "filters.h"

// Constants for NEON vectorization
constexpr int NEON_LANES = 4;
constexpr int MAX_CLONES = 16;
constexpr int CLONE_GROUPS = MAX_CLONES / NEON_LANES;
constexpr int DELAY_MAX_MS = 500;
constexpr int DELAY_MAX_SAMPLES = DELAY_MAX_MS * 48;
constexpr int DELAY_MASK = DELAY_MAX_SAMPLES - 1;
constexpr int LFO_TABLE_SIZE = 256;

/**
 * Per-clone parameters - FULLY VECTORIZED
 */
typedef struct {
    float32x4_t delay_offsets;    // Micro-delay for vibrato (4 clones)
    float32x4_t left_gains;        // Left channel pan (4 clones)
    float32x4_t right_gains;       // Right channel pan (4 clones)
    float32x4_t mod_phases;        // LFO phases (4 clones)
    uint32x4_t phase_flags;         // Phase inversion flags
} clone_group_t;

// Ensure structure is 16-byte aligned for NEON
static_assert(sizeof(clone_group_t) % 16 == 0,
              "clone_group_t must be 16-byte aligned");

/**
 * INTERLEAVED delay line structure for vld4 optimization
 */
typedef struct {
    float32x4_t left[NEON_LANES];   // Left samples for 4 clones at one time position
    float32x4_t right[NEON_LANES];   // Right samples for 4 clones at one time position
} interleaved_frame_t;

/**
 * Spatial mode enumeration
 */
typedef enum {
    MODE_TRIBAL = 0,      // Circular panning
    MODE_MILITARY = 1,    // Linear array
    MODE_ANGEL = 2        // Stochastic positioning
} spatial_mode_t;

/**
 * Main Delay Effect Class
 */
class PercussionSpatializer {
public:
    /*===========================================================================*/
    /* Lifecycle Methods */
    /*===========================================================================*/

    PercussionSpatializer()
        : delay_line_(nullptr)
        , write_ptr_(0)
        , clone_count_(4)
        , current_mode_(MODE_TRIBAL)
        , bypass_(true)  // Default to bypass until initialized
        , initialized_(false)
        , sample_rate_(48000) {

        // Initialize phase increment vector
        phase_inc_ = vdupq_n_f32(0.0f);

        // Clear parameter arrays
        memset(params_, 0, sizeof(params_));
        memset(last_params_, 0, sizeof(last_params_));
    }

    ~PercussionSpatializer() {
        // Free allocated memory
        if (delay_line_ != nullptr) {
            free(delay_line_);
            delay_line_ = nullptr;
        }
    }

    inline int8_t Init(const unit_runtime_desc_t* desc) {
        if (desc->samplerate != 48000) return k_unit_err_samplerate;
        if (desc->input_channels != 2 || desc->output_channels != 2)
            return k_unit_err_geometry;

        sample_rate_ = desc->samplerate;

        // =================================================================
        // FIXED: Check allocation success
        // =================================================================
        delay_line_ = (interleaved_frame_t*)memalign(16, DELAY_MAX_SAMPLES * sizeof(interleaved_frame_t));
        if (delay_line_ == nullptr) {
            // Allocation failed - unit will bypass
            initialized_ = false;
            bypass_ = true;
            return k_unit_err_memory;
        }

        initialized_ = true;
        bypass_ = false;

        Reset();
        return k_unit_err_none;
    }

    inline void Teardown() {
        // Cleanup handled in destructor
    }

    inline void Reset() {
        if (!initialized_ || delay_line_ == nullptr) {
            bypass_ = true;
            return;
        }

        // Clear interleaved delay line using NEON
        float32x4_t zero = vdupq_n_f32(0.0f);
        interleaved_frame_t zero_frame;
        for (int i = 0; i < NEON_LANES; i++) {
            zero_frame.left[i] = zero;
            zero_frame.right[i] = zero;
        }

        for (int i = 0; i < DELAY_MAX_SAMPLES; i++) {
            delay_line_[i] = zero_frame;
        }
        write_ptr_ = 0;

        // Reset clone parameters
        init_clone_parameters();
    }

    inline void Resume() {}
    inline void Suspend() {}

    /*===========================================================================*/
    /* Core Processing */
    /*===========================================================================*/

    fast_inline void Process(const float* in, float* out, size_t frames) {
        // =================================================================
        // FIXED: Check initialization before processing
        // =================================================================
        if (bypass_ || !initialized_ || delay_line_ == nullptr) {
            // Fast bypass - just copy input to output
            memcpy(out, in, frames * 2 * sizeof(float));
            return;
        }

        const float* in_p = in;
        float* out_p = out;
        const float* out_e = out_p + (frames << 1);

        // Process in blocks of 4 samples for NEON efficiency
        for (; out_p < out_e; in_p += 8, out_p += 8) {
            // Load 4 stereo samples
            float32x4_t in_l4 = vld1q_f32(in_p);      // Left channel, 4 samples
            float32x4_t in_r4 = vld1q_f32(in_p + 4);  // Right channel, 4 samples

            // Write to delay line
            write_to_delay(in_l4, in_r4);

            // Generate clones based on current mode
            float32x4_t out_l4, out_r4;
            generate_clones_neon(in_l4, in_r4, &out_l4, &out_r4);

            // Apply wet/dry mix
            apply_mix_neon(in_l4, in_r4, out_l4, out_r4, &out_l4, &out_r4);

            // Store results
            vst1q_f32(out_p, out_l4);
            vst1q_f32(out_p + 4, out_r4);
        }
    }

    /*===========================================================================*/
    /* Parameter Interface */
    /*===========================================================================*/

    inline void setParameter(uint8_t index, int32_t value) {
        if (index < 24) {
            params_[index] = value;
        }

        switch (index) {
            case 0: // Clone Count
                set_clone_count(value);
                break;
            case 1: // Mode
                set_mode(static_cast<spatial_mode_t>(value));
                break;
            case 2: // Depth
                depth_ = value / 100.0f;
                break;
            case 3: // Rate
                rate_ = 0.1f + (value / 100.0f) * 9.9f;
                phase_inc_ = vdupq_n_f32(rate_ / sample_rate_);
                break;
            case 4: // Spread
                spread_ = value / 100.0f;
                update_panning();
                break;
            case 5: // Mix
                mix_ = value / 100.0f;
                break;
        }
    }

    inline int32_t getParameterValue(uint8_t index) const {
        if (index < 24) {
            return params_[index];
        }
        return 0;
    }

    inline const char* getParameterStrValue(uint8_t index, int32_t value) const {
        static const char* mode_names[] = {"Tribal", "Military", "Angel"};
        static const char* clone_names[] = {"4", "8", "16"};

        switch (index) {
            case 0:
                if (value >= 0 && value <= 2) return clone_names[value];
                break;
            case 1:
                if (value >= 0 && value <= 2) return mode_names[value];
                break;
            default:
                break;
        }
        return nullptr;
    }

    inline void LoadPreset(uint8_t idx) { (void)idx; }
    inline uint8_t getPresetIndex() const { return 0; }
    static inline const char* getPresetName(uint8_t idx) { return nullptr; }

private:
    /*===========================================================================*/
    /* Private Methods - NEON Optimized */
    /*===========================================================================*/

    /**
     * FIXED: Correct ARMv7 horizontal sum for float32x4_t
     * @param v Vector of 4 floats
     * @return Sum of all 4 elements
     */
    fast_inline float horizontal_sum_f32x4(float32x4_t v) {
        // Step 1: Pairwise add low and high halves
        // vget_low_f32(v) = [v0, v1]
        // vget_high_f32(v) = [v2, v3]
        // vpadd_f32 adds adjacent pairs: [v0+v1, v2+v3]
        float32x2_t sum_halves = vpadd_f32(vget_low_f32(v), vget_high_f32(v));

        // Step 2: Add the two results together
        // vget_lane_f32(sum_halves, 0) = v0+v1
        // vget_lane_f32(sum_halves, 1) = v2+v3
        return vget_lane_f32(sum_halves, 0) + vget_lane_f32(sum_halves, 1);
    }

    /**
     * Alternative: Single-instruction for ARMv8/AArch64
     * Kept for reference but not used on drumlogue
     */
    fast_inline float horizontal_sum_f32x4_alt(float32x4_t v) {
        #if defined(__aarch64__)
        return vaddvq_f32(v);
        #else
        float32x2_t sum_halves = vpadd_f32(vget_low_f32(v), vget_high_f32(v));
        return vget_lane_f32(sum_halves, 0) + vget_lane_f32(sum_halves, 1);
        #endif
    }

    /**
     * Initialize clone parameters
     */
    void init_clone_parameters() {
        if (!initialized_) return;

        for (int group = 0; group < CLONE_GROUPS; group++) {
            clone_group_t* g = &clone_groups_[group];

            float base_delay = group * 0.5f;
            float offsets[NEON_LANES];
            float phases[NEON_LANES];

            for (int i = 0; i < NEON_LANES; i++) {
                offsets[i] = base_delay + (i * 0.1f);
                phases[i] = (float)(group * NEON_LANES + i) / MAX_CLONES;
            }

            g->delay_offsets = vld1q_f32(offsets);
            g->mod_phases = vld1q_f32(phases);
            g->left_gains = vdupq_n_f32(0.0f);
            g->right_gains = vdupq_n_f32(0.0f);
            g->phase_flags = vdupq_n_u32(0);
        }
        update_panning();
    }

    /**
     * Write 4 stereo samples to interleaved delay line
     */
    fast_inline void write_to_delay(float32x4_t in_l, float32x4_t in_r) {
        uint32_t pos = write_ptr_ & DELAY_MASK;

        float l_vals[NEON_LANES], r_vals[NEON_LANES];
        vst1q_f32(l_vals, in_l);
        vst1q_f32(r_vals, in_r);

        for (int i = 0; i < NEON_LANES; i++) {
            uint32_t time_pos = (pos + i) & DELAY_MASK;
            delay_line_[time_pos].left[i] = vdupq_n_f32(l_vals[i]);
            delay_line_[time_pos].right[i] = vdupq_n_f32(r_vals[i]);
        }

        write_ptr_ += NEON_LANES;
    }

    /**
     * Generate clone outputs
     */
    fast_inline void generate_clones_neon(float32x4_t in_l, float32x4_t in_r,
                                          float32x4_t* out_l, float32x4_t* out_r) {
        float32x4_t acc_l = vdupq_n_f32(0.0f);
        float32x4_t acc_r = vdupq_n_f32(0.0f);

        uint32_t num_groups = (clone_count_ + NEON_LANES - 1) / NEON_LANES;

        for (uint32_t g = 0; g < num_groups; g++) {
            clone_group_t* group = &clone_groups_[g];

            float32x4_t lfo = generate_lfo_neon(group->mod_phases);

            float32x4_t mod_delays = vmlaq_f32(group->delay_offsets, lfo, vdupq_n_f32(depth_));
            float32x4_t sample_offsets = vmulq_n_f32(mod_delays, 48.0f);

            float32x4_t delayed_l = read_delayed_neon(g, sample_offsets, 0);
            float32x4_t delayed_r = read_delayed_neon(g, sample_offsets, 1);

            apply_mode_filters(&mode_filters_, g, &delayed_l, &delayed_r);

            acc_l = vmlaq_f32(acc_l, delayed_l, group->left_gains);
            acc_r = vmlaq_f32(acc_r, delayed_r, group->right_gains);
        }

        *out_l = acc_l;
        *out_r = acc_r;
    }

    /**
     * Generate LFO using triangle wave
     */
    fast_inline float32x4_t generate_lfo_neon(float32x4_t phases) {
        float32x4_t half = vdupq_n_f32(0.5f);
        float32x4_t two = vdupq_n_f32(2.0f);
        float32x4_t diff = vsubq_f32(phases, half);
        float32x4_t abs_diff = vabsq_f32(diff);
        return vmulq_f32(abs_diff, two);
    }

    /**
     * Read from delay line
     */
    fast_inline float32x4_t read_delayed_neon(uint32_t group,
                                               float32x4_t offsets,
                                               int channel) {
        (void)group;

        uint32_t base_read = (write_ptr_ - 32) & DELAY_MASK;
        uint32_t indices[NEON_LANES];
        float offsets_f[NEON_LANES];
        vst1q_f32(offsets_f, offsets);

        for (int i = 0; i < NEON_LANES; i++) {
            uint32_t offset_samples = (uint32_t)(offsets_f[i] * 48.0f);
            indices[i] = (base_read - offset_samples) & DELAY_MASK;
        }

        float32x4_t result;
        if (channel == 0) {
            for (int i = 0; i < NEON_LANES; i++) {
                result = vsetq_lane_f32(vgetq_lane_f32(delay_line_[indices[i]].left[i], 0), result, i);
            }
        } else {
            for (int i = 0; i < NEON_LANES; i++) {
                result = vsetq_lane_f32(vgetq_lane_f32(delay_line_[indices[i]].right[i], 0), result, i);
            }
        }
        return result;
    }

    /**
     * Apply wet/dry mix
     */
    fast_inline void apply_mix_neon(float32x4_t dry_l, float32x4_t dry_r,
                                    float32x4_t wet_l, float32x4_t wet_r,
                                    float32x4_t* out_l, float32x4_t* out_r) {
        float32x4_t wet_gain = vdupq_n_f32(mix_);
        float32x4_t dry_gain = vdupq_n_f32(1.0f - mix_);

        *out_l = vaddq_f32(vmulq_f32(dry_l, dry_gain), vmulq_f32(wet_l, wet_gain));
        *out_r = vaddq_f32(vmulq_f32(dry_r, dry_gain), vmulq_f32(wet_r, wet_gain));
    }

    /**
     * Update panning based on mode
     */
    void update_panning() {
        // Simplified implementation
        for (int group = 0; group < CLONE_GROUPS; group++) {
            clone_group_t* g = &clone_groups_[group];
            for (int i = 0; i < NEON_LANES; i++) {
                float pos = (float)(group * NEON_LANES + i) / (clone_count_ - 1);
                float angle = pos * 2.0f * M_PI;

                // Simple sin/cos panning
                float left = sinf(angle) * spread_;
                float right = cosf(angle) * spread_;

                // Update gains using scalar for simplicity
                float left_vals[4], right_vals[4];
                vst1q_f32(left_vals, g->left_gains);
                vst1q_f32(right_vals, g->right_gains);
                left_vals[i] = left;
                right_vals[i] = right;
                g->left_gains = vld1q_f32(left_vals);
                g->right_gains = vld1q_f32(right_vals);
            }
        }
    }

    /**
     * Set clone count
     */
    void set_clone_count(int32_t value) {
        switch (value) {
            case 0: clone_count_ = 4; break;
            case 1: clone_count_ = 8; break;
            case 2: clone_count_ = 16; break;
            default: clone_count_ = 4;
        }
        update_panning();
    }

    /**
     * Set spatial mode
     */
    void set_mode(spatial_mode_t mode) {
        current_mode_ = mode;
        update_panning();
    }

    /*===========================================================================*/
    /* Private Member Variables */
    /*===========================================================================*/

    // INTERLEAVED delay line
    interleaved_frame_t* delay_line_ __attribute__((aligned(16)));
    uint32_t write_ptr_;

    // Clone processing groups
    clone_group_t clone_groups_[CLONE_GROUPS] __attribute__((aligned(16)));

    // Mode-specific filters
    mode_filters_t mode_filters_;

    // Parameters
    spatial_mode_t current_mode_;
    uint32_t clone_count_;
    float depth_;
    float rate_;
    float spread_;
    float mix_;
    uint32_t sample_rate_;
    bool bypass_;
    bool initialized_;

    // Parameter storage
    int32_t params_[24];
    int32_t last_params_[24];

    // Vectorized phase increment
    float32x4_t phase_inc_;

    // Flags for atomic operations
    std::atomic_uint_fast32_t flags_;
};
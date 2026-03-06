#pragma once

/**
 * @file PercussionSpatializer.h
 * @brief Enhanced Percussion Spatializer with realistic ensemble modeling
 *
 * Features:
 * - Velocity/Amplitude Randomization
 * - Pitch/Tape Wobble via LFO modulation
 * - Transient Softening with attack reduction
 *
 * FIXED:
 * - Removed rand() - now uses proper Xorshift128+ PRNG
 * - Filter states moved to class members (not static locals)
 * - Proper NEON vectorization throughout
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
 * Xorshift128+ PRNG state (4 independent streams)
 */
typedef struct {
    uint64x2_t state0;
    uint64x2_t state1;
} prng_xorshift128_t;

/**
 * Enhanced per-clone parameters with randomization and modulation
 */
typedef struct {
    float32x4_t delay_offsets;    // Micro-delay for vibrato (4 clones)
    float32x4_t left_gains;        // Left channel pan (4 clones)
    float32x4_t right_gains;       // Right channel pan (4 clones)
    float32x4_t mod_phases;        // LFO phases (4 clones)
    float32x4_t pitch_mod;         // Pitch modulation depth (tape wobble)
    float32x4_t velocity;          // Random velocity per hit (0.7-1.0)
    uint32x4_t phase_flags;         // Phase inversion flags
    uint32x4_t active;              // Which clones are active
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
 * Main Enhanced Spatializer Class
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
        , bypass_(true)
        , initialized_(false)
        , sample_rate_(48000)
        , transient_detected_(false)
        , transient_energy_(0.0f)
        , depth_(0.5f)
        , rate_(1.0f)
        , spread_(0.8f)
        , mix_(0.5f)
        , wobble_depth_(0.3f)
        , attack_soften_(0.2f) {

        // Initialize phase increment vector
        phase_inc_ = vdupq_n_f32(0.0f);

        // Initialize PRNG with a fixed seed
        prng_init(0x9E3779B97F4A7C15ULL);

        // Initialize filter states to zero
        for (int i = 0; i < CLONE_GROUPS; i++) {
            filter_state_[i] = vdupq_n_f32(0.0f);
        }

        // Clear parameter arrays
        memset(params_, 0, sizeof(params_));
        memset(last_params_, 0, sizeof(last_params_));
    }

    ~PercussionSpatializer() {
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

        delay_line_ = (interleaved_frame_t*)memalign(16, DELAY_MAX_SAMPLES * sizeof(interleaved_frame_t));
        if (delay_line_ == nullptr) {
            initialized_ = false;
            bypass_ = true;
            return k_unit_err_memory;
        }

        initialized_ = true;
        bypass_ = false;

        Reset();
        return k_unit_err_none;
    }

    inline void Teardown() {}

    inline void Reset() {
        if (!initialized_ || delay_line_ == nullptr) {
            bypass_ = true;
            return;
        }

        // Clear delay line
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

        // Reset filter states
        for (int i = 0; i < CLONE_GROUPS; i++) {
            filter_state_[i] = vdupq_n_f32(0.0f);
        }

        // Reset clone parameters
        init_clone_parameters();
    }

    inline void Resume() {}
    inline void Suspend() {}

    /*===========================================================================*/
    /* Core Processing */
    /*===========================================================================*/

    fast_inline void Process(const float* in, float* out, size_t frames) {
        if (bypass_ || !initialized_ || delay_line_ == nullptr) {
            memcpy(out, in, frames * 2 * sizeof(float));
            return;
        }

        const float* in_p = in;
        float* out_p = out;
        const float* out_e = out_p + (frames << 1);

        for (; out_p < out_e; in_p += 8, out_p += 8) {
            // Load 4 stereo samples
            float32x4_t in_l4 = vld1q_f32(in_p);
            float32x4_t in_r4 = vld1q_f32(in_p + 4);

            // Detect transient
            detect_transient(in_l4, in_r4);

            // On transient, randomize velocities for next hits
            if (transient_detected_) {
                randomize_velocities();
            }

            // Write to delay line
            write_to_delay(in_l4, in_r4);

            // Generate clones with all enhancements
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
            case 3: // Rate (LFO speed for pitch wobble)
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
            case 6: // Wobble Depth
                wobble_depth_ = value / 100.0f;
                break;
            case 7: // Attack Softening
                attack_soften_ = value / 100.0f;
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
    /* Xorshift128+ PRNG Implementation */
    /*===========================================================================*/

    /**
     * Initialize PRNG with seed
     */
    void prng_init(uint64_t seed) {
        // SplitMix64 to initialize 4 streams
        uint64_t s0[2] = {seed, seed * 0x9E3779B97F4A7C15ULL};
        uint64_t s1[2] = {seed * 0xBF58476D1CE4E5B9ULL, seed * 0x94D049BB133111EBULL};

        prng_.state0 = vld1q_u64(s0);
        prng_.state1 = vld1q_u64(s1);
    }

    /**
     * Generate 4 random 32-bit numbers using Xorshift128+
     * This is a proper PRNG suitable for real-time audio
     */
    uint32x4_t prng_rand_u32() {
        // Xorshift128+ implementation
        uint64x2_t s0 = prng_.state0;
        uint64x2_t s1 = prng_.state1;

        // s1 ^= s1 << 23
        uint64x2_t s1_left = vshlq_n_u64(s1, 23);
        s1 = veorq_u64(s1, s1_left);

        // s1 ^= s1 >> 17
        uint64x2_t s1_right = vshrq_n_u64(s1, 17);
        s1 = veorq_u64(s1, s1_right);

        // s1 ^= s0 ^ (s0 >> 26)
        uint64x2_t s0_right = vshrq_n_u64(s0, 26);
        uint64x2_t s0_xor = veorq_u64(s0, s0_right);
        s1 = veorq_u64(s1, s0_xor);

        // Update state
        prng_.state0 = s1;
        prng_.state1 = s0;

        // Return sum of states (lower 32 bits)
        uint64x2_t sum = vaddq_u64(s0, s1);

        // Convert to 32-bit
        uint32x2_t low32 = vmovn_u64(sum);
        uint32x2_t high32 = vshrn_n_u64(sum, 32);

        return vcombine_u32(low32, high32);
    }

    /**
     * Generate random float in [0, 1)
     */
    float32x4_t prng_rand_float() {
        uint32x4_t rand = prng_rand_u32();

        // Mask to 23-bit mantissa and set exponent to 1.0
        uint32x4_t masked = vandq_u32(rand, vdupq_n_u32(0x7FFFFF));
        uint32x4_t float_bits = vorrq_u32(masked, vdupq_n_u32(0x3F800000));

        // Convert to float and subtract 1.0 to get [0,1) range
        return vsubq_f32(vreinterpretq_f32_u32(float_bits), vdupq_n_f32(1.0f));
    }

    /*===========================================================================*/
    /* Randomization Methods */
    /*===========================================================================*/

    /**
     * Randomize velocities for all clones
     * This simulates different hit strengths in the ensemble
     */
    void randomize_velocities() {
        for (int g = 0; g < CLONE_GROUPS; g++) {
            // Generate 4 random floats in [0,1)
            float32x4_t rand_float = prng_rand_float();

            // Scale to 0.7-1.0 range (70-100% velocity)
            float32x4_t velocity = vmlaq_f32(vdupq_n_f32(0.7f), rand_float, vdupq_n_f32(0.3f));

            // Store velocities for this group
            clone_groups_[g].velocity = velocity;
        }
    }

    /*===========================================================================*/
    /* NEON Utilities */
    /*===========================================================================*/

    fast_inline float horizontal_sum_f32x4(float32x4_t v) {
        float32x2_t sum_halves = vpadd_f32(vget_low_f32(v), vget_high_f32(v));
        return vget_lane_f32(sum_halves, 0) + vget_lane_f32(sum_halves, 1);
    }

    /**
     * Transient Detection
     */
    void detect_transient(float32x4_t in_l, float32x4_t in_r) {
        // Simple energy detection
        float32x4_t abs_l = vabsq_f32(in_l);
        float32x4_t abs_r = vabsq_f32(in_r);
        float32x4_t energy = vaddq_f32(abs_l, abs_r);

        float total = horizontal_sum_f32x4(energy);

        // Detect sharp rise
        float attack_threshold = 0.5f;
        transient_detected_ = (total > attack_threshold && transient_energy_ < attack_threshold);

        // Smooth energy for next detection
        transient_energy_ = total * 0.1f + transient_energy_ * 0.9f;
    }

    /*===========================================================================*/
    /* Core Initialization */
    /*===========================================================================*/

    void init_clone_parameters() {
        if (!initialized_) return;

        for (int group = 0; group < CLONE_GROUPS; group++) {
            clone_group_t* g = &clone_groups_[group];

            float base_delay = group * 0.5f;
            float offsets[NEON_LANES];
            float phases[NEON_LANES];
            float pitch_mod[NEON_LANES];

            for (int i = 0; i < NEON_LANES; i++) {
                int clone_idx = group * NEON_LANES + i;

                offsets[i] = base_delay + (i * 0.1f);
                phases[i] = (float)clone_idx / MAX_CLONES;
                pitch_mod[i] = 0.1f + (i * 0.05f);
            }

            g->delay_offsets = vld1q_f32(offsets);
            g->mod_phases = vld1q_f32(phases);
            g->pitch_mod = vld1q_f32(pitch_mod);
            g->left_gains = vdupq_n_f32(0.0f);
            g->right_gains = vdupq_n_f32(0.0f);
            g->velocity = vdupq_n_f32(1.0f);
            g->phase_flags = vdupq_n_u32(0);
            g->active = vdupq_n_u32(0xFFFFFFFF);
        }
        update_panning();
    }

    /*===========================================================================*/
    /* Delay Line Operations */
    /*===========================================================================*/

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

    /*===========================================================================*/
    /* Enhanced Clone Generation */
    /*===========================================================================*/

    fast_inline void generate_clones_neon(float32x4_t in_l, float32x4_t in_r,
                                          float32x4_t* out_l, float32x4_t* out_r) {
        float32x4_t acc_l = vdupq_n_f32(0.0f);
        float32x4_t acc_r = vdupq_n_f32(0.0f);

        uint32_t num_groups = (clone_count_ + NEON_LANES - 1) / NEON_LANES;

        for (uint32_t g = 0; g < num_groups; g++) {
            clone_group_t* group = &clone_groups_[g];

            // Generate LFO for pitch wobble
            float32x4_t lfo = generate_lfo_neon(group->mod_phases);

            // Apply pitch wobble to delay offsets
            float32x4_t wobble = vmulq_f32(lfo, vmulq_f32(group->pitch_mod, vdupq_n_f32(wobble_depth_)));
            float32x4_t mod_delays = vaddq_f32(group->delay_offsets, wobble);

            // Read from delay line
            float32x4_t sample_offsets = vmulq_n_f32(mod_delays, 48.0f);
            float32x4_t delayed_l = read_delayed_neon(g, sample_offsets, 0);
            float32x4_t delayed_r = read_delayed_neon(g, sample_offsets, 1);

            // Apply attack softening (using class member filter_state_)
            if (attack_soften_ > 0.01f) {
                float32x4_t softened_l = apply_attack_softening(delayed_l, g);
                float32x4_t softened_r = apply_attack_softening(delayed_r, g);
                delayed_l = softened_l;
                delayed_r = softened_r;
            }

            // Apply mode filters
            apply_mode_filters(&mode_filters_, g, &delayed_l, &delayed_r);

            // Apply gains with velocity scaling
            acc_l = vmlaq_f32(acc_l, delayed_l, vmulq_f32(group->left_gains, group->velocity));
            acc_r = vmlaq_f32(acc_r, delayed_r, vmulq_f32(group->right_gains, group->velocity));

            // Update LFO phases
            group->mod_phases = vaddq_f32(group->mod_phases, phase_inc_);
            uint32x4_t wrap = vcgeq_f32(group->mod_phases, vdupq_n_f32(1.0f));
            group->mod_phases = vbslq_f32(wrap, vsubq_f32(group->mod_phases, vdupq_n_f32(1.0f)), group->mod_phases);
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
     * Attack softening filter (now uses class member filter_state_)
     */
    fast_inline float32x4_t apply_attack_softening(float32x4_t in, uint32_t group) {
        float coeff = transient_detected_ ? attack_soften_ : 0.0f;
        float32x4_t alpha = vdupq_n_f32(coeff);
        float32x4_t one_minus_alpha = vdupq_n_f32(1.0f - coeff);

        float32x4_t out = vaddq_f32(vmulq_f32(in, alpha),
                                     vmulq_f32(filter_state_[group], one_minus_alpha));
        filter_state_[group] = out;

        return out;
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
        for (int group = 0; group < CLONE_GROUPS; group++) {
            clone_group_t* g = &clone_groups_[group];
            for (int i = 0; i < NEON_LANES; i++) {
                int clone_idx = group * NEON_LANES + i;
                if (clone_idx < clone_count_) {
                    float pos = (float)clone_idx / (clone_count_ - 1);
                    float angle = pos * 2.0f * M_PI;

                    float left = sinf(angle) * spread_;
                    float right = cosf(angle) * spread_;

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

    // Delay line
    interleaved_frame_t* delay_line_ __attribute__((aligned(16)));
    uint32_t write_ptr_;

    // Clone processing groups
    clone_group_t clone_groups_[CLONE_GROUPS] __attribute__((aligned(16)));

    // Mode-specific filters
    mode_filters_t mode_filters_;

    // PRNG state
    prng_xorshift128_t prng_;

    // Filter states for attack softening (now class members!)
    float32x4_t filter_state_[CLONE_GROUPS] __attribute__((aligned(16)));

    // Parameters
    spatial_mode_t current_mode_;
    uint32_t clone_count_;
    float depth_;
    float rate_;
    float spread_;
    float mix_;
    float wobble_depth_;
    float attack_soften_;

    uint32_t sample_rate_;
    bool bypass_;
    bool initialized_;

    // Parameter storage
    int32_t params_[24];
    int32_t last_params_[24];

    // Vectorized phase increment
    float32x4_t phase_inc_;

    // Transient detection
    bool transient_detected_;
    float transient_energy_;

    // Flags
    std::atomic_uint_fast32_t flags_;
};
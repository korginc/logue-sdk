#pragma once
/**
 * PercussionSpatializer.h
 *
 * Percussion micro-ensemble / spacing effect for Korg Drumlogue.
 *
 * Five clone-set values:
 *   2, 4, 6, 8, 10
 *
 * New parameter:
 *   gap_ = percussion spacing / separation
 *
 * Notes:
 * - Scatter controls looseness and detachment.
 * - Gap controls actual delay separation between clones.
 * - Render is block-based (4 frames), with NEON clone batching.
 */

#include <arm_neon.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "constants.h"
#include "unit.h"
#include "spatial_modes.h"
#include "float_math.h"

#ifndef fast_inline
#define fast_inline inline __attribute__((always_inline))
#endif

struct delay_line_t {
    static constexpr int kLen  = 32768;
    static constexpr int kMask = kLen - 1;

    float l[kLen];
    float r[kLen];
    int write = 0;

    void clear() {
        std::memset(l, 0, sizeof(l));
        std::memset(r, 0, sizeof(r));
        write = 0;
    }

    fast_inline void push(float in_l, float in_r) {
        l[write] = in_l;
        r[write] = in_r;
        write = (write + 1) & kMask;
    }

    fast_inline void read5(int base, float* sl, float* sr) const {
        for (int s = 0; s < 5; ++s) {
            const int idx = (base + s) & kMask;
            sl[s] = l[idx];
            sr[s] = r[idx];
        }
    }
};

// Update the clone_t struct to include filter parameters
typedef struct {
    float delay_samples;
    float wobble_depth_samples;
    float scatter_samples;
    float pan_gain_l;
    float pan_gain_r;
    float base_gain;
    float net_gain_l;
    float net_gain_r;
    float wobble_phase;
    float wobble_rate_mul;

    // Add these for real one-pole IIR low-pass filtering
    float lp_coef;
    float lp_state_l;
    float lp_state_r;
} clone_t;

enum params {
    k_clones = 0,
    k_mode,
    k_depth,
    k_rate,
    k_spread,
    k_mix,
    k_wobble,
    k_scatter,
    k_attack_softening,
    k_gap,
    k_total,
};

class PercussionSpatializer {
public:
    PercussionSpatializer();
    ~PercussionSpatializer() = default;

    int8_t Init(const unit_runtime_desc_t* desc);
    void   Teardown();
    void   Reset();
    void   Resume() {}
    void   Suspend() {}

    void setParameter(uint8_t index, int32_t value);
    int32_t getParameterValue(uint8_t index) const;
    const char* getParameterStrValue(uint8_t index, int32_t value) const;
    const uint8_t* getParameterBmpValue(uint8_t index, int32_t value) const;

    void Render(const float* in, float* out, size_t frames);
    static fast_inline float horizontal_sum4(float32x4_t v) {
#if defined(__aarch64__)
      return vaddvq_f32(v);
#else
      float32x2_t s = vpadd_f32(vget_low_f32(v), vget_high_f32(v));
      s = vpadd_f32(s, s);
      return vget_lane_f32(s, 0);
#endif
    }
    static fast_inline float horizontal_sum2(float32x2_t v) {
      float32x2_t s = vpadd_f32(v, v);
      return vget_lane_f32(s, 0);
    }
    // setters and getters are by definition public
    void set_clone_count_index(int idx);
    void set_mode(spatial_mode_t mode);
    void set_depth(float norm);
    void set_rate(float norm);
    void set_spread(float norm);
    void set_mix(float norm);
    void set_wobble(float norm);
    void set_scatter(float norm);
    void set_attack_softening(float norm);
    void set_gap(float norm);

    void set_delay(float in_l, float in_r);
    float get_depth();
    float get_spread();
    float get_gap();
    float get_rate();
    float get_mix();
    float get_wobble();
    float get_attack_softening();
    int   get_clone_count();
    clone_t* get_clones();
    float get_scatter();
    delay_line_t& get_delay();

        private : static constexpr int kMaxClones = 10;
    static constexpr uint32_t kSmoothBlocks = 120;

    void rebuild_profile();
    void randomize_hit();
    void update_clone_dynamics();
    void advance_smoothing();


    void render_block4(const float* in, float* out);
    void render_scalar_frame(const float* in, float* out);

    static fast_inline float clamp01(float x) { return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); }


private:
    delay_line_t delay_;
    uint32_t sample_rate_ = 48000;
    bool initialized_ = false;

    spatial_mode_t mode_ = MODE_TRIBAL;
    int clone_set_index_ = CLONE_SET_4;
    int clone_count_ = 4;

    int8_t params_[k_total] = {};

    float depth_  = 0.50f;
    float spread_ = 0.80f;
    float gap_    = 0.10f;

    float rate_     = 1.00f;  float rate_target_     = 1.00f;
    float mix_      = 0.42f;  float mix_target_      = 0.42f;
    float wobble_   = 0.25f;  float wobble_target_   = 0.25f;
    float scatter_  = 0.25f;  float scatter_target_  = 0.25f;
    float soft_atk_ = 0.20f;  float soft_atk_target_ = 0.20f;

    clone_t clones_[kMaxClones]{};
    spatial_profile_t profile_{};

    uint32_t smoothing_remaining_ = 0;
    uint32_t rng_state_ = 0x9E3779B9u;
    bool pending_profile_rebuild_ = true;
    float prev_mag_ = 0.0f;
};

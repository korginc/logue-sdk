/**
 * @file PercussionSpatializer.cc
 */

#include "PercussionSpatializer.h"
#include <cmath>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
constexpr float twopi = 6.2831853f;
static inline float xorshift_f32(uint32_t& s) {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return (float)(s & 0x00FFFFFFu) / (float)0x01000000u;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

PercussionSpatializer::PercussionSpatializer() {
    memset(params_, 0, sizeof(params_));
    clone_set_index_ = CLONE_SET_4;
    clone_count_     = kCloneValues[clone_set_index_];
}

int8_t PercussionSpatializer::Init(const unit_runtime_desc_t* desc) {
    if (desc->samplerate != 48000) return k_unit_err_samplerate;
    if (desc->input_channels != 2 || desc->output_channels != 2)
        return k_unit_err_geometry;
    sample_rate_  = desc->samplerate;
    initialized_  = true;
    Reset();
    return k_unit_err_none;
}

void PercussionSpatializer::Teardown() {}

void PercussionSpatializer::Reset() {
    if (!initialized_) return;
    delay_.clear();
    rng_state_  = 0x9E3779B9u;
    prev_mag_   = 0.0f;
    smoothing_remaining_ = 0;

    // Clear filter histories to prevent pops on reset
    for (int i = 0; i < kMaxClones; ++i) {
        clones_[i].lp_state_l = 0.0f;
        clones_[i].lp_state_r = 0.0f;
        clones_[i].wobble_sine_start = 0.0f;
        clones_[i].wobble_sine_delta = 0.0f;
        clones_[i].lp_state_r = 0.0f;
        clones_[i].hit_accent = 1.0f; // Initialize baseline
    }

    rebuild_profile();
    randomize_hit();
}

// ---------------------------------------------------------------------------
// Parameter setters
// ---------------------------------------------------------------------------

void PercussionSpatializer::set_clone_count_index(int idx) {
    idx = idx < 0 ? 0 : (idx >= CLONE_SET_CNT ? CLONE_SET_CNT - 1 : idx);
    clone_set_index_         = idx;
    clone_count_             = kCloneValues[idx];
    pending_profile_rebuild_ = true;
}

void PercussionSpatializer::set_mode(spatial_mode_t mode) {
    if (mode >= MODE_COUNT) mode = MODE_TRIBAL;
    mode_ = mode;
    pending_profile_rebuild_ = true;
}

// Spatial parameters: immediate rebuild at next block boundary
void PercussionSpatializer::set_depth(float norm)  { depth_  = clamp01(norm); pending_profile_rebuild_ = true; }
void PercussionSpatializer::set_spread(float norm) { spread_ = clamp01(norm); pending_profile_rebuild_ = true; }
void PercussionSpatializer::set_gap(float norm)    { gap_    = clamp01(norm); pending_profile_rebuild_ = true; }

void PercussionSpatializer::set_scatter(float norm) {
    scatter_target_ = clamp01(norm);
    smoothing_remaining_ = kSmoothBlocks;
    pending_profile_rebuild_ = true;   // rebuild once now with current scatter, again at end
}

// Dynamic parameters: smoothed, no full rebuild (update_clone_dynamics handles them)
void PercussionSpatializer::set_rate(float norm)            { rate_target_     = 0.05f + clamp01(norm) * 9.95f; smoothing_remaining_ = kSmoothBlocks; }
void PercussionSpatializer::set_mix(float norm)             { mix_target_      = clamp01(norm);  smoothing_remaining_ = kSmoothBlocks; }
void PercussionSpatializer::set_wobble(float norm)          { wobble_target_   = clamp01(norm);  smoothing_remaining_ = kSmoothBlocks; }
void PercussionSpatializer::set_attack_softening(float norm){ soft_atk_target_ = clamp01(norm);  smoothing_remaining_ = kSmoothBlocks; }

void PercussionSpatializer::set_delay(float in_l, float in_r) {
  delay_.push(in_l, in_r);
}

// ---------------------------------------------------------------------------
// Parameter getters
// ---------------------------------------------------------------------------
float PercussionSpatializer::get_depth() {
  return depth_;
}
float PercussionSpatializer::get_spread() {
  return spread_;
}
float PercussionSpatializer::get_gap() {
  return gap_;
}
float PercussionSpatializer::get_rate() {
  return rate_;
}
float PercussionSpatializer::get_mix() {
  return mix_;  // smoothed value — not mix_target_ which would bypass smoothing
}
float PercussionSpatializer::get_wobble() {
  return wobble_target_;
}
float PercussionSpatializer::get_attack_softening() {
  return soft_atk_target_;
}
int PercussionSpatializer::get_clone_count() {
  return clone_count_;
}
clone_t* PercussionSpatializer::get_clones() {
  return clones_;
}
float PercussionSpatializer::get_scatter() {
  return scatter_;
}
delay_line_t& PercussionSpatializer::get_delay() {
  return delay_;
}
spatial_mode_t PercussionSpatializer::get_mode() {
    return mode_;
}
float* PercussionSpatializer::get_net_gains_l() {
    return net_gains_l_;
}
float* PercussionSpatializer::get_net_gains_r() {
    return net_gains_r_;
}
float* PercussionSpatializer::get_lp_coefs() {
    return lp_coefs_;
}
float* PercussionSpatializer::get_lp_states_l() {
    return lp_states_l_;
}
float* PercussionSpatializer::get_lp_states_r() {
    return lp_states_r_;
}

// ---------------------------------------------------------------------------
// Smoothing — called once per 4-frame block
// Does NOT trigger rebuild_profile().  update_clone_dynamics() updates gains
// cheaply.  One full rebuild fires at the end of the smoothing window.
// ---------------------------------------------------------------------------
void PercussionSpatializer::advance_smoothing() {
    if (smoothing_remaining_ == 0) return;

    const float inv_rem = 1.0f / (float)smoothing_remaining_;
    rate_     += (rate_target_     - rate_)     * inv_rem;
    mix_      += (mix_target_      - mix_)      * inv_rem;
    wobble_   += (wobble_target_   - wobble_)   * inv_rem;
    scatter_  += (scatter_target_  - scatter_)  * inv_rem;
    soft_atk_ += (soft_atk_target_ - soft_atk_) * inv_rem;

    update_clone_dynamics();  // cheap: no pow/sqrt

    if (--smoothing_remaining_ == 0) {
        rate_     = rate_target_;
        mix_      = mix_target_;
        wobble_   = wobble_target_;
        scatter_  = scatter_target_;
        soft_atk_ = soft_atk_target_;
        pending_profile_rebuild_ = true;   // final rebuild with settled values
    }
}

// ---------------------------------------------------------------------------
// update_clone_dynamics — cheap per-block refresh of net gains + wobble depth
// Only arithmetic: no fasterpowf, no my_sqrt_f.
// ---------------------------------------------------------------------------
void PercussionSpatializer::update_clone_dynamics() {
    const float wobble_ms = 0.20f + wobble_ * 2.8f;
    const float inv_cnt1  = 1.0f / (float)(clone_count_ > 1 ? clone_count_ - 1 : 1);
    const float gap_boost = 1.0f + gap_ * 0.22f + scatter_ * 0.10f;

    for (int i = 0; i < clone_count_; ++i) {
        const float t        = (float)i * inv_cnt1;
        const float soft_fac = (i == 0) ? 1.0f : (0.82f + 0.18f * (1.0f - soft_atk_));
        clones_[i].wobble_depth_samples = wobble_ms * (0.20f + 0.30f * t) * (sample_rate_ * 0.001f);

        net_gains_l_[i] = clones_[i].base_gain * soft_fac * clones_[i].pan_gain_l * clones_[i].hit_accent * clones_[i].dynamic_gain_factor * gap_boost;
        net_gains_r_[i] = clones_[i].base_gain * soft_fac * clones_[i].pan_gain_r * clones_[i].hit_accent * clones_[i].dynamic_gain_factor * gap_boost;

        // Multiply by hit_accent at the very end of gain assembly
        clones_[i].net_gain_l = clones_[i].base_gain * soft_fac * clones_[i].pan_gain_l * clones_[i].hit_accent * clones_[i].dynamic_gain_factor;
        clones_[i].net_gain_r = clones_[i].base_gain * soft_fac * clones_[i].pan_gain_r * clones_[i].hit_accent * clones_[i].dynamic_gain_factor;
    }
}

// ---------------------------------------------------------------------------
// Mode helpers
// ---------------------------------------------------------------------------
static float mode_pan_exponent(spatial_mode_t mode) {
    switch (mode) {
        case MODE_MILITARY: return 1.12f;
        case MODE_ANGEL:    return 0.78f;
        default:            return 0.90f;
    }
}

static pan_model_t mode_pan_model(spatial_mode_t mode) {
    switch (mode) {
        case MODE_MILITARY: return PAN_MODEL_LINE;
        case MODE_ANGEL:    return PAN_MODEL_SCATTER;
        default:            return PAN_MODEL_CIRCLE;
    }
}

// ---------------------------------------------------------------------------
// rebuild_profile — full spatial rebuild; called at most twice per parameter
// change (once at first block after change, once at end of smoothing).
// ---------------------------------------------------------------------------
void PercussionSpatializer::rebuild_profile() {
    // Base delay times (ms) per mode
    // Base delays (ms): span 50-450ms so each clone sits ~40ms apart — the minimum
    // perceptual gap to hear distinct hits rather than a flam/shake. Previous values
    // (18-115ms, ~11ms gaps) collapsed all clones into a single smeared transient.
    // compress the delay times to pull them back into the ensemble fusion window, drop the fake lp_attn scalar,
    // and calculate a standard, stable one-pole low-pass filter coefficient using:
    // $$\omega = \frac{2\pi \cdot f_c}{f_s}$$$$\alpha = \frac{\omega}{\omega + 1}$$
    static const float tribal[MAX_CLONES]   = {  8.f, 16.f, 25.f, 35.f, 46.f, 58.f, 70.f, 82.f, 94.f, 106.f };
    // Military: slightly earlier entries, wider mid-range spread, ensemble punch
    static const float military[MAX_CLONES] = {  6.f, 12.f, 19.f, 27.f, 36.f, 46.f, 57.f, 69.f, 81.f,  93.f };
    // Angel: long-tailed, wider gaps for airy / spatially diffuse feel
    static const float angel[MAX_CLONES]    = { 10.f, 19.f, 29.f, 40.f, 52.f, 65.f, 79.f, 94.f, 110.f, 126.f };


    const float* base_delay = (mode_ == MODE_MILITARY) ? military :
                              (mode_ == MODE_ANGEL)    ? angel    : tribal;

    // Profile fields consumed by randomize_hit()
    profile_.jitter_ms      = 0.4f + depth_ * 1.6f + gap_ * 0.8f; // tightened jitter
    profile_.scatter_amount = (mode_ == MODE_ANGEL ? 0.18f : 0.08f) + scatter_ * (mode_ == MODE_ANGEL ? 0.82f : 0.55f);
    profile_.pan_exponent   = mode_pan_exponent(mode_);
    profile_.pan_model      = mode_pan_model(mode_);

    // HP/LP base frequencies and per-follower multipliers
    float hp_base, lp_base, hp_follow;
    if (mode_ == MODE_TRIBAL) {
        hp_base = 60.0f; lp_base = 3500.0f; hp_follow = 0.36f;
    } else if (mode_ == MODE_MILITARY) {
        // Lower HP preserves the body/punch of each stroke.
        // Was 900 Hz (killed everything below — followers became inaudible).
        hp_base = 130.0f; lp_base = 7500.0f; hp_follow = 0.28f;
    } else {
        hp_base = 200.0f; lp_base = 5500.0f; hp_follow = 0.34f;
    }

    // Gain rolloff: exponential avoids the linear formula going negative at i>=9
    // Military: 0.92^i — slow rolloff keeps late clones audible (ensemble presence)
    // Faster gain decay profile to control the tail length
    const float gain_step = (mode_ == MODE_MILITARY) ? 0.84f :
                            (mode_ == MODE_ANGEL)    ? 0.78f : 0.86f;

    const float ms_to_smp = (float)sample_rate_ * 0.001f;
    const float wobble_ms = 0.15f + wobble_ * 1.8f; // reduced extreme wobble depth
    const float inv_cnt1  = 1.0f / (float)(clone_count_ > 1 ? clone_count_ - 1 : 1);
    const float gap_ms    = 3.0f + gap_ * 22.0f; // tighter gap scaling

    for (int i = 0; i < clone_count_; ++i) {
        const float t        = (float)i * inv_cnt1;
        const float follower = t;

        // Delay and wobble (samples)
        const float gap_spread = gap_ms * (0.40f + 0.95f * t);
        float dms = base_delay[i] * (0.70f + 0.50f * depth_) + depth_ * 10.0f * t + gap_spread;
        clones_[i].delay_samples = fmaxf(2.0f, dms * ms_to_smp);
        clones_[i].wobble_depth_samples = wobble_ms * (0.20f + 0.30f * t) * ms_to_smp;

        // Per-clone wobble rate stagger (makes each clone drift independently)
        clones_[i].wobble_rate_mul = 0.70f + 0.30f * t;
        // Decorrelate L/R by offsetting the starting phase based on clone index
        // This ensures the "clones" aren't moving in perfect sync, which
        // prevents metallic phasing artifacts.
        clones_[i].wobble_phase = (xorshift_f32(rng_state_) + (float)i * 0.125f) * 2.0f * M_PI;

        // Pan position
        float base_x = 0.0f;
        switch (profile_.pan_model) {
            case PAN_MODEL_CIRCLE: {
                float arc = t * M_PI - 1.5707963f;
                base_x = fastersinfullf(arc);
                break;
            }
            case PAN_MODEL_LINE:
                base_x = t * 2.0f - 1.0f;
                break;
            case PAN_MODEL_SCATTER: {
                float center = t * 2.0f - 1.0f;
                float jit    = (xorshift_f32(rng_state_) * 2.0f - 1.0f) * profile_.scatter_amount;
                base_x       = clampf(center + jit, -1.0f, 1.0f);
                break;
            }
        }

        float scatter_shape = profile_.scatter_amount * (0.20f + 0.80f * t);
        float random_off    = (xorshift_f32(rng_state_) * 2.0f - 1.0f) * scatter_shape * spread_;
        float px            = clampf(base_x + random_off, -1.0f, 1.0f);
        float a     = 0.5f * (px + 1.0f);
        float pl    = fasterpowf(1.0f - a, profile_.pan_exponent);
        float pr    = fasterpowf(a, profile_.pan_exponent);
        // pn normalizes the pair so the sum-of-squares is 1; divide (not multiply).
        // Multiplying by pn inverts the normalization — 4.7 dB deficit at center.
        float pn    = my_sqrt_f(pl * pl + pr * pr + 1e-12f);
        float inv_pn = 1.0f / (pn + 1e-20f);
        float pan_l = pl * inv_pn * spread_;
        float pan_r = pr * inv_pn * spread_;

        // HP/LP attenuation baked into pan_gain (eliminates per-sample division)
        float hp_hz   = hp_base * (1.0f + follower * hp_follow);
        float lp_hz   = lp_base * (1.0f - follower * (mode_ == MODE_TRIBAL ? 0.38f : 0.25f)); // wider filter dispersion
        lp_hz = clampf(lp_hz, 20.0f, 20000.0f);
        float hp_attn = 1.0f / (1.0f + hp_hz * 0.0012f);

        // Compute actual one-pole filter coefficient for this clone
        float lp_omega = 2.0f * M_PI * lp_hz * inverse_sample_rate_;
        clones_[i].lp_coef = lp_omega / (lp_omega + 1.0f);
        lp_coefs_[i] = clones_[i].lp_coef;

        // Clear states in SoA
        lp_states_l_[i] = 0.0f;
        lp_states_r_[i] = 0.0f;

        // Store gains without the artificial amplitude-only LPF attenuation
        clones_[i].pan_gain_l = pan_l * hp_attn; // HPF attenuation baked in
        clones_[i].pan_gain_r = pan_r * hp_attn; // HPF attenuation baked in

        // Base gain: exponential rolloff × scatter detachment
        float raw_gain       = fasterpowf(gain_step, (float)i);
        float detachment     = fmaxf(0.42f, 1.0f - profile_.scatter_amount * (0.10f + 0.35f * follower) - gap_ * 0.12f * follower);
        clones_[i].base_gain = raw_gain * detachment;

        // net_gain: apply current soft_atk (will be refreshed each block by update_clone_dynamics)
        // lp_coef_base: store for per-hit randomization
        clones_[i].lp_coef_base = clones_[i].lp_coef;
        float soft_fac        = (i == 0) ? 1.0f : (0.82f + 0.18f * (1.0f - soft_atk_));
        clones_[i].net_gain_l = clones_[i].base_gain * soft_fac * clones_[i].pan_gain_l; // dynamic_gain_factor applied in update_clone_dynamics
        clones_[i].net_gain_r = clones_[i].base_gain * soft_fac * clones_[i].pan_gain_r; // dynamic_gain_factor applied in update_clone_dynamics
        clones_[i].precalculated_delay_base = fmaxf(2.0f, clones_[i].delay_samples + clones_[i].scatter_samples);
    }

    pending_profile_rebuild_ = false;
}

// ---------------------------------------------------------------------------
// randomize_hit — called on transient detection
// Pre-computes scatter_samples per clone so the render loop is xorshift-free.
// ---------------------------------------------------------------------------
void PercussionSpatializer::randomize_hit() {
    const float ms_to_smp  = (float)sample_rate_ * 0.001f;
    const float inv_cnt1   = 1.0f / (float)(clone_count_ > 1 ? clone_count_ - 1 : 1);
    const float global_jit = (xorshift_f32(rng_state_) * 2.0f - 1.0f) * profile_.jitter_ms;
    const float gap_detach = 1.0f + gap_ * 1.7f;

    // Reset all clones to standard baseline accentuation first
    for (int i = 0; i < clone_count_; ++i) {
        clones_[i].hit_accent = 1.0f;

        const float follower  = (float)i * inv_cnt1;
        const float max_sct   = profile_.scatter_amount * (0.6f + 1.8f * follower) * gap_detach;
        const float clone_jit = (xorshift_f32(rng_state_) * 2.0f - 1.0f) * max_sct;
        clones_[i].scatter_samples = (global_jit + clone_jit) * ms_to_smp * gap_detach;
        clones_[i].wobble_phase    = xorshift_f32(rng_state_) * 2.0f * M_PI;
        // Randomize LPF cutoff factor per hit (e.g., 0.7 to 1.3)
        clones_[i].lp_cutoff_rand_factor = 0.7f + xorshift_f32(rng_state_) * 0.6f; // Range 0.7 to 1.3
        clones_[i].lp_coef = clampf(clones_[i].lp_coef_base * clones_[i].lp_cutoff_rand_factor, 0.01f, 0.99f);
        clones_[i].dynamic_gain_factor = 0.7f + xorshift_f32(rng_state_) * 0.6f; // Range 0.7 to 1.3
        clones_[i].precalculated_delay_base = fmaxf(2.0f, clones_[i].delay_samples + clones_[i].scatter_samples);
        // Update SoA
        lp_coefs_[i] = clones_[i].lp_coef;
    }

    // Humanize: Pick 1 or 2 random secondary clones to accent or damp
    if (clone_count_ > 2) {
        // Pick a random clone index excluding the first master clone (i=0)
        int random_clone_1 = 1 + (int)(xorshift_f32(rng_state_) * (clone_count_ - 1));
        // Give it a variance of +/- 3dB (~0.7 to ~1.4 amplitude)
        clones_[random_clone_1].hit_accent = 0.7f + (xorshift_f32(rng_state_) * 0.7f);

        if (clone_count_ > 4) {
            int random_clone_2 = 1 + (int)(xorshift_f32(rng_state_) * (clone_count_ - 1));
            if (random_clone_2 != random_clone_1) {
                // Make the second one a localized damping/ghost hit drop
                clones_[random_clone_2].hit_accent = 0.4f + (xorshift_f32(rng_state_) * 0.5f);
            }
            if ((mode_ == MODE_TRIBAL) || (mode_ == MODE_ANGEL)) {
                int random_clone_3 = 1 + (int)(xorshift_f32(rng_state_) * (clone_count_ - 1));
                if (random_clone_3 != random_clone_1) {
                    // Make the third one a localized damping/ghost hit drop
                    clones_[random_clone_3].hit_accent = 0.45f + (xorshift_f32(rng_state_) * 0.55f);
                }
            }
            if ((mode_ == MODE_TRIBAL) && (clone_count_ > 6)) {
                int random_clone_4 = 1 + (int)(xorshift_f32(rng_state_) * (clone_count_ - 1));
                if (random_clone_4 != random_clone_1) {
                    // Make the fourth one a localized damping/ghost hit drop
                    clones_[random_clone_4].hit_accent = 0.55f + (xorshift_f32(rng_state_) * 0.65f);
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// render_block4 — 4 frames, NEON interpolation across time
// ---------------------------------------------------------------------------
static fast_inline void mix_clone_batch4(clone_t* clones,
                                         int base,
                                         const delay_line_t& delay,
                                         bool is_angel,
                                         const float* contiguous_gains_l,
                                         const float* contiguous_gains_r,
                                         float* contiguous_lp_l,
                                         float* contiguous_lp_r,
                                         const float* contiguous_lp_c,
                                         float& wet_l,
                                         float& wet_r,
                                         int sample_idx_in_block) {
    alignas(16) float s0l[4], s1l[4], s0r[4], s1r[4], frs[4];

    for (int lane = 0; lane < 4; ++lane) {
        clone_t& c = clones[base + lane];

        // Smooth LFO calculation
        float total_d = c.precalculated_delay_base + (c.wobble_sine_start + c.wobble_sine_delta * sample_idx_in_block) * c.wobble_depth_samples;

        float raw_pos = (float)(delay.write - 1) - total_d;
        float safe_pos = raw_pos + (float)(delay_line_t::kLen * 8);
        int base_int = (int)safe_pos;
        frs[lane] = safe_pos - (float)base_int;
        int i0 = base_int & delay_line_t::kMask;
        int i1 = (i0 + 1) & delay_line_t::kMask;

        s0l[lane] = delay.l[i0];
        s1l[lane] = delay.l[i1];
        s0r[lane] = delay.r[i0];
        s1r[lane] = delay.r[i1];
    }

    float32x4_t v_fr   = vld1q_f32(frs);
    float32x4_t v_lpc  = vld1q_f32(contiguous_lp_c + base);
    float32x4_t v_lpsl = vld1q_f32(contiguous_lp_l + base);
    float32x4_t v_lpsr = vld1q_f32(contiguous_lp_r + base);

    // 1. Vectorized Linear Interpolation: out = s0 + fr * (s1 - s0)
    float32x4_t v_raw_dl = vmlaq_f32(vld1q_f32(s0l), v_fr, vsubq_f32(vld1q_f32(s1l), vld1q_f32(s0l)));
    float32x4_t v_raw_dr = vmlaq_f32(vld1q_f32(s0r), v_fr, vsubq_f32(vld1q_f32(s1r), vld1q_f32(s0r)));

    // 2. Vectorized IIR Filter Update: state = state + coef * (target - state)
    v_lpsl = vmlaq_f32(v_lpsl, v_lpc, vsubq_f32(v_raw_dl, v_lpsl));
    v_lpsr = vmlaq_f32(v_lpsr, v_lpc, vsubq_f32(v_raw_dr, v_lpsr));

    // 3. Convert Right Channel to HPF if Angel Mode is Active ---
    float32x4_t v_out_r = v_lpsr;
    if (is_angel) {
        v_out_r = vsubq_f32(v_raw_dr, v_lpsr); // HPF = Input - LPF
    }

    // 4. Final mix and horizontal sum (with stereo filtering)
    wet_l += PercussionSpatializer::horizontal_sum4(vmulq_f32(v_lpsl, vld1q_f32(contiguous_gains_l + base)));
    wet_r += PercussionSpatializer::horizontal_sum4(vmulq_f32(v_out_r, vld1q_f32(contiguous_gains_r + base)));

    // 5. Update clone persistent states
    vst1q_f32(contiguous_lp_l + base, v_lpsl);
    vst1q_f32(contiguous_lp_r + base, v_lpsr);
}

static fast_inline void mix_clone_batch2(clone_t* clones,
                                         int base,
                                         const delay_line_t& delay,
                                         bool is_angel,
                                         const float* gains_l,
                                         const float* gains_r,
                                         float& wet_l,
                                         float& wet_r,
                                         int sample_idx_in_block) {
    alignas(16) float dl[2], dr[2];

    for (int lane = 0; lane < 2; ++lane) {
        clone_t& c = clones[base + lane];
        float total_d = c.precalculated_delay_base + (c.wobble_sine_start + c.wobble_sine_delta * sample_idx_in_block) * c.wobble_depth_samples;

        float raw_pos = (float)(delay.write - 1) - total_d;
        float safe_pos = raw_pos + (float)(delay_line_t::kLen * 8);
        int base_int = (int)safe_pos;
        float fr = safe_pos - (float)base_int;
        int i0 = base_int & delay_line_t::kMask;
        int i1 = (i0 + 1) & delay_line_t::kMask;

        float raw_dl = delay.l[i0] + (delay.l[i1] - delay.l[i0]) * fr;
        float raw_dr = delay.r[i0] + (delay.r[i1] - delay.r[i0]) * fr;

        // --- ACTUAL IIR FILTERING ---
        c.lp_state_l += c.lp_coef * (raw_dl - c.lp_state_l);
        c.lp_state_r += c.lp_coef * (raw_dr - c.lp_state_r);
        // stereo filtering for Angel mode: convert the right channel to a high-pass by subtracting the LPF output from the input
        dl[lane] = c.lp_state_l;
        dr[lane] = is_angel ? (raw_dr - c.lp_state_r) : c.lp_state_r;
    }

    float32x2_t vdl = vld1_f32(dl);
    float32x2_t vdr = vld1_f32(dr);
    wet_l += PercussionSpatializer::horizontal_sum2(vmul_f32(vdl, vld1_f32(gains_l + base)));
    wet_r += PercussionSpatializer::horizontal_sum2(vmul_f32(vdr, vld1_f32(gains_r + base)));
}

static fast_inline void mix_clone_scalar(clone_t& c,
                                         int idx,
                                         const delay_line_t& delay,
                                         bool is_angel,
                                         float gain_l,
                                         float gain_r,
                                         float& wet_l, float& wet_r,
                                         int sample_idx_in_block) {
    (void)idx; // Unused parameter
    float total_d = c.precalculated_delay_base + (c.wobble_sine_start + c.wobble_sine_delta * sample_idx_in_block) * c.wobble_depth_samples;

    float raw_pos = (float)(delay.write - 1) - total_d;
    float safe_pos = raw_pos + (float)(delay_line_t::kLen * 8);
    int base_int = (int)safe_pos;
    float fr = safe_pos - (float)base_int;
    int i0 = base_int & delay_line_t::kMask;
    int i1 = (i0 + 1) & delay_line_t::kMask;

    float raw_dl = delay.l[i0] + (delay.l[i1] - delay.l[i0]) * fr;
    float raw_dr = delay.r[i0] + (delay.r[i1] - delay.r[i0]) * fr;

    // --- ACTUAL IIR FILTERING ---
    c.lp_state_l += c.lp_coef * (raw_dl - c.lp_state_l);
    c.lp_state_r += c.lp_coef * (raw_dr - c.lp_state_r);
    // stereo filtering for Angel mode: convert the right channel to a high-pass by subtracting the LPF output from the input
    wet_l += c.lp_state_l * gain_l;
    float final_r = is_angel ? (raw_dr - c.lp_state_r) : c.lp_state_r;
    wet_r += final_r * gain_r;
}

static fast_inline void render_one_frame(PercussionSpatializer* self,
                                         int sample_idx_in_block,
                                         float& out_l, float& out_r) {
    float wet_l = 0.0f;
    float wet_r = 0.0f;

    int i = 0;
    const bool is_angel = (self->get_mode() == MODE_ANGEL);
    clone_t* clones = self->get_clones();
    for (; i + 3 < self->get_clone_count(); i += 4) {
        mix_clone_batch4(clones, i, self->get_delay(), is_angel,
                      self->get_net_gains_l(), self->get_net_gains_r(),
                      self->get_lp_states_l(), self->get_lp_states_r(), self->get_lp_coefs(),
                      wet_l, wet_r, sample_idx_in_block);
    }
    for (; i + 1 < self->get_clone_count(); i += 2) {
        mix_clone_batch2(clones, i, self->get_delay(), is_angel,
                         self->get_net_gains_l(), self->get_net_gains_r(),
                         wet_l, wet_r, sample_idx_in_block);
    }
    for (; i < self->get_clone_count(); ++i) {
        mix_clone_scalar(clones[i], i, self->get_delay(), is_angel,
                       self->get_net_gains_l()[i], self->get_net_gains_r()[i],
                       wet_l, wet_r, sample_idx_in_block);
    }

    // wet_drive and gap_boost are already baked into net_gains during update_clone_dynamics

    out_l = wet_l * self->get_mix();
    out_r = wet_r * self->get_mix();
}

void PercussionSpatializer::render_block4(const float* in, float* out) {
    // Vectorized Transient detection
    float32x4x2_t v_in = vld2q_f32(in);
    float32x4_t v_mag = vmulq_n_f32(vaddq_f32(vabsq_f32(v_in.val[0]), vabsq_f32(v_in.val[1])), 0.5f);
    #if defined(__aarch64__)
    float mag_max = vaddvq_f32(v_mag); // Optimization: sum/max reduction
    #else
    float32x2_t v_tmp = vmax_f32(vget_low_f32(v_mag), vget_high_f32(v_mag));
    float mag_max = fmaxf(vget_lane_f32(v_tmp, 0), vget_lane_f32(v_tmp, 1));
    #endif

    const bool transient = (mag_max > prev_mag_ * 1.9f) && (mag_max > 0.002f);
    // Decay envelope: ~10 ms half-life mapped to block rate
    prev_mag_ = fmaxf(mag_max, prev_mag_ * fasterexpf(-400.0f * inverse_sample_rate_));

    // Batch delay line writes
    for (int s = 0; s < 4; ++s) delay_.push(in[s * 2], in[s * 2 + 1]);

    // 3. Advance smoothing + rebuild if needed; THEN randomize on transient
    //    (ensures randomize_hit uses the freshly rebuilt profile)
    advance_smoothing();

    // Pre-calculate wobble sine values for the entire block
    const float phase_inc_per_sample = twopi * rate_ * inverse_sample_rate_;
    for (int j = 0; j < clone_count_; ++j) {
        clone_t& c = clones_[j];
        float start_phase = c.wobble_phase;
        float end_phase = start_phase + phase_inc_per_sample * c.wobble_rate_mul * 4; // Phase after 4 samples

        // Branchless wrap
        if (end_phase >= twopi) end_phase -= twopi;
        else if (end_phase < 0.0f) end_phase += twopi;

        c.wobble_sine_start = fastersinfullf(start_phase);
        c.wobble_sine_delta = (fastersinfullf(end_phase) - c.wobble_sine_start) * 0.25f;
        c.wobble_phase = end_phase; // Update phase for next block
    }
    if (pending_profile_rebuild_) rebuild_profile();
    if (transient) {
        randomize_hit();
        update_clone_dynamics(); // Forces instant re-calculation for the current block!
    }

    const float inv_mix = (1.0f - get_mix());
    const float wet_drive = 1.0f + 0.18f * get_gap() + 0.12f * get_scatter();

    // Calculate phase increment per sample
    for (int s = 0; s < 4; ++s) {
        float ol = 0.0f, orr = 0.0f;
        render_one_frame(this, s, ol, orr);

        // Mix dry/wet with hoisted drive factor
        out[s * 2]     = (in[s * 2] * inv_mix) + (ol * wet_drive);
        out[s * 2 + 1] = (in[s * 2 + 1] * inv_mix) + (orr * wet_drive);
    }
}

// ---------------------------------------------------------------------------
// render_scalar_frame — scalar fallback for the tail (0-3 frames)
// ---------------------------------------------------------------------------
void PercussionSpatializer::render_scalar_frame(const float* in, float* out) {
    delay_.push(in[0], in[1]);

    float ol = 0.0f, orr = 0.0f;
    render_one_frame(this, 0, ol, orr); // For scalar frame, assume sample_idx_in_block is 0

    const float inv_mix = (1.0f - get_mix());
    const float wet_drive = 1.0f + 0.18f * get_gap() + 0.12f * get_scatter();

    out[0] = (in[0] * inv_mix) + (ol * wet_drive);
    out[1] = (in[1] * inv_mix) + (orr * wet_drive);
}


// ---------------------------------------------------------------------------
// Render — block4 path + scalar tail
// ---------------------------------------------------------------------------
void PercussionSpatializer::Render(const float* in, float* out, size_t frames) {
    if (!initialized_) {
        std::memset(out, 0, frames * 2 * sizeof(float));
        return;
    }

    size_t i = 0;
    for (; i + 3 < frames; i += 4) {
        render_block4(in + i * 2, out + i * 2);
    }
    for (; i < frames; ++i) {
        render_scalar_frame(in + i * 2, out + i * 2);
    }
}

// ---------------------------------------------------------------------------
// Parameter accessors
// ---------------------------------------------------------------------------

void PercussionSpatializer::setParameter(uint8_t index, int32_t value) {
    if (index >= k_total) return;
    params_[index] = (int8_t)value;
    const float norm = clamp01((float)value * 0.01f);

    switch (index) {
        case k_clones:
            set_clone_count_index(value < 0 ? 0 : (value >= CLONE_SET_CNT ? CLONE_SET_CNT - 1 : value));
            break;
        case k_mode:
            set_mode((spatial_mode_t)(value < 0 ? 0 : (value >= (int)MODE_COUNT ? (int)MODE_COUNT - 1 : value)));
            break;
        case k_depth:            set_depth(norm);            break;
        case k_rate:             set_rate(norm);             break;
        case k_spread:           set_spread(norm);           break;
        case k_mix:              set_mix(norm);              break;
        case k_wobble:           set_wobble(norm);           break;
        case k_scatter:          set_scatter(norm);          break;
        case k_attack_softening: set_attack_softening(norm); break;
        case k_gap:              set_gap(norm);              break;
        default: break;
    }
}

int32_t PercussionSpatializer::getParameterValue(uint8_t index) const {
    if (index >= k_total) return 0;
    return params_[index];
}

const char* PercussionSpatializer::getParameterStrValue(uint8_t index, int32_t value) const {
    static const char* mode_names[]  = { "Tribale", "Militare", "Angeli" };
    static const char* clone_names[] = { "2cloni",  "4cloni",   "6cloni", "8cloni", "10cloni" };
    switch (index) {
        case k_mode:   if (value >= 0 && value < MODE_COUNT)    return mode_names[value];  break;
        case k_clones: if (value >= 0 && value < CLONE_SET_CNT) return clone_names[value]; break;
        default: break;
    }
    return nullptr;
}

const uint8_t* PercussionSpatializer::getParameterBmpValue(uint8_t, int32_t) const {
    return nullptr;
}

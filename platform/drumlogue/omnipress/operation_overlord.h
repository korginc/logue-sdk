// New file: operation_overlord.h

#pragma once
/**
 * @file operation_overlord.h
 * @brief EHX Operation Overlord drum machine overdrive emulation
 *
 * Features:
 * - Tube-like asymmetric clipping
 * - Bass/Treble EQ (pre-drive)
 * - Blend control (parallel drive)
 * - Presence control (post-drive high-end)
 */

#include <arm_neon.h>
#include <filters.h>
#include <cmath>

// 1-pole IIR DC Blocker State Tracker
typedef struct {
    float x_prev;
    float y_prev;
} dc_blocker_state_t;

typedef struct {
    // EQ Filter States (Separate per channel to eliminate crosstalk)
    biquad_state_t bass_boost_l;
    biquad_state_t treble_boost_l;
    biquad_state_t presence_l;
    biquad_state_t bass_boost_r;
    biquad_state_t treble_boost_r;
    biquad_state_t presence_r;

    // Vectorized DC Blocker States
    dc_blocker_state_t dc_l;
    dc_blocker_state_t dc_r;

    // Pirkle Triode State Variables (Dynamic Bias Excursion Capacitors)
    float dyn_bias_l1; // Stage 1 Dynamic Bias Tracking (Left)
    float dyn_bias_r1; // Stage 1 Dynamic Bias Tracking (Right)
    float dyn_bias_l2; // Stage 2 Dynamic Bias Tracking (Left)
    float dyn_bias_r2; // Stage 2 Dynamic Bias Tracking (Right)

    // Parameters
    float drive;          // 0.0 to 1.0
    float bass;           // 0.0 to 1.0
    float treble;         // 0.0 to 1.0
    float blend;          // 0.0 to 1.0
    float presence;       // 0.0 to 1.0
} overlord_t;

// Initialize state-space coefficients
fast_inline void overlord_init(overlord_t* ov, float sample_rate) {
    ov->drive = 0.0f;
    ov->bass = 0.5f;
    ov->treble = 0.5f;
    ov->blend = 1.0f;
    ov->presence = 0.5f;

    biquad_init_state(&ov->bass_boost_l);
    biquad_init_state(&ov->treble_boost_l);
    biquad_init_state(&ov->presence_l);
    biquad_init_state(&ov->bass_boost_r);
    biquad_init_state(&ov->treble_boost_r);
    biquad_init_state(&ov->presence_r);

    ov->dc_l.x_prev = 0.0f; ov->dc_l.y_prev = 0.0f;
    ov->dc_r.x_prev = 0.0f; ov->dc_r.y_prev = 0.0f;

    // Initialize tubes at quiescent state (no dynamic bias shift)
    ov->dyn_bias_l1 = 0.0f; ov->dyn_bias_r1 = 0.0f;
    ov->dyn_bias_l2 = 0.0f; ov->dyn_bias_r2 = 0.0f;
}

// Unrolled 4-lane IIR DC Blocker (Restores dynamic symmetry downstream)
fast_inline float32x4_t dc_block_process(dc_blocker_state_t* state, float32x4_t in, float R) {
    float x[4], y[4];
    vst1q_f32(x, in);

    y[0] = x[0] - state->x_prev + R * state->y_prev;
    y[1] = x[1] - x[0] + R * y[0];
    y[2] = x[2] - x[1] + R * y[1];
    y[3] = x[3] - x[2] + R * y[2];

    state->x_prev = x[3];
    state->y_prev = y[3];

    return vld1q_f32(y);
}

// Helper to extract the mean of a 4-sample vector for envelope sidechains
fast_inline float vmeanq_f32(float32x4_t vec) {
    float32x2_t sum2 = vadd_f32(vget_low_f32(vec), vget_high_f32(vec));
    return (vget_lane_f32(sum2, 0) + vget_lane_f32(sum2, 1)) * 0.25f;
}

// Ultra-fast Asymmetrical Triode Shaper using hardware reciprocal square root engines
fast_inline float32x4_t pirkle_triode_shaper(float32x4_t v_gk, float shape_pos, float shape_neg) {
    float32x4_t v_zero = vdupq_n_f32(0.0f);
    float32x4_t v_one  = vdupq_n_f32(1.0f);

    // Isolate positive and negative grid excursions
    float32x4_t v_pos = vmaxq_f32(v_gk, v_zero);
    float32x4_t v_neg = vminq_f32(v_gk, v_zero);

    // Polynomial squaring for non-linear coefficients
    float32x4_t v_pos2 = vmulq_f32(v_pos, v_pos);
    float32x4_t v_neg2 = vmulq_f32(v_neg, v_neg);

    float32x4_t v_shp_pos = vmulq_f32(v_pos2, vdupq_n_f32(shape_pos));
    float32x4_t v_shp_neg = vmulq_f32(v_neg2, vdupq_n_f32(shape_neg));

    // Combine paths into a joint denominator: 1 + (Bp * pos^2) + (Bn * neg^2)
    float32x4_t denom = vaddq_f32(v_one, vaddq_f32(v_shp_pos, v_shp_neg));

    // ARM NEON pipeline optimization: Hardware reciprocal square root approximation
    float32x4_t rsq_est  = vrsqrteq_f32(denom);
    float32x4_t rsq_step = vrsqrtsq_f32(vmulq_f32(rsq_est, rsq_est), denom); // Newton-Raphson precision iteration
    float32x4_t rec_sqrt = vmulq_f32(rsq_est, rsq_step);

    // Scale output by reciprocal square root
    float32x4_t out = vmulq_f32(v_gk, rec_sqrt);

    // CRITICAL: A true common-cathode valve inverts signal phase natively
    return vnegq_f32(out);
}

/**
 * Set drive amount (0-100%)
 */
fast_inline void overlord_set_drive(overlord_t* ov, float drive_percent) {
    float drive = drive_percent * 0.01f;
    ov->drive = drive;
}


// Baxandall bass/treble EQ: low shelf into high shelf in series
fast_inline float32x4_t baxandall_eq(float32x4_t in,
                                     biquad_state_t* bass_state,
                                     biquad_state_t* treble_state,
                                     float bass,
                                     float treble,
                                     float sample_rate) {
    float bass_gain = -12.0f + bass * 24.0f;    // -12 to +12 dB
    float treble_gain = -12.0f + treble * 24.0f;

    // Low shelf at 100 Hz — bass cut/boost
    float32x4_t bass_shelf = low_shelf_filter(in, bass_state, 100.0f, bass_gain, 1.0f, sample_rate);

    // High shelf at 10 kHz — treble cut/boost (fed from bass shelf output)
    float32x4_t treble_shelf = high_shelf_filter(bass_shelf, treble_state, 10000.0f, treble_gain, 0.0f, sample_rate);

    return treble_shelf;
}

// EQ-only path (no tube saturation) — for use in distressor mode
// Applies bass/treble Baxandall EQ and presence shelf without adding drive distortion
fast_inline float32x4x2_t overlord_apply_eq(overlord_t* ov,
                                             float32x4_t in_l,
                                             float32x4_t in_r,
                                             float sample_rate) {
    float active_threshold = 0.01f;
    if (fabsf(ov->bass - 0.5f) < active_threshold &&
        fabsf(ov->treble - 0.5f) < active_threshold &&
        fabsf(ov->presence - 0.5f) < active_threshold) {
        float32x4x2_t bypass;
        bypass.val[0] = in_l;
        bypass.val[1] = in_r;
        return bypass;
    }

    float32x4_t eq_l = baxandall_eq(in_l, &ov->bass_boost_l, &ov->treble_boost_l, ov->bass, ov->treble, sample_rate);
    float32x4_t eq_r = baxandall_eq(in_r, &ov->bass_boost_r, &ov->treble_boost_r, ov->bass, ov->treble, sample_rate);

    // presence=0.5 → flat (0 dB), 0.0 → −12 dB, 1.0 → +12 dB
    float presence_gain = (ov->presence - 0.5f) * 24.0f;
    float32x4_t out_l = high_shelf_filter(eq_l, &ov->presence_l, 5000.0f,
                                           presence_gain, 1.0f, sample_rate);
    float32x4_t out_r = high_shelf_filter(eq_r, &ov->presence_r, 5000.0f,
                                           presence_gain, 1.0f, sample_rate);

    float32x4x2_t out;
    out.val[0] = out_l;
    out.val[1] = out_r;
    return out;
}

// ============================================================================
// HYBRID ARCHITECTURE: FIXED CONTINUOUS PRE-AMP CASCADED INTO DYNAMIC TRIODE
// ============================================================================
// Optimized continuous asymmetric tube saturation stage
// Stage 1: Continuous Rational Shaper (Fixed Bias, High Headroom)
fast_inline float32x4_t stage1_continuous_preamp(float32x4_t in, float32x4_t v_drive, float bias) {
    float32x4_t v_bias = vdupq_n_f32(bias);
    float32x4_t v_one  = vdupq_n_f32(1.0f);

    float32x4_t biased = vaddq_f32(vmulq_f32(in, v_drive), v_bias);
    float32x4_t abs_biased = vabsq_f32(biased);
    float32x4_t denom = vaddq_f32(v_one, abs_biased);

    // NEON Newton-Raphson Reciprocal Approximation
    float32x4_t rec_est  = vrecpeq_f32(denom);
    float32x4_t rec_step = vrecpsq_f32(denom, rec_est);
    float32x4_t rec      = vmulq_f32(rec_est, rec_step);

    float32x4_t saturated = vmulq_f32(biased, rec);
    float32x4_t v_offset  = vdupq_n_f32(bias / (1.0f + fabsf(bias)));

    // Returns a native phase (+) saturated signal
    return vsubq_f32(saturated, v_offset);
}

// Stage 2: Pirkle Triode Shaper (Dynamic Bias, Phase Inverting)
fast_inline float32x4_t stage2_pirkle_triode(float32x4_t v_gk, float shape_pos, float shape_neg) {
    float32x4_t v_zero = vdupq_n_f32(0.0f);
    float32x4_t v_one  = vdupq_n_f32(1.0f);

    float32x4_t v_pos = vmaxq_f32(v_gk, v_zero);
    float32x4_t v_neg = vminq_f32(v_gk, v_zero);

    float32x4_t v_shp_pos = vmulq_f32(vmulq_f32(v_pos, v_pos), vdupq_n_f32(shape_pos));
    float32x4_t v_shp_neg = vmulq_f32(vmulq_f32(v_neg, v_neg), vdupq_n_f32(shape_neg));
    float32x4_t denom = vaddq_f32(v_one, vaddq_f32(v_shp_pos, v_shp_neg));

    // NEON Reciprocal Square Root Engine
    float32x4_t rsq_est  = vrsqrteq_f32(denom);
    float32x4_t rsq_step = vrsqrtsq_f32(vmulq_f32(rsq_est, rsq_est), denom);
    float32x4_t rec_sqrt = vmulq_f32(rsq_est, rsq_step);

    float32x4_t out = vmulq_f32(v_gk, rec_sqrt);

    // CRITICAL: Mimics common-cathode inversion (-)
    return vnegq_f32(out);
}

/**
 * @brief models into a Cascaded Dual-Stage Hybrid Architecture, you gain the best of both worlds
 * (a clean, high-headroom JFET booster feeding into a saggy, high-gain 12AX7 vacuum tube stage):
 * Stage 1 (Continuous Rational Shaper): Acts as a warm, fixed-bias input amplifier.
 * It softly rounds off sharp drum transients (like the initial transient spike of a snare or kick)
 * and adds a consistent baseline of low-order even harmonics without choking.
 * Stage 2 (Pirkle Dynamic Triode): Receives the pre-compressed, harmonically dense signal from Stage 1.
 * Because the signal is already dense, it drives Pirkle's grid-current envelope tracker harder and more uniformly,
 * yielding a highly reactive, breathing "blocking distortion" that thickens the sustain of the drums.
 *
 * @param ov
 * @param in_l
 * @param in_r
 * @param sample_rate
 * @return float32x4x2_t
 */

// Main Processing Entry Point
fast_inline float32x4x2_t overlord_process(overlord_t* ov, float32x4_t in_l, float32x4_t in_r, float sample_rate) {
    // Standard bypass safety evaluation
    if (ov->drive < 0.005f && fabsf(ov->bass - 0.5f) < 0.005f && fabsf(ov->treble - 0.5f) < 0.005f) {
        float32x4x2_t bypass = { {in_l, in_r} };
        return bypass;
    }

    float32x4_t dry_l = in_l;
    float32x4_t dry_r = in_r;

    // Macro Gain Staging
    float drive_sq = ov->drive * ov->drive;
    float32x4_t v_drive_stage1 = vdupq_n_f32(1.0f + (drive_sq * 35.0f)); // Warm preamp push
    float32x4_t v_drive_stage2 = vdupq_n_f32(1.0f + (ov->drive * 4.5f)); // Harder triode slam

    float32x4_t v_static_bias2 = vdupq_n_f32(-0.18f); // Triode operating cutoff point
    const float alpha_bias = 0.0025f;                 // Capacitor discharge tracker

    // ==========================================
    // CHANNEL LEFT HYBRID LAYER
    // ==========================================

    // 1. Run through smooth, non-inverting rational preamp stage
    float32x4_t pre_l = stage1_continuous_preamp(dry_l, v_drive_stage1, 0.12f); // Mild asymmetric warming

    // 2. Cascade into the dynamic-bias Pirkle triode stage
    float32x4_t v_gk_l = vaddq_f32(vmulq_f32(pre_l, v_drive_stage2), vaddq_f32(v_static_bias2, vdupq_n_f32(ov->dyn_bias_l1)));
    float32x4_t wet_l  = stage2_pirkle_triode(v_gk_l, 4.8f, 1.5f);

    // Track grid current envelope from Stage 2 input
    float grid_curr_l = vmeanq_f32(vmaxq_f32(v_gk_l, vdupq_n_f32(0.0f)));
    ov->dyn_bias_l1 += alpha_bias * (grid_curr_l * -1.7f - ov->dyn_bias_l1);

    // 3. PHASE ALIGNMENT CORRECTION
    // Stage 2 inverted the phase. We must multiply by -1 to bring it back in-phase with dry_l
    wet_l = vnegq_f32(wet_l);

    // 4. Clear accumulated offset shifts via IIR DC Blocker
    wet_l = dc_block_process(&ov->dc_l, wet_l, 0.996f);

    // ==========================================
    // CHANNEL RIGHT HYBRID LAYER
    // ==========================================

    float32x4_t pre_r = stage1_continuous_preamp(dry_r, v_drive_stage1, 0.12f);

    float32x4_t v_gk_r = vaddq_f32(vmulq_f32(pre_r, v_drive_stage2), vaddq_f32(v_static_bias2, vdupq_n_f32(ov->dyn_bias_r1)));
    float32x4_t wet_r  = stage2_pirkle_triode(v_gk_r, 4.8f, 1.5f);

    float grid_curr_r = vmeanq_f32(vmaxq_f32(v_gk_r, vdupq_n_f32(0.0f)));
    ov->dyn_bias_r1 += alpha_bias * (grid_curr_r * -1.7f - ov->dyn_bias_r1);

    wet_r = vnegq_f32(wet_r); // Phase correction step
    wet_r = dc_block_process(&ov->dc_r, wet_r, 0.996f);

    // ==========================================
    // POST FILTERS & LINEAR PARALLEL BLEND
    // ==========================================

    // Run the hybrid saturation into your tone stack filters
    float32x4_t eq_l = baxandall_eq(wet_l, &ov->bass_boost_l, &ov->treble_boost_l, ov->bass, ov->treble, sample_rate);
    float32x4_t eq_r = baxandall_eq(wet_r, &ov->bass_boost_r, &ov->treble_boost_r, ov->bass, ov->treble, sample_rate);

    float presence_gain = (ov->presence - 0.5f) * 24.0f;
    wet_l = high_shelf_filter(eq_l, &ov->presence_l, 5500.0f, presence_gain, 1.0f, sample_rate);
    wet_r = high_shelf_filter(eq_r, &ov->presence_r, 5500.0f, presence_gain, 1.0f, sample_rate);

    // Blend safely knowing phase vectors match perfectly
    float32x4_t wet_gain = vdupq_n_f32(ov->blend);
    float32x4_t dry_gain = vdupq_n_f32(1.0f - ov->blend);

    float32x4x2_t out;
    out.val[0] = vaddq_f32(vmulq_f32(dry_l, dry_gain), vmulq_f32(wet_l, wet_gain));
    out.val[1] = vaddq_f32(vmulq_f32(dry_r, dry_gain), vmulq_f32(wet_r, wet_gain));

    return out;
}

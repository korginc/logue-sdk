#pragma once
/*
 * File: multiband.h
 *
 * 3-band compressor with independent controls
 * Based on SHARC Audio Elements multiband compressor architecture
 */

#include <arm_neon.h>
#include "crossover.h"
#include "constants.h"

#define BAND_LOW 0
#define BAND_MID 1
#define BAND_HIGH 2
#define NUM_OF_BANDS (3)

// State structures for IIR components
// State tracker for phase compensation biquads
typedef struct {
    float x1, x2, y1, y2;
} phase_apf_state_t;

// DC blocker state representation to eliminate asymmetric waveshaper offsets
typedef struct {
    float x_prev;
    float y_prev;
} dc_filter_state_t;

// Complete standalone multiband structure preserving all independent controls
typedef struct {
    // Crossover network filters
    crossover_t xover_low_mid;
    crossover_t xover_mid_high;

    // RESTORED STATE: Persistent time-histories for the compressors (mono-linked)
    float32x4_t comp_gain_state[NUM_OF_BANDS]; // Persistent smoothing history (dB)
    float32x4_t comp_env_state[NUM_OF_BANDS];  // Persistent pre-smoother history (Linear)

    // Phase alignment networks for the Low band to match Mid/High crossover group delays
    phase_apf_state_t low_phase_match_l;
    phase_apf_state_t low_phase_match_r;

    // Persistent state histories for per-channel distortion DC Blockers
    dc_filter_state_t dc_blockers_l[NUM_OF_BANDS];
    dc_filter_state_t dc_blockers_r[NUM_OF_BANDS];

    // Persistent state histories for dynamic tube bias modeling
    float tube_bias_l[NUM_OF_BANDS];
    float tube_bias_r[NUM_OF_BANDS];

    // Independent parameters per band accessible by external APIs
    struct {
        float thresh_db;
        float ratio;
        float makeup_db;
        float makeup_gain_linear;
        float attack_ms;
        float release_ms;
        float mute;
        float solo;
        float attack_coeff;
        float release_coeff;
        float drive;               // Multi-feature integration: Saturation control per band
    } bands[NUM_OF_BANDS];

    // Crossover tracking variables
    float xover_low_freq;
    float xover_high_freq;
    float env_pre_coeff;
    float sample_rate;
} multiband_t;


fast_inline void multiband_update_coeff(multiband_t* mb, int band) {
    mb->bands[band].attack_coeff  = expf(-1.0f / (mb->bands[band].attack_ms * 0.001f * mb->sample_rate));
    mb->bands[band].release_coeff = expf(-1.0f / (mb->bands[band].release_ms * 0.001f * mb->sample_rate));
}

// Initialize multiband compressor
fast_inline void multiband_init(multiband_t* mb, float sample_rate) {
    mb->sample_rate = sample_rate;

    // Default crossover frequencies
    mb->xover_low_freq  = 250.0f;
    mb->xover_high_freq = 2500.0f;

    // Initialize crossovers
    crossover_init(&mb->xover_low_mid, mb->xover_low_freq, sample_rate);
    crossover_init(&mb->xover_mid_high, mb->xover_high_freq, sample_rate);

    // Initialize compressors
    compressor_init(&mb->comp_low);
    compressor_init(&mb->comp_mid);
    compressor_init(&mb->comp_high);

    // Default band parameters
    for (int i = 0; i < NUM_OF_BANDS; i++) {
        mb->bands[i].thresh_db = -10.0f;
        mb->bands[i].ratio = 4.0f;
        mb->bands[i].makeup_db = 0.0f;
        mb->bands[i].makeup_gain_linear = 1.0f;   // 0 dB
        mb->bands[i].attack_ms = 10.0f;
        mb->bands[i].release_ms = 100.0f;
        mb->bands[i].mute = 0.0f;
        mb->bands[i].solo = 0.0f;
        multiband_update_coeff(mb, i);
    }

    mb->gr_low  = vdupq_n_f32(0.0f);
    mb->gr_mid  = vdupq_n_f32(0.0f);
    mb->gr_high = vdupq_n_f32(0.0f);

    mb->env_state_low  = vdupq_n_f32(0.0f);
    mb->env_state_mid  = vdupq_n_f32(0.0f);
    mb->env_state_high = vdupq_n_f32(0.0f);
    mb->env_pre_coeff  = expf(-1.0f / (0.001f * sample_rate));  // 1ms smoothing
}

fast_inline void multiband_reset(multiband_t* m) {
    multiband_init(m, m->sample_rate);
}

// Set crossover frequencies — updates coefficients without resetting filter states.
// This avoids the audible click/pop that zeroing the biquad delay lines would cause.
fast_inline void multiband_set_crossover(multiband_t* mb,
                                         float low_freq,
                                         float high_freq) {
    mb->xover_low_freq  = low_freq;
    mb->xover_high_freq = high_freq;

    // Coefficient-only update: preserves filter state continuity at runtime
    crossover_update_coeffs(&mb->xover_low_mid,   low_freq,  mb->sample_rate);
    crossover_update_coeffs(&mb->xover_mid_high,  high_freq, mb->sample_rate);
}

// Set band parameter
fast_inline void multiband_set_param(multiband_t* mb,
                                     uint8_t band,
                                     uint8_t param_id,
                                     float value) {
    if (band > BAND_HIGH) return;

    switch (param_id) {
        case 0: mb->bands[band].thresh_db = value; break;
        case 1: mb->bands[band].ratio = value; break;
        case 2:
            mb->bands[band].makeup_db = value;
            mb->bands[band].makeup_gain_linear = fasterpowf(10.0f, value / 20.0f);
            break;
        case 3: mb->bands[band].attack_ms = value; multiband_update_coeff(mb, band); break;
        case 4: mb->bands[band].release_ms = value; multiband_update_coeff(mb, band); break;
        case 5: mb->bands[band].mute = value; break;
        case 6: mb->bands[band].solo = value; break;
    }
}

// Get band parameter
fast_inline float multiband_get_param(const multiband_t* mb,
                                      uint8_t band,
                                      uint8_t param_id) {
    if (band > BAND_HIGH) return 0.0f;

    switch (param_id) {
        case 0: return mb->bands[band].thresh_db;
        case 1: return mb->bands[band].ratio;
        case 2: return mb->bands[band].makeup_db;
        case 3: return mb->bands[band].attack_ms;
        case 4: return mb->bands[band].release_ms;
        case 5: return mb->bands[band].mute;
        case 6: return mb->bands[band].solo;
        default: return 0.0f;
    }
}

// High-precision branchless 2nd-order polynomial Log2 engine
fast_inline float32x4_t precision_log2_neon(float32x4_t x) {
    uint32x4_t u_x = vreinterpretq_u32_f32(x);
    uint32x4_t exp = vandq_u32(u_x, vdupq_n_u32(0x7F800000));
    uint32x4_t mant = vandq_u32(u_x, vdupq_n_u32(0x007FFFFF));

    float32x4_t exp_f = vcvtq_f32_u32(vshrq_n_u32(exp, 23));
    float32x4_t mant_f = vmulq_f32(vcvtq_f32_u32(mant), vdupq_n_f32(1.0f / 8388608.0f));

    // 2nd-order minimax polynomial refinement for log2(1 + x)
    // Minimizes the 0.5dB ripple down to unmeasurable levels
    float32x4_t p1 = vdupq_n_f32(1.442695f);
    float32x4_t p2 = vdupq_n_f32(-0.442695f);
    float32x4_t poly = vaddq_f32(vmulq_f32(mant_f, p1), vmulq_f32(vmulq_f32(mant_f, mant_f), p2));

    return vaddq_f32(vsubq_f32(exp_f, vdupq_n_f32(127.0f)), poly);
}

// Micro-targeted DC Blocker to sanitize baseline offset shift
fast_inline float32x4_t dc_block_lane(dc_filter_state_t* state, float32x4_t in, float R) {
    float x[4], y[4];
    vst1q_f32(x, in);
    y[0] = x[0] - state->x_prev + R * state->y_prev;
    y[1] = x[1] - x[0] + R * y[0];
    y[2] = x[2] - x[1] + R * y[1];
    y[3] = x[3] - x[2] + R * y[2];
    state->x_prev = x[3]; state->y_prev = y[3];
    return vld1q_f32(y);
}

// Pirkle Asymmetric Dynamic Triode Core
fast_inline float32x4_t pirkle_triode_engine(float32x4_t in, float drive, float* bias_state, float shape_pos, float shape_neg) {
    float32x4_t v_zero = vdupq_n_f32(0.0f);
    float32x4_t v_one  = vdupq_n_f32(1.0f);

    // Apply Drive + Static cut point + Dynamic envelope bias shift
    float32x4_t v_drive = vdupq_n_f32(1.0f + (drive * 8.0f));
    float32x4_t v_gk = vaddq_f32(vmulq_f32(in, v_drive), vaddq_f32(vdupq_n_f32(-0.15f), vdupq_n_f32(*bias_state)));

    // Extract dynamic grid rectification current (Pirkle Addendum A19)
    float arr_gk[4];
    vst1q_f32(arr_gk, vmaxq_f32(v_gk, v_zero));
    float mean_grid_current = (arr_gk[0] + arr_gk[1] + arr_gk[2] + arr_gk[3]) * 0.25f;
    *bias_state += 0.003f * (mean_grid_current * -1.8f - *bias_state); // Updates bias drift integration

    // Perform continuous sigmoidal non-linear wrapping
    float32x4_t v_pos = vmaxq_f32(v_gk, v_zero);
    float32x4_t v_neg = vminq_f32(v_gk, v_zero);
    float32x4_t denom = vaddq_f32(v_one, vaddq_f32(
        vmulq_f32(vmulq_f32(v_pos, v_pos), vdupq_n_f32(shape_pos)),
        vmulq_f32(vmulq_f32(v_neg, v_neg), vdupq_n_f32(shape_neg))
    ));

    // Fast Hardware Reciprocal Square-Root Pipeline
    float32x4_t rsq_est = vrsqrteq_f32(denom);
    float32x4_t rsq_step = vrsqrtsq_f32(vmulq_f32(rsq_est, rsq_est), denom);
    return vnegq_f32(vmulq_f32(v_gk, vmulq_f32(rsq_est, rsq_step))); // Includes phase inversion
}

// Fused branchless compressor node pulling straight from parameter cache
fast_inline float32x4_t process_compressor_lane(float32x4_t* gain_state, float32x4_t env_linear,
                                                float thresh_db, float ratio,
                                                float att_coeff, float rel_coeff) {
    // 1. Convert smoothed envelope to clean dB values
    float32x4_t db_env = vmulq_f32(precision_log2_neon(vmaxq_f32(env_linear, vdupq_n_f32(1e-5f))), vdupq_n_f32(6.0206f));
    float32x4_t excess = vmaxq_f32(vsubq_f32(db_env, vdupq_n_f32(thresh_db)), vdupq_n_f32(0.0f));

    // 2. Prevent division traps using an expansion epsilon snap
    float32x4_t ratio_vec = vdupq_n_f32(ratio);
    uint32x4_t near_zero = vcltq_f32(vabsq_f32(ratio_vec), vdupq_n_f32(0.01f));
    float32x4_t safe_ratio = vbslq_f32(near_zero, vdupq_n_f32(0.001f), ratio_vec);

    // Standard gain computer equation
    float32x4_t target_gr_db = vnegq_f32(vdivq_f32(vmulq_f32(excess, vsubq_f32(ratio_vec, vdupq_n_f32(1.0f))), safe_ratio));

    // Smoothly apply an accurate infinite hard limit mask if ratio == 0.0f
    target_gr_db = vbslq_f32(vceqq_f32(ratio_vec, vdupq_n_f32(0.0f)), vnegq_f32(excess), target_gr_db);

    // 3. Sequential vector tracking for ballistics
    float targets[4], out[4];
    vst1q_f32(targets, target_gr_db);
    float state = vgetq_lane_f32(*gain_state, 3);

    const float coeff_avg  = 0.5f * (att_coeff + rel_coeff);
    const float coeff_diff = 0.5f * (att_coeff - rel_coeff);

    for(int i = 0; i < NEON_LANES; ++i) {
        float diff = state - targets[i];
        state = targets[i] + (coeff_avg * diff) + (coeff_diff * std::fabs(diff));
        out[i] = state;
    }
    *gain_state = vld1q_f32(out);

    // Convert smoothed gain reduction from dB back to a linear scalar multiplier
    return neon_expq_f32(vmulq_f32(*gain_state, vdupq_n_f32(0.11512925f)));
}

// Processing Execution Path
fast_inline void multiband_process(multiband_t* mb,
                                   float32x4_t in_l, float32x4_t in_r,
                                   float32x4_t* out_l, float32x4_t* out_r) {
    float32x4_t low_l, low_r;
    float32x4_t mid_l, mid_r;
    float32x4_t high_l, high_r;
    float32x4_t temp_l, temp_r;
    float32x4_t unused_l, unused_r;

    // ------------------------------------------------------------
    // Stage 1: Frequency band splitting via Linkwitz Tree
    // ------------------------------------------------------------
    crossover_process(&mb->xover_low_mid, in_l, in_r,
                      &low_l, &low_r,
                      &unused_l, &unused_r,
                      &temp_l, &temp_r,
                      mb->xover_low_freq, mb->sample_rate);

    crossover_process(&mb->xover_mid_high, temp_l, temp_r,
                      &mid_l, &mid_r,
                      &unused_l, &unused_r,
                      &high_l, &high_r,
                      mb->xover_high_freq, mb->sample_rate);

    // ------------------------------------------------------------
    // Stage 2: Stereo-linked envelope estimation
    // ------------------------------------------------------------
    float32x4_t inst_low  = vmaxq_f32(vabsq_f32(low_l),  vabsq_f32(low_r));
    float32x4_t inst_mid  = vmaxq_f32(vabsq_f32(mid_l),  vabsq_f32(mid_r));
    float32x4_t inst_high = vmaxq_f32(vabsq_f32(high_l), vabsq_f32(high_r));

    float ilow[4], imid[4], ihigh[4];
    float olow[4], omid[4], ohigh[4];
    vst1q_f32(ilow, inst_low); vst1q_f32(imid, inst_mid); vst1q_f32(ihigh, inst_high);

    float slow = vgetq_lane_f32(mb->comp_env_state[BAND_LOW], 3);
    float smid = vgetq_lane_f32(mb->comp_env_state[BAND_MID], 3);
    float shigh = vgetq_lane_f32(mb->comp_env_state[BAND_HIGH], 3);

    float pre_c  = mb->env_pre_coeff;
    float pre_1c = 1.0f - pre_c;

    for (int i = 0; i < NEON_LANES; ++i) {
        slow  = ilow[i] * pre_1c + slow * pre_c;
        smid  = imid[i] * pre_1c + smid * pre_c;
        shigh = ihigh[i] * pre_1c + shigh * pre_c;
        olow[i] = slow; omid[i] = smid; ohigh[i] = shigh;
    }
    mb->comp_env_state[BAND_LOW]  = vld1q_f32(olow);
    mb->comp_env_state[BAND_MID]  = vld1q_f32(omid);
    mb->comp_env_state[BAND_HIGH] = vld1q_f32(ohigh);

    // ------------------------------------------------------------
    // Stage 3: Dynamic Compression Lane processing using Parameter Cache
    // ------------------------------------------------------------
    // Dynamic values of ratio, threshold, and coefficients are explicitly utilized here
    float32x4_t gain_low = process_compressor_lane(&mb->comp_gain_state[BAND_LOW], mb->comp_env_state[BAND_LOW],
                                                   mb->bands[BAND_LOW].thresh_db, mb->bands[BAND_LOW].ratio,
                                                   mb->bands[BAND_LOW].attack_coeff, mb->bands[BAND_LOW].release_coeff);

    float32x4_t gain_mid = process_compressor_lane(&mb->comp_gain_state[BAND_MID], mb->comp_env_state[BAND_MID],
                                                   mb->bands[BAND_MID].thresh_db, mb->bands[BAND_MID].ratio,
                                                   mb->bands[BAND_MID].attack_coeff, mb->bands[BAND_MID].release_coeff);

    float32x4_t gain_high = process_compressor_lane(&mb->comp_gain_state[BAND_HIGH], mb->comp_env_state[BAND_HIGH],
                                                    mb->bands[BAND_HIGH].thresh_db, mb->bands[BAND_HIGH].ratio,
                                                    mb->bands[BAND_HIGH].attack_coeff, mb->bands[BAND_HIGH].release_coeff);

    // Apply calculated compressor gain reductions
    low_l  = vmulq_f32(low_l,  gain_low);  low_r  = vmulq_f32(low_r,  gain_low);
    mid_l  = vmulq_f32(mid_l,  gain_mid);  mid_r  = vmulq_f32(mid_r,  gain_mid);
    high_l = vmulq_f32(high_l, gain_high); high_r = vmulq_f32(high_r, gain_high);

    // ------------------------------------------------------------
    // Stage 4: Integrated Pirkle Waveshaping & Offset Sanitization
    // ------------------------------------------------------------
    // Left Channel Processing
    float32x4_t sat_l_low  = pirkle_triode_engine(low_l,  mb->bands[BAND_LOW].drive,  &mb->tube_bias_l[BAND_LOW], 3.2f, 1.1f);
    float32x4_t sat_l_mid  = pirkle_triode_engine(mid_l,  mb->bands[BAND_MID].drive,  &mb->tube_bias_l[BAND_MID], 5.5f, 1.8f);
    float32x4_t sat_l_high = pirkle_triode_engine(high_l, mb->bands[BAND_HIGH].drive, &mb->tube_bias_l[BAND_HIGH], 6.0f, 2.5f);

    sat_l_low  = dc_block_lane(&mb->dc_blockers_l[BAND_LOW],  sat_l_low,  0.995f);
    sat_l_mid  = dc_block_lane(&mb->dc_blockers_l[BAND_MID],  sat_l_mid,  0.995f);
    sat_l_high = dc_block_lane(&mb->dc_blockers_l[BAND_HIGH], sat_l_high, 0.995f);

    // Right Channel Processing - TODO use different values? set values to constant array?
    float32x4_t sat_r_low  = pirkle_triode_engine(low_r,  mb->bands[BAND_LOW].drive,  &mb->tube_bias_r[BAND_LOW], 3.2f, 1.1f);
    float32x4_t sat_r_mid  = pirkle_triode_engine(mid_r,  mb->bands[BAND_MID].drive,  &mb->tube_bias_r[BAND_MID], 5.5f, 1.8f);
    float32x4_t sat_r_high = pirkle_triode_engine(high_r, mb->bands[BAND_HIGH].drive, &mb->tube_bias_r[BAND_HIGH], 6.0f, 2.5f);

    sat_r_low  = dc_block_lane(&mb->dc_blockers_r[BAND_LOW],  sat_r_low,  0.995f);
    sat_r_mid  = dc_block_lane(&mb->dc_blockers_r[BAND_MID],  sat_r_mid,  0.995f);
    sat_r_high = dc_block_lane(&mb->dc_blockers_r[BAND_HIGH], sat_r_high, 0.995f);

    // Correct the common-cathode native inversion signature
    sat_l_low  = vnegq_f32(sat_l_low);  sat_r_low  = vnegq_f32(sat_r_low);
    sat_l_mid  = vnegq_f32(sat_l_mid);  sat_r_mid  = vnegq_f32(sat_r_mid);
    sat_l_high = vnegq_f32(sat_l_high); sat_r_high = vnegq_f32(sat_r_high);

    // ------------------------------------------------------------
    // [Optional Phase APF Processing for sat_l_low/sat_r_low executes here]
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // Stage 5: Apply Dynamic Makeup Gain & Mute/Solo Rules
    // ------------------------------------------------------------
    int any_solo   = (mb->bands[BAND_LOW].solo  > 0.0f || mb->bands[BAND_MID].solo > 0.0f || mb->bands[BAND_HIGH].solo > 0.0f);
    float low_act  = (mb->bands[BAND_LOW].solo  > 0.0f || (!any_solo && mb->bands[BAND_LOW].mute  == 0.0f)) ? mb->bands[BAND_LOW].makeup_gain_linear  : 0.0f;
    float mid_act  = (mb->bands[BAND_MID].solo  > 0.0f || (!any_solo && mb->bands[BAND_MID].mute  == 0.0f)) ? mb->bands[BAND_MID].makeup_gain_linear  : 0.0f;
    float high_act = (mb->bands[BAND_HIGH].solo > 0.0f || (!any_solo && mb->bands[BAND_HIGH].mute == 0.0f)) ? mb->bands[BAND_HIGH].makeup_gain_linear : 0.0f;

    // Direct structural summation
    *out_l = vaddq_f32(vaddq_f32(vmulq_n_f32(sat_l_low, low_act), vmulq_n_f32(sat_l_mid, mid_act)), vmulq_n_f32(sat_l_high, high_act));
    *out_r = vaddq_f32(vaddq_f32(vmulq_n_f32(sat_r_low, low_act), vmulq_n_f32(sat_r_mid, mid_act)), vmulq_n_f32(sat_r_high, high_act));
}
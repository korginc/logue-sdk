#pragma once
/*
 * File: multiband.h
 *
 * 3-band compressor with independent controls
 * Based on SHARC Audio Elements multiband compressor architecture
 */

#include <arm_neon.h>
#include "compressor_core.h"
#include "crossover.h"
#include "constants.h"

#define BAND_LOW 0
#define BAND_MID 1
#define BAND_HIGH 2
#define NUM_OF_BANDS (3)

// Multiband compressor state
typedef struct {
    // Crossover filters
    crossover_t xover_low_mid;   // Crossover between low and mid
    crossover_t xover_mid_high;  // Crossover between mid and high

    // Independent compressors for each band
    compressor_t comp_low;
    compressor_t comp_mid;
    compressor_t comp_high;

    // Independent parameters per band
    struct {
        float thresh_db;
        float ratio;
        float makeup_db;
        float makeup_gain_linear;  // fasterpowf(10, makeup_db/20) — pre-computed at set time
        float attack_ms;
        float release_ms;
        float mute;          // 0=off, 1=on (for solo/mute)
        float solo;          // 0=off, 1=on
        // per‑band time constants
        float attack_coeff;
        float release_coeff;
    } bands[NUM_OF_BANDS];

    // Crossover frequencies
    float xover_low_freq;    // e.g., 250 Hz
    float xover_high_freq;   // e.g., 2500 Hz

    // Gain reduction meters (for visualization)
    float32x4_t gr_low;
    float32x4_t gr_mid;
    float32x4_t gr_high;

    // Per-band envelope pre-smoother states (~1ms one-pole, eliminates half-cycle gain modulation)
    float32x4_t env_state_low;
    float32x4_t env_state_mid;
    float32x4_t env_state_high;
    float env_pre_coeff;

    // Sample rate
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

// Process multiband compressor (stereo, 4 samples at a time)
fast_inline void multiband_process(multiband_t* mb,
                                   float32x4_t in_l,
                                   float32x4_t in_r,
                                   float32x4_t* out_l,
                                   float32x4_t* out_r) {

    float32x4_t low_l, low_r;          // final low band
    float32x4_t mid_l, mid_r;          // final mid band
    float32x4_t high_l, high_r;        // final high band
    float32x4_t temp_l, temp_r;        // HP output of first crossover (mid+high)
    float32x4_t unused_l, unused_r;    // unused mid output of first crossover

    // ------------------------------------------------------------
    // Stage 1: split input into low band and (mid+high) HP
    // ------------------------------------------------------------
    crossover_process(&mb->xover_low_mid, in_l, in_r,
                      &low_l, &low_r,        // LP output = low band
                      &unused_l, &unused_r,  // mid complement (allpass - LP - HP), not used
                      &temp_l, &temp_r,      // HP output = mid+high
                      mb->xover_low_freq, mb->sample_rate);

    // temp_l/r now hold everything above xover_low_freq (mid+high)

    // ------------------------------------------------------------
    // Stage 2: split (mid+high) into mid and high
    // ------------------------------------------------------------
    crossover_process(&mb->xover_mid_high, temp_l, temp_r,
                      &mid_l, &mid_r,        // LP output = mid band
                      &unused_l, &unused_r,  // mid complement, not used
                      &high_l, &high_r,      // HP output = high band
                      mb->xover_high_freq, mb->sample_rate);

    // ------------------------------------------------------------
    // Now low_l, mid_l, high_l (and R) contain the three bands correctly.
    // ------------------------------------------------------------

    // Stereo-linked instantaneous envelope (max of L and R absolute values)
    float32x4_t inst_low  = vmaxq_f32(vabsq_f32(low_l),  vabsq_f32(low_r));
    float32x4_t inst_mid  = vmaxq_f32(vabsq_f32(mid_l),  vabsq_f32(mid_r));
    float32x4_t inst_high = vmaxq_f32(vabsq_f32(high_l), vabsq_f32(high_r));

    // One-pole pre-smoothing (~1ms) prevents gain modulation at signal frequency
    const float32x4_t pre_c  = vdupq_n_f32(mb->env_pre_coeff);
    const float32x4_t pre_1c = vdupq_n_f32(1.0f - mb->env_pre_coeff);
    mb->env_state_low  = vaddq_f32(vmulq_f32(inst_low,  pre_1c), vmulq_f32(mb->env_state_low,  pre_c));
    mb->env_state_mid  = vaddq_f32(vmulq_f32(inst_mid,  pre_1c), vmulq_f32(mb->env_state_mid,  pre_c));
    mb->env_state_high = vaddq_f32(vmulq_f32(inst_high, pre_1c), vmulq_f32(mb->env_state_high, pre_c));

    float32x4_t env_low  = mb->env_state_low;
    float32x4_t env_mid  = mb->env_state_mid;
    float32x4_t env_high = mb->env_state_high;

    // Compute gain reduction for each band (compressor_calc_gain converts linear→dB internally)
    float32x4_t low_gr  = compressor_calc_gain(&mb->comp_low,  env_low,
                                                mb->bands[BAND_LOW].thresh_db,
                                                mb->bands[BAND_LOW].ratio);
    float32x4_t mid_gr  = compressor_calc_gain(&mb->comp_mid,  env_mid,
                                                mb->bands[BAND_MID].thresh_db,
                                                mb->bands[BAND_MID].ratio);
    float32x4_t high_gr = compressor_calc_gain(&mb->comp_high, env_high,
                                                mb->bands[BAND_HIGH].thresh_db,
                                                mb->bands[BAND_HIGH].ratio);

    // Smooth gain reduction per band with individual attack/release coefficients
    mb->gr_low = compressor_smooth(&mb->comp_low, low_gr,
                                    mb->bands[BAND_LOW].attack_coeff,
                                    mb->bands[BAND_LOW].release_coeff);
    mb->gr_mid = compressor_smooth(&mb->comp_mid, mid_gr,
                                    mb->bands[BAND_MID].attack_coeff,
                                    mb->bands[BAND_MID].release_coeff);
    mb->gr_high = compressor_smooth(&mb->comp_high, high_gr,
                                    mb->bands[BAND_HIGH].attack_coeff,
                                    mb->bands[BAND_HIGH].release_coeff);
    // Convert gain reduction from dB to linear (ARMv7-compatible)
    float32x4_t low_gain = neon_expq_f32(vmulq_f32(mb->gr_low, vdupq_n_f32(0.115129f)));
    float32x4_t mid_gain = neon_expq_f32(vmulq_f32(mb->gr_mid, vdupq_n_f32(0.115129f)));
    float32x4_t high_gain = neon_expq_f32(vmulq_f32(mb->gr_high, vdupq_n_f32(0.115129f)));

    // Apply makeup gain (pre-computed at parameter-set time — no fasterpowf in hot path)
    low_gain  = vmulq_n_f32(low_gain,  mb->bands[BAND_LOW].makeup_gain_linear);
    mid_gain  = vmulq_n_f32(mid_gain,  mb->bands[BAND_MID].makeup_gain_linear);
    high_gain = vmulq_n_f32(high_gain, mb->bands[BAND_HIGH].makeup_gain_linear);

    // Apply mute/solo logic
    int any_solo = (mb->bands[BAND_LOW].solo > 0.0f ||
                    mb->bands[BAND_MID].solo  > 0.0f ||
                    mb->bands[BAND_HIGH].solo > 0.0f);

    // A band is active if: it is soloed, OR (no solo active AND it is not muted)
    float low_active_f  = (mb->bands[BAND_LOW].solo  > 0.0f || (!any_solo && mb->bands[BAND_LOW].mute  == 0.0f)) ? 1.0f : 0.0f;
    float mid_active_f  = (mb->bands[BAND_MID].solo  > 0.0f || (!any_solo && mb->bands[BAND_MID].mute  == 0.0f)) ? 1.0f : 0.0f;
    float high_active_f = (mb->bands[BAND_HIGH].solo > 0.0f || (!any_solo && mb->bands[BAND_HIGH].mute == 0.0f)) ? 1.0f : 0.0f;

    float32x4_t low_active  = vdupq_n_f32(low_active_f);
    float32x4_t mid_active  = vdupq_n_f32(mid_active_f);
    float32x4_t high_active = vdupq_n_f32(high_active_f);

    // Apply gains to each band
    float32x4_t out_low_l  = vmulq_f32(vmulq_f32(low_l,  low_gain),  low_active);
    float32x4_t out_low_r  = vmulq_f32(vmulq_f32(low_r,  low_gain),  low_active);

    float32x4_t out_mid_l  = vmulq_f32(vmulq_f32(mid_l,  mid_gain),  mid_active);
    float32x4_t out_mid_r  = vmulq_f32(vmulq_f32(mid_r,  mid_gain),  mid_active);

    float32x4_t out_high_l = vmulq_f32(vmulq_f32(high_l, high_gain), high_active);
    float32x4_t out_high_r = vmulq_f32(vmulq_f32(high_r, high_gain), high_active);

    // Sum bands
    *out_l = vaddq_f32(vaddq_f32(out_low_l, out_mid_l), out_high_l);
    *out_r = vaddq_f32(vaddq_f32(out_low_r, out_mid_r), out_high_r);
}
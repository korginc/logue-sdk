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

// Band selection for parameters
#define BAND_LOW   0
#define BAND_MID   1
#define BAND_HIGH  2
#define BAND_ALL   3

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
        float attack_ms;
        float release_ms;
        float mute;          // 0=off, 1=on (for solo/mute)
        float solo;          // 0=off, 1=on
    } bands[3];

    // Crossover frequencies
    float xover_low_freq;    // e.g., 250 Hz
    float xover_high_freq;   // e.g., 2500 Hz

    // Gain reduction meters (for visualization)
    float32x4_t gr_low;
    float32x4_t gr_mid;
    float32x4_t gr_high;

    // Sample rate
    float sample_rate;
} multiband_t;

// Initialize multiband compressor
fast_inline void multiband_init(multiband_t* mb, float sample_rate) {
    mb->sample_rate = sample_rate;

    // Default crossover frequencies
    mb->xover_low_freq = 250.0f;
    mb->xover_high_freq = 2500.0f;

    // Initialize crossovers
    crossover_init(&mb->xover_low_mid, mb->xover_low_freq, sample_rate);
    crossover_init(&mb->xover_mid_high, mb->xover_high_freq, sample_rate);

    // Initialize compressors
    compressor_init(&mb->comp_low);
    compressor_init(&mb->comp_mid);
    compressor_init(&mb->comp_high);

    // Default band parameters
    for (int i = 0; i < 3; i++) {
        mb->bands[i].thresh_db = -20.0f;
        mb->bands[i].ratio = 4.0f;
        mb->bands[i].makeup_db = 0.0f;
        mb->bands[i].attack_ms = 10.0f;
        mb->bands[i].release_ms = 100.0f;
        mb->bands[i].mute = 0.0f;
        mb->bands[i].solo = 0.0f;
    }

    mb->gr_low = vdupq_n_f32(0.0f);
    mb->gr_mid = vdupq_n_f32(0.0f);
    mb->gr_high = vdupq_n_f32(0.0f);
}

fast_inline void multiband_reset(multiband_t* m) {
    multiband_init(m);
}

// Set crossover frequencies
fast_inline void multiband_set_crossover(multiband_t* mb,
                                         float low_freq,
                                         float high_freq) {
    mb->xover_low_freq = low_freq;
    mb->xover_high_freq = high_freq;

    // Reinitialize crossovers
    crossover_init(&mb->xover_low_mid, low_freq, mb->sample_rate);
    crossover_init(&mb->xover_mid_high, high_freq, mb->sample_rate);
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
        case 2: mb->bands[band].makeup_db = value; break;
        case 3: mb->bands[band].attack_ms = value; break;
        case 4: mb->bands[band].release_ms = value; break;
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

    // Split into bands
    float32x4_t low_l, low_r, mid_l, mid_r, high_l, high_r;

    // First crossover: separate low from mid+high
    float32x4_t temp_mid_high_l, temp_mid_high_r;
    crossover_process(&mb->xover_low_mid, in_l, in_r,
                      &low_l, &low_r,
                      &temp_mid_high_l, &temp_mid_high_r,
                      &mid_l, &mid_r,  // These will be overwritten
                      mb->xover_low_freq, mb->sample_rate);

    // Second crossover: separate mid from high
    crossover_process(&mb->xover_mid_high, temp_mid_high_l, temp_mid_high_r,
                      &mid_l, &mid_r,
                      &high_l, &high_r,
                      &high_l, &high_r,  // Dummy - high is output
                      mb->xover_high_freq, mb->sample_rate);

    // Calculate envelope for each band (using absolute value for peak detection)
    float32x4_t env_low = vabsq_f32(low_l);
    float32x4_t env_mid = vabsq_f32(mid_l);
    float32x4_t env_high = vabsq_f32(high_l);

    // Convert to dB (simplified approximation)
    // log10(x) ≈ log2(x) * 0.30103
    uint32x4_t exp_mask = vdupq_n_u32(0x7F800000);
    uint32x4_t mant_mask = vdupq_n_u32(0x007FFFFF);

    // Process each band through its compressor
    float32x4_t low_gr, mid_gr, high_gr;

    // Low band
    float32x4_t low_db = vdupq_n_f32(0.0f);  // Simplified - proper dB conversion
    low_gr = compressor_calc_gain(&mb->comp_low, env_low,
                                   mb->bands[BAND_LOW].thresh_db,
                                   mb->bands[BAND_LOW].ratio);

    // Mid band
    float32x4_t mid_db = vdupq_n_f32(0.0f);
    mid_gr = compressor_calc_gain(&mb->comp_mid, env_mid,
                                   mb->bands[BAND_MID].thresh_db,
                                   mb->bands[BAND_MID].ratio);

    // High band
    float32x4_t high_db = vdupq_n_f32(0.0f);
    high_gr = compressor_calc_gain(&mb->comp_high, env_high,
                                    mb->bands[BAND_HIGH].thresh_db,
                                    mb->bands[BAND_HIGH].ratio);

    // Smooth gain reduction (attack/release)
    float attack_coeff = expf(-1.0f / (mb->bands[0].attack_ms * 0.001f * mb->sample_rate));
    float release_coeff = expf(-1.0f / (mb->bands[0].release_ms * 0.001f * mb->sample_rate));

    mb->gr_low = compressor_smooth(&mb->comp_low, low_gr, attack_coeff, release_coeff);
    mb->gr_mid = compressor_smooth(&mb->comp_mid, mid_gr, attack_coeff, release_coeff);
    mb->gr_high = compressor_smooth(&mb->comp_high, high_gr, attack_coeff, release_coeff);

    // Convert gain reduction from dB to linear
    float32x4_t low_gain = vexpq_f32(vmulq_f32(mb->gr_low, vdupq_n_f32(0.115129f)));
    float32x4_t mid_gain = vexpq_f32(vmulq_f32(mb->gr_mid, vdupq_n_f32(0.115129f)));
    float32x4_t high_gain = vexpq_f32(vmulq_f32(mb->gr_high, vdupq_n_f32(0.115129f)));

    // Apply makeup gain
    low_gain = vmulq_f32(low_gain, vdupq_n_f32(powf(10.0f, mb->bands[BAND_LOW].makeup_db / 20.0f)));
    mid_gain = vmulq_f32(mid_gain, vdupq_n_f32(powf(10.0f, mb->bands[BAND_MID].makeup_db / 20.0f)));
    high_gain = vmulq_f32(high_gain, vdupq_n_f32(powf(10.0f, mb->bands[BAND_HIGH].makeup_db / 20.0f)));

    // Apply mute/solo logic
    float32x4_t solo_any = vdupq_n_f32(0.0f);
    if (mb->bands[0].solo > 0.0f || mb->bands[1].solo > 0.0f || mb->bands[2].solo > 0.0f) {
        solo_any = vdupq_n_f32(1.0f);
    }

    float32x4_t low_active = vbslq_f32(vdupq_n_u32(mb->bands[0].solo > 0.0f),
                                       vdupq_n_f32(1.0f),
                                       vdupq_n_f32(mb->bands[0].mute > 0.0f ? 0.0f : 1.0f));
    low_active = vbslq_f32(vdupq_n_u32(mb->bands[0].solo == 0.0f && vgetq_lane_f32(solo_any,0) > 0.0f),
                          vdupq_n_f32(0.0f), low_active);

    // Apply gains to each band
    float32x4_t out_low_l = vmulq_f32(vmulq_f32(low_l, low_gain), low_active);
    float32x4_t out_low_r = vmulq_f32(vmulq_f32(low_r, low_gain), low_active);

    float32x4_t out_mid_l = vmulq_f32(vmulq_f32(mid_l, mid_gain),
                                      vdupq_n_f32(mb->bands[1].mute > 0.0f ? 0.0f : 1.0f));
    float32x4_t out_mid_r = vmulq_f32(vmulq_f32(mid_r, mid_gain),
                                      vdupq_n_f32(mb->bands[1].mute > 0.0f ? 0.0f : 1.0f));

    float32x4_t out_high_l = vmulq_f32(vmulq_f32(high_l, high_gain),
                                       vdupq_n_f32(mb->bands[2].mute > 0.0f ? 0.0f : 1.0f));
    float32x4_t out_high_r = vmulq_f32(vmulq_f32(high_r, high_gain),
                                       vdupq_n_f32(mb->bands[2].mute > 0.0f ? 0.0f : 1.0f));

    // Sum bands
    *out_l = vaddq_f32(vaddq_f32(out_low_l, out_mid_l), out_high_l);
    *out_r = vaddq_f32(vaddq_f32(out_low_r, out_mid_r), out_high_r);
}
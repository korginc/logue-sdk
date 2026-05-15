#pragma once
/*
 * File: distressor_mode.h
 *
 * Distressor-style compression curves and harmonic generation
 * Emulates Empirical Labs EL8 Distressor
 */

#include <arm_neon.h>
#include <math.h>
#include "filters.h"
#include "constants.h"

// Detector modes (bit flags)
#define DETECT_NONE      0
#define DETECT_HPF       (1 << 0)  // 100 Hz high-pass
#define DETECT_BAND_EMPH (1 << 1)  // 6 kHz boost
#define DETECT_LINK      (1 << 2)  // Stereo link

// Audio modes (bit flags)
#define AUDIO_NONE       0
#define AUDIO_HPF        (1 << 0)  // 80 Hz Bessel HPF
#define AUDIO_DIST2      (1 << 1)  // 2nd harmonic
#define AUDIO_DIST3      (1 << 2)  // 3rd harmonic

// Distressor Mode String Display
static const char* distressor_dist_strings[5] = {
    "Off",      // 0 - Clean
    "Dist2",    // 1 - Tube-like 2nd harmonic
    "Dist3",    // 2 - Tape-like 3rd harmonic
    "Both",     // 3 - Combined harmonics
    "Wave",     // 4 - Wavefolder (new)
};

// Display strings for UI
static const char* distressor_ratio_strings[8] = {
    "1:1", "2:1", "3:1", "4:1", "6:1", "Opto", "20:1", "NUKE"
};

// Display strings for UI
static const char* distressor_wave_type[5] = {
    "Soft", "Hard", "Trg", "Sine", "SubOct"
};


// Distressor state structure
typedef struct {
    // Mode selection
    uint8_t ratio_mode;      // 0-7
    uint8_t dist_mode;       // 0-4 (CLEAN, DIST2, DIST3, BOTH, WAVE)
    uint8_t detector_mode;   // Bit flags

    // Time constants
    float attack_ms;         // 0.05 to 30 ms
    float release_ms;        // 50 to 3500 ms (up to 20000 in opto)
    float attack_coeff;
    float release_coeff;

    // Harmonic generation state
    float32x4_t harmonic_state;
    float32x4_t last_input;

    // Opto mode state (slow release simulation)
    float opto_release_mult;  // Up to 20s in opto mode
    float opto_coeff ;        // deduced from above

    // NEW: Distressor-specific detector components
    sidechain_hpf_t detect_hpf;      // 100 Hz HPF for detector (mono / L channel)
    sidechain_hpf_t detect_emph;     // 6 kHz emphasis for detector (mono / L channel)
    sidechain_hpf_t detect_hpf_r;   // 100 Hz HPF — R channel (DETECT_LINK only)
    sidechain_hpf_t detect_emph_r;  // 6 kHz emphasis — R channel (DETECT_LINK only)
    envelope_detector_t distressor_env;  // Dedicated envelope detector
    float32x4_t detector_state;

} distressor_t;

fast_inline void update_opto_coeff(distressor_t* d, float release_coeff_) {
    // Opto mode slows release by raising the coefficient to 1/mult power,
    // which is equivalent to multiplying the release time constant by mult
    // while keeping the coefficient safely in (0,1).
    d->opto_coeff = (d->opto_release_mult > 1.0f)
        ? fasterpowf(release_coeff_, 1.0f / d->opto_release_mult): release_coeff_;
}

// Initialize Distressor with detector
fast_inline void distressor_init(distressor_t* d, float sample_rate) {
    d->ratio_mode = DIST_RATIO_4_1;
    d->dist_mode = DIST_MODE_CLEAN;
    d->detector_mode = DETECT_NONE;
    d->attack_ms = 0.5f;      // Much faster than standard (0.5ms)
    d->release_ms = 200.0f;
    d->attack_coeff = expf(-1.0f / (d->attack_ms * 0.001f * sample_rate));
    d->release_coeff = expf(-1.0f / (d->release_ms * 0.001f * sample_rate));
    d->harmonic_state = vdupq_n_f32(0.0f);
    d->last_input = vdupq_n_f32(0.0f);
    d->opto_release_mult = 1.0f;
    d->opto_coeff = 0.0f;   // to be updated according to opto_release_mult
    d->detector_state = vdupq_n_f32(0.0f);

    // Initialize detector HPF at 100 Hz (removes low-end pumping)
    sidechain_hpf_init(&d->detect_hpf,   100.0f, sample_rate);
    sidechain_hpf_init(&d->detect_hpf_r, 100.0f, sample_rate);

    // Initialize emphasis filter at 6 kHz (boosts transients)
    sidechain_hpf_init(&d->detect_emph,   6000.0f, sample_rate);
    sidechain_hpf_init(&d->detect_emph_r, 6000.0f, sample_rate);

    // Initialize distressor envelope detector
    envelope_detector_init(&d->distressor_env, sample_rate);

    // Set faster attack/release for distressor detector
    envelope_set_attack_release(&d->distressor_env, d->attack_ms, d->release_ms);
}

// Distressor-specific mono/summed envelope detector.
// DETECT_HPF and DETECT_BAND_EMPH flags are evaluated here so the caller
// does not need to know which filters are active.
fast_inline float32x4_t distressor_detect(distressor_t* d,
                                           float32x4_t sidechain,
                                           float sample_rate) {
    (void)sample_rate;
    float32x4_t detected = sidechain;

    // 1. Optional 100 Hz HPF (DETECT_HPF): removes low-end pumping
    if (d->detector_mode & DETECT_HPF)
        detected = sidechain_hpf_process(&d->detect_hpf, detected);

    // 2. Optional 6 kHz emphasis (DETECT_BAND_EMPH): increases transient sensitivity
    if (d->detector_mode & DETECT_BAND_EMPH)
        detected = sidechain_hpf_process(&d->detect_emph, detected);

    // 3. Full-wave rectification
    detected = vabsq_f32(detected);

    // 4. Distressor-specific envelope smoothing
    return envelope_detect(&d->distressor_env, detected);
}

// Stereo-linked envelope detector (DETECT_LINK).
// Processes L and R through independent filter chains, then takes the
// per-sample maximum so the loudest channel drives gain reduction on both.
fast_inline float32x4_t distressor_detect_stereo(distressor_t* d,
                                                   float32x4_t sc_l,
                                                   float32x4_t sc_r,
                                                   float sample_rate) {
    (void)sample_rate;

    if (d->detector_mode & DETECT_HPF) {
        sc_l = sidechain_hpf_process(&d->detect_hpf,   sc_l);
        sc_r = sidechain_hpf_process(&d->detect_hpf_r, sc_r);
    }
    if (d->detector_mode & DETECT_BAND_EMPH) {
        sc_l = sidechain_hpf_process(&d->detect_emph,   sc_l);
        sc_r = sidechain_hpf_process(&d->detect_emph_r, sc_r);
    }

    // Per-sample max of absolute values — the louder channel controls GR
    float32x4_t linked = vmaxq_f32(vabsq_f32(sc_l), vabsq_f32(sc_r));
    return envelope_detect(&d->distressor_env, linked);
}

fast_inline void distressor_reset(distressor_t* d, float sample_rate) {
    distressor_init(d, sample_rate);
}

// Distressor-specific smoothing with opto mode support
fast_inline float32x4_t distressor_smooth(distressor_t* d,
                                          float32x4_t target_db,
                                          float attack_coeff,
                                          float release_coeff) {
    // One-pole smoothing with mode-specific adjustments
    uint32x4_t attacking = vcltq_f32(target_db, d->harmonic_state);
    float32x4_t coeff = vbslq_f32(attacking,
                                  vdupq_n_f32(attack_coeff),
                                  vdupq_n_f32(release_coeff));

    d->harmonic_state = vaddq_f32(vmulq_f32(target_db, vsubq_f32(vdupq_n_f32(1.0f), coeff)),
                                   vmulq_f32(d->harmonic_state, coeff));

    return d->harmonic_state;
}

// Set ratio mode (updates compression curve and opto behavior)
fast_inline void distressor_set_ratio(distressor_t* d, uint8_t mode) {
    d->ratio_mode = mode;

    // In 10:1 "Opto" mode, release can be up to 20 seconds [citation:4]
    if (mode == DIST_RATIO_10_1) {
        d->opto_release_mult = 5.7f;  // Scale release to 20s max
    } else {
        d->opto_release_mult = 1.0f;
    }
}

// Generate 2nd/3rd harmonics using bounded asymmetric/symmetric saturators.
// Old polynomial (x + 0.5*x^2, x - x^3/3) diverged for |x|>1 and was
// inaudible at typical post-compression levels.  These use NEON fast_div_neon
// to produce clean harmonic content bounded within ±1 (DIST3) / ~±1.5 (DIST2).
fast_inline float32x4_t generate_harmonics(distressor_t* d,
                                           float32x4_t in,
                                           uint8_t mode) {
    (void)d;
    float32x4_t pos, neg, sat_pos, sat_neg;

    switch (mode) {
        case DIST_MODE_DIST2: {
            // Asymmetric saturator → even harmonics (tube warmth).
            // Positive clips softly (asymptote 2), negative clips harder (asymptote 0.5).
            // Ratio at x=±1: +0.67 / −0.33 → strong 2nd harmonic character.
            pos = vmaxq_f32(in, vdupq_n_f32(0.0f));
            neg = vminq_f32(in, vdupq_n_f32(0.0f));
            sat_pos = fast_div_neon(pos,
                          vaddq_f32(vdupq_n_f32(1.0f), vmulq_f32(pos, vdupq_n_f32(0.5f))));
            sat_neg = fast_div_neon(neg,
                          vaddq_f32(vdupq_n_f32(1.0f), vmulq_f32(vabsq_f32(neg), vdupq_n_f32(2.0f))));
            return vaddq_f32(sat_pos, sat_neg);
        }

        case DIST_MODE_DIST3: {
            // Symmetric saturator → odd harmonics only (tape warmth).
            // y = x / (1 + |x|) — bounded ±1, zero-phase, purely odd harmonics.
            return fast_div_neon(in, vaddq_f32(vdupq_n_f32(1.0f), vabsq_f32(in)));
        }

        case DIST_MODE_BOTH: {
            // Mild asymmetry on positive, symmetric on negative → both even and odd.
            pos = vmaxq_f32(in, vdupq_n_f32(0.0f));
            neg = vminq_f32(in, vdupq_n_f32(0.0f));
            sat_pos = fast_div_neon(pos,
                          vaddq_f32(vdupq_n_f32(1.0f), vmulq_f32(pos, vdupq_n_f32(0.8f))));
            sat_neg = fast_div_neon(neg,
                          vaddq_f32(vdupq_n_f32(1.0f), vabsq_f32(neg)));
            return vaddq_f32(sat_pos, sat_neg);
        }

        default:
            return in;
    }
}

// Distressor gain computer (8 unique curves)
fast_inline float32x4_t distressor_gain_computer(distressor_t* d,
                                                  float32x4_t envelope_db,
                                                  float thresh_db) {
    float32x4_t gain_db = vdupq_n_f32(0.0f);
    float32x4_t thresh = vdupq_n_f32(thresh_db);
    float32x4_t excess = vsubq_f32(envelope_db, thresh);
    excess = vmaxq_f32(excess, vdupq_n_f32(0.0f));

    switch (d->ratio_mode) {
        case DIST_RATIO_1_1:
            // 1:1 - no compression, just harmonics
            // gain_db = vdupq_n_f32(0.0f); // avoid unnecessary operations
            break;

        case DIST_RATIO_2_1:
            // 2:1 - gentle parabolic knee
            // GR = excess * 0.5 with soft knee extending up to 30dB [citation:4]
            gain_db = vmulq_f32(excess, vdupq_n_f32(0.5f));
            break;

        case DIST_RATIO_3_1:
            gain_db = vmulq_f32(excess, vdupq_n_f32(0.667f));
            break;

        case DIST_RATIO_4_1:
            // 4:1 - steeper knee
            gain_db = vmulq_f32(excess, vdupq_n_f32(0.75f));
            break;

        case DIST_RATIO_6_1:
            gain_db = vmulq_f32(excess, vdupq_n_f32(0.833f));
            break;

        case DIST_RATIO_10_1:
            // "Opto" mode - special circuit emulating optical compressors [citation:4]
            // Uses separate detection circuitry
            gain_db = vmulq_f32(excess, vdupq_n_f32(0.9f));
            break;

        case DIST_RATIO_20_1:
            // Hard limiting
            gain_db = vmulq_f32(excess, vdupq_n_f32(0.95f));
            break;

        case DIST_RATIO_NUKE:
            // True brick-wall (∞:1): apply exactly the excess as gain reduction,
            // so output is clamped to threshold regardless of how far above it.
            // (Fixed 40dB was too aggressive for small overshoots and too weak for large ones.)
            gain_db = excess;
            break;
    }

    return vnegq_f32(gain_db);  // Return negative dB for gain reduction
}
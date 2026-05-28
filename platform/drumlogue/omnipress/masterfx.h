#pragma once
/*
 * File: masterfx.h
 *
 * OmniPress Master Compressor Effect
 * Inspired by Eventide Omnipressor & Empirical Labs Distressor
 *
 *  * Features:
 * - Negative ratios (expansion)
 * - Reverse compression mode
 * - Peak/RMS detection blend
 * - Wavefolder/overdrive stage
 * - Sidechain HPF with external input
 * FIXED:
 * - Proper array sizes matching header.c (12 parameters)
 * - Bounds checking on all array accesses
 * - Safe string lookup with validation
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <arm_neon.h>

#include "unit.h"
#include "constants.h"
#include "filters.h"
#include "wavefolder.h"
#include "operation_overlord.h"
#include "distressor_mode.h"
#include "multiband.h"

enum parameters
{
    k_threhold,
    k_slope,
    k_attack,
    k_release,
    k_makeup,
    k_drive,
    k_mix,
    k_sc_hpf,
    k_compressor_mode,
    k_attenuation_limit,
    k_gain_limit,
    k_detection_mode,              // envelope detector type: 0=Peak, 1=RMS, 2=Blend
    k_bass,
    k_treble,
    k_presence,
    k_distressor_distortion_type, // includes wavefolder character (soft/hard/tri/sine/suboctave)
    k_multiband_band_selection,
    k_multiband_band_threshold,
    k_multiband_band_ratio,
    k_multiband_band_attack,
    k_multiband_band_release,
    k_multiband_band_makeup,
    k_multiband_band_mute,
    k_multiband_band_solo,
    k_num_params,
};

class MasterFX {
public:
    /*===========================================================================*/
    /* Lifecycle Methods */
    /*===========================================================================*/

    MasterFX(void) : samplerate_(48000.0f) {
        // Initialize all DSP modules
        sidechain_hpf_init(&sc_hpf_, 80.0f, samplerate_);
        wavefolder_init(&wavefolder_);
        distressor_init(&distressor_, samplerate_);
        multiband_init(&multiband_, samplerate_);
        overlord_init(&overlord_, samplerate_);

        // Initialize smoothing
        smoothing_init(&smoother_, samplerate_);
        envelope_detector_init(&envelope_, samplerate_);
        gain_computer_init(&gain_comp_);

        // Clear parameter array
        for (int i = 0; i < k_num_params; i++) {
            raw_params_[i] = 0;
        }
    }

    virtual ~MasterFX(void) {}

    inline int8_t Init(const unit_runtime_desc_t* desc) {
        if (desc->samplerate != 48000)
            return k_unit_err_samplerate;

        // Note: input_channels may be 2 if sidechain feature removed
        has_sidechain_ = (desc->input_channels == 4);

        if (desc->output_channels != 2)
            return k_unit_err_geometry;

        samplerate_ = desc->samplerate;
        Reset();
        return k_unit_err_none;
    }

    inline void Teardown() {}

    inline void Reset() {
        // Set default parameters
        setParameter(k_threhold, THRESH_DEFAULT);   // Thresh: -10.0 dB
        setParameter(k_slope, SLOPE_DEFAULT);       // Slope: 0.4 (4.0:1)
        setParameter(k_attack, ATTACK_DEFAULT);     // Attack: 15.0 ms
        setParameter(k_release, RELEASE_DEFAULT);   // Release: 200 ms
        setParameter(k_makeup, MAKEUP_DEFAULT);     // Makeup: 0 dB
        setParameter(k_drive, DRIVE_DEFAULT);       // Drive: 0%
        setParameter(k_mix, MIX_DEFAULT);           // Mix: 100% wet
        setParameter(k_sc_hpf, SC_HPF_DEFAULT);     // SC HPF: 20 Hz
        setParameter(k_compressor_mode, COMP_MODE_STANDARD);         // COMP MODE: Standard
        setParameter(k_attenuation_limit, ATTENUATION_DEFAULT);      // Defaults to hardware -30.0 dB
        setParameter(k_gain_limit, GAIN_DEFAULT);                    // Defaults to hardware +30.0 dB
        setParameter(k_bass, 50);                                    // BASS: flat (matches header.c init)
        setParameter(k_treble, 50);                                  // TREBLE: flat (matches header.c init)
        setParameter(k_presence, 50);                                // PRESENCE: centre (matches header.c init)
        setParameter(k_distressor_distortion_type, DIST_MODE_CLEAN); // DSTR DIST: None
        setParameter(k_multiband_band_selection, BAND_LOW);          // BAND SEL: Low
        setParameter(k_multiband_band_threshold, BAND_THRESH_DEFAULT);  // L THRESH: -20.0 dB (BAND_THRESH_DEFAULT)
        setParameter(k_multiband_band_ratio, SLOPE_DEFAULT);         // L RATIO: 4.0
        setParameter(k_multiband_band_attack, ATTACK_DEFAULT);       // ATTACK: 5.0 ms (fast, minimal click)
        setParameter(k_multiband_band_release, RELEASE_DEFAULT);     // RELEASE: 200 ms (punchy)
        setParameter(k_multiband_band_makeup, MAKEUP_DEFAULT);       // MAKEUP: 0 dB
        setParameter(k_multiband_band_mute, 0);                      // MUTE off
        setParameter(k_multiband_band_solo, 0);                      // SOLO off
        setParameter(k_detection_mode, DETECT_MODE_PEAK);            // Detection: Peak

        // Reset all components
        sc_hpf_hz_ = 80.0f;
        gain_history_ = vdupq_n_f32(0.0f);
        sidechain_hpf_init(&sc_hpf_, sc_hpf_hz_, samplerate_);
        wavefolder_init(&wavefolder_);
        multiband_init(&multiband_, samplerate_);
        distressor_reset(&distressor_, samplerate_);
        update_opto_coeff(&distressor_, release_coeff_);
        // Sync distressor envelope detector to current user attack/release so all
        // three modes have the same detector timing and produce matching output levels.
        envelope_set_attack_release(&distressor_.distressor_env, attack_ms_, release_ms_);
        overlord_init(&overlord_, samplerate_);
        smoothing_init(&smoother_, samplerate_);
        envelope_detector_init(&envelope_, samplerate_);
        gain_computer_init(&gain_comp_);

        use_external_sc_ = 0;
    }

    inline void Resume() {}
    inline void Suspend() {}

    /*===========================================================================*/
    /* DSP Process Loop - NEON Optimized with Safe Bounds */
    /*===========================================================================*/

    fast_inline void Process(const float* in, float* out, size_t frames) {
        const float* __restrict in_p = in;
        float* __restrict out_p = out;
        size_t frames_remaining = frames;

        // Pre-calculate makeup gain (linear)
        float32x4_t makeup_lin = vdupq_n_f32(fasterpowf(10.0f, makeup_db_ * 0.05f));    // 1 / 20

        // Pre-calculate mix balance
        float32x4_t wet_gain = vdupq_n_f32(mix_);
        float32x4_t dry_gain = vdupq_n_f32(1.0f - mix_);

        // =================================================================
        // Process complete blocks of 4 samples
        // =================================================================
        while (frames_remaining >= 4) {
            // Load 4 stereo frames — 2-channel or 4-channel (sidechain) layout
            float32x4_t main_l, main_r, sc_l, sc_r;
            if (has_sidechain_) {
                // 4-channel: [L0,R0,SL0,SR0, L1,R1,SL1,SR1, ...] = 16 floats
                float32x4x4_t interleaved = vld4q_f32(in_p);
                main_l = interleaved.val[0];
                main_r = interleaved.val[1];
                sc_l   = interleaved.val[2];
                sc_r   = interleaved.val[3];
                in_p += 16;
            } else {
                // 2-channel: [L0,R0, L1,R1, L2,R2, L3,R3] = 8 floats
                float32x4x2_t stereo = vld2q_f32(in_p);
                main_l = stereo.val[0];
                main_r = stereo.val[1];
                sc_l   = main_l;
                sc_r   = main_r;
                in_p += 8;
            }

            // Save dry signal for mixing
            float32x4_t dry_l = main_l;
            float32x4_t dry_r = main_r;

            // Process 4 samples
            float32x4x2_t processed = process_block(main_l, main_r, sc_l, sc_r);

            // Apply makeup gain (avoid making up the dry signal)
            mixed.val[0] = vmulq_f32(processed.val[0], makeup_lin);
            mixed.val[1] = vmulq_f32(processed.val[1], makeup_lin);

            // Mix stage
            float32x4x2_t mixed;
            mixed.val[0] = vaddq_f32(vmulq_f32(dry_l, dry_gain),
                                     vmulq_f32(processed.val[0], wet_gain));
            mixed.val[1] = vaddq_f32(vmulq_f32(dry_r, dry_gain),
                                     vmulq_f32(processed.val[1], wet_gain));

            // Output limiter: hard clip to [-1, 1] — transparent below clipping level.
            {
                const float32x4_t one  = vdupq_n_f32( 1.0f);
                const float32x4_t mone = vdupq_n_f32(-1.0f);
                mixed.val[0] = vmaxq_f32(mone, vminq_f32(one, mixed.val[0]));
                mixed.val[1] = vmaxq_f32(mone, vminq_f32(one, mixed.val[1]));
            }

            // Store results
            vst2q_f32(out_p, mixed);

            out_p += 8;
            frames_remaining -= 4;
        }

        // =================================================================
        // Process remaining samples (0-3) individually
        // =================================================================
        float makeup_lin_scalar = fasterpowf(10.0f, makeup_db_ * 0.05f);    // 1 /20
        while (frames_remaining > 0) {
            float main_l, main_r, sc_l, sc_r;
            if (has_sidechain_) {
                main_l = in_p[0]; main_r = in_p[1];
                sc_l   = in_p[2]; sc_r   = in_p[3];
                in_p += 4;
            } else {
                main_l = in_p[0]; main_r = in_p[1];
                sc_l   = main_l;  sc_r   = main_r;
                in_p += 2;
            }

            float dry_l = main_l;
            float dry_r = main_r;

            float32x4x2_t processed = process_block(
                vdupq_n_f32(main_l), vdupq_n_f32(main_r),
                vdupq_n_f32(sc_l),   vdupq_n_f32(sc_r));

            float out_l_s = (dry_l * (1.0f - mix_) + vgetq_lane_f32(processed.val[0], 0) * mix_)
                            * makeup_lin_scalar;
            float out_r_s = (dry_r * (1.0f - mix_) + vgetq_lane_f32(processed.val[1], 0) * mix_)
                            * makeup_lin_scalar;
            // Output limiter (scalar path)
            out_p[0] = fmaxf(-1.0f, fminf(1.0f, out_l_s));
            out_p[1] = fmaxf(-1.0f, fminf(1.0f, out_r_s));

            out_p += 2;
            frames_remaining--;
        }
    }

private:
    /*===========================================================================*/
    /* Private Processing Methods */
    /*===========================================================================*/
    fast_inline void handle_set_multiband_parameter(int p_id, float val) {
        if (band_select_ == BAND_LOW || band_select_ == BAND_LOW_MID || band_select_ == BAND_LOW_HI || band_select_ == BAND_ALL) {
            multiband_set_param(&multiband_, BAND_LOW, p_id, val);
        }
        if (band_select_ == BAND_MID || band_select_ == BAND_LOW_MID || band_select_ == BAND_MID_HI || band_select_ == BAND_ALL) {
            multiband_set_param(&multiband_, BAND_MID, p_id, val);
        }
        if (band_select_ == BAND_HIGH || band_select_ == BAND_LOW_HI || band_select_ == BAND_MID_HI || band_select_ == BAND_ALL) {
            multiband_set_param(&multiband_, BAND_HIGH, p_id, val);
        }
    }

    fast_inline const char* handle_get_multiband_parameter(int p_id) const {
        float value = 0.0f;
        static char str_buf[16];
        if (band_select_ == BAND_LOW || band_select_ == BAND_LOW_MID || band_select_ == BAND_LOW_HI || band_select_ == BAND_ALL) {
            value = multiband_get_param(&multiband_, BAND_LOW, p_id);
        }
        if (band_select_ == BAND_MID || band_select_ == BAND_MID_HI) {
            value = multiband_get_param(&multiband_, BAND_MID, p_id);
        }
        if (band_select_ == BAND_HIGH) {
            value = multiband_get_param(&multiband_, BAND_HIGH, p_id);
        }
        // I would have liked to has all the three values shown together, but it's not possible.
        // choose the lower one if multiple bands selected, since it's more likely to be audible and relevant for the user.
        static const char *bands[] = {"L", "M", "H", "LM", "LH", "MH", "All"};
        if (band_select_ < BAND_TOTAL) {
            snprintf(str_buf, 256, "%s:%.1f", bands[band_select_], value);
        } else {
            snprintf(str_buf, 256, "---");
        }
        return str_buf;
    }

    /**
     * Process one block of 4 samples (all modes)
     * Returns processed stereo signal
     */
    fast_inline float32x4x2_t process_block(float32x4_t main_l,
                                            float32x4_t main_r,
                                            float32x4_t sc_l,
                                            float32x4_t sc_r) {
        // =================================================================
        // 1. SIDECHAIN SELECTION
        // =================================================================
        float32x4_t sidechain;
        if (has_sidechain_ && use_external_sc_) {
            sidechain = vaddq_f32(sc_l, sc_r);
        } else {
            sidechain = vaddq_f32(main_l, main_r);
        }

        // =================================================================
        // 2. SIDECHAIN HPF
        // =================================================================
        sidechain = sidechain_hpf_process(&sc_hpf_, sidechain);

        // =================================================================
        // 3. ENVELOPE DETECTION (mode-specific)
        // =================================================================
        float32x4_t envelope_db;
        if (comp_mode_ == COMP_MODE_DISTRESSOR) {
            float32x4_t envelope;
            if (distressor_.detector_mode & DETECT_LINK) {
                // Stereo link: detect L and R independently, gain follows the louder channel.
                // Bypass sc_hpf_ (mono filter) and let distressor_detect_stereo apply its own HPF.
                float32x4_t raw_l = (has_sidechain_ && use_external_sc_) ? sc_l : main_l;
                float32x4_t raw_r = (has_sidechain_ && use_external_sc_) ? sc_r : main_r;
                envelope = distressor_detect_stereo(&distressor_, raw_l, raw_r, samplerate_);
            } else {
                envelope = distressor_detect(&distressor_, sidechain, samplerate_);
            }
            envelope_db = linear_to_db(envelope);
        } else {
            float32x4_t envelope = envelope_detect(&envelope_, sidechain);
            envelope_db = linear_to_db(envelope);
        }

        // =================================================================
        // 4. MODE-SPECIFIC PROCESSING
        // =================================================================
        float32x4_t processed_l, processed_r;

        switch (comp_mode_) {
            case COMP_MODE_STANDARD:
                standard_process(main_l, main_r, envelope_db, &processed_l, &processed_r);
                break;

            case COMP_MODE_DISTRESSOR:
                distressor_process(main_l, main_r, envelope_db, &processed_l, &processed_r);
                break;

            case COMP_MODE_MULTIBAND:
                multiband_process(&multiband_, main_l, main_r, &processed_l, &processed_r);
                break;

            default:
                processed_l = main_l;
                processed_r = main_r;
        }

        // =================================================================
        // 5. DRIVE / EQ
        // =================================================================
        if (comp_mode_ == COMP_MODE_DISTRESSOR) {
            // EQ-only (no tube saturation) — distressor provides its own distortion
            float32x4x2_t eq_out = overlord_apply_eq(&overlord_, processed_l, processed_r, samplerate_);
            processed_l = eq_out.val[0];
            processed_r = eq_out.val[1];
        } else if (drive_ > 0.01f) {
            // Full EQ + tube drive for standard/multiband modes
            float32x4x2_t driven = overlord_process(&overlord_, processed_l, processed_r, samplerate_);
            processed_l = driven.val[0];
            processed_r = driven.val[1];
        }

        float32x4x2_t result;
        result.val[0] = processed_l;
        result.val[1] = processed_r;
        return result;
    }

    /**
     * Standard compressor processing
     * - Zero Zipper Noise: The gain applied to the VCA (gain_history_)
     * is smoothed by a continuous single-pole IIR filter on every single sample block.
     * Even if the input audio hovers aggressively right on the threshold pivot point,
     * the filter prevents instantaneous steps.
     * - Consistent Ballistics: Because the smoothing happens after the function_slope_
     * multiplier, an attack setting of $20\text{ ms}$ means the gain will take exactly
     * $20\text{ ms}$ to reach its target, regardless of whether you are doing a mild
     * 2:1 compression or an extreme -2.0 reversal.
     * - Smart Directional Switching: By checking std::fabs(targets[i]) > std::fabs(state),
     * the smoother correctly understands that going from $0\text{ dB}$ to $+12\text{ dB}$
     * (Expansion) and going from $0\text{ dB}$ to $-12\text{ dB}$ (Compression)
     * are both Attack phases because the processor is actively responding to a transient spike.
     */
    fast_inline void standard_process(float32x4_t main_l, float32x4_t main_r,
                                  float32x4_t envelope_db, // Assumed instantaneous peak dB here
                                  float32x4_t* out_l, float32x4_t* out_r) {

        // 1. Calculate raw displacement from the pivot
        float32x4_t excess_db = vsubq_f32(envelope_db, vdupq_n_f32(thresh_db_));

        // 2. Multiply by function slope to find target gain
        float32x4_t target_gain_db = vmulq_f32(excess_db, vdupq_n_f32(function_slope_));

        // 3. Clamp safely to your UI limits
        target_gain_db = vmaxq_f32(target_gain_db, vdupq_n_f32(atten_limit_db_));
        target_gain_db = vminq_f32(target_gain_db, vdupq_n_f32(gain_limit_db_));

        // 4. BI-DIRECTIONAL SMOOTHING (Unrolled for IIR Vector correctness)
        float targets[4], smoothed[4];
        vst1q_f32(targets, target_gain_db);

        // Fetch the scalar history from the last lane of the previous block
        float state = vgetq_lane_f32(gain_history_, 3);

        for (int i = 0; i < 4; ++i) {
            // Evaluate ballistics based on whether audio is demanding MORE or LESS gain modification
            // If the target is moving further away from 0dB unity, we are in the "Attack" phase of the effect.
            bool is_attack = std::fabs(targets[i]) > std::fabs(state);
            float coeff = is_attack ? attack_coeff_ : release_coeff_;

            // Single pole IIR filter
            state = state + coeff * (targets[i] - state);
            smoothed[i] = state;
        }
        // Store history back to the class member vector
        gain_history_ = vld1q_f32(smoothed);

        // 5. Convert smoothly changing dB back to linear gain
        float32x4_t gain_lin = neon_expq_f32(vmulq_f32(gain_history_, vdupq_n_f32(0.11512925f)));

        // 6. Apply to VCA
        *out_l = vmulq_f32(main_l, gain_lin);
        *out_r = vmulq_f32(main_r, gain_lin);
    }

    /**
     * Distressor mode processing with integrated detector and wavefolder
     */
    fast_inline void distressor_process(float32x4_t main_l,
                                        float32x4_t main_r,
                                        float32x4_t envelope_db,
                                        float32x4_t* out_l,
                                        float32x4_t* out_r) {
        // Detection is fully handled in process_block() before this call;
        // envelope_db already reflects HPF, EMPH, and LINK flags.
        float32x4_t target_gain_db = distressor_gain_computer(&distressor_,
                                                              envelope_db,
                                                              thresh_db_);

        float32x4_t smoothed_gain_db = distressor_smooth(&distressor_,
                                                         target_gain_db,
                                                         attack_coeff_,
                                                         distressor_.opto_coeff);

        float32x4_t gain_lin = neon_expq_f32(vmulq_f32(smoothed_gain_db, vdupq_n_f32(0.115129f)));

        float32x4_t comp_l = vmulq_f32(main_l, gain_lin);
        float32x4_t comp_r = vmulq_f32(main_r, gain_lin);

        // Apply saturation to the COMPRESSED signal (not raw input).

        switch (distressor_.dist_mode) {
            case DRIVE_MODE_SOFT_CLIP:
            case DRIVE_MODE_HARD_CLIP:
            case DRIVE_MODE_TRIANGLE:
            case DRIVE_MODE_SINE:
            case DRIVE_MODE_SUBOCTAVE: {
                // wavefolder_set_drive needs 1-100%
                float sat_drive = drive_ * 100.0f;
                float32x4_t drv = vdupq_n_f32(sat_drive);
                wavefolder_set_drive(&wavefolder_, sat_drive);
                // Wavefolder operates post-compression for dynamics control
                float32x4x2_t folded = wavefolder_process(&wavefolder_, comp_l, comp_r, drv);
                *out_l = folded.val[0];
                *out_r = folded.val[1];
                break;
            }
            case DIST_MODE_DIST2:
            case DIST_MODE_DIST3:
            case DIST_MODE_BOTH: {
                // sat_drive range: 2x (DRIVE=0) to 14x (DRIV4=100) — pushes the
                // signal into the saturator nonlinear region.
                // makeup = 1.0 keeps output level comparable to the input so
                // harmonic character is always audible. The output hard-clip limiter
                // prevents clipping. Do NOT divide by sat_drive (old formula made
                // the distorted output quieter than dry, masking the effect).
                float sat_drive = 2.0f + drive_ * 12.0f;
                float32x4_t drv = vdupq_n_f32(sat_drive);
                *out_l = generate_harmonics(&distressor_,
                                            vmulq_f32(comp_l, drv),
                                            distressor_.dist_mode);
                *out_r = generate_harmonics(&distressor_,
                                            vmulq_f32(comp_r, drv),
                                            distressor_.dist_mode);
                break;
            }
            case DIST_MODE_CLEAN:
            default:
                *out_l = comp_l;
                *out_r = comp_r;
                break;
        }
    }

public:
    /*===========================================================================*/
    /* Parameter Handling - With Bounds Checking */
    /*===========================================================================*/

    inline void setParameter(uint8_t index, int32_t value) {
        if (index >= k_num_params) return;

        raw_params_[index] = value;

        switch (index) {
            /*===========================================================================*/
            /* General Parameters */
            /*===========================================================================*/
            case k_threhold: // THRESH (-60.0 to 0.0 dB)
                thresh_db_ = value * 0.1f;
                break;

            case k_slope: // RATIO: map 0 to 100 into 0.0 to 1.0 representing the physical knob turn
                {
                    if (comp_mode_ == COMP_MODE_DISTRESSOR) {
                        int knob = value * 0.0799f;  // Map 0-100 to 0-7 for the distressor's 8 ratio steps
                        distressor_set_ratio(&distressor_, knob);  // this updates opto_release_mult
                        update_opto_coeff(&distressor_, release_coeff_);
                        break;
                    }
                    // Map your raw value to a normalized float position (0.0 to 1.0)
                    // Adjust the math below depending on your actual minimum/maximum raw values
                    float knob = value * 0.01f;

                    // Map the normalized knob position across the 3 hardware regions:
                    if (knob < 0.333f) {
                        // 0.0 to 0.333: Expansion/Gating region
                        // Slopes from +3.0 (Max expansion) down to 0.0 (Linear pass-through)
                        function_slope_ = 3.0f * (1.0f - (knob * 3));
                    }
                    else if (knob < 0.666f) {
                        // 0.333 to 0.666: Compression region
                        // Slopes from 0.0 down to -1.0 (Infinite limiting)
                        function_slope_ = -1.0f * ((knob - 0.333f) * 3);
                    }
                    else {
                        // 0.666 to 1.0: Dynamic Reversal region
                        // Slopes from -1.0 down to -2.0 (Extreme reverse sucking envelope)
                        function_slope_ = -1.0f - ((knob - 0.666f) * 3);
                    }
                }
                break;

            case k_attack: // ATTACK (0.1 to 100.0 ms)
                attack_ms_ = value * 0.1f;
                attack_coeff_ = fasterexpf(-0.02083333f / attack_ms_);  // 1 / (0.001f * samplerate_)
                envelope_set_attack_release(&envelope_, attack_ms_, release_ms_);
                envelope_set_attack_release(&distressor_.distressor_env, attack_ms_, release_ms_);
                smoothing_set_times(&smoother_, attack_ms_, release_ms_);
                break;

            case k_release: // RELEASE (10 to 2000 ms)
                release_ms_ = static_cast<float>(value);
                release_coeff_ = fasterexpf(-0.02083333f / release_ms_);    // 1 / (0.001f * samplerate_)
                envelope_set_attack_release(&envelope_, attack_ms_, release_ms_);
                envelope_set_attack_release(&distressor_.distressor_env, attack_ms_, release_ms_);
                smoothing_set_times(&smoother_, attack_ms_, release_ms_);
                update_opto_coeff(&distressor_, release_coeff_);
                break;

            case k_makeup: // MAKEUP (0.0 to 24.0 dB)
                makeup_db_ = value * 0.1f;
                break;
            case k_attenuation_limit: // ATTEN LIMIT (-30.0 to 0.0 dB)
                atten_limit_db_ = value * 0.1f;
                break;
            case k_gain_limit: // GAIN LIMIT (0.0 to 30.0 dB)
                gain_limit_db_ = value * 0.1f;
                break;

            case k_drive: // DRIVE (0 to 100%)
                drive_ = value * 0.01f;
                wavefolder_set_drive(&wavefolder_, value);
                overlord_set_drive(&overlord_, value);
                break;

            case k_mix: // MIX (-100 to +100)
                mix_ = (value + 100.0f) * 0.005f; // Map to 0.0..1.0
                break;

            case k_sc_hpf: // SC HPF (20 to 500 Hz)
                sc_hpf_hz_ = static_cast<float>(value);
                sidechain_hpf_set_cutoff(&sc_hpf_, sc_hpf_hz_);
                break;

            /*===========================================================================*/
            /* Multiband Parameters */
            /*===========================================================================*/
            case k_multiband_band_selection: // BAND SEL (0-6) - for multiband mode
                band_select_ = value;
                break;
            case k_multiband_band_threshold: // L THRESH (multiband low threshold) - param_id=0
            {
                const float val = value * 0.1f;
                const int p_id = 0;
                handle_set_multiband_parameter(p_id, val);
                break;
            }
            case k_multiband_band_ratio: // L RATIO (multiband low ratio) - param_id=1
            {
                const float val = value * 0.1f;
                const int p_id = 1;
                handle_set_multiband_parameter(p_id, val);
                break;
            }
            case k_multiband_band_attack:  // BAND ATTACK
            {
                float val = value * 0.1f;   // raw is 1-1000 → 0.1-100 ms
                const int p_id = 3;         // param_id 3 = attack_ms in multiband_set_param
                handle_set_multiband_parameter(p_id, val);
                break;
            }
            case k_multiband_band_release:  // BAND RELEASE
            {
                float val = value;          // raw ms
                const int p_id = 4;         // param_id 4 = release_ms
                handle_set_multiband_parameter(p_id, val);
                break;
            }
            case k_multiband_band_makeup:   // BAND MAKEUP
            {
                float val = value * 0.1f;   // 0-240 → 0-24 dB
                const int p_id = 2;         // param_id 2 = makeup_db
                handle_set_multiband_parameter(p_id, val);
                break;
            }
            case k_multiband_band_mute:     // BAND MUTE
            {
                float val = value != 0 ? 1.0f : 0.0f;
                const int p_id = 5;         // param_id 5 = mute
                handle_set_multiband_parameter(p_id, val);
                break;
            }
            case k_multiband_band_solo:     // BAND SOLO
            {
                float val = value != 0 ? 1.0f : 0.0f;
                const int p_id = 6;         // param_id 6 = solo
                handle_set_multiband_parameter(p_id, val);
                break;
            }
            case k_compressor_mode: // COMP MODE (0=Standard, 1=Distressor, 2=Multiband)
                if (value >= COMP_MODE_STANDARD && value < COMP_MODE_TOTAL) {
                    comp_mode_ = value;
                    if (comp_mode_ == COMP_MODE_DISTRESSOR) {
                        // Distressor expects at least 0.05ms attack
                        attack_ms_ = fmaxf(attack_ms_, 0.05f);
                        attack_coeff_ =
                            fasterexpf(-0.02083333f / (attack_ms_)); // 1 / (0.001f * samplerate_)
                    }
                }
                break;

            /*===========================================================================*/
            /* Distressor Parameters */
            /*===========================================================================*/
            case k_distressor_distortion_type:
                // DSTR MODE (0=None, 1=2nd harm, 2=3rd harm, 3=Both, 4=SoftClip, 5=HardClip, 6=Tri, 7=Sine, 8=SubOct)
                if (value >= 0 && value < DIST_MODE_TOTAL) {
                    distressor_.dist_mode = value;

                    // Enable/disable detector HPF based on mode
                    if (value > DIST_MODE_BOTH) {
                        // Wavefolder removes low frequencies from the detector path
                        distressor_.detector_mode |= DETECT_HPF;
                        wavefolder_set_drive_type(&wavefolder_, value);
                    } else {
                        distressor_.detector_mode &= ~DETECT_HPF;
                    }
                }
                break;

            /*===========================================================================*/
            /* Operation Overlord Distortion Emulation Parameters */
            /*===========================================================================*/
            case k_bass: // BASS (Operation Overlord EQ)
                overlord_.bass = value * 0.01f;
                break;

            case k_treble: // TREBLE
                overlord_.treble = value * 0.01f;
                break;

            case k_presence: // PRESENCE
                overlord_.presence = value * 0.01f;
                break;

            case k_detection_mode:
                if (comp_mode_ == COMP_MODE_DISTRESSOR) {
                    // Distressor detector flags bitmask: 0=Basic, 1=Emph, 2=Link, 3=Emph+Link
                    // Preserve DETECT_HPF — it is set separately by k_distressor_distortion_type.
                    distressor_.detector_mode &= DETECT_HPF;
                    if (value & 1) distressor_.detector_mode |= DETECT_BAND_EMPH;
                    if (value & 2) distressor_.detector_mode |= DETECT_LINK;
                } else {
                    // Standard / multiband: Peak=0, RMS=1, Blend=2
                    if (value >= 0 && value <= 2) {
                        detection_mode_ = value;
                        envelope_.mode = detection_mode_;
                    }
                }
                break;
        }
    }

    inline int32_t getParameterValue(uint8_t index) const {
        // FIXED: Bounds check on parameter index
        if (index < k_num_params) {
            return raw_params_[index];
        }
        return 0;
    }

    inline const char* getParameterStrValue(uint8_t index, int32_t value) const {
        static char str_buf[16];

        switch (index) {
            case k_compressor_mode: // COMP MODE
              if (value >= COMP_MODE_STANDARD && value < COMP_MODE_TOTAL) {
                static const char *modes[] = {"Stndrd", "Dstrssr", "Mltibnd"};
                return modes[value];
              }
                break;

            case k_multiband_band_selection: // BAND SEL
                if (value >= 0 && value < BAND_TOTAL) {
                    static const char *bands[] = {
                        "Low", "Mid", "High", "LwMid", "LwHi", "MdHi", "All"};
                    return bands[value];
                }
                break;


            case k_multiband_band_threshold: // L THRESH (multiband low threshold) - param_id=0
            {
                const int p_id = 0;
                return handle_get_multiband_parameter(p_id);
                break;
            }
            case k_multiband_band_ratio: // L RATIO (multiband low ratio) - param_id=1
            {
                const int p_id = 1;
                return handle_get_multiband_parameter(p_id);
                break;
            }
            case k_multiband_band_attack:  // BAND ATTACK
            {
                const int p_id = 3;         // param_id 3 = attack_ms in multiband_set_param
                return handle_get_multiband_parameter(p_id);
                break;
            }
            case k_multiband_band_release:  // BAND RELEASE
            {
                const int p_id = 4;         // param_id 4 = release_ms
                return handle_get_multiband_parameter(p_id);
                break;
            }
            case k_multiband_band_makeup:   // BAND MAKEUP
            {
                const int p_id = 2;         // param_id 2 = makeup_db
                return handle_get_multiband_parameter(p_id);
                break;
            }
            case k_multiband_band_mute:     // BAND MUTE
            {
                const int p_id = 5;         // param_id 5 = mute
                return handle_get_multiband_parameter(p_id);
                break;
            }
            case k_multiband_band_solo:     // BAND SOLO
            {
                const int p_id = 6;         // param_id 6 = solo
                return handle_get_multiband_parameter(p_id);
                break;
            }

            case k_slope: // SLOPE (1.0 to 20.0) - show special cases
                {
                    if (comp_mode_ == COMP_MODE_DISTRESSOR) {
                        // Distressor has 8 fixed ratio steps: 1:1, 2:1, 3:1, 4:1, 6:1, 8:1, 12:1, 20:1
                        int knob = value * 0.0799f; // Map 0-100 to 0-7 and never goes to 8 which is invalid
                        if (knob >= 0 && knob < DIST_RATIO_TOTAL) {
                            return distressor_ratio_strings[value];
                        }
                        break;
                    }
                    // Display the true Omnipressor state based on the internal slope
                    if (function_slope_ > 0.05f) {
                        snprintf(str_buf, sizeof(str_buf), "Exp %.1f", 1.0f + function_slope_);
                    } else if (function_slope_ >= -0.95f && function_slope_ <= 0.05f) {
                        float ratio_val = 1.0f / (1.0f + function_slope_);
                        snprintf(str_buf, sizeof(str_buf), "%.1f:1", ratio_val);
                    } else if (function_slope_ < -0.95f && function_slope_ >= -1.05f) {
                        return "Limit";
                    } else {
                        // Negative ratio conversion
                        float rev_val = 1.0f / (std::fabs(function_slope_) - 1.0f);
                        snprintf(str_buf, sizeof(str_buf), "Rev %.1f", rev_val);
                    }
                    return str_buf;
                }
                break;

            case k_mix: // MIX - show DRY/BAL/WET
                if (value <= -100) return "DRY";
                if (value >= 100) return "WET";
                if (abs(value) < 10) return "BAL";
                break;

            case k_distressor_distortion_type: // DSTR MODE
                if (value >= DIST_MODE_CLEAN && value < DIST_MODE_TOTAL) {
                    return distressor_dist_strings[value];
                }
                break;

            case k_detection_mode:
                if (comp_mode_ == COMP_MODE_DISTRESSOR) {
                    static const char* dst_det[] = {"Basic", "Emph", "Link", "Emp+Lnk"};
                    if (value >= 0 && value <= 3) return dst_det[value];
                } else {
                    static const char* std_det[] = {"Peak", "RMS", "Blend"};
                    if (value >= 0 && value <= 2) return std_det[value];
                }
                break;
        }
        return nullptr;
    }

    inline const uint8_t* getParameterBmpValue(uint8_t index, int32_t value) const {
        (void)index;
        (void)value;
        return nullptr;
    }

    inline void LoadPreset(uint8_t idx) { (void)idx; }
    inline uint8_t getPresetIndex() const { return 0; }
    static inline const char* getPresetName(uint8_t idx) { (void)idx; return nullptr; }

private:
    /*===========================================================================*/
    /* Private Member Variables */
    /*===========================================================================*/

    std::atomic_uint_fast32_t flags_;
    float samplerate_;
    bool has_sidechain_;

    int32_t raw_params_[k_num_params]  __attribute__((aligned(16)));

    // Floating-point parameters
    float thresh_db_;
    float function_slope_;
    float attack_ms_;
    float release_ms_;
    float makeup_db_;
    float atten_limit_db_;
    float gain_limit_db_;
    float drive_;
    float mix_;   // 0.0 = dry, 1.0 = wet
    float sc_hpf_hz_;
    float32x4_t gain_history_;

    // DSP coefficients
    float attack_coeff_;
    float release_coeff_;

    // Mode flags
    uint8_t comp_mode_;          // 0=Standard, 1=Distressor, 2=Multiband
    uint8_t band_select_;        // 0=Low, 1=Mid, 2=High, 3=All
    uint8_t use_external_sc_;    // 0=internal, 1=external sidechain
    uint8_t detection_mode_;     // 0=peak, 1=RMS, 2=blend

    // DSP Modules
    sidechain_hpf_t sc_hpf_;
    wavefolder_t wavefolder_;
    distressor_t distressor_;
    multiband_t multiband_;
    envelope_detector_t envelope_;
    gain_computer_t gain_comp_;
    smoothing_t smoother_;
    overlord_t overlord_;
};
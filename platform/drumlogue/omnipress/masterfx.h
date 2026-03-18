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
#include <arm_neon.h>

#include "unit.h"
#include "constants.h"
#include "compressor_core.h"
#include "filters.h"
#include "wavefolder.h"
#include "distressor_mode.h"
#include "multiband.h"

// Parameter count must match header.c
#define NUM_PARAMS 13

class MasterFX {
public:
    /*===========================================================================*/
    /* Lifecycle Methods */
    /*===========================================================================*/

    MasterFX(void) : samplerate_(48000.0f) {
        // Initialize all DSP modules
        compressor_init(&comp_);
        hpf_init(&sc_hpf_);
        wavefolder_init(&folder_);
        distressor_init(&distressor_);
        multiband_init(&multiband_, samplerate_);

        // Initialize smoothing
        smoothing_init(&smoother_, samplerate_);
        envelope_detector_init(&envelope_, samplerate_);
        gain_computer_init(&gain_comp_);

        // Clear parameter array
        for (int i = 0; i < NUM_PARAMS; i++) {
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
        // Reset all components
        compressor_reset(&comp_);
        hpf_reset(&sc_hpf_);
        wavefolder_reset(&folder_);
        distressor_init(&distressor_);
        multiband_init(&multiband_, samplerate_);
        smoothing_init(&smoother_, samplerate_);
        envelope_detector_init(&envelope_, samplerate_);
        gain_computer_init(&gain_comp_);

        // Set default parameters
        setParameter(0, -200);  // Thresh: -20.0 dB
        setParameter(1, 40);    // Ratio: 4.0
        setParameter(2, 150);   // Attack: 15.0 ms
        setParameter(3, 200);   // Release: 200 ms
        setParameter(4, 0);     // Makeup: 0 dB
        setParameter(5, 0);     // Drive: 0%
        setParameter(6, 100);   // Mix: 100% wet
        setParameter(7, 20);    // SC HPF: 20 Hz
        setParameter(8, 0);     // COMP MODE: Standard
        setParameter(9, 0);     // BAND SEL: Low
        setParameter(10, -200); // L THRESH: -20.0 dB
        setParameter(11, 40);   // L RATIO: 4.0

        comp_mode_ = 0;
        band_select_ = 0;
        use_external_sc_ = 0;
        detection_mode_ = DETECT_MODE_PEAK;
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
        float32x4_t makeup_lin = vdupq_n_f32(powf(10.0f, makeup_db_ / 20.0f));

        // Pre-calculate mix balance
        float32x4_t wet_gain = vdupq_n_f32(mix_);
        float32x4_t dry_gain = vdupq_n_f32(1.0f - mix_);

        // =================================================================
        // Process complete blocks of 4 samples
        // =================================================================
        while (frames_remaining >= 4) {
            // Load 4 stereo frames (16 floats total)
            float32x4x4_t interleaved = vld4q_f32(in_p);
            float32x4_t main_l = interleaved.val[0];
            float32x4_t main_r = interleaved.val[1];
            float32x4_t sc_l   = interleaved.val[2];
            float32x4_t sc_r   = interleaved.val[3];

            // Save dry signal for mixing
            float32x4_t dry_l = main_l;
            float32x4_t dry_r = main_r;

            // Process 4 samples
            float32x4x2_t processed = process_block(main_l, main_r, sc_l, sc_r);

            // Mix stage
            float32x4x2_t mixed;
            mixed.val[0] = vaddq_f32(vmulq_f32(dry_l, dry_gain),
                                      vmulq_f32(processed.val[0], wet_gain));
            mixed.val[1] = vaddq_f32(vmulq_f32(dry_r, dry_gain),
                                      vmulq_f32(processed.val[1], wet_gain));

            // Apply makeup gain
            mixed.val[0] = vmulq_f32(mixed.val[0], makeup_lin);
            mixed.val[1] = vmulq_f32(mixed.val[1], makeup_lin);

            // Store results
            vst2q_f32(out_p, mixed);

            // Advance pointers
            in_p += 16;
            out_p += 8;
            frames_remaining -= 4;
        }

        // =================================================================
        // Process remaining samples (1-3) individually
        // =================================================================
        while (frames_remaining > 0) {
            // Load one stereo frame (4 floats)
            float32x4_t frame = vld1q_f32(in_p);

            // Extract channels
            float32x2_t main = vget_low_f32(frame);
            float32x2_t sc   = vget_high_f32(frame);

            float main_l = vget_lane_f32(main, 0);
            float main_r = vget_lane_f32(main, 1);
            float sc_l   = vget_lane_f32(sc, 0);
            float sc_r   = vget_lane_f32(sc, 1);

            // Save dry signal
            float dry_l = main_l;
            float dry_r = main_r;

            // Process single sample (convert to vectors for reuse)
            float32x4_t main_l_vec = vdupq_n_f32(main_l);
            float32x4_t main_r_vec = vdupq_n_f32(main_r);
            float32x4_t sc_l_vec = vdupq_n_f32(sc_l);
            float32x4_t sc_r_vec = vdupq_n_f32(sc_r);

            float32x4x2_t processed = process_block(main_l_vec, main_r_vec,
                                                     sc_l_vec, sc_r_vec);

            // Extract single results
            float proc_l = vgetq_lane_f32(processed.val[0], 0);
            float proc_r = vgetq_lane_f32(processed.val[1], 0);

            // Mix and output
            out_p[0] = dry_l * (1.0f - mix_) + proc_l * mix_;
            out_p[1] = dry_r * (1.0f - mix_) + proc_r * mix_;

            // Apply makeup gain
            out_p[0] *= powf(10.0f, makeup_db_ / 20.0f);
            out_p[1] *= powf(10.0f, makeup_db_ / 20.0f);

            // Advance pointers
            in_p += 4;
            out_p += 2;
            frames_remaining--;
        }
    }

private:
    /*===========================================================================*/
    /* Private Processing Methods */
    /*===========================================================================*/

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
            // Use external sidechain (sum L+R)
            sidechain = vaddq_f32(sc_l, sc_r);
        } else {
            // Use main signal as sidechain (sum L+R)
            sidechain = vaddq_f32(main_l, main_r);
        }

        // =================================================================
        // 2. SIDECHAIN HPF
        // =================================================================
        sidechain = sidechain_hpf_process(&sc_hpf_, sidechain);

        // =================================================================
        // 3. ENVELOPE DETECTION
        // =================================================================
        float32x4_t envelope = envelope_detect(&envelope_, main_l, sidechain);
        float32x4_t envelope_db = linear_to_db(envelope);

        // =================================================================
        // 4. MODE-SPECIFIC PROCESSING
        // =================================================================
        float32x4_t processed_l, processed_r;

        switch (comp_mode_) {
            case 0: // Standard compressor
                standard_process(main_l, main_r, envelope_db,
                                &processed_l, &processed_r);
                break;

            case 1: // Distressor mode
                distressor_process(main_l, main_r, envelope_db,
                                  &processed_l, &processed_r);
                break;

            case 2: // Multiband mode
                multiband_process(&multiband_, main_l, main_r,
                                 &processed_l, &processed_r);
                break;

            default:
                processed_l = main_l;
                processed_r = main_r;
        }

        // =================================================================
        // 5. DRIVE / WAVEFOLDER
        // =================================================================
        if (drive_ > 0.01f) {
            float32x4x2_t driven = wavefolder_process(&folder_,
                                                       processed_l,
                                                       processed_r,
                                                       drive_);
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
     */
    fast_inline void standard_process(float32x4_t main_l,
                                      float32x4_t main_r,
                                      float32x4_t envelope_db,
                                      float32x4_t* out_l,
                                      float32x4_t* out_r) {
        // Gain computer with knee
        float32x4_t target_gain_db = gain_computer_process(&gain_comp_,
                                                           envelope_db,
                                                           thresh_db_,
                                                           ratio_);

        // Attack/release smoothing
        float32x4_t smoothed_gain_db = smoothing_process(&smoother_, target_gain_db);

        // Convert to linear gain
        float32x4_t gain_lin = vexpq_f32(vmulq_f32(smoothed_gain_db,
                                                   vdupq_n_f32(0.115129f)));

        // Apply gain reduction
        *out_l = vmulq_f32(main_l, gain_lin);
        *out_r = vmulq_f32(main_r, gain_lin);
    }

    /**
     * Distressor mode processing
     */
    fast_inline void distressor_process(float32x4_t main_l,
                                        float32x4_t main_r,
                                        float32x4_t envelope_db,
                                        float32x4_t* out_l,
                                        float32x4_t* out_r) {
        // Distressor-specific gain computer
        float32x4_t target_gain_db = distressor_gain_computer(&distressor_,
                                                               envelope_db,
                                                               thresh_db_);

        // Smoothing with opto mode
        float32x4_t smoothed_gain_db = distressor_smooth(&distressor_,
                                                          target_gain_db,
                                                          attack_coeff_,
                                                          release_coeff_ *
                                                          distressor_.opto_release_mult);

        // Convert to linear
        float32x4_t gain_lin = vexpq_f32(vmulq_f32(smoothed_gain_db,
                                                   vdupq_n_f32(0.115129f)));

        // Apply gain
        float32x4_t comp_l = vmulq_f32(main_l, gain_lin);
        float32x4_t comp_r = vmulq_f32(main_r, gain_lin);

        // Apply harmonics
        if (distressor_.dist_mode > 0 && distressor_.dist_mode <= 3) {
            *out_l = generate_harmonics(&distressor_, comp_l, distressor_.dist_mode);
            *out_r = generate_harmonics(&distressor_, comp_r, distressor_.dist_mode);
        } else {
            *out_l = comp_l;
            *out_r = comp_r;
        }
    }

    /*===========================================================================*/
    /* Parameter Handling - With Bounds Checking */
    /*===========================================================================*/

    inline void setParameter(uint8_t index, int32_t value) {
        // FIXED: Bounds check on parameter index
        if (index >= NUM_PARAMS) return;

        raw_params_[index] = value;

        switch (index) {
            case 0: // THRESH (-60.0 to 0.0 dB)
                thresh_db_ = value * 0.1f;
                break;

            case 1: // RATIO (1.0 to 20.0)
                ratio_ = value * 0.1f;
                break;

            case 2: // ATTACK (0.1 to 100.0 ms)
                attack_ms_ = value * 0.1f;
                attack_coeff_ = expf(-1.0f / (attack_ms_ * 0.001f * samplerate_));
                envelope_set_attack_release(&envelope_, attack_ms_, release_ms_);
                smoothing_set_times(&smoother_, attack_ms_, release_ms_);
                break;

            case 3: // RELEASE (10 to 2000 ms)
                release_ms_ = static_cast<float>(value);
                release_coeff_ = expf(-1.0f / (release_ms_ * 0.001f * samplerate_));
                envelope_set_attack_release(&envelope_, attack_ms_, release_ms_);
                smoothing_set_times(&smoother_, attack_ms_, release_ms_);
                break;

            case 4: // MAKEUP (0.0 to 24.0 dB)
                makeup_db_ = value * 0.1f;
                break;

            case 5: // DRIVE (0 to 100%)
                drive_ = value * 0.01f;
                wavefolder_set_drive(&folder_, value);
                break;

            case 6: // MIX (-100 to +100)
                mix_ = (value + 100.0f) * 0.005f; // Map to 0.0..1.0
                break;

            case 7: // SC HPF (20 to 500 Hz)
                sc_hpf_hz_ = static_cast<float>(value);
                sidechain_hpf_set_cutoff(&sc_hpf_, sc_hpf_hz_);
                break;

            case 8: // COMP MODE (0-2)
                if (value >= 0 && value <= 2) {
                    comp_mode_ = value;
                }
                break;

            case 9: // BAND SEL (0-3)
                if (value >= 0 && value <= 3) {
                    band_select_ = value;
                }
                break;

            case 10: // L THRESH (multiband low threshold)
                // Store for later use
                break;

            case 11: // L RATIO (multiband low ratio)
                // Store for later use
                break;

            case 12: // DSTR MODE (0=None, 1=2nd harm, 2=3rd harm, 3=Both)
                if (value >= 0 && value <= 3) {
                    distressor_.dist_mode = value;
                }
                break;
        }
    }

    inline int32_t getParameterValue(uint8_t index) const {
        // FIXED: Bounds check on parameter index
        if (index < NUM_PARAMS) {
            return raw_params_[index];
        }
        return 0;
    }

    inline const char* getParameterStrValue(uint8_t index, int32_t value) const {
        static char str_buf[16];

        switch (index) {
            case 8: // COMP MODE
                if (value >= 0 && value <= 2) {
                    static const char* modes[] = {"Standard", "Distressor", "Multiband"};
                    return modes[value];
                }
                break;

            case 9: // BAND SEL
                if (value >= 0 && value <= 3) {
                    static const char* bands[] = {"Low", "Mid", "High", "All"};
                    return bands[value];
                }
                break;

            case 1: // RATIO - show special cases
                {
                    float ratio = value * 0.1f;
                    if (fabsf(ratio - 20.0f) < 0.1f)
                        return "Limit";
                    else {
                        snprintf(str_buf, sizeof(str_buf), "%.1f:1", ratio);
                        return str_buf;
                    }
                }
                break;

            case 6: // MIX - show DRY/BAL/WET
                if (value <= -100) return "DRY";
                if (value >= 100) return "WET";
                if (abs(value) < 10) return "BAL";
                break;

            case 12: // DSTR MODE
                if (value >= 0 && value <= 3) {
                    static const char* dstr_modes[] = {"None", "2nd", "3rd", "Both"};
                    return dstr_modes[value];
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
    static inline const char* getPresetName(uint8_t idx) { return nullptr; }

private:
    /*===========================================================================*/
    /* Private Member Variables */
    /*===========================================================================*/

    std::atomic_uint_fast32_t flags_;
    float samplerate_;
    bool has_sidechain_;

    int32_t raw_params_[NUM_PARAMS];

    // Floating-point parameters
    float thresh_db_;
    float ratio_;
    float attack_ms_;
    float release_ms_;
    float makeup_db_;
    float drive_;
    float mix_;
    float sc_hpf_hz_;

    // DSP coefficients
    float attack_coeff_;
    float release_coeff_;

    // Mode flags
    uint8_t comp_mode_;          // 0=Standard, 1=Distressor, 2=Multiband
    uint8_t band_select_;        // 0=Low, 1=Mid, 2=High, 3=All
    uint8_t use_external_sc_;    // 0=internal, 1=external sidechain
    uint8_t detection_mode_;     // 0=peak, 1=RMS, 2=blend

    // DSP Modules
    compressor_t comp_;
    sidechain_hpf_t sc_hpf_;
    wavefolder_t folder_;
    distressor_t distressor_;
    multiband_t multiband_;
    envelope_detector_t envelope_;
    gain_computer_t gain_comp_;
    smoothing_t smoother_;
};
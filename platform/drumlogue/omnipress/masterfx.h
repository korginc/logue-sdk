#pragma once
/*
 * File: masterfx.h
 *
 * OmniPress Master Compressor Effect
 * Inspired by Eventide Omnipressor & Empirical Labs Distressor
 *
 * Features:
 * - Negative ratios (expansion)
 * - Reverse compression mode
 * - Peak/RMS detection blend
 * - Wavefolder/overdrive stage
 * - Sidechain HPF with external input
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <arm_neon.h>

#include "unit.h"
#include "compressor_core.h"
#include "filters.h"
#include "wavefolder.h"
#include "distressor_mode.h"
#include "multiband.h"

class MasterFX {
public:
    /*===========================================================================*/
    /* Lifecycle Methods */
    /*===========================================================================*/

    MasterFX(void) : samplerate_(48000.0f) {
        // Initialize all DSP modules
        compressor_init(&comp_);
        hpf_init(&hpf_);
        wavefolder_init(&wavefolder_);
        distressor_init(&distressor_);
        multiband_init(&multiband_, samplerate_);
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
        compressor_reset(&comp_);
        sidechain_hpf_init(&sc_hpf_, sc_hpf_hz_, samplerate_);
        envelope_detector_init(&envelope_, samplerate_);
        gain_computer_init(&gain_comp_);
        smoothing_init(&smoother_, samplerate_);
        wavefolder_init(&wavefolder_);
        distressor_reset(&distressor_);
        multiband_reset(&multiband_, samplerate_);


        // Set default parameters
        setParameter(0, -200);  // Thresh: -20.0 dB
        setParameter(1, 40);    // Ratio: 4.0
        setParameter(2, 150);   // Attack: 15.0 ms
        setParameter(3, 200);   // Release: 200 ms
        setParameter(4, 0);     // Makeup: 0 dB
        setParameter(5, 0);     // Drive: 0%
        setParameter(6, 100);   // Mix: 100% wet
        setParameter(7, 20);    // SC HPF: 20 Hz
        setParameter(12, 0);     // Distressor: off

        distressor_mode_ = 0;
        comp_mode_ = 0;
        band_select_ = 0;
    }

    inline void Resume() {}
    inline void Suspend() {}

    /*===========================================================================*/
    /* DSP Process Loop - NEON Optimized */
    /*===========================================================================*/

    fast_inline void Process(const float* in, float* out, size_t frames) {
    const float* __restrict in_p = in;
    float* __restrict out_p = out;
    const float* out_e = out_p + (frames << 1);

    // Pre-calculate makeup gain (linear)
    float32x4_t makeup_lin = vdupq_n_f32(powf(10.0f, makeup_db_ / 20.0f));

    // Pre-calculate mix balance
    float32x4_t wet_gain = vdupq_n_f32(mix_);
    float32x4_t dry_gain = vdupq_n_f32(1.0f - mix_);

    // Process 4 samples at a time for NEON efficiency
    for (; out_p != out_e; in_p += 16, out_p += 8) {
        // =================================================================
        // 1. INPUT LOADING (4 samples)
        // =================================================================
        // Input format: [L, R, SC L, SC R] interleaved
        float32x4x4_t interleaved = vld4q_f32(in_p);
        float32x4_t main_l = interleaved.val[0];  // L0, L1, L2, L3
        float32x4_t main_r = interleaved.val[1];  // R0, R1, R2, R3
        float32x4_t sc_l   = interleaved.val[2];  // SC L0, SC L1, SC L2, SC L3
        float32x4_t sc_r   = interleaved.val[3];  // SC R0, SC R1, SC R2, SC R3

        // Save dry signal for later mixing
        float32x4_t dry_l = main_l;
        float32x4_t dry_r = main_r;

        // =================================================================
        // 2. SIDECHAIN PROCESSING (Common to all modes)
        // =================================================================
        // Select sidechain source (internal or external)
        float32x4_t sidechain;
        if (has_sidechain_ && use_external_sc_) {
            // Use external sidechain (sum L+R)
            sidechain = vaddq_f32(sc_l, sc_r);
        } else {
            // Use main signal as sidechain (sum L+R)
            sidechain = vaddq_f32(main_l, main_r);
        }

        // Apply sidechain HPF
        sidechain = sidechain_hpf_process(&sc_hpf_, sidechain);

        // =================================================================
        // 3. ENVELOPE DETECTION (Common to all modes)
        // =================================================================
        float32x4_t envelope = envelope_detect(&envelope_, main_l, sidechain);
        float32x4_t envelope_db = linear_to_db(envelope);

        // =================================================================
        // 4. MODE-SPECIFIC PROCESSING
        // =================================================================
        float32x4_t processed_l, processed_r;

        switch (comp_mode_) {
            // -----------------------------------------------------------------
            // MODE 0: STANDARD COMPRESSOR
            // -----------------------------------------------------------------
            case 0: {
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
                float32x4_t comp_l = vmulq_f32(main_l, gain_lin);
                float32x4_t comp_r = vmulq_f32(main_r, gain_lin);

                // Optional drive/wavefolder stage
                if (drive_ > 0.01f) {
                    float32x4x2_t driven = wavefolder_process(&wavefolder_, comp_l, comp_r);
                    processed_l = driven.val[0];
                    processed_r = driven.val[1];
                } else {
                    processed_l = comp_l;
                    processed_r = comp_r;
                }
                break;
            }

            // -----------------------------------------------------------------
            // MODE 1: DISTRESSOR MODE
            // -----------------------------------------------------------------
            case 1: {
                // Use distressor-specific gain computer (8 ratios, special curves)
                float32x4_t target_gain_db = distressor_gain_computer(&distressor_,
                                                                       envelope_db,
                                                                       thresh_db_);

                // Distressor-specific attack/release (opto mode has longer release)
                float32x4_t smoothed_gain_db = distressor_smooth(&distressor_,
                                                                  target_gain_db,
                                                                  attack_coeff_,
                                                                  release_coeff_ * distressor_.opto_release_mult);

                // Convert to linear gain
                float32x4_t gain_lin = vexpq_f32(vmulq_f32(smoothed_gain_db,
                                                          vdupq_n_f32(0.115129f)));

                // Apply gain reduction
                float32x4_t comp_l = vmulq_f32(main_l, gain_lin);
                float32x4_t comp_r = vmulq_f32(main_r, gain_lin);

                // Apply Dist 2 / Dist 3 harmonics
                if (distressor_.dist_mode > 0) {
                    comp_l = generate_harmonics(&distressor_, comp_l, distressor_.dist_mode);
                    comp_r = generate_harmonics(&distressor_, comp_r, distressor_.dist_mode);
                }

                // Optional wavefolder
                if (drive_ > 0.01f) {
                    float32x4x2_t driven = wavefolder_process(&wavefolder_, comp_l, comp_r);
                    processed_l = driven.val[0];
                    processed_r = driven.val[1];
                } else {
                    processed_l = comp_l;
                    processed_r = comp_r;
                }
                break;
            }

            // -----------------------------------------------------------------
            // MODE 2: MULTIBAND COMPRESSOR
            // -----------------------------------------------------------------
            case 2: {
                // Multiband processing
                float32x4_t mb_out_l, mb_out_r;
                multiband_process(&multiband_, main_l, main_r, &mb_out_l, &mb_out_r);

                // Apply wavefolder if desired
                if (drive_ > 0.01f) {
                    float32x4x2_t driven = wavefolder_process(&wavefolder_, mb_out_l, mb_out_r);
                    processed_l = driven.val[0];
                    processed_r = driven.val[1];
                } else {
                    processed_l = mb_out_l;
                    processed_r = mb_out_r;
                }
                break;
            }

            // -----------------------------------------------------------------
            // MODE 3: FUTURE EXPANSION
            // -----------------------------------------------------------------
            default:
                processed_l = main_l;
                processed_r = main_r;
        }

        // =================================================================
        // 5. MIX STAGE (Dry/Wet Blend) - COMMON TO ALL MODES
        // =================================================================
        float32x4x2_t mixed;
        mixed.val[0] = vaddq_f32(vmulq_f32(dry_l, dry_gain),
                                  vmulq_f32(processed_l, wet_gain));
        mixed.val[1] = vaddq_f32(vmulq_f32(dry_r, dry_gain),
                                  vmulq_f32(processed_r, wet_gain));

        // =================================================================
        // 6. MAKEUP GAIN - COMMON TO ALL MODES
        // =================================================================
        mixed.val[0] = vmulq_f32(mixed.val[0], makeup_lin);
        mixed.val[1] = vmulq_f32(mixed.val[1], makeup_lin);

        // =================================================================
        // 7. OUTPUT STORAGE
        // =================================================================
        vst2q_f32(out_p, mixed);
    }
}

    /*===========================================================================*/
    /* Parameter Handling */
    /*===========================================================================*/

    inline void setParameter(uint8_t index, int32_t value) {
        raw_params_[index] = value;

        switch (index) {
            case 0: // THRESH
                thresh_db_ = value * 0.1f;
                break;

            case 1: // RATIO
                ratio_ = value * 0.1f;
                break;

            case 2: // ATTACK
                attack_ms_ = value * 0.1f;
                envelope_set_attack_release(&envelope_, attack_ms_, release_ms_);
                smoothing_set_times(&smoother_, attack_ms_, release_ms_);
                break;

            case 3: // RELEASE
                release_ms_ = static_cast<float>(value);
                envelope_set_attack_release(&envelope_, attack_ms_, release_ms_);
                smoothing_set_times(&smoother_, attack_ms_, release_ms_);
                break;

            case 4: // MAKEUP
                makeup_db_ = value * 0.1f;
                break;

            case 5: // DRIVE
                drive_ = value * 0.01f;
                wavefolder_set_drive(&wavefolder_, value);
                break;

            case 6: // MIX
                mix_ = (value + 100.0f) * 0.005f;  // -100..100 to 0..1
                break;

            case 7: // SC HPF
                sc_hpf_hz_ = static_cast<float>(value);
                sidechain_hpf_set_cutoff(&sc_hpf_, sc_hpf_hz_);
                break;

            case 8: // COMP MODE -  0=Standard, 1=Distressor, 2=Multiband
                comp_mode_ = value;
                break;

            case 9: // BAND SEL
                band_select_ = value;
                break;

            case 10: // L THRESH (Low band threshold)
                multiband_set_param(&multiband_, band_select_, 0, value * 0.1f);
                break;

            case 11: // L RATIO (Low band ratio)
                multiband_set_param(&multiband_, band_select_, 1, value * 0.1f);
                break;

            case 12: // Distressor Mode
                distressor_mode_ = value;
                distressor_set_ratio(&distressor_, value);
                break;
        }
    }

    inline int32_t getParameterValue(uint8_t index) const {
        if (index < 8) return raw_params_[index];
        return 0;
    }

    inline const char* getParameterStrValue(uint8_t index, int32_t value) const {
        static char str_buf[16];

        switch (index) {
            case 1: { // Ratio - show special cases
                float ratio = (value - 100) * 0.2f;
                if (fabsf(ratio - 20.0f) < 0.1f)
                    return "Limit";
                else if (fabsf(ratio + 20.0f) < 0.1f)
                    return "Gate";
                else {
                    snprintf(str_buf, sizeof(str_buf), "%.1f:1", ratio);
                    return str_buf;
                }
            }
            case 6: { // Mix - show D/BAL/W
                if (value <= -100) return "DRY";
                if (value >= 100) return "WET";
                if (abs(value) < 10) return "BAL";
                return nullptr;
            }
            case 12: // Distressor
                return distressor_mode_strings[distressor_mode_];
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

    // Raw integer parameters from UI
    int32_t raw_params_[8];

    // Floating-point parameters
    float thresh_db_;
    float ratio_;
    float attack_ms_;
    float release_ms_;
    float makeup_db_;
    float drive_;
    float mix_;
    float sc_hpf_hz_;
    float sc_hpf_norm_;

    // DSP coefficients
    float attack_coeff_;
    float release_coeff_;

    // Mode flags (future expansion)
    uint8_t detection_mode_;  // 0=peak, 1=RMS, 2=blend
    uint8_t use_external_sc_; // 0=internal, 1=external sidechain

    // Core DSP components
    sidechain_hpf_t sc_hpf_;
    envelope_detector_t envelope_;
    gain_computer_t gain_comp_;
    smoothing_t smoother_;
    wavefolder_t wavefolder_;

    // Mix stage
    float mix_;  // 0.0 = dry, 1.0 = wet

    // Distressor
    distressor_t distressor_;
    uint8_t distressor_mode_;

    // multiband compressor
    uint8_t comp_mode_;          // 0=Standard, 1=Distressor, 2=Multiband
    uint8_t band_select_;        // 0=Low, 1=Mid, 2=High, 3=All

    // Multiband specific
    multiband_t multiband_;
    float xover_low_freq_;
    float xover_high_freq_;

    // Distressor specific
    distressor_t distressor_;

};
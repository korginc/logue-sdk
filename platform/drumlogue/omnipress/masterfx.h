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

class MasterFX {
public:
    /*===========================================================================*/
    /* Lifecycle Methods */
    /*===========================================================================*/

    MasterFX(void) : samplerate_(48000.0f) {
        // Initialize all DSP modules
        compressor_init(&comp_);
        hpf_init(&hpf_);
        wavefolder_init(&folder_);
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
        hpf_reset(&hpf_);
        wavefolder_reset(&folder_);
        
        // Set default parameters
        setParameter(0, -200);  // Thresh: -20.0 dB
        setParameter(1, 40);    // Ratio: 4.0
        setParameter(2, 150);   // Attack: 15.0 ms
        setParameter(3, 200);   // Release: 200 ms
        setParameter(4, 0);     // Makeup: 0 dB
        setParameter(5, 0);     // Drive: 0%
        setParameter(6, 100);   // Mix: 100% wet
        setParameter(7, 20);    // SC HPF: 20 Hz
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
        for (; out_p < out_e; in_p += 16, out_p += 8) {  // 16 inputs, 8 outputs per 4 samples
            // Load 4 stereo frames (16 floats total)
            // Format: [L0, R0, SC L0, SC R0, L1, R1, SC L1, SC R1, ...]
            float32x4x4_t interleaved = vld4q_f32(in_p);
            
            // interleaved.val[0] = L0, L1, L2, L3
            // interleaved.val[1] = R0, R1, R2, R3
            // interleaved.val[2] = SC L0, SC L1, SC L2, SC L3
            // interleaved.val[3] = SC R0, SC R1, SC R2, SC R3
            
            float32x4_t main_l = interleaved.val[0];
            float32x4_t main_r = interleaved.val[1];
            float32x4_t sc_l = interleaved.val[2];
            float32x4_t sc_r = interleaved.val[3];
            
            // --- 1. Sidechain Processing ---
            float32x4_t sidechain;
            if (has_sidechain_ && use_external_sc_) {
                // Use external sidechain (sum L+R)
                sidechain = vaddq_f32(sc_l, sc_r);
            } else {
                // Use main signal as sidechain (sum L+R)
                sidechain = vaddq_f32(main_l, main_r);
            }
            
            // Apply HPF to sidechain
            sidechain = hpf_process_4(&hpf_, sidechain, sc_hpf_norm_);
            
            // --- 2. Envelope Detection ---
            // Convert to absolute value (peak detection)
            float32x4_t abs_sc = vabsq_f32(sidechain);
            
            // Blend between peak and RMS based on detection_mode_
            float32x4_t envelope;
            if (detection_mode_ == 0) {  // Peak
                envelope = abs_sc;
            } else if (detection_mode_ == 1) {  // RMS (simplified)
                // Use squared value with 1-pole smoothing
                float32x4_t sq_sc = vmulq_f32(sidechain, sidechain);
                envelope = compressor_rms_process(&comp_, sq_sc);
            } else {  // Blend
                float32x4_t peak = abs_sc;
                float32x4_t rms = compressor_rms_process(&comp_, 
                                    vmulq_f32(sidechain, sidechain));
                envelope = vaddq_f32(vmulq_f32(peak, vdupq_n_f32(0.5f)),
                                     vmulq_f32(rms, vdupq_n_f32(0.5f)));
            }
            
            // --- 3. Gain Computer (with negative ratio support) ---
            float32x4_t gain_db = compressor_calc_gain(&comp_, envelope, 
                                                        thresh_db_, ratio_);
            
            // Apply attack/release smoothing
            float32x4_t smoothed_gain_db = compressor_smooth(&comp_, gain_db,
                                                              attack_coeff_,
                                                              release_coeff_);
            
            // Convert to linear gain
            float32x4_t gain_lin = vexpq_f32(vmulq_f32(smoothed_gain_db, 
                                                        vdupq_n_f32(0.115129f)));  // ln(10)/20
            
            // --- 4. Apply Gain to Main Signal ---
            float32x4_t comp_l = vmulq_f32(main_l, gain_lin);
            float32x4_t comp_r = vmulq_f32(main_r, gain_lin);
            
            // --- 5. Wavefolder/Drive Stage ---
            float32x4x2_t driven = wavefolder_process_4(&folder_, comp_l, comp_r, drive_);
            float32x4_t driven_l = driven.val[0];
            float32x4_t driven_r = driven.val[1];
            
            // --- 6. Mix Stage ---
            float32x4_t out_l = vaddq_f32(vmulq_f32(main_l, dry_gain),
                                          vmulq_f32(driven_l, wet_gain));
            float32x4_t out_r = vaddq_f32(vmulq_f32(main_r, dry_gain),
                                          vmulq_f32(driven_r, wet_gain));
            
            // Apply makeup gain
            out_l = vmulq_f32(out_l, makeup_lin);
            out_r = vmulq_f32(out_r, makeup_lin);
            
            // Store results (interleaved stereo output)
            float32x4x2_t out_interleaved;
            out_interleaved.val[0] = out_l;  // L0, L1, L2, L3
            out_interleaved.val[1] = out_r;  // R0, R1, R2, R3
            
            vst2q_f32(out_p, out_interleaved);
        }
    }

    /*===========================================================================*/
    /* Parameter Handling */
    /*===========================================================================*/

    inline void setParameter(uint8_t index, int32_t value) {
        raw_params_[index] = value;
        
        switch (index) {
            case 0: // THRESH (-60.0 to 0.0 dB)
                thresh_db_ = value * 0.1f;
                break;
                
            case 1: // RATIO (1.0 to 20.0, but we support -20 to 20 for negative)
                // Map 0-200 to -20.0 to 20.0
                ratio_ = (value - 100) * 0.2f;
                break;
                
            case 2: // ATTACK (0.1 to 100.0 ms)
                attack_ms_ = value * 0.1f;
                attack_coeff_ = expf(-1.0f / (attack_ms_ * 0.001f * samplerate_));
                break;
                
            case 3: // RELEASE (10 to 2000 ms)
                release_ms_ = static_cast<float>(value);
                release_coeff_ = expf(-1.0f / (release_ms_ * 0.001f * samplerate_));
                break;
                
            case 4: // MAKEUP (0.0 to 24.0 dB)
                makeup_db_ = value * 0.1f;
                break;
                
            case 5: // DRIVE (0 to 100%)
                drive_ = value * 0.01f;
                break;
                
            case 6: // MIX (-100 to +100)
                mix_ = (value + 100.0f) * 0.005f; // Map to 0.0..1.0
                break;
                
            case 7: // SC HPF (20 to 500 Hz)
                sc_hpf_hz_ = static_cast<float>(value);
                sc_hpf_norm_ = sc_hpf_hz_ / samplerate_;
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
    
    // DSP Modules
    compressor_t comp_;
    hpf_t hpf_;
    wavefolder_t folder_;
};
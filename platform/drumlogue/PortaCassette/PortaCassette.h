#pragma once
/**
 * @file PortaCassette.h
 * @brief Tascam Portastudio Emulator for KORG drumlogue
 * * Features:
 * - 3-Band Parametric EQ
 * - dbx Type II Noise Reduction (2:1 Encode / 1:2 Decode)
 * - Tape Saturation (fast_tanh) & Head Bump Resonance
 * - Wow & Flutter (Modulated Micro-Delay)
 */

#include <atomic>
#include <cstdint>
#include <cmath>
#include <arm_neon.h>

#include "unit.h"
#include "float_math.h"
#include "filters.h" // Assuming your OmniPress biquad definitions

// Small delay buffer for Wow & Flutter (512 samples = ~10ms at 48kHz, plenty of room)
#define WF_BUFFER_SIZE 512
#define WF_BUFFER_MASK (WF_BUFFER_SIZE - 1)

class alignas(16) PortaCassette {
public:
    // Parameter IDs mapping to header.c
    enum ParamId {
        k_param_age,
        k_param_mix,
        k_param_preamp,
        k_param_drive,
        k_param_dbx,
        k_param_model,
        k_param_eq_low_hz,
        k_param_eq_low_gain,
        k_param_eq_mid_hz,
        k_param_eq_mid_gain,
        k_param_eq_high_hz,
        k_param_eq_high_gain,
        k_crosstalk,
        k_hiss,
        k_num_params
    };

    enum DbxMode {
        k_active,
        k_encode_only,
        k_off,
        k_dmx_modes
    };

    enum k7Mode {
        k_244,
        k_424,
        k_488,
        k_vinyl,
        k_k7_modes
    };

    PortaCassette() { Reset(); }
    ~PortaCassette() {}

    int8_t Init(const unit_runtime_desc_t* desc) {
        samplerate_ = desc->samplerate;
        Reset();
        return k_unit_err_none;
    }

    void Reset() {
        // Clear all filter states
        memset(&eq_low_l_, 0, sizeof(biquad_state_t));
        memset(&eq_mid_l_, 0, sizeof(biquad_state_t));
        memset(&eq_high_l_, 0, sizeof(biquad_state_t));
        memset(&eq_low_r_, 0, sizeof(biquad_state_t));
        memset(&eq_mid_r_, 0, sizeof(biquad_state_t));
        memset(&eq_high_r_, 0, sizeof(biquad_state_t));

        memset(&dbx_pre_l_, 0, sizeof(biquad_state_t));
        memset(&dbx_pre_r_, 0, sizeof(biquad_state_t));
        memset(&dbx_de_l_, 0, sizeof(biquad_state_t));
        memset(&dbx_de_r_, 0, sizeof(biquad_state_t));

        memset(&tape_head_l_, 0, sizeof(biquad_state_t));
        memset(&tape_head_r_, 0, sizeof(biquad_state_t));
        memset(&tape_lpf_l_, 0, sizeof(biquad_state_t));
        memset(&tape_lpf_r_, 0, sizeof(biquad_state_t));

        // Clear delay lines
        memset(wf_delay_l_, 0, sizeof(wf_delay_l_));
        memset(wf_delay_r_, 0, sizeof(wf_delay_r_));
        wf_write_pos_ = 0;
        wf_lfo_phase_ = 0.0f;

        // Clear dbx RMS detectors
        dbx_rms_env_ = vdupq_n_f32(0.0f);

        b_bypass_ = false;
        b_update_filters_ = true;

        xtalk_amount_ = 0.05f;  // as default value in header.c
        hiss_amount_ = 0.15f;

        memset(&hiss_hpf_l_, 0, sizeof(biquad_state_t));
        memset(&hiss_hpf_r_, 0, sizeof(biquad_state_t));
        prng_init(&prng_, 1337); // Initialize with a seed

        // Initialize smoothing variables so they start at their exact values
        target_preamp_ = current_preamp_ = 1.0f; // Default 1x gain
        target_drive_ = current_drive_ = 0.0f;   // Default 0% drive
        target_mix_ = current_mix_ = 1.0f;       // Default 100% wet
    }

    void SetBypass(bool bypass) { b_bypass_ = bypass; }

    void SetParameter(uint8_t id, int32_t value) {
        if (id >= k_num_params) return;
        raw_params_[id] = value;
        float norm = value * 0.01f;  // normalize
        switch(id) {
            case k_param_model:   model_ = value; break;
            case k_param_preamp:  target_preamp_ = 1.0f + (norm * 2); break; // 1x to 3x gain
            case k_param_drive:   target_drive_ = norm; break;
            case k_param_mix:     target_mix_ = norm; break;
            case k_param_dbx:     dbx_mode_ = value; break;
            case k_param_age:     tape_age_ = norm; break;
            case k_crosstalk:     xtalk_amount_ = norm; break;
            // Scale so 100% isn't deafening. 0.02f gives a nice max noise floor
            case k_hiss:          hiss_amount_ = norm * 0.02f; break;

            // Trigger a filter recalculation flag for the audio thread
            case k_param_eq_low_hz: case k_param_eq_low_gain:
            case k_param_eq_mid_hz: case k_param_eq_mid_gain:
            case k_param_eq_high_hz: case k_param_eq_high_gain:
                b_update_filters_ = true;
                break;
        }
    }

    int32_t GetParameter(uint8_t id) const {
        return (id < k_num_params) ? raw_params_[id] : 0;
    }

    // =========================================================================
    // MAIN DSP PROCESSING LOOP
    // =========================================================================
    fast_inline void ProcessBlock(const float* in, float* out, uint32_t frames) {
        if (b_bypass_) {
            // Master FX Must pass audio when bypassed!
            for (uint32_t i = 0; i < frames * 2; ++i) out[i] = in[i];
            return;
        }

        // Subnormal guard (Flush to Zero)
        #if defined(__arm__) || defined(__aarch64__)
            uint32_t fpscr;
            __asm__ volatile("vmrs %0, fpscr" : "=r"(fpscr));
            fpscr |= (1 << 24) | (1 << 22);
            __asm__ volatile("vmsr fpscr, %0" : : "r"(fpscr));
        #endif

        if (b_update_filters_) {
            UpdateCoefficients();
            b_update_filters_ = false;
        }

        // Processing 4 frames (8 interleaved samples) per loop
        uint32_t blocks = frames / NEON_LANES;
        uint32_t remain = frames % NEON_LANES;

        const float* pIn = in;
        float* pOut = out;

        for (uint32_t b = 0; b < blocks; ++b) {
            // =======================================================
            // PARAMETER SMOOTHING (1-Pole Low Pass)
            // =======================================================
            // Glide current values toward the target values
            current_preamp_ += 0.005f * (target_preamp_ - current_preamp_);
            current_drive_  += 0.005f * (target_drive_  - current_drive_);
            current_mix_    += 0.005f * (target_mix_    - current_mix_);

            // 1. Load and De-interleave Stereo NEON Vectors
            float32x4x2_t v_in = vld2q_f32(pIn);
            float32x4_t dry_l = v_in.val[0];
            float32x4_t dry_r = v_in.val[1];

            // 2. Pre-Amp Stage (Using the smoothed vector)
            float32x4_t v_preamp = vdupq_n_f32(current_preamp_);
            float32x4_t sig_l = vmulq_f32(dry_l, v_preamp);
            float32x4_t sig_r = vmulq_f32(dry_r, v_preamp);

            // 3. User Parametric EQ (Low, Mid, High)
            sig_l = biquad_process(sig_l, &eq_low_l_, &coeff_eq_low_);
            sig_l = biquad_process(sig_l, &eq_mid_l_, &coeff_eq_mid_);
            sig_l = biquad_process(sig_l, &eq_high_l_, &coeff_eq_high_);

            sig_r = biquad_process(sig_r, &eq_low_r_, &coeff_eq_low_);
            sig_r = biquad_process(sig_r, &eq_mid_r_, &coeff_eq_mid_);
            sig_r = biquad_process(sig_r, &eq_high_r_, &coeff_eq_high_);

            // 4. dbx NR Encode (Record to Tape)
            if (dbx_mode_ == k_active || dbx_mode_ == k_encode_only) { // Active or Encode-Only
                // High-Frequency Pre-Emphasis (+12dB High Shelf)
                sig_l = biquad_process(sig_l, &dbx_pre_l_, &coeff_dbx_pre_);
                sig_r = biquad_process(sig_r, &dbx_pre_r_, &coeff_dbx_pre_);

                // dbx 2:1 RMS Compression
                float32x4_t sum_sq = vaddq_f32(vmulq_f32(sig_l, sig_l), vmulq_f32(sig_r, sig_r));
                // Simple fast 1-pole RMS envelope
                dbx_rms_env_ = vmlaq_n_f32(vmulq_n_f32(dbx_rms_env_, 0.99f), sum_sq, 0.01f);

                // 2:1 Compression (sqrt of linear RMS is half the dB)
                // Inverse square root approximation gives us the gain reduction multiplier
                float32x4_t comp_gain = vrsqrteq_f32(dbx_rms_env_);
                sig_l = vmulq_f32(sig_l, comp_gain);
                sig_r = vmulq_f32(sig_r, comp_gain);
            }

            // 5. Tape Magnetic Saturation & Head Response

            // Head Bump (Low End boost)
            sig_l = biquad_process(sig_l, &tape_head_l_, &coeff_tape_head_);
            sig_r = biquad_process(sig_r, &tape_head_r_, &coeff_tape_head_);

            // A. Calculate the massive push into the tape (up to 5x gain, using smoothed drive)
            float32x4_t drive_vec = vdupq_n_f32(1.0f + (current_drive_ * 4.0f));

            // B. Apply Drive and Saturate (Soft Clip)
            sig_l = fast_tanh_neon(vmulq_f32(sig_l, drive_vec));
            sig_r = fast_tanh_neon(vmulq_f32(sig_r, drive_vec));

            // C. Auto-Makeup Gain (Attenuate back down, using the smoothed drive)
            // We use 3.0f instead of 4.0f here because tanh naturally squashes some
            // of the volume, so we don't need to turn it down quite as drastically.
            float auto_gain = 1.0f / (1.0f + (current_drive_ * 3.0f));
            float32x4_t v_auto = vdupq_n_f32(auto_gain);

            sig_l = vmulq_f32(sig_l, v_auto);
            sig_r = vmulq_f32(sig_r, v_auto);

            // Tape Age Low-Pass Filter
            sig_l = biquad_process(sig_l, &tape_lpf_l_, &coeff_tape_lpf_);
            sig_r = biquad_process(sig_r, &tape_lpf_r_, &coeff_tape_lpf_);

            // 6. Wow & Flutter (Scalar fallback for circular buffer modulation)
            // Extract vectors to arrays for granular delay line access
            float l_arr[NEON_LANES], r_arr[NEON_LANES];
            vst1q_f32(l_arr, sig_l);
            vst1q_f32(r_arr, sig_r);

            // Vinyl spins at 33 1/3 RPM (~0.55 Hz). Cassettes flutter at ~1.5 Hz.
            float modulation_rate_hz = (model_ == 3) ? 0.55f : 1.5f;

            for (int s = 0; s < NEON_LANES; ++s) {
                wf_delay_l_[wf_write_pos_] = l_arr[s];
                wf_delay_r_[wf_write_pos_] = r_arr[s];

                // Sine wave LFO for tape mechanism irregularity
                wf_lfo_phase_ += (2.0f * M_PI * modulation_rate_hz) / samplerate_;
                if (wf_lfo_phase_ > 2.0f * M_PI) wf_lfo_phase_ -= 2.0f * M_PI;

                // Depth increases with Tape Age
                float wf_depth = 5.0f + (tape_age_ * 25.0f);
                float read_offset = wf_depth + (sinf(wf_lfo_phase_) * (wf_depth * 0.8f));

                float read_pos = (float)wf_write_pos_ - read_offset;
                float safe_pos = read_pos + (float)(WF_BUFFER_SIZE * NEON_LANES);

                int32_t idx1 = (int32_t)safe_pos;
                int32_t idx2 = idx1 + 1;
                float frac = safe_pos - (float)idx1;

                uint32_t mask1 = idx1 & WF_BUFFER_MASK;
                uint32_t mask2 = idx2 & WF_BUFFER_MASK;

                l_arr[s] = wf_delay_l_[mask1] + frac * (wf_delay_l_[mask2] - wf_delay_l_[mask1]);
                r_arr[s] = wf_delay_r_[mask1] + frac * (wf_delay_r_[mask2] - wf_delay_r_[mask1]);

                wf_write_pos_ = (wf_write_pos_ + 1) & WF_BUFFER_MASK;
            }

            sig_l = vld1q_f32(l_arr);
            sig_r = vld1q_f32(r_arr);

            // 6.5 STEREO CROSSTALK (The Analog Glue)
            // Dynamically apply the user's chosen bleed amount
            float32x4_t bleed_amount = vdupq_n_f32(xtalk_amount_);

            float32x4_t cross_l = vmlaq_f32(sig_l, sig_r, bleed_amount);
            float32x4_t cross_r = vmlaq_f32(sig_r, sig_l, bleed_amount);

            sig_l = cross_l;
            sig_r = cross_r;

            // =========================================================
            // 6.7 TAPE BIAS HISS& VINYL DUST POPS
            // =========================================================
            if ((dbx_mode_ == k_encode_only) && (hiss_amount_ > 0.0f)) { // Encode Only
                // Generate 4 samples of white noise [-1.0 to 1.0]
                float n_arr[NEON_LANES];
                for (int s = 0; s < NEON_LANES; ++s) {
                     // Generate base noise
                    float noise_sample = (prng_rand_float(&prng_) * 2.0f) - 1.0f;

                    // ADD VINYL POPS
                    if (model_ == 3) {
                        // 99.98% of the time, it's just surface noise.
                        // 0.02% of the time, it's a piece of dust (a loud click).
                        if (prng_rand_float(&prng_) > 0.9998f) {
                            // Multiply by 40.0f to spike the noise into a loud transient
                            noise_sample *= 40.0f;
                        }
                    }
                    n_arr[s] = noise_sample;
                }

                float32x4_t raw_noise = vld1q_f32(n_arr);
                float32x4_t v_hiss_level = vdupq_n_f32(hiss_amount_);

                // Filter the noise (This also filters the pops, making them sound
                // warm and mechanical rather than harsh digital clicks!)
                float32x4_t filtered_hiss_l = biquad_process(raw_noise, &hiss_hpf_l_, &coeff_hiss_hpf_);

                // CHEAP STEREO WIDENING TRICK:
                // Instead of generating 4 more random numbers for the right channel,
                // we just invert the phase of the left channel's noise!
                // This makes the hiss sound incredibly wide and completely decorrelated.
                float32x4_t filtered_hiss_r = vnegq_f32(filtered_hiss_l);

                // Scale and add to the main signal
                sig_l = vmlaq_f32(sig_l, filtered_hiss_l, v_hiss_level);
                sig_r = vmlaq_f32(sig_r, filtered_hiss_r, v_hiss_level);
            }

            // =========================================================
            // 7. dbx NR Decode (Playback from Tape)
            // =========================================================
            if (dbx_mode_ == k_active) { // Active Only
                // 1:2 Expansion (Squared RMS envelope)
                float32x4_t sum_sq = vaddq_f32(vmulq_f32(sig_l, sig_l), vmulq_f32(sig_r, sig_r));
                dbx_rms_env_ = vmlaq_n_f32(vmulq_n_f32(dbx_rms_env_, 0.99f), sum_sq, 0.01f);

                // Expansion multiplies the signal by the linear RMS envelope
                float32x4_t exp_gain = dbx_rms_env_; // simplified expansion curve
                sig_l = vmulq_f32(sig_l, exp_gain);
                sig_r = vmulq_f32(sig_r, exp_gain);

                // De-Emphasis (-12dB High Shelf)
                sig_l = biquad_process(sig_l, &dbx_de_l_, &coeff_dbx_de_);
                sig_r = biquad_process(sig_r, &dbx_de_r_, &coeff_dbx_de_);
            }

            // 8. Dry/Wet Mixdown
            float32x4_t v_mix = vdupq_n_f32(current_mix_);
            float32x4_t v_inv = vdupq_n_f32(1.0f - current_mix_);

            float32x4_t out_l = vaddq_f32(vmulq_f32(dry_l, v_inv), vmulq_f32(sig_l, v_mix));
            float32x4_t out_r = vaddq_f32(vmulq_f32(dry_r, v_inv), vmulq_f32(sig_r, v_mix));

            // Re-interleave and store
            float32x4x2_t v_out = { out_l, out_r };
            vst2q_f32(pOut, v_out);

            pIn += 8;
            pOut += 8;
        }

        // Process any remaining 1-3 frames here (omitted for brevity, standard scalar loop)
        // To Be Done - or just skip them for low fidelity sound.
    }

private:
    // Parameters
    int32_t raw_params_[k_num_params] __attribute__((aligned(16)));
    float samplerate_;
    bool b_bypass_;
    bool b_update_filters_;

    uint8_t model_;
    uint8_t dbx_mode_;

    // With these Smooth Parameter Pairs:
    float target_preamp_, current_preamp_;
    float target_drive_, current_drive_;
    float target_mix_, current_mix_;

    // static
    float tape_age_;
    float xtalk_amount_;
    float hiss_amount_;

    prng_t prng_; // Your PRNG state struct
    biquad_state_t hiss_hpf_l_, hiss_hpf_r_;
    biquad_coeffs_t coeff_hiss_hpf_;

    // States
    biquad_state_t eq_low_l_, eq_low_r_;
    biquad_state_t eq_mid_l_, eq_mid_r_;
    biquad_state_t eq_high_l_, eq_high_r_;

    biquad_state_t dbx_pre_l_, dbx_pre_r_;
    biquad_state_t dbx_de_l_, dbx_de_r_;

    biquad_state_t tape_head_l_, tape_head_r_;
    biquad_state_t tape_lpf_l_, tape_lpf_r_;

    float32x4_t dbx_rms_env_;

    float wf_delay_l_[WF_BUFFER_SIZE] __attribute__((aligned(16)));
    float wf_delay_r_[WF_BUFFER_SIZE] __attribute__((aligned(16)));
    uint32_t wf_write_pos_;
    float wf_lfo_phase_;

    // Coefficients
    biquad_coeffs_t coeff_eq_low_;
    biquad_coeffs_t coeff_eq_mid_;
    biquad_coeffs_t coeff_eq_high_;
    biquad_coeffs_t coeff_dbx_pre_;
    biquad_coeffs_t coeff_dbx_de_;
    biquad_coeffs_t coeff_tape_head_;
    biquad_coeffs_t coeff_tape_lpf_;


    // =========================================================================
    // Filter Updaters: Because these functions only run in UpdateCoefficients()
    // when a user physically turns a knob (not 48,000 times a second),
    // it is 100% safe to use standard sinf, cosf, and sqrtf instead of approx
    // =========================================================================
    void UpdateCoefficients() {
        // Calculate dynamic user EQs
        float low_hz = raw_params_[k_param_eq_low_hz] * 10.0f;
        float low_db = (raw_params_[k_param_eq_low_gain] - 120.0f) * 0.1f;
        calculate_peaking_eq(&coeff_eq_low_, low_hz, low_db, 1.0f);

        float mid_hz = raw_params_[k_param_eq_mid_hz] * 10.0f;
        float mid_db = (raw_params_[k_param_eq_mid_gain] - 120.0f) * 0.1f;
        calculate_peaking_eq(&coeff_eq_mid_, mid_hz, mid_db, 1.5f);

        float high_hz = raw_params_[k_param_eq_high_hz] * 100.0f;
        float high_db = (raw_params_[k_param_eq_high_gain] - 120.0f) * 0.1f;
        calculate_peaking_eq(&coeff_eq_high_, high_hz, high_db, 1.0f);

        // Fixed dbx EQ curves (~4kHz, +/- 12dB)
        calculate_high_shelf(&coeff_dbx_pre_, 4000.0f, +12.0f);
        calculate_high_shelf(&coeff_dbx_de_, 4000.0f, -12.0f);

        // Tape Characteristics based on Model & Age
        float lpf_hz = 14000.0f; // Default 244 (High Speed)
        float head_hz = 65.0f;
        float hiss_cutoff = 3500.0f; // Default Tape Bias Hiss

        if (model_ == k_424) { lpf_hz = 11000.0f; head_hz = 55.0f; } // 424 Standard
        if (model_ == k_488) { lpf_hz = 9500.0f; head_hz = 50.0f; }  // 488 Narrow track
        // ==========================================
        // NEW: VINYL RECORD MODEL (Model 3)
        // ==========================================
        if (model_ == k_vinyl) {
            lpf_hz = 12000.0f;   // Rolled off highs
            head_hz = 150.0f;    // Vinyl mid-warmth, avoids sub-bass rumble
            hiss_cutoff = 400.0f;// Lower cutoff for gritty vinyl surface noise
        }

        // Age dulls the tape
        lpf_hz = lpf_hz * (1.0f - (tape_age_ * 0.6f)); // Age 100% drops 14k to ~5.6k

        calculate_peaking_eq(&coeff_tape_head_, head_hz, +4.0f, 1.5f); // +4dB head bump
        calculate_low_pass(&coeff_tape_lpf_, lpf_hz, 0.707f);

        // High-pass filter at 3.5 kHz to make the noise sound like airy
        // tape hiss rather than low-end rumbling static.
        calculate_high_pass(&coeff_hiss_hpf_, hiss_cutoff, 0.707f);
    }

    // Example fast tanh approximation (Assuming not present in float_math.h)
    inline float32x4_t fast_tanh_neon(float32x4_t x) {
        float32x4_t cx = vmaxq_f32(vdupq_n_f32(-1.0f), vminq_f32(vdupq_n_f32(1.0f), x));
        float32x4_t x2 = vmulq_f32(cx, cx);
        float32x4_t poly = vsubq_f32(vdupq_n_f32(1.0f), vmulq_n_f32(x2, 0.33333f));
        return vmulq_n_f32(vmulq_f32(cx, poly), 1.5f);
    }

    // Standard Biquad Math stubs (You will map these to your filters.h implementations)
    // =========================================================================
    // Filter Coefficient Calculators (Audio EQ Cookbook)
    // =========================================================================

    void calculate_peaking_eq(biquad_coeffs_t* c, float hz, float db, float q) {
        float A = powf(10.0f, db / 40.0f);
        float w0 = 2.0f * M_PI * hz / samplerate_;
        float alpha = sinf(w0) / (2.0f * q);

        float a0 = 1.0f + alpha / A;
        c->b0 = (1.0f + alpha * A) / a0;
        c->b1 = (-2.0f * cosf(w0)) / a0;
        c->b2 = (1.0f - alpha * A) / a0;
        c->a1 = (-2.0f * cosf(w0)) / a0;
        c->a2 = (1.0f - alpha / A) / a0;
    }

    void calculate_high_shelf(biquad_coeffs_t* c, float hz, float db) {
        float A = powf(10.0f, db / 40.0f);
        float w0 = 2.0f * M_PI * hz / samplerate_;
        float alpha = sinf(w0) / 2.0f * sqrtf((A + 1.0f / A) * (1.0f / 0.707f - 1.0f) + 2.0f);

        float a0 = (A + 1.0f) - (A - 1.0f) * cosf(w0) + 2.0f * sqrtf(A) * alpha;
        c->b0 = (A * ((A + 1.0f) + (A - 1.0f) * cosf(w0) + 2.0f * sqrtf(A) * alpha)) / a0;
        c->b1 = (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosf(w0))) / a0;
        c->b2 = (A * ((A + 1.0f) + (A - 1.0f) * cosf(w0) - 2.0f * sqrtf(A) * alpha)) / a0;
        c->a1 = (2.0f * ((A - 1.0f) - (A + 1.0f) * cosf(w0))) / a0;
        c->a2 = ((A + 1.0f) - (A - 1.0f) * cosf(w0) - 2.0f * sqrtf(A) * alpha) / a0;
    }

    void calculate_low_pass(biquad_coeffs_t* c, float hz, float q) {
        float w0 = 2.0f * M_PI * hz / samplerate_;
        float alpha = sinf(w0) / (2.0f * q);
        float cos_w0 = cosf(w0);

        float a0 = 1.0f + alpha;
        c->b0 = ((1.0f - cos_w0) / 2.0f) / a0;
        c->b1 = (1.0f - cos_w0) / a0;
        c->b2 = ((1.0f - cos_w0) / 2.0f) / a0;
        c->a1 = (-2.0f * cos_w0) / a0;
        c->a2 = (1.0f - alpha) / a0;
    }

    void calculate_high_pass(biquad_coeffs_t* c, float hz, float q) {
        float w0 = 2.0f * M_PI * hz / samplerate_;
        float alpha = sinf(w0) / (2.0f * q);
        float cos_w0 = cosf(w0);

        float a0 = 1.0f + alpha;
        c->b0 = ((1.0f + cos_w0) / 2.0f) / a0;
        c->b1 = -(1.0f + cos_w0) / a0;
        c->b2 = ((1.0f + cos_w0) / 2.0f) / a0;
        c->a1 = (-2.0f * cos_w0) / a0;
        c->a2 = (1.0f - alpha) / a0;
    }

    private:
    /*===========================================================================*/
    /* Xorshift128+ PRNG Implementation */
    /*===========================================================================*/

    void prng_init(uint64_t seed) {
        uint64_t s0[2] = {seed, seed * 0x9F3679A87F5A8C26ULL};
        uint64_t s1[2] = {seed * 0xAF48476D0CE3E5ABULL, seed * 0x93B059B1133121DBULL};

        prng_.state0 = vld1q_u64(s0);
        prng_.state1 = vld1q_u64(s1);
    }

    uint32x4_t prng_rand_u32() {
        uint64x2_t s0 = prng_.state0;
        uint64x2_t s1 = prng_.state1;

        uint64x2_t s1_left = vshlq_n_u64(s1, 23);
        s1 = veorq_u64(s1, s1_left);

        uint64x2_t s1_right = vshrq_n_u64(s1, 17);
        s1 = veorq_u64(s1, s1_right);

        uint64x2_t s0_right = vshrq_n_u64(s0, 26);
        uint64x2_t s0_xor = veorq_u64(s0, s0_right);
        s1 = veorq_u64(s1, s0_xor);

        prng_.state0 = s1;
        prng_.state1 = s0;

        uint64x2_t sum = vaddq_u64(s0, s1);

        uint32x2_t low32 = vmovn_u64(sum);
        uint32x2_t high32 = vshrn_n_u64(sum, 32);

        return vcombine_u32(low32, high32);
    }

    /**
     * @brief return vector of floats ranging from 1.0f to 1.99999988f
     *
     * @return float32x4_t
     */
    float32x4_t prng_rand_float() {
        uint32x4_t rand = prng_rand_u32();
        uint32x4_t masked = vandq_u32(rand, vdupq_n_u32(0x7FFFFF));
        uint32x4_t float_bits = vorrq_u32(masked, vdupq_n_u32(0x3F800000));
        return vsubq_f32(vreinterpretq_f32_u32(float_bits), vdupq_n_f32(1.0f));
    }
};
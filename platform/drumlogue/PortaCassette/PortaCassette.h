#pragma once
/**
 * @file PortaCassette.h
 * @brief Tascam Portastudio Emulator for KORG drumlogue
 *
 * DSP chain:
 *   PreAmp → PEQ(3-band) → dbx Encode → Head Bump → Drive/Sat → Tape LPF
 *   → Wow&Flutter → Crosstalk → Hiss → dbx Decode → Dry/Wet Mix
 */

#include <atomic>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <arm_neon.h>

#include "unit.h"
#include "float_math.h"
#include "biquad.h"

// Wow & Flutter delay: 512 samples ≈ 10.7 ms at 48 kHz, max depth ≤ 30 samples
#define WF_BUFFER_SIZE 512
#define WF_BUFFER_MASK (WF_BUFFER_SIZE - 1)
#define NEON_LANES (4)

// PRNG state (Xorshift128+, two 64-bit lanes each)
struct prng_t { uint64x2_t state0, state1; };

class alignas(16) PortaCassette {
public:
    enum ParamId {
        k_param_age = 0,
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

    enum DbxMode { k_active = 0, k_encode_only, k_off };
    enum k7Mode  { k_244    = 0, k_424, k_488, k_vinyl };

    PortaCassette() : samplerate_(48000.0f), b_update_filters_(false) { Reset(); }
    ~PortaCassette() {}

    int8_t Init(const unit_runtime_desc_t* desc) {
        samplerate_ = desc->samplerate;
        Reset();
        return k_unit_err_none;
    }

    void Reset() {
        biquad_reset(&eq_low_l_);    biquad_reset(&eq_low_r_);
        biquad_reset(&eq_mid_l_);    biquad_reset(&eq_mid_r_);
        biquad_reset(&eq_high_l_);   biquad_reset(&eq_high_r_);
        biquad_reset(&dbx_pre_l_);   biquad_reset(&dbx_pre_r_);
        biquad_reset(&dbx_de_l_);    biquad_reset(&dbx_de_r_);
        biquad_reset(&tape_head_l_); biquad_reset(&tape_head_r_);
        biquad_reset(&tape_lpf_l_);  biquad_reset(&tape_lpf_r_);
        biquad_reset(&hiss_hpf_l_);  biquad_reset(&hiss_hpf_r_);
        biquad_reset(&dbx_hf_l_);    biquad_reset(&dbx_hf_r_);
        biquad_reset(&xtalk_hpf_l_); biquad_reset(&xtalk_hpf_r_);

        memset(wf_delay_l_, 0, sizeof(wf_delay_l_));
        memset(wf_delay_r_, 0, sizeof(wf_delay_r_));
        wf_write_pos_        = 0;
        wf_lfo_phase_        = 0.0f;
        wf_flutter_phase_    = 0.0f;
        wf_flutter_phase_inc_ = M_TWOPI * 25.0f / samplerate_;

        // Small positive seed prevents 1/sqrt(0) divergence on first block
        dbx_enc_rms_    = vdupq_n_f32(1e-6f);
        dbx_hf_enc_rms_ = vdupq_n_f32(1e-6f);

        b_bypass_ = false;
        b_update_filters_.store(true);

        prng_init(1337u);

        // Defaults matching header.c init values
        model_    = k_244;
        dbx_mode_ = k_active;
        tape_age_ = 0.10f;       // init=10  → norm=0.10
        xtalk_amount_  = 0.05f;  // init=5  → norm=0.05
        hiss_amount_   = 0.003f; // init=15 → norm=0.15 * 0.02
        pop_env_decay_ = 0.92f;  // ~12 ms decay at 48 kHz, controls dust pop duration
        pop_env_       = prng_rand_float(); // envelope for simulating dust pop decay/release

        target_preamp_ = current_preamp_ = 1.0f + 0.20f * 2.0f; // init=20
        target_drive_  = current_drive_  = 0.40f;               // init=40
        target_mix_    = current_mix_    = 1.0f;                // init=100

        // set initial tape saturation
        x_prev = vdupq_n_f32(0.0f);
        y_prev = vdupq_n_f32(0.0f);
        R = vdupq_n_f32(0.995f);

        wf_phase_inc_ = M_TWOPI * 1.5f / samplerate_;

        memset(raw_params_, 0, sizeof(raw_params_));
    }

    void SetBypass(bool bypass) { b_bypass_ = bypass; }

    void SetParameter(uint8_t id, int32_t value) {
        if (id >= k_num_params) return;
        raw_params_[id] = value;
        const float norm = value * 0.01f;
        switch (id) {
            case k_param_model:
                model_ = (uint8_t)value;
                wf_phase_inc_ = M_TWOPI * ((model_ == k_vinyl) ? 0.55f : 1.5f) / samplerate_;
                b_update_filters_.store(true);  // tape model affects LPF/head coeffs
                break;
            case k_param_preamp: target_preamp_ = 1.0f + norm * 2.0f; break;
            case k_param_drive:  target_drive_  = norm;               break;
            case k_param_mix:    target_mix_    = norm;               break;
            case k_param_dbx:    dbx_mode_ = (uint8_t)value;          break;
            case k_param_age:
                tape_age_ = norm;
                b_update_filters_.store(true);  // age affects tape LPF cutoff
                break;
            case k_crosstalk:    xtalk_amount_ = norm;                 break;
            case k_hiss:         hiss_amount_  = norm * 0.02f;         break;
            case k_param_eq_low_hz:   case k_param_eq_low_gain:
            case k_param_eq_mid_hz:   case k_param_eq_mid_gain:
            case k_param_eq_high_hz:  case k_param_eq_high_gain:
                b_update_filters_.store(true);
                break;
            default: break;
        }
    }

    int32_t GetParameter(uint8_t id) const {
        return (id < k_num_params) ? raw_params_[id] : 0;
    }

    fast_inline void parametric_eq(float32x4_t &sig_l, float32x4_t &sig_r) {
        sig_l = biquad_process4(&eq_low_l_,  &coeff_eq_low_,  sig_l);
        sig_l = biquad_process4(&eq_mid_l_,  &coeff_eq_mid_,  sig_l);
        sig_l = biquad_process4(&eq_high_l_, &coeff_eq_high_, sig_l);
        sig_r = biquad_process4(&eq_low_r_,  &coeff_eq_low_,  sig_r);
        sig_r = biquad_process4(&eq_mid_r_,  &coeff_eq_mid_,  sig_r);
        sig_r = biquad_process4(&eq_high_r_, &coeff_eq_high_, sig_r);
    }

    fast_inline float32x4_t dbx_nr_encode(float32x4_t &sig_l, float32x4_t &sig_r) {
        float32x4_t dec_gain = vdupq_n_f32(1.0f);  // set inside encode block when active
        if (dbx_mode_ != k_off) {
            sig_l = biquad_process4(&dbx_pre_l_, &coeff_dbx_pre_, sig_l);
            sig_r = biquad_process4(&dbx_pre_r_, &coeff_dbx_pre_, sig_r);

            float attack  = 0.0050f;    // slower attach
            float release = 0.0002f;    // much slower release

            // HF spectral envelope (HPF'd signal above ~2 kHz)
            float32x4_t hf_l = biquad_process4(&dbx_hf_l_, &coeff_dbx_hf_, sig_l);
            float32x4_t hf_r = biquad_process4(&dbx_hf_r_, &coeff_dbx_hf_, sig_r);
            float32x4_t hf_sq = vaddq_f32(vmulq_f32(hf_l, hf_l), vmulq_f32(hf_r, hf_r));
            dbx_hf_enc_rms_ = vmlaq_n_f32(vmulq_n_f32(dbx_hf_enc_rms_, attack), hf_sq, release);

            // Wideband envelope + NR-refined rsqrt for accurate 2:1 compression
            float32x4_t wb_sq = vaddq_f32(vmulq_f32(sig_l, sig_l), vmulq_f32(sig_r, sig_r));
            dbx_enc_rms_ = vmlaq_n_f32(vmulq_n_f32(dbx_enc_rms_, attack), wb_sq, release);
            float32x4_t env = vmaxq_f32(
                vmlaq_n_f32(vmulq_n_f32(dbx_enc_rms_, 0.7f), dbx_hf_enc_rms_, 0.3f),
                vdupq_n_f32(1e-6f));
            float32x4_t enc_gain = vrsqrteq_f32(env);
            enc_gain = vmulq_f32(vrsqrtsq_f32(vmulq_f32(env, enc_gain), enc_gain), enc_gain);
            // Clamp encode gain: prevents 1000x boost during silence → loud transient onset.
            // 3.0x ≈ +9.5 dB max NR action; less than typical dbx Type II spec.
            enc_gain = vminq_f32(enc_gain, vdupq_n_f32(3.0f));
            // Synchronized decode gain = 1/enc_gain.  Computing it here (from the
            // same enc_gain used to boost the signal) guarantees enc × dec = 1.0
            // at all times, eliminating the timing mismatch between independent
            // encode/decode RMS followers that caused rhythm-matched clicks.
            dec_gain = vrecpeq_f32(enc_gain);
            dec_gain = vmulq_f32(vrecpsq_f32(enc_gain, dec_gain), dec_gain);
            sig_l = vmulq_f32(sig_l, enc_gain);
            sig_r = vmulq_f32(sig_r, enc_gain);
        }
        return dec_gain;
    }

    fast_inline void head_bump(float32x4_t &sig_l, float32x4_t &sig_r) {
        sig_l = biquad_process4(&tape_head_l_, &coeff_tape_head_, sig_l);
        sig_r = biquad_process4(&tape_head_r_, &coeff_tape_head_, sig_r);
    }

    fast_inline void drive_into_soft_saturation(float32x4_t &sig_l, float32x4_t &sig_r) {
        const float drive  = 1.0f + current_drive_ * 3.0f;
        const float makeup =
            (current_drive_ > 0.0f) ? Q_rsqrt(current_drive_) : 0.85f; // makeup gain reduces as drive increases, but never below -3 dB
        if (model_ == k_vinyl)
        {
            sig_l = vmulq_n_f32(sat_neon(vmulq_n_f32(sig_l, drive)), makeup);
            sig_r = vmulq_n_f32(sat_neon(vmulq_n_f32(sig_r, drive)), makeup);
        }
        else    // tape like
        {
            sig_l = vmulq_n_f32(sat_neon_new(vmulq_n_f32(sig_l, drive)), makeup);
            sig_r = vmulq_n_f32(sat_neon_new(vmulq_n_f32(sig_r, drive)), makeup);
        }
    }

    fast_inline void tape_age(float32x4_t &sig_l, float32x4_t &sig_r) {
        sig_l = biquad_process4(&tape_lpf_l_, &coeff_tape_lpf_, sig_l);
        sig_r = biquad_process4(&tape_lpf_r_, &coeff_tape_lpf_, sig_r);
    }

    fast_inline void preamp(float32x4_t &sig_l, float32x4_t &sig_r) {
        sig_l = vmulq_n_f32(sig_l, current_preamp_);
        sig_r = vmulq_n_f32(sig_r, current_preamp_);
    }

    // Interpolates a single point given 4 control points and a fractional offset f
    fast_inline float hermite4_neon(float p0, float p1, float p2, float p3, float f) {
        // 1. Load points into a NEON vector
        float points[4] = {p0, p1, p2, p3};
        float32x4_t P = vld1q_f32(points);

        // 2. Compute powers of f: [1, f, f*f, f*f*f]
        float t2 = f * f;
        float t3 = t2 * f;
        float32x4_t T = {1.0f, f, t2, t3};

        // 3. Define the Hermite/Catmull-Rom matrix columns (transposed)
        // Coeff 0: P0*0 + P1*2 + P2*0 + P3*0
        // Coeff 1: P0*(-1) + P1*0 + P2*1 + P3*0
        // Coeff 2: P0*2 + P1*(-5) + P2*4 + P3*(-1)
        // Coeff 3: P0*(-1) + P1*3 + P2*(-3) + P3*1
        float32x4_t C0 = { 0.0f, -1.0f,  2.0f, -1.0f};
        float32x4_t C1 = { 2.0f,  0.0f, -5.0f,  3.0f};
        float32x4_t C2 = { 0.0f,  1.0f,  4.0f, -3.0f};
        float32x4_t C3 = { 0.0f,  0.0f, -1.0f,  1.0f};

        // 4. Dot product per channel (Polynomial evaluation)
        float32x4_t V0 = vmulq_f32(vdupq_n_f32(vgetq_lane_f32(P, 0)), C0);
        float32x4_t V1 = vmulq_f32(vdupq_n_f32(vgetq_lane_f32(P, 1)), C1);
        float32x4_t V2 = vmulq_f32(vdupq_n_f32(vgetq_lane_f32(P, 2)), C2);
        float32x4_t V3 = vmulq_f32(vdupq_n_f32(vgetq_lane_f32(P, 3)), C3);

        // 5. Sum the weighted columns
        float32x4_t sum_vec = vaddq_f32(vaddq_f32(V0, V1), vaddq_f32(V2, V3));

        // 6. Multiply by T and divide by 2
        float32x4_t final_poly = vmulq_f32(sum_vec, T);
        float result = (vgetq_lane_f32(final_poly, 0) +
                        vgetq_lane_f32(final_poly, 1) +
                        vgetq_lane_f32(final_poly, 2) +
                        vgetq_lane_f32(final_poly, 3)) * 0.5f;

        return result;
    }

    fast_inline void wow_and_flutter(float32x4_t &sig_l, float32x4_t &sig_r) {
        const float lfo_val     = fastersinfullf(wf_lfo_phase_);
        const float flutter_val = fastersinfullf(wf_flutter_phase_);
        wf_lfo_phase_     += wf_phase_inc_         * 4.0f;
        wf_flutter_phase_ += wf_flutter_phase_inc_ * 4.0f;
        if (wf_lfo_phase_     >= M_TWOPI) wf_lfo_phase_     -= M_TWOPI;
        if (wf_flutter_phase_ >= M_TWOPI) wf_flutter_phase_ -= M_TWOPI;

        const float wow_depth     = 5.0f + tape_age_ * 25.0f;
        const float flutter_depth = 0.8f + tape_age_ * 2.0f;
        const float rd_off = wow_depth * (1.0f + 0.8f * lfo_val)
                            + flutter_depth * flutter_val;

        float l4[NEON_LANES], r4[NEON_LANES];
        vst1q_f32(l4, sig_l);
        vst1q_f32(r4, sig_r);

        for (int s = 0; s < NEON_LANES; ++s) {
            wf_delay_l_[wf_write_pos_] = l4[s];
            wf_delay_r_[wf_write_pos_] = r4[s];

            // Read with linear interpolation from the past
            float pos = (float)wf_write_pos_ - rd_off + (float)WF_BUFFER_SIZE;
            int32_t  i0 = (int32_t)pos;
            float    fr = pos - (float)i0;
            uint32_t m0 = (uint32_t)i0 & WF_BUFFER_MASK;
            uint32_t m1 = (m0 + 1u) & WF_BUFFER_MASK;
            uint32_t m2 = (m0 + 2u) & WF_BUFFER_MASK;
            uint32_t m3 = (m0 + 3u) & WF_BUFFER_MASK;
            // cubic interpolation from 4 points: https://www.paulinternet.nl/?page=bicubic
            l4[s] = hermite4_neon(wf_delay_l_[m0], wf_delay_l_[m1], wf_delay_l_[m2], wf_delay_l_[m3], fr);
            r4[s] = hermite4_neon(wf_delay_r_[m0], wf_delay_r_[m1], wf_delay_r_[m2], wf_delay_r_[m3], fr);
            // linear: a + frac*(b-a)
            // l4[s] = wf_delay_l_[m0] + fr * (wf_delay_l_[m1] - wf_delay_l_[m0]);
            // r4[s] = wf_delay_r_[m0] + fr * (wf_delay_r_[m1] - wf_delay_r_[m0]);

            wf_write_pos_ = (wf_write_pos_ + 3u) & WF_BUFFER_MASK;
        }
        sig_l = vld1q_f32(l4);
        sig_r = vld1q_f32(r4);
    }

    fast_inline void crosstalk(float32x4_t &sig_l, float32x4_t &sig_r) {
            float32x4_t bleed_r = biquad_process4(&xtalk_hpf_r_, &coeff_xtalk_hpf_, sig_r);
            float32x4_t bleed_l = biquad_process4(&xtalk_hpf_l_, &coeff_xtalk_hpf_, sig_l);
            float32x4_t xl = vmlaq_n_f32(sig_l, bleed_r, xtalk_amount_);
            float32x4_t xr = vmlaq_n_f32(sig_r, bleed_l, xtalk_amount_);
            sig_l = xl; sig_r = xr;
    }

    fast_inline void tape_hiss_vinyl_dust(float32x4_t &sig_l, float32x4_t &sig_r) {
        if (hiss_amount_ > 0.0f && dbx_mode_ != k_active) {
            // 4 white-noise samples at once: [0,1) → [-1,1)
            float32x4_t noise = vsubq_f32(
                vmulq_n_f32(prng_rand_float(), 2.0f), vdupq_n_f32(1.0f));
                if (model_ == k_vinyl) {
                    // ~0.02% chance of a dust pop (3× spike) per sample
                    uint32x4_t is_pop = vcgtq_f32(prng_rand_float(), vdupq_n_f32(0.9998f));
                    noise = vmulq_f32(noise,
                        vbslq_f32(is_pop, vdupq_n_f32(3.0f), vdupq_n_f32(1.0f)));
                    // Trigger pop envelope on any pop sample
                    noise = vaddq_f32(vmulq_n_f32(noise, 0.7f), vmulq_f32(vmulq_n_f32(previous_noise, 0.3f), pop_env_));
                    previous_noise = noise; // store pre-decay noise for shaping
            }

            // Filter to coloured hiss; invert for R to widen the stereo image
            float32x4_t fl = biquad_process4(&hiss_hpf_l_, &coeff_hiss_hpf_, noise);
            float32x4_t fr = vmlaq_n_f32(fl, prng_rand_float(), 0.7f); // decorrelate R from L for a wider image
            // Older tape sheds oxide → noisier floor; scale hiss with age
            const float hiss_scaled = hiss_amount_ * (1.0f + tape_age_ * 2.0f);
            sig_l = vmlaq_n_f32(sig_l, fl, hiss_scaled);
            sig_r = vmlaq_n_f32(sig_r, fr, hiss_scaled);
        }
    }

    fast_inline void dbx_nr_decode(float32x4_t &sig_l, float32x4_t &sig_r, float32x4_t dec_gain) {
        if (dbx_mode_ == k_active) {
            // Use the synchronized dec_gain = 1/enc_gain computed in step 4.
                    // This guarantees enc_gain × dec_gain = 1.0 at every block, making the
                    // NR system transparent (net gain = 0 dB) except for tape nonlinearity.
                    // The old approach (independent dec_rms tracking) caused enc/dec timing
                    // mismatch: on the first hit after silence enc_gain = 6.0× but dec_gain
                    // was still sqrt(1e-6) ≈ 0.001, producing a near-zero decoded output
                    // that sounded like a loud click followed by silence.
            sig_l = vmulq_f32(sig_l, dec_gain);
            sig_r = vmulq_f32(sig_r, dec_gain);
            sig_l = biquad_process4(&dbx_de_l_, &coeff_dbx_de_, sig_l);
            sig_r = biquad_process4(&dbx_de_r_, &coeff_dbx_de_, sig_r);
        }
    }

    // =========================================================================
    // MAIN DSP PROCESSING LOOP
    // =========================================================================
        fast_inline void ProcessBlock(const float* in, float* out, uint32_t frames) {
        if (b_bypass_) {
            if (in != out) memcpy(out, in, frames * 2u * sizeof(float));
            return;
        }

        // Flush denormals to zero (prevents CPU stalls on subnormal arithmetic)
#if defined(__arm__) || defined(__aarch64__)
        uint32_t fpscr;
        __asm__ volatile("vmrs %0, fpscr" : "=r"(fpscr));
        fpscr |= (1u << 24) | (1u << 22);
        __asm__ volatile("vmsr fpscr, %0" : : "r"(fpscr));
#endif

        if (b_update_filters_.exchange(false))
            UpdateCoefficients();

        const float* pIn  = in;
        float*       pOut = out;

        for (uint32_t b = 0, blocks = frames / 4u; b < blocks; ++b, pIn += 8, pOut += 8) {
            // ------------------------------------------------------------------
            // Parameter smoothing (1-pole, one step per 4-sample block)
            // ------------------------------------------------------------------
            current_preamp_ += 0.01f * (target_preamp_ - current_preamp_);  // use 0.01 instead of 0.005 for faster response
            current_drive_  += 0.01f * (target_drive_  - current_drive_);
            current_mix_    += 0.01f * (target_mix_    - current_mix_);

            // 1. Load & de-interleave stereo
            float32x4x2_t vi = vld2q_f32(pIn);
            const float32x4_t dry_l = vi.val[0];    // dry
            const float32x4_t dry_r = vi.val[1];
            float32x4_t sig_l = dry_l;        // wet (processed) signal starts as dry and is transformed in-place through the chain; used for both encode and decode paths to maintain correct timing of the NR system's envelope followers, which track the actual signal hitting the tape nonlinearity
            float32x4_t sig_r = dry_r;

            // 2. Pre-amp
            preamp(sig_l, sig_r);

            // 3. Parametric EQ (3-band, L and R share coefficients, independent states)
            parametric_eq(sig_l, sig_r);

            // 4. dbx NR Encode (Record path: HF emphasis + 2:1 RMS compression)
            //    Type II HF spectral path: HF content tracked separately and blended
            //    into the total envelope so loud transients with HF get more reduction.
            float32x4_t dec_gain = dbx_nr_encode(sig_l, sig_r);

            // 4.5 Tape bias hiss / vinyl dust pops: moved before saturation to get hiss naturally softened.
            //     Added in k_encode_only and k_off modes (dbx active suppresses it naturally)
            tape_hiss_vinyl_dust(sig_l, sig_r);

            // 5. Tape Magnetic Saturation
            //    a) Head bump (low-mid resonance)
            head_bump(sig_l, sig_r);

            //    b) Drive into soft saturation, auto-makeup to maintain level
            drive_into_soft_saturation(sig_l, sig_r);

            //    c) Tape age → HF roll-off
            tape_age(sig_l, sig_r);

            // 6. Wow & Flutter — both LFOs computed once per block
            //    Wow (~1.5 Hz): slow capstan irregularities (5-30 samples depth)
            //    Flutter (~25 Hz): roller eccentricity (0.8-2.8 samples depth)
            wow_and_flutter(sig_l, sig_r);

            // 6.5 Stereo crosstalk — treble bleeds more than bass on cassette tape
            //     (shorter wavelengths flux-leak more easily between adjacent tracks)
            crosstalk(sig_l, sig_r);

            // 7. dbx NR Decode (Playback path: 1:2 expansion + de-emphasis)
            dbx_nr_decode(sig_l, sig_r, dec_gain);

            // 8. Dry/wet mix
            const float inv_mix = 1.0f - current_mix_;
            const float target_output_gain_ = 2.0f; // add +3dB
            float32x4_t out_l = vmlaq_n_f32(vmulq_n_f32(dry_l, inv_mix), sig_l, current_mix_ * target_output_gain_);
            float32x4_t out_r = vmlaq_n_f32(vmulq_n_f32(dry_r, inv_mix), sig_r, current_mix_ * target_output_gain_);

            float32x4x2_t vo = {{ out_l, out_r }};
            vst2q_f32(pOut, vo);
        }

        // Remaining 0-3 samples processed in scalar — important when frames % 4 != 0
        for (uint32_t s = frames & ~3u; s < frames; ++s, pIn += 2, pOut += 2) {
            process_scalar(pIn[0], pIn[1], pOut[0], pOut[1]);
        }
    }

private:
    // =========================================================================
    // Members
    // =========================================================================
    int32_t raw_params_[k_num_params] __attribute__((aligned(16)));
    float   samplerate_;
    bool    b_bypass_;
    std::atomic<bool> b_update_filters_;

    uint8_t model_;
    uint8_t dbx_mode_;

    float target_preamp_, current_preamp_;
    float target_drive_,  current_drive_;
    float target_mix_,    current_mix_;
    float tape_age_;
    float xtalk_amount_;
    float hiss_amount_;
    float wf_phase_inc_;
    float wf_flutter_phase_;
    float wf_flutter_phase_inc_;  // precomputed per-sample increment for 25 Hz flutter

    float32x4_t previous_noise; // for noise shaping the tape hiss (stores the previous output sample to create a 1st-order noise shaping filter)
    float32x4_t pop_env_; // envelope to shape vinyl dust pops (attack-decay, triggered on each pop sample, decays faster with age)
    float       pop_env_decay_; // precomputed per-sample decay increment for the pop envelope
    // State variables for the DC blocker (assumes 4 independent channels)
    float32x4_t x_prev;
    float32x4_t y_prev;

    // R controls the cutoff frequency. 0.995 is a standard coefficient
    // for a pole very close to DC (roughly 20Hz at 44.1kHz).
    float32x4_t R;


    prng_t prng_;

    // Biquad states — L and R fully independent for correct stereo processing
    biquad_state_t eq_low_l_,    eq_low_r_;
    biquad_state_t eq_mid_l_,    eq_mid_r_;
    biquad_state_t eq_high_l_,   eq_high_r_;
    biquad_state_t dbx_pre_l_,   dbx_pre_r_;
    biquad_state_t dbx_de_l_,    dbx_de_r_;
    biquad_state_t tape_head_l_, tape_head_r_;
    biquad_state_t tape_lpf_l_,  tape_lpf_r_;
    biquad_state_t hiss_hpf_l_,  hiss_hpf_r_;
    biquad_state_t dbx_hf_l_,    dbx_hf_r_;    // dbx Type II HF spectral path
    biquad_state_t xtalk_hpf_l_, xtalk_hpf_r_; // frequency-dependent crosstalk

    // dbx RMS envelope states
    float32x4_t dbx_enc_rms_;
    // decode gain is now derived as 1/enc_gain (synchronized) rather than
    // from an independent post-tape RMS tracker.
    float32x4_t dbx_hf_enc_rms_; // HF spectral envelope (encode side)

    float    wf_delay_l_[WF_BUFFER_SIZE] __attribute__((aligned(16)));
    float    wf_delay_r_[WF_BUFFER_SIZE] __attribute__((aligned(16)));
    uint32_t wf_write_pos_;
    float    wf_lfo_phase_;

    // Coefficients (computed in UpdateCoefficients, shared L+R)
    biquad_coeffs_t coeff_eq_low_;
    biquad_coeffs_t coeff_eq_mid_;
    biquad_coeffs_t coeff_eq_high_;
    biquad_coeffs_t coeff_dbx_pre_;
    biquad_coeffs_t coeff_dbx_de_;
    biquad_coeffs_t coeff_tape_head_;
    biquad_coeffs_t coeff_tape_lpf_;
    biquad_coeffs_t coeff_hiss_hpf_;
    biquad_coeffs_t coeff_dbx_hf_;      // HPF ~2 kHz for dbx HF spectral path
    biquad_coeffs_t coeff_xtalk_hpf_;   // HPF ~1.5 kHz for frequency-dep crosstalk

    // =========================================================================
    // Tape saturation via fastertanhf Padé polynomial — not odd-harmonic dominant,
    // softer knee than x/(1+|x|), matches real magnetic saturation character.
    // Clamped at ±5 where tanh(5)≈0.9999 and polynomial error < 0.5%.
    // =========================================================================
    static fast_inline float32x4_t sat_neon(float32x4_t x) {
        const float32x4_t lim = vdupq_n_f32(5.0f);
        x = vmaxq_f32(vminq_f32(x, lim), vnegq_f32(lim));
        // Horner-form Padé rational polynomial (same coefficients as fastertanhf)
        float32x4_t n = vmulq_n_f32(x, 0.3357335044280075e-1f);
        n = vmulq_f32(vaddq_f32(n, vdupq_n_f32(0.583691066395175e-1f)), x);
        n = vmulq_f32(vaddq_f32(n, vdupq_n_f32(0.2468149110712040f)),   x);
        n = vaddq_f32(n, vdupq_n_f32(-0.67436811832e-5f));
        float32x4_t d = vmulq_n_f32(x, 0.2874707922475963e-1f);
        d = vmulq_f32(vaddq_f32(d, vdupq_n_f32(0.1086202599228572f)),   x);
        d = vmulq_f32(vaddq_f32(d, vdupq_n_f32(0.609347197060491e-1f)), x);
        d = vaddq_f32(d, vdupq_n_f32(0.2464845986383725f));
        float32x4_t rcp = vrecpeq_f32(d);
        rcp = vmulq_f32(vrecpsq_f32(d, rcp), rcp);
        return vmulq_f32(n, rcp);
    }

    // =========================================================================
    // Fixed Tape saturation: Enforces odd symmetry and f(0) = 0
    // =========================================================================
    fast_inline float32x4_t sat_neon_new(float32x4_t x, bool enable_dc_blocker = true) {
        const float32x4_t lim = vdupq_n_f32(5.0f);
        x = vmaxq_f32(vminq_f32(x, lim), vnegq_f32(lim));

        // 1. Extract the sign bit and compute absolute value for symmetric evaluation
        uint32x4_t sign_mask = vandq_u32(vreinterpretq_u32_f32(x), vdupq_n_u32(0x80000000));
        float32x4_t abs_x = vabsq_f32(x);

        // 2. Evaluate polynomial using abs_x
        float32x4_t n = vmulq_n_f32(abs_x, 0.3357335044280075e-1f);
        n = vmulq_f32(vaddq_f32(n, vdupq_n_f32(0.583691066395175e-1f)), abs_x);
        n = vmulq_f32(vaddq_f32(n, vdupq_n_f32(0.2468149110712040f)),   abs_x);
        // Changed constant to 0.0f to guarantee f(0) = 0 and stop baseline DC
        n = vaddq_f32(n, vdupq_n_f32(0.0f));

        float32x4_t d = vmulq_n_f32(abs_x, 0.2874707922475963e-1f);
        d = vmulq_f32(vaddq_f32(d, vdupq_n_f32(0.1086202599228572f)),   abs_x);
        d = vmulq_f32(vaddq_f32(d, vdupq_n_f32(0.609347197060491e-1f)), abs_x);
        d = vaddq_f32(d, vdupq_n_f32(0.2464845986383725f));

        float32x4_t rcp = vrecpeq_f32(d);
        rcp = vmulq_f32(vrecpsq_f32(d, rcp), rcp);

        // 3. Compute magnitude, then re-apply the original sign
        float32x4_t mag = vmulq_f32(n, rcp);
        float32x4_t sat_x = vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(mag), sign_mask));

        // 4. Optional DC Blocker
        if (enable_dc_blocker) {
            // Formula: y[n] = x[n] - x[n-1] + R * y[n-1]
            float32x4_t y = vsubq_f32(sat_x, x_prev);
            y = vmlaq_f32(y, R, y_prev);

            x_prev = sat_x;
            y_prev = y;

            return y;
        }

        return sat_x;
    }


    static fast_inline float sat1(float x) {
        if (x >  5.0f) x =  5.0f;
        if (x < -5.0f) x = -5.0f;
        return fastertanhf(x);
    }

    // =========================================================================
    // Scalar path — mirrors the NEON block path for 0-3 remainder samples
    // =========================================================================
    inline void process_scalar(float dry_l, float dry_r, float& out_l, float& out_r) {
        float dec_gain = 1.0f;  // synchronized decode gain; set in encode block below
        float sl = dry_l * current_preamp_;
        float sr = dry_r * current_preamp_;

        sl = biquad_process1(&eq_low_l_,  &coeff_eq_low_,  sl);
        sl = biquad_process1(&eq_mid_l_,  &coeff_eq_mid_,  sl);
        sl = biquad_process1(&eq_high_l_, &coeff_eq_high_, sl);
        sr = biquad_process1(&eq_low_r_,  &coeff_eq_low_,  sr);
        sr = biquad_process1(&eq_mid_r_,  &coeff_eq_mid_,  sr);
        sr = biquad_process1(&eq_high_r_, &coeff_eq_high_, sr);

        if (dbx_mode_ != k_off) {
            sl = biquad_process1(&dbx_pre_l_, &coeff_dbx_pre_, sl);
            sr = biquad_process1(&dbx_pre_r_, &coeff_dbx_pre_, sr);
            // Advance HF filter state for IIR continuity; use block-level envelopes
            (void)biquad_process1(&dbx_hf_l_, &coeff_dbx_hf_, sl);
            (void)biquad_process1(&dbx_hf_r_, &coeff_dbx_hf_, sr);
            float wb_env = fmaxf(vgetq_lane_f32(dbx_enc_rms_,    0), 1e-6f);
            float hf_env = fmaxf(vgetq_lane_f32(dbx_hf_enc_rms_, 0), 1e-6f);
            float gain = fminf(1.0f / sqrtf(wb_env * 0.7f + hf_env * 0.3f), 6.0f);
            dec_gain   = 1.0f / gain;  // synchronized decode gain
            sl *= gain; sr *= gain;
        }

        sl = biquad_process1(&tape_head_l_, &coeff_tape_head_, sl);
        sr = biquad_process1(&tape_head_r_, &coeff_tape_head_, sr);
        {
            const float drive  = 1.0f + current_drive_ * 4.0f;
            const float makeup = 1.0f / (1.0f + current_drive_ * 3.0f);
            sl = sat1(sl * drive) * makeup;
            sr = sat1(sr * drive) * makeup;
        }
        sl = biquad_process1(&tape_lpf_l_, &coeff_tape_lpf_, sl);
        sr = biquad_process1(&tape_lpf_r_, &coeff_tape_lpf_, sr);

        {
            const float lfo_val     = fastersinfullf(wf_lfo_phase_);
            const float flutter_val = fastersinfullf(wf_flutter_phase_);
            wf_lfo_phase_     += wf_phase_inc_;
            wf_flutter_phase_ += wf_flutter_phase_inc_;
            if (wf_lfo_phase_     >= M_TWOPI) wf_lfo_phase_     -= M_TWOPI;
            if (wf_flutter_phase_ >= M_TWOPI) wf_flutter_phase_ -= M_TWOPI;
            const float wow_depth     = 5.0f + tape_age_ * 25.0f;
            const float flutter_depth = 0.8f + tape_age_ * 2.0f;
            const float rd_off = wow_depth * (1.0f + 0.8f * lfo_val)
                               + flutter_depth * flutter_val;

            wf_delay_l_[wf_write_pos_] = sl;
            wf_delay_r_[wf_write_pos_] = sr;
            float    pos = (float)wf_write_pos_ - rd_off + (float)WF_BUFFER_SIZE;
            int32_t  i0  = (int32_t)pos;
            float    fr  = pos - (float)i0;
            uint32_t m0  = (uint32_t)i0 & WF_BUFFER_MASK;
            uint32_t m1  = (m0 + 1u) & WF_BUFFER_MASK;
            sl = wf_delay_l_[m0] + fr * (wf_delay_l_[m1] - wf_delay_l_[m0]);
            sr = wf_delay_r_[m0] + fr * (wf_delay_r_[m1] - wf_delay_r_[m0]);
            wf_write_pos_ = (wf_write_pos_ + 1u) & WF_BUFFER_MASK;
        }

        {
            float bleed_r = biquad_process1(&xtalk_hpf_r_, &coeff_xtalk_hpf_, sr);
            float bleed_l = biquad_process1(&xtalk_hpf_l_, &coeff_xtalk_hpf_, sl);
            float xl = sl + bleed_r * xtalk_amount_;
            float xr = sr + bleed_l * xtalk_amount_;
            sl = xl; sr = xr;
        }

        if (hiss_amount_ > 0.0f && dbx_mode_ != k_active) {
            float n  = vgetq_lane_f32(prng_rand_float(), 0) * 2.0f - 1.0f;
            if (model_ == k_vinyl && vgetq_lane_f32(prng_rand_float(), 0) > 0.9998f)
            {
                n *= 2.5f + vgetq_lane_f32(prng_rand_float(), 2) * 3.0f;
            }
            float fn = biquad_process1(&hiss_hpf_l_, &coeff_hiss_hpf_, n);
            const float hiss_scaled = hiss_amount_ * (1.0f + tape_age_ * 2.0f);
            sl += fn * hiss_scaled;
            sr -= fn * hiss_scaled;
        }

        if (dbx_mode_ == k_active) {
            sl *= dec_gain; sr *= dec_gain;
            sl = biquad_process1(&dbx_de_l_, &coeff_dbx_de_, sl);
            sr = biquad_process1(&dbx_de_r_, &coeff_dbx_de_, sr);
        }

        out_l = dry_l * (1.0f - current_mix_) + sl * current_mix_;
        out_r = dry_r * (1.0f - current_mix_) + sr * current_mix_;
    }

    // =========================================================================
    // Filter coefficient calculators (Audio EQ Cookbook)
    // Called only on parameter change — sinf/cosf/powf are fine here.
    // =========================================================================
    void UpdateCoefficients() {
        const float low_hz  = raw_params_[k_param_eq_low_hz]   * 10.0f;
        const float low_db  = (raw_params_[k_param_eq_low_gain]  - 120.0f) * 0.1f;
        const float mid_hz  = raw_params_[k_param_eq_mid_hz]   * 10.0f;
        const float mid_db  = (raw_params_[k_param_eq_mid_gain]  - 120.0f) * 0.1f;
        const float high_hz = raw_params_[k_param_eq_high_hz]  * 100.0f;
        const float high_db = (raw_params_[k_param_eq_high_gain] - 120.0f) * 0.1f;

        calc_peaking(&coeff_eq_low_,  low_hz,  low_db,  1.0f);
        calc_peaking(&coeff_eq_mid_,  mid_hz,  mid_db,  1.5f);
        calc_peaking(&coeff_eq_high_, high_hz, high_db, 1.0f);

        calc_high_shelf(&coeff_dbx_pre_, 4000.0f, +12.0f);
        calc_high_shelf(&coeff_dbx_de_,  4000.0f, -12.0f);

        float lpf_hz   = 14000.0f;
        float head_hz  = 65.0f;
        float hiss_cut = 3500.0f;

        switch (model_) {
            case k_424:  lpf_hz = 11000.0f; head_hz = 55.0f;  break;
            case k_488:  lpf_hz =  9500.0f; head_hz = 50.0f;  break;
            case k_vinyl: lpf_hz = 12000.0f; head_hz = 150.0f; hiss_cut = 400.0f; break;
            default: break;  // k_244: defaults above
        }

        // Tape age dulls the high end: 100% age drops 14 kHz → 5.6 kHz
        lpf_hz *= (1.0f - tape_age_ * 0.6f);

        calc_peaking(&coeff_tape_head_, head_hz, +4.0f,  1.5f);
        calc_low_pass(&coeff_tape_lpf_,  lpf_hz,  0.707f);
        calc_high_pass(&coeff_hiss_hpf_, hiss_cut, 0.707f);
        calc_high_pass(&coeff_dbx_hf_,   2000.0f,  0.707f); // dbx Type II HF spectral path
        calc_high_pass(&coeff_xtalk_hpf_, 1500.0f,  0.5f);  // treble-biased crosstalk bleed
    }

    void calc_peaking(biquad_coeffs_t* c, float hz, float db, float q) {
        const float A    = powf(10.0f, db / 40.0f);
        const float w0   = 2.0f * M_PI * hz / samplerate_;
        const float cosw = cosf(w0);
        const float alpha = sinf(w0) / (2.0f * q);
        const float inv_a0 = 1.0f / (1.0f + alpha / A);
        c->b0 =  (1.0f + alpha * A) * inv_a0;
        c->b1 = (-2.0f * cosw)      * inv_a0;
        c->b2 =  (1.0f - alpha * A) * inv_a0;
        c->a1 = c->b1;                          // same as b1 (peaking EQ property)
        c->a2 =  (1.0f - alpha / A) * inv_a0;
    }

    void calc_high_shelf(biquad_coeffs_t* c, float hz, float db) {
        const float A    = powf(10.0f, db / 40.0f);
        const float w0   = 2.0f * M_PI * hz / samplerate_;
        const float cosw = cosf(w0), sinw = sinf(w0);
        const float alpha = (sinw / 2.0f) *
                            sqrtf((A + 1.0f/A) * (1.0f/0.707f - 1.0f) + 2.0f);
        const float sqA2   = 2.0f * sqrtf(A) * alpha;
        const float inv_a0 = 1.0f / ((A+1.0f) - (A-1.0f)*cosw + sqA2);
        c->b0 =  A * ((A+1.0f) + (A-1.0f)*cosw + sqA2)   * inv_a0;
        c->b1 = -2.0f*A * ((A-1.0f) + (A+1.0f)*cosw)     * inv_a0;
        c->b2 =  A * ((A+1.0f) + (A-1.0f)*cosw - sqA2)   * inv_a0;
        c->a1 =  2.0f * ((A-1.0f) - (A+1.0f)*cosw)       * inv_a0;
        c->a2 = ((A+1.0f) - (A-1.0f)*cosw - sqA2)        * inv_a0;
    }

    void calc_low_pass(biquad_coeffs_t* c, float hz, float q) {
        const float w0   = 2.0f * M_PI * hz / samplerate_;
        const float cosw = cosf(w0), sinw = sinf(w0);
        const float alpha  = sinw / (2.0f * q);
        const float inv_a0 = 1.0f / (1.0f + alpha);
        c->b0 = (1.0f - cosw) * 0.5f * inv_a0;
        c->b1 = (1.0f - cosw)        * inv_a0;
        c->b2 = c->b0;
        c->a1 = -2.0f * cosw         * inv_a0;
        c->a2 = (1.0f - alpha)       * inv_a0;
    }

    void calc_high_pass(biquad_coeffs_t* c, float hz, float q) {
        const float w0   = 2.0f * M_PI * hz / samplerate_;
        const float cosw = cosf(w0), sinw = sinf(w0);
        const float alpha  = sinw / (2.0f * q);
        const float inv_a0 = 1.0f / (1.0f + alpha);
        c->b0 =  (1.0f + cosw) * 0.5f * inv_a0;
        c->b1 = -(1.0f + cosw)        * inv_a0;
        c->b2 = c->b0;
        c->a1 = -2.0f * cosw          * inv_a0;
        c->a2 = (1.0f - alpha)        * inv_a0;
    }

    // =========================================================================
    // PRNG — Xorshift128+ operating on prng_ member directly
    // =========================================================================
    void prng_init(uint32_t seed) {
        const uint64_t s = seed;
        uint64_t s0[2] = { s,  s * 0x9E3779B97F4A7C15ULL };
        uint64_t s1[2] = { s * 0xBF58476D1CE4E5B9ULL, s * 0x94D049BB133111EBULL };
        prng_.state0 = vld1q_u64(s0);
        prng_.state1 = vld1q_u64(s1);
    }

    uint32x4_t prng_rand_u32() {
        uint64x2_t s0 = prng_.state0, s1 = prng_.state1;
        s1 = veorq_u64(s1, vshlq_n_u64(s1, 23));
        s1 = veorq_u64(s1, vshrq_n_u64(s1, 17));
        s1 = veorq_u64(s1, veorq_u64(s0, vshrq_n_u64(s0, 26)));
        prng_.state0 = s1; prng_.state1 = s0;
        uint64x2_t sum = vaddq_u64(s1, s0);
        return vcombine_u32(vmovn_u64(sum), vshrn_n_u64(sum, 32));
    }

    // Returns 4 independent floats in [0, 1)
    float32x4_t prng_rand_float() {
        uint32x4_t r = prng_rand_u32();
        uint32x4_t m = vorrq_u32(vandq_u32(r, vdupq_n_u32(0x7FFFFFu)),
                                  vdupq_n_u32(0x3F800000u));
        return vsubq_f32(vreinterpretq_f32_u32(m), vdupq_n_f32(1.0f));
    }
};

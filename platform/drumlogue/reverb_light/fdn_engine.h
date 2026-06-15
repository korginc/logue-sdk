#pragma once

#include <arm_neon.h>
#include <float_math.h>
#include <cmath>
#include <cstdlib>
#include <cstring>

#define FDN_CHANNELS 8
#define FDN_BUFFER_SIZE 32768
#define FDN_BUFFER_MASK (FDN_BUFFER_SIZE - 1)
#define PREDELAY_BUFFER_SIZE 16384
#define PREDELAY_MASK (PREDELAY_BUFFER_SIZE - 1)
#define BUFFER_SIZE (4096)
#define NUM_RESONATORS (6)
#define NEON_BLOCKS (4)
#define SAMPLE_RATE (48000.0f)

#ifndef fast_inline
#define fast_inline inline __attribute__((always_inline, optimize("Ofast")))
#endif

// Biquad definitions for standard paths
typedef struct {
    float b0, b1, b2, a1, a2;
} biquad_coeffs_t;

typedef struct {
    float z1, z2;
} biquad_state_t;

// Parallel NEON layout structures for Path 4 (Color Resonators)
typedef struct {
    float32x4_t b0, b1, b2, a1, a2;
} biquad_coeffs_x4_t;

typedef struct {
    float32x4_t z1, z2;
} biquad_state_x4_t;

fast_inline float process_biquad(float in, biquad_state_t* state, biquad_coeffs_t* c) {
    float out = in * c->b0 + state->z1;
    state->z1 = in * c->b1 - out * c->a1 + state->z2;
    state->z2 = in * c->b2 - out * c->a2;
    return out;
}

enum k_parameters {
    k_paramProgram, k_dark, k_bright, k_glow,
    k_color, k_spark, k_size, k_pdly,
    k_decay, k_bass, k_color_shift,
    k_rate, k_irid, k_wdth,
    k_total
};

typedef enum {
    k_stanzaNeon,
    k_vicoBuio,
    k_strobo,
    k_bruciato,
    k_preset_number,
} preset_numer_t;

// ============================================================================
// Presets
// ============================================================================
static const char* k_preset_names[k_preset_number] = {
    "StanzaNeon", // 0: Tight, bright, standard drum room
    "VicoBuio",   // 1: Long decay, heavy LPF, spooky
    "Strobo",     // 2: High pre-delay, short decay, heavily modulated
    "Bruciato"    // 3: Massive size, max decay, floating
};

// Values:
//  { NAME, DARK, BRIG, GLOW, COLR, SPRK, SIZE, PDLY, DCAY, BASS, CLRQ, RATE, IRID, WDTH }
static const int32_t k_presets[k_preset_number][k_total] = {
    { k_stanzaNeon, 20, 50, 10,  0,  5, 30,  5,  65,  30,  0, 20,  0, 70 },  // StanzaNeon: medium decay, wide
    { k_vicoBuio,   50, 20,  0, 10,  0, 60, 15,  85,  10,  0, 10, 15, 50 },  // VicoBuio:   long dark, iridiscence
    { k_strobo,     20, 10, 20, 20, 20, 20, 80,  45,  50,  0, 40,  0,100 },  // Strobo:     short, fast LFO, wide
    { k_bruciato,   70,  0, 40, 10,  0, 90, 10,  95,  20,  0, 15, 30, 60 }   // Bruciato:   massive, iridiscence
};

// ============================================================================
// Main Class
// ============================================================================
class FDNEngine {
public:
    FDNEngine() : sampleRate(SAMPLE_RATE) {
        initialized = false;
        Reset();
    }

    // ========================================================================
    // NEW PARALLEL PARAMETERS
    // ========================================================================
    float dark_amt = 10.0f;
    float glow_amt = 10.0f;
    float bright_amt = 10.0f;
    float color_amt = 10.0f;
    float spark_amt = 10.0f;

    float sizeScale = 1.0f;
    float predelayScale = 0.0f;
    float decay = 0.8f;         // Feedback gain: 0.1 (short) .. 0.98 (infinite)
    float hpf_coeff = 0.95f;    // Per-channel one-pole HPF coefficient: 0.85..0.99

    // ========================================================================
    // STATE VARIABLES
    // ========================================================================
    // FDN Core States (Aligned for 128-bit vector processing)
    float fdnMem[FDN_CHANNELS * FDN_BUFFER_SIZE] __attribute__((aligned(16)));
    float baseDelayTimes[FDN_CHANNELS] __attribute__((aligned(16)));
    float fdnState[FDN_CHANNELS] __attribute__((aligned(16)));
    int writePos = 0;


    // internal parameters
    int32_t params_[k_total] __attribute__((aligned(16)));
    int32_t current_preset_ = 0;

    // Vectorized One-Pole HPF state arrays  (applied to each delay output to cut bass buildup)
    float hpf_x_prev[FDN_CHANNELS] __attribute__((aligned(16)));    // previous input sample
    float hpf_y_prev[FDN_CHANNELS] __attribute__((aligned(16)));    // previous output sample

    // Predelay
    float preDelayBuffer[PREDELAY_BUFFER_SIZE] __attribute__((aligned(16)));
    int preDelayWritePos = 0;

    // Path 1: Glow (Modulated Chamberlin SVF)
    float glow_lfo_phase = 0.0f;
    float glow_lp_l = 0.0f;
    float glow_bp_l = 0.0f;
    float glow_lp_r = 0.0f;
    float glow_bp_r = 0.0f;

    // Path 2: Dark (Organic Granular Sub-Octave)
    float dark_buffer[BUFFER_SIZE] __attribute__((aligned(16)));
    int dark_write = 0;
    float dark_phase = 0.0f;
    float dark_lpf_state = 0.0f;

    // Path 3: Bright (Harmonic Exciter States)
    biquad_state_t  bright_hpf_l;
    biquad_state_t  bright_hpf_r;
    biquad_coeffs_t bright_coeffs;

    // Path 4: Color (6 Visual Spectrum Resonators) (Optimized Structure-of-Arrays Layout for NEON SIMD)
    biquad_state_x4_t  color_filters_l_neon[2] __attribute__((aligned(16)));
    biquad_state_x4_t  color_filters_r_neon[2] __attribute__((aligned(16)));
    biquad_coeffs_x4_t color_coeffs_neon[2]    __attribute__((aligned(16)));

    // Path 5: Sparkle (Stereo Granular S&H)
    float sparkle_buffer_l[BUFFER_SIZE] __attribute__((aligned(16)));
    float sparkle_buffer_r[BUFFER_SIZE] __attribute__((aligned(16)));
    int spark_write = 0;
    float spark_read = 0.0f;
    float spark_speed = 2.0f;
    float spark_pan_l = 0.5f;
    float spark_pan_r = 0.5f;
    int spark_countdown = 0;
    int spark_duration = 0;   // total grain length for envelope computation

    float sampleRate;
    float inverseSampleRate;
    float glowLfoRate = 0.0f;
    float glow_rate_hz = 0.4f;
    float irid_amt = 0.0f;
    float width_amt = 1.0f;
    bool initialized;

    // Path 6: Iridescence (Granular Octave-Up)
    float iridiscensce_buffer[BUFFER_SIZE] __attribute__((aligned(16)));
    int   iridiscensce_write = 0;
    float irid_phase = 0.0f;
    float irid_lpf_l = 0.0f;
    float irid_lpf_r = 0.0f;

    // ========================================================================
    // INITIALIZATION & MATH
    // ========================================================================
    bool init(float sr) {
        sampleRate = sr;
        inverseSampleRate = 1 / sampleRate;


        // Base prime delay times for 8 channels
        const float primes[FDN_CHANNELS] = {1103.0f, 1511.0f, 1999.0f, 2503.0f, 3011.0f, 3511.0f, 3989.0f, 4513.0f};
        for (int i = 0; i < FDN_CHANNELS; i++) {
            baseDelayTimes[i] = primes[i] * (sampleRate / SAMPLE_RATE);
        }

        // LFO rate: controlled by RATE parameter (default 0.4 Hz).
        // Depth scales with glow_amt so GLOW=0 → no filter modulation.
        glowLfoRate = glow_rate_hz * inverseSampleRate;

        update_color_resonators(1.0f);
        initialize_brightness_harmonic_exciter();
        Reset();
        initialized = true;
        return true;
    }

    // 5kHz Butterworth HPF
    void initialize_brightness_harmonic_exciter() {
        // NOTE since it's at initialization stage, no fast approximation used
        float w0_bright = 2.0f * M_PI * 5000.0f * inverseSampleRate;
        float alpha_bright = sinf(w0_bright) / (2.0f * 0.707f);
        float a0_bright = 1.0f + alpha_bright;

        bright_coeffs.b0 = ((1.0f + cosf(w0_bright)) * 0.5f) / a0_bright;
        bright_coeffs.b1 = -(1.0f + cosf(w0_bright)) / a0_bright;
        bright_coeffs.b2 = ((1.0f + cosf(w0_bright)) * 0.5f) / a0_bright;
        bright_coeffs.a1 = (-2.0f * cosf(w0_bright)) / a0_bright;
        bright_coeffs.a2 = (1.0f - alpha_bright) / a0_bright;
    }

    // TODO - possibly use LFO for this?
    // shift_factor from the UI: e.g., 0.5f to 2.0f (-1 octave to +1 octave)
    void update_color_resonators(float shift_factor) {
        // EXACT VISUAL SPECTRUM FREQUENCIES (in Hz)
        const float base_freqs[NUM_RESONATORS] = {4100.0f, 5000.0f, 5200.0f, 5800.0f, 6600.0f, 7200.0f};

        // Try possibly a value of Q: 8.0f to makes them "ring" like physical modal resonators.
        // In case 4.0f is too wide/damped to sound like a distinct pitch.
        const float Q = 6.0f + shift_factor * 2.0f;

        float tmp_b0[8] = {0}, tmp_b1[8] = {0}, tmp_b2[8] = {0}, tmp_a1[8] = {0}, tmp_a2[8] = {0};

        for (int i = 0; i < NUM_RESONATORS; i++) {
            // Apply the UI shift
            float hz = base_freqs[i] * shift_factor;

            // Safety Clamp: Prevent high frequencies from crashing the filter near Nyquist
            if (hz > sampleRate * 0.45f) hz = sampleRate * 0.45f;

            // Constant Peak Gain Bandpass Biquad Math
            float w0 = 2.0f * M_PI * hz * inverseSampleRate;
            float alpha = fastersinfullf(w0) / (2.0f * Q);
            float a0 = 1.0f + alpha;

            tmp_b0[i] = alpha / a0;
            tmp_b1[i] = 0.0f;
            tmp_b2[i] = -alpha / a0;
            tmp_a1[i] = -2.0f * fastercosfullf(w0) / a0;
            tmp_a2[i] = (1.0f - alpha) / a0;
        }
        // Slots 6 & 7 remain initialized to 0 (padded silence filters)

        for (int block = 0; block < 2; block++) {
            color_coeffs_neon[block].b0 = vld1q_f32(&tmp_b0[block * NEON_BLOCKS]);
            color_coeffs_neon[block].b1 = vld1q_f32(&tmp_b1[block * NEON_BLOCKS]);
            color_coeffs_neon[block].b2 = vld1q_f32(&tmp_b2[block * NEON_BLOCKS]);
            color_coeffs_neon[block].a1 = vld1q_f32(&tmp_a1[block * NEON_BLOCKS]);
            color_coeffs_neon[block].a2 = vld1q_f32(&tmp_a2[block * NEON_BLOCKS]);
        }
    }

    // Public reset — called by unit_reset()
    void reset() { Reset(); }

    void Reset() {
        memset(fdnMem, 0, sizeof(fdnMem));
        memset(fdnState, 0, sizeof(fdnState));
        memset(preDelayBuffer, 0, sizeof(preDelayBuffer));
        memset(color_filters_l_neon, 0, sizeof(color_filters_l_neon));
        memset(color_filters_r_neon, 0, sizeof(color_filters_r_neon));
        memset(sparkle_buffer_l, 0, sizeof(sparkle_buffer_l));
        memset(sparkle_buffer_r, 0, sizeof(sparkle_buffer_r));
        writePos = 0;
        preDelayWritePos = 0;
        memset(dark_buffer, 0, sizeof(dark_buffer));
        dark_write = 0;
        dark_phase = 0.0f;
        dark_lpf_state = 0.0f;
        memset(&bright_hpf_l, 0, sizeof(biquad_state_t));
        memset(&bright_hpf_r, 0, sizeof(biquad_state_t));
        glow_lfo_phase = 0.0f;
        glow_lp_l = 0.0f; glow_bp_l = 0.0f;
        glow_lp_r = 0.0f; glow_bp_r = 0.0f;
        memset(hpf_x_prev, 0, sizeof(hpf_x_prev));
        memset(hpf_y_prev, 0, sizeof(hpf_y_prev));
        memset(iridiscensce_buffer, 0, sizeof(iridiscensce_buffer));
        iridiscensce_write = 0;
        irid_phase = 0.0f;
        irid_lpf_l = 0.0f; irid_lpf_r = 0.0f;
    }

    /*===========================================================================*/
    /* Parameter Interface */
    /*===========================================================================*/

    inline void loadPreset(uint8_t index) {
        if (index >= k_preset_number) return;
        if (current_preset_ != index){
            current_preset_ = index;
            for (uint8_t i = 0; i < k_total; i++) {
                setParameter(i, k_presets[index][i]);
            }
        }
    }

    inline int32_t getPreset() { return current_preset_; }
    inline int32_t getParameterValue(uint8_t index) { return (index >= k_total) ? -1 : params_[index]; }

    inline void setParameter(uint8_t index, int32_t value) {
        if (index >= k_total) return;
        params_[index] = value;   // store into local DB

        const float norm = value * 0.01f; // 0..100 → 0.0..1.0


        switch (index) {
            case k_paramProgram: loadPreset(value); break;
            case k_dark:        setDarkness(norm); break;
            case k_bright:      setBrightness(norm); break;
            case k_glow:        setGlow(norm); break;
            case k_color:       setColor(norm); break;
            case k_spark:       setSpark(norm); break;
            case k_size:        setSize(norm); break;
            case k_pdly:        setPreDelay(norm); break;
            case k_decay:       setDecay(norm); break;
            case k_bass:        setHpfCoeff(norm); break;
            // Base-2 exponential mapping: 2^norm
            // val = -1.0  -> 0.5x multiplier (-1 Octave)
            // val =  0.0  -> 1.0x multiplier (No shift)
            // val = +1.0  -> 2.0x multiplier (+1 Octave)
            case k_color_shift: update_color_resonators(fasterpow2f(norm)); break;
            case k_rate:        setRate(norm); break;
            case k_irid:        setIridiscence(norm); break;
            case k_wdth:        setWidth(norm); break;
            default: break;
        }
    }

    //==============
    // Setters  (all take normalised float 0.0-1.0)
    //==============
    void setDarkness(float val) { dark_amt = val; }
    void setBrightness(float val) { bright_amt = val; }
    void setGlow(float val) { glow_amt = val; }
    void setColor(float val) { color_amt = val; }
    void setSpark(float val) { spark_amt = val; }
    // val is normalised 0.0-1.0; mapped to sizeScale 0.1-2.0
    void setSize(float val) { sizeScale = 0.1f + val * 1.9f; }
    // val is normalised 0.0-1.0; mapped to up to ~330ms pre-delay
    void setPreDelay(float val) { predelayScale = val; }
    // val normalised 0.0-1.0; maps to feedback gain 0.1..0.98 (short to near-infinite)
    void setDecay(float val) { decay = 0.1f + val * 0.88f; }
    // val normalised 0.0-1.0; maps HPF cutoff from minimal (0.99) to moderate (0.85).
    // The one-pole HPF formula is: y[n] = x[n] - x[n-1] + coeff * y[n-1]
    // coeff = 0.99 → fc ≈ 76 Hz (just DC blocking)
    // coeff = 0.85 → fc ≈ 1146 Hz (removes bass buildup in dense reverb tails)
    // Higher knob value = more bass removed from the reverb tail.
    void setHpfCoeff(float val) { hpf_coeff = 0.99f - (val * 0.14f); }
    // 0.0..1.0 → 0.05..4.0 Hz via 2^(val*6)*0.05 (exponential for musical feel)
    void setRate(float val) {
        glow_rate_hz = 0.05f * fasterpow2f(val * 6.0f);
        glowLfoRate = glow_rate_hz * inverseSampleRate;
    }
    void setIridiscence(float val) { irid_amt = val; }
    // 0.0..1.0 → 0.0..2.0 (0=mono, 1=unity, 2=extra wide)
    void setWidth(float val) { width_amt = val * 2.0f; }

    // ========================================================================
    // ACCELERATED CORE FDN (SIMD One-Pole HPF & Radix-2 Fast Hadamard)
    // ========================================================================
    fast_inline void step_core_fdn(float in_l, float in_r, float* out_l, float* out_r) {
        float fdnOut[FDN_CHANNELS] __attribute__((aligned(16)));

        // 1. Read and Interpolate from Delay Lines (Keep scalar to avoid non-contiguous memory gather overhead)
        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            float dt = baseDelayTimes[ch] * sizeScale;
            float readPos = (float)writePos - dt;
            if (readPos < 0.0f) readPos += FDN_BUFFER_SIZE;

            int idx1 = (int)readPos & FDN_BUFFER_MASK;
            int idx2 = (idx1 + 1) & FDN_BUFFER_MASK;
            float frac = readPos - (float)(int)readPos;

            float val1 = fdnMem[ch * FDN_BUFFER_SIZE + idx1];
            float val2 = fdnMem[ch * FDN_BUFFER_SIZE + idx2];
            fdnOut[ch] = val1 + frac * (val2 - val1);
        }

        // 2. Vectorized 1-Pole HPF Execution (4 Channels at a Time)
        // Apply per-channel one-pole HPF to kill DC and bass buildup in the tail.
        //    Formula: y[n] = x[n] - x[n-1] + hpf_coeff * y[n-1]
        float32x4_t v_hpf_coeff = vdupq_n_f32(hpf_coeff);

        // Channels 0-3
        float32x4_t v_x0 = vld1q_f32(&fdnOut[0]);
        float32x4_t v_xp0 = vld1q_f32(&hpf_x_prev[0]);
        float32x4_t v_yp0 = vld1q_f32(&hpf_y_prev[0]);
        float32x4_t v_y0 = vsubq_f32(v_x0, v_xp0);
        v_y0 = vmlaq_f32(v_y0, v_hpf_coeff, v_yp0);
        vst1q_f32(&hpf_x_prev[0], v_x0);
        vst1q_f32(&hpf_y_prev[0], v_y0);
        vst1q_f32(&fdnOut[0], v_y0);

        // Channels 4-7
        float32x4_t v_x1 = vld1q_f32(&fdnOut[NEON_BLOCKS]);
        float32x4_t v_xp1 = vld1q_f32(&hpf_x_prev[NEON_BLOCKS]);
        float32x4_t v_yp1 = vld1q_f32(&hpf_y_prev[NEON_BLOCKS]);
        float32x4_t v_y1 = vsubq_f32(v_x1, v_xp1);
        v_y1 = vmlaq_f32(v_y1, v_hpf_coeff, v_yp1);
        vst1q_f32(&hpf_x_prev[NEON_BLOCKS], v_x1);
        vst1q_f32(&hpf_y_prev[NEON_BLOCKS], v_y1);
        vst1q_f32(&fdnOut[NEON_BLOCKS], v_y1);

        // 3. Stereo Mixdown Mix
        *out_l = fdnOut[0] + fdnOut[1] + fdnOut[2] + fdnOut[3];
        *out_r = fdnOut[4] + fdnOut[5] + fdnOut[6] + fdnOut[7];

        // 4. Radix-2 SIMD Fast Hadamard Transform (FHT)
        // Eliminates 64 matrix multiplications down to pure register shuffles

        // Stage 1 (Stride 4)
        float32x4_t s1_u = vaddq_f32(v_y0, v_y1);
        float32x4_t s1_l = vsubq_f32(v_y0, v_y1);

        // Stage 2 (Stride 2)
        float32x4_t s1_u_flip = vcombine_f32(vget_high_f32(s1_u), vget_low_f32(s1_u));
        float32x4_t s2_sum_u  = vaddq_f32(s1_u, s1_u_flip);
        float32x4_t s2_diff_u = vsubq_f32(s1_u, s1_u_flip);
        float32x4_t s2_u      = vcombine_f32(vget_low_f32(s2_sum_u), vget_low_f32(s2_diff_u));

        float32x4_t s1_l_flip = vcombine_f32(vget_high_f32(s1_l), vget_low_f32(s1_l));
        float32x4_t s2_sum_l  = vaddq_f32(s1_l, s1_l_flip);
        float32x4_t s2_diff_l = vsubq_f32(s1_l, s1_l_flip);
        float32x4_t s2_l      = vcombine_f32(vget_low_f32(s2_sum_l), vget_low_f32(s2_diff_l));

        // Stage 3 (Stride 1)
        float32x4_t s2_u_rev = vrev64q_f32(s2_u);
        float32x4_t s3_sum_u = vaddq_f32(s2_u, s2_u_rev);
        float32x4_t s3_diff_u = vsubq_f32(s2_u, s2_u_rev);
        float32x4x2_t trn_u  = vtrnq_f32(s3_sum_u, s3_diff_u);
        float32x4_t final_u  = trn_u.val[0];

        float32x4_t s2_l_rev = vrev64q_f32(s2_l);
        float32x4_t s3_sum_l = vaddq_f32(s2_l, s2_l_rev);
        float32x4_t s3_diff_l = vsubq_f32(s2_l, s2_l_rev);
        float32x4x2_t trn_l  = vtrnq_f32(s3_sum_l, s3_diff_l);
        float32x4_t final_l  = trn_l.val[0];

        // 5. Apply Input Injection & Scaling Matrix-normalization via fused MAC
        float32x4_t v_in_l = vdupq_n_f32(in_l);
        float32x4_t v_in_r = vdupq_n_f32(in_r);
        float32x4_t v_decay = vdupq_n_f32(decay);

        float32x4_t v_fb_u = vmlaq_f32(v_in_l, final_u, v_decay);
        float32x4_t v_fb_l = vmlaq_f32(v_in_r, final_l, v_decay);

        // Scatter back to non-interleaved FDN memory
        fdnMem[0 * FDN_BUFFER_SIZE + writePos] = vgetq_lane_f32(v_fb_u, 0);
        fdnMem[1 * FDN_BUFFER_SIZE + writePos] = vgetq_lane_f32(v_fb_u, 1);
        fdnMem[2 * FDN_BUFFER_SIZE + writePos] = vgetq_lane_f32(v_fb_u, 2);
        fdnMem[3 * FDN_BUFFER_SIZE + writePos] = vgetq_lane_f32(v_fb_u, 3);
        fdnMem[4 * FDN_BUFFER_SIZE + writePos] = vgetq_lane_f32(v_fb_l, 0);
        fdnMem[5 * FDN_BUFFER_SIZE + writePos] = vgetq_lane_f32(v_fb_l, 1);
        fdnMem[6 * FDN_BUFFER_SIZE + writePos] = vgetq_lane_f32(v_fb_l, 2);
        fdnMem[7 * FDN_BUFFER_SIZE + writePos] = vgetq_lane_f32(v_fb_l, 3);

        writePos = (writePos + 1) & FDN_BUFFER_MASK;
    }

    // ========================================================================
    // PARALLEL AUDIO BLOCK PROCESSOR
    // ========================================================================
    void processBlock(const float* in, float* out, int num_samples) {
        if (!initialized) return;

        #if defined(__arm__) || defined(__aarch64__)
            uint32_t fpscr;
            __asm__ volatile("vmrs %0, fpscr" : "=r"(fpscr));
            fpscr |= (1 << 24) | (1 << 22); // Fast flush-to-zero enforcement
            __asm__ volatile("vmsr fpscr, %0" : : "r"(fpscr));
        #endif

        int preDelaySamps = (int)(predelayScale * 16000.0f);
        float path_sum = dark_amt + glow_amt + bright_amt + color_amt + spark_amt + irid_amt;
        float path_norm = path_sum > 0.0f ? (1.0f / fmaxf(1.0f, path_sum)) : 0.0f;

        for (int i = 0; i < num_samples; i += 2) {
            float in_l = in[i];
            float in_r = in[i+1];

            // PREDELAY
            float mono_in = (in_l + in_r) * 0.5f;
            preDelayBuffer[preDelayWritePos] = mono_in;
            int pd_read = (preDelayWritePos - preDelaySamps + PREDELAY_BUFFER_SIZE) & PREDELAY_MASK;
            float pd_sig = preDelayBuffer[pd_read];
            preDelayWritePos = (preDelayWritePos + 1) & PREDELAY_MASK;

            // 1. CORE FDN (Pure acoustic delays)
            float rev_l, rev_r;
            step_core_fdn(pd_sig, pd_sig, &rev_l, &rev_r);

            // Now 5 parallel paths to be summed up at the end


            // ==========================================
            // PATH 1: GLOW (Stereo Swirling SVF)
            // ==========================================
            float glow_l = 0.0f;
            float glow_r = 0.0f;
            if (glow_amt > 0.0f) {
                glow_lfo_phase += glowLfoRate;
                if (glow_lfo_phase > 1.0f) glow_lfo_phase -= 1.0f;

                // Left Channel: Modulate coefficient directly.
                // f_coeff = 0.15 (~1150Hz base) +/- (0.10 * glow_amt) depth
                // 2. Shift the sweep up slightly:
                // Base 0.18 +/- 0.08 keeps the sweep entirely out of the muddy bass frequencies
                float lfo_val_l = fastersinfullf(glow_lfo_phase);
                float f_coeff_l = 0.18f + (0.08f * glow_amt * lfo_val_l);
                float q_coeff   = 0.8f; // Mild resonance to accentuate the sweep

                glow_lp_l += f_coeff_l * glow_bp_l;
                glow_bp_l += f_coeff_l * (rev_l - glow_lp_l - q_coeff * glow_bp_l);
                glow_l = glow_lp_l; // Take the Low-Pass output for warmth

                // Right Channel: 90-degree phase offset for stereo widening
                float phase_r = glow_lfo_phase + 0.25f;
                if (phase_r > 1.0f) phase_r -= 1.0f;

                float lfo_val_r = fastersinfullf(phase_r);
                float f_coeff_r = 0.15f + (0.10f * glow_amt * lfo_val_r);

                glow_lp_r += f_coeff_r * glow_bp_r;
                glow_bp_r += f_coeff_r * (rev_r - glow_lp_r - q_coeff * glow_bp_r);
                glow_r = glow_lp_r;
            }

            // ==========================================
            // PATH 2: DARK (Organic Pitch-Shifted Mono Sub-Bass)
            // ==========================================
            float dark_sig = 0.0f;
            float rev_mono = (rev_l + rev_r) * 0.5f;
            if (dark_amt > 0.0f) {
                // 1. Write the mono reverb tail to the dark buffer
                dark_buffer[dark_write] = rev_mono;
                dark_write = (dark_write + 1) & 4095;

                // 2. Advance the pitch shift phase (0.5 = exactly 1 octave down)
                dark_phase += 0.5f;
                if (dark_phase >= 2048.0f) dark_phase -= 2048.0f; // 42ms grain window

                // 3. Read Head 1 (Calculates interpolated audio)
                float r1 = (float)dark_write - dark_phase;
                if (r1 < 0.0f) r1 += (float)BUFFER_SIZE;
                int i1 = (int)r1;
                float f1 = r1 - i1;
                float out1 = dark_buffer[i1 & 4095] + f1 * (dark_buffer[(i1 + 1) & 4095] - dark_buffer[i1 & 4095]);

                // 4. Read Head 2 (Offset by exactly half the window to hide the looping click)
                // Replace fmodf (expensive libc call) with conditional subtraction.
                // dark_phase ∈ [0, 2048), so dark_phase + 1024 ∈ [1024, 3072) → at most one wrap.
                float dp2 = dark_phase + 1024.0f;
                if (dp2 >= 2048.0f) dp2 -= 2048.0f;
                float r2 = (float)dark_write - dp2;
                if (r2 < 0.0f) r2 += (float)BUFFER_SIZE;
                int i2 = (int)r2;
                float f2 = r2 - i2;
                float out2 = dark_buffer[i2 & 4095] + f2 * (dark_buffer[(i2 + 1) & 4095] - dark_buffer[i2 & 4095]);

                // 5. Crossfade Envelope (Triangle wave tracking the phase)
                // Head 1 is at maximum volume halfway through its window, and muted when it snaps back.
                float fade1 = 1.0f - fabsf((dark_phase - 1024.0f) / 1024.0f);
                float fade2 = 1.0f - fade1;

                float pitched_down = (out1 * fade1) + (out2 * fade2);

                // 6. Smooth Low-Pass Filter (Removes any granular artifacts and leaves pure, deep sub)
                // A coefficient of 0.05f creates a heavy low-pass around ~300Hz
                dark_lpf_state += 0.05f * (pitched_down - dark_lpf_state);

                // 7. Final output with makeup gain to compensate for the heavy filtering
                dark_sig = dark_lpf_state * 2.5f;
            }

            // ==========================================
            // PATH 3: BRIGHT (Harmonic Exciter Air)
            // ==========================================
            float bright_l = 0.0f;
            float bright_r = 0.0f;
            if (bright_amt > 0.0f) {
                // 1. Isolate the extreme highs using the 2nd-order Butterworth HPF
                float hp_l = process_biquad(rev_l, &bright_hpf_l, &bright_coeffs);
                float hp_r = process_biquad(rev_r, &bright_hpf_r, &bright_coeffs);

                // 2. Drive the isolated highs to prepare for saturation
                // Clamp to prevent polynomial foldback explosion
                float drive_l = fmaxf(-1.0f, fminf(1.0f, hp_l * 4.0f));
                float drive_r = fmaxf(-1.0f, fminf(1.0f, hp_r * 4.0f));

                // 3. Polynomial soft-clipping (x - x^3/3)
                // This squashes the peaks, synthesizing beautiful 2nd and 3rd order "sizzle"
                bright_l = drive_l * (1.0f - (drive_l * drive_l * 0.33333f));
                bright_r = drive_r * (1.0f - (drive_r * drive_r * 0.33333f));
            }

            // ==========================================
            // PATH 4: COLOR (Structure-of-Arrays Parallel Stereo Visual Spectrum Resonators)
            // ==========================================
            // Drive from the FDN reverb output so the resonators colour the
            // reverb tail rather than the dry signal.  Driving from in_l/in_r
            // produced no audible effect for low-mid drum content because those
            // signals carry negligible energy in the 4-7 kHz resonator band.
            // The reverb tail has broad-band content and reliably excites the
            // high-Q resonators, creating a metallic / spring-like colouring.
            float color_l = 0.0f;
            float color_r = 0.0f;
            if (color_amt > 0.0f) {
                float32x4_t v_in_l = vdupq_n_f32(rev_l);
                float32x4_t v_in_r = vdupq_n_f32(rev_r);
                float32x4_t v_acc_l = vdupq_n_f32(0.0f);
                float32x4_t v_acc_r = vdupq_n_f32(0.0f);

                // Run 8 filters (6 valid, 2 zero-padded) completely inside SIMD vector registers
                for (int b = 0; b < 2; b++) {
                    // Left Resonators processing block
                    float32x4_t v_out_l = vmlaq_f32(color_filters_l_neon[b].z1, v_in_l, color_coeffs_neon[b].b0);
                    float32x4_t v_z1_n_l = vsubq_f32(vmulq_f32(v_in_l, color_coeffs_neon[b].b1), vmulq_f32(v_out_l, color_coeffs_neon[b].a1));
                    color_filters_l_neon[b].z1 = vaddq_f32(v_z1_n_l, color_filters_l_neon[b].z2);
                    color_filters_l_neon[b].z2 = vsubq_f32(vmulq_f32(v_in_l, color_coeffs_neon[b].b2), vmulq_f32(v_out_l, color_coeffs_neon[b].a2));
                    v_acc_l = vaddq_f32(v_acc_l, v_out_l);

                    // Right Resonators processing block
                    float32x4_t v_out_r = vmlaq_f32(color_filters_r_neon[b].z1, v_in_r, color_coeffs_neon[b].b0);
                    float32x4_t v_z1_n_r = vsubq_f32(vmulq_f32(v_in_r, color_coeffs_neon[b].b1), vmulq_f32(v_out_r, color_coeffs_neon[b].a1));
                    color_filters_r_neon[b].z1 = vaddq_f32(v_z1_n_r, color_filters_r_neon[b].z2);
                    color_filters_r_neon[b].z2 = vsubq_f32(vmulq_f32(v_in_r, color_coeffs_neon[b].b2), vmulq_f32(v_out_r, color_coeffs_neon[b].a2));
                    v_acc_r = vaddq_f32(v_acc_r, v_out_r);
                }

                // Horizontal vector accumulation down to stereo float scalars
                float tmp_l[NEON_BLOCKS], tmp_r[NEON_BLOCKS];
                vst1q_f32(tmp_l, v_acc_l);
                vst1q_f32(tmp_r, v_acc_r);
                color_l = tmp_l[0] + tmp_l[1] + tmp_l[2] + tmp_l[3];
                color_r = tmp_r[0] + tmp_r[1] + tmp_r[2] + tmp_r[3];
            }

            // ==========================================
            // PATH 5: SPARKLE (Stereo Pitched-up S&H Pops)
            // ==========================================
            float spark_l = 0.0f;
            float spark_r = 0.0f;
            if (spark_amt > 0.0f) {
                sparkle_buffer_l[spark_write] = rev_l;
                sparkle_buffer_r[spark_write] = rev_r;
                spark_write = (spark_write + 1) & (BUFFER_SIZE - 1);

                if (spark_countdown > 0) {
                    int r_idx = (int)spark_read & (BUFFER_SIZE - 1);
                    // Parabolic envelope: rises and falls over the grain duration.
                    // env = 4 * pos * (1 - pos)  where pos runs 0→1 over the grain.
                    float pos = 1.0f - (float)spark_countdown / (float)spark_duration;
                    float env = 4.0f * pos * (1.0f - pos);
                    spark_l = sparkle_buffer_l[r_idx] * spark_pan_l * env;
                    spark_r = sparkle_buffer_r[r_idx] * spark_pan_r * env;
                    spark_read += spark_speed;
                    spark_countdown--;
                } else {
                    // Xorshift inline PRNG
                    static uint32_t seed = 2463574242UL;
                    seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
                    float rand_val = (float)seed / 4294967295.0f;

                    if (rand_val < (0.0001f + (spark_amt * 0.005f))) {
                        // Halved grain duration: 5-15 ms (was 10-30 ms)
                        spark_duration = 250 + (int)(seed % 500);
                        spark_countdown = spark_duration;
                        spark_read = (float)spark_write - (float)spark_countdown;
                        if(spark_read < 0.0f) spark_read += (float)BUFFER_SIZE;

                        // More varied pitch ratios: +5, +7, +12, +19, +24 semitones
                        static const float kSpeeds[5] = {1.41f, 1.68f, 2.0f, 2.83f, 4.0f};
                        spark_speed = kSpeeds[seed % 5];

                        spark_pan_l = (float)(seed % 100) * 0.01f;
                        spark_pan_r = 1.0f - spark_pan_l;
                    }
                }
            }

            // ==========================================
            // PATH 6: IRIDESCENZA (Swirling Optical Halo)
            // ==========================================
            // A swirling, stereo-panning, saturated optical halo.
            float irid_l = 0.0f;
            float irid_r = 0.0f;
            if (irid_amt > 0.0f) {
                // 1. Refraction: Speed wobbles microscopically around .9x
                // Assuming you have an LFO available (like the one used in GLOW)
                float irid_speed = 0.9f + (fastersinfullf(glow_lfo_phase * 0.5f) * 0.015f);
                irid_phase += irid_speed;
                if (irid_phase >= 2048.0f) irid_phase -= 2048.0f;

                iridiscensce_buffer[iridiscensce_write] = rev_mono;
                iridiscensce_write = (iridiscensce_write + 1) & 4095;

                float rs1 = (float)iridiscensce_write - irid_phase;
                if (rs1 < 0.0f) rs1 += (float)BUFFER_SIZE;
                int is1 = (int)rs1;
                float fs1 = rs1 - is1;
                float so1 = iridiscensce_buffer[is1 & 4095] + fs1 * (iridiscensce_buffer[(is1+1) & 4095] - iridiscensce_buffer[is1 & 4095]);

                float phase_b = irid_phase + 1024.0f;
                if (phase_b >= 2048.0f) phase_b -= 2048.0f;
                float rs2 = (float)iridiscensce_write - phase_b;
                if (rs2 < 0.0f) rs2 += (float)BUFFER_SIZE;
                int is2 = (int)rs2;
                float fs2 = rs2 - is2;
                float so2 = iridiscensce_buffer[is2 & 4095] + fs2 * (iridiscensce_buffer[(is2+1) & 4095] - iridiscensce_buffer[is2 & 4095]);

                // 2. The "Holo-Fade"
                float fade = 1.0f - fabsf((irid_phase - 1024.0f) / 1024.0f);

                // 3. Chromatic Aberration (Stereo Splitting)
                // Instead of summing to mono, Head 1 favors Left and Head 2 favors Right!
                // As they fade in and out, the iridiscence swirls across the stereo image.
                float irid_raw_l = (so1 * fade) + (so2 * (1.0f - fade) * 0.29f);
                float irid_raw_r = (so2 * fade) + (so1 * (1.0f - fade) * 0.31f);

                // 4. Luminescence (Soft Saturation)
                // Pushing it into a fast_tanh creates high-frequency density ("glow")
                // Asymmetric drive for harmonic stereo widening
                irid_raw_l = fast_tanh(irid_raw_l * 1.359f);
                irid_raw_r = fast_tanh(irid_raw_r * 1.703f);

                // 5. Taming the harshness (Stereo LPF)
                // Asymmetric filtering: Right side is brighter and fizzier
                irid_lpf_l += 0.1409f * (irid_raw_l - irid_lpf_l);
                irid_lpf_r += 0.1603f * (irid_raw_r - irid_lpf_r);

                irid_l = irid_lpf_l;
                irid_r = irid_lpf_r;
            }

            // ==========================================
            // FINAL PARALLEL MIXDOWN
            // ==========================================
            // FDN reverb (rev_l/rev_r) is always the base — SIZE/DECAY/BASS remain
            // audible even when all path modifiers are at zero.
            float path_l = (dark_sig * dark_amt) + (glow_l * glow_amt) + (bright_l * bright_amt) +
                           (color_l * color_amt) + (spark_l * spark_amt) + (irid_l * irid_amt);
            float path_r = (dark_sig * dark_amt) + (glow_r * glow_amt) + (bright_r * bright_amt) +
                           (color_r * color_amt) + (spark_r * spark_amt) + (irid_r * irid_amt);

            float mix_l = rev_l + path_l * path_norm;
            float mix_r = rev_r + path_r * path_norm;

            // Mid-side stereo width on wet signal
            float mid  = (mix_l + mix_r) * 0.5f;
            float side = (mix_l - mix_r) * 0.5f * width_amt;

            out[i]   = mid + side;
            out[i+1] = mid - side;
        }
    }
};
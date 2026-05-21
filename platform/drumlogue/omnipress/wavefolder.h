#pragma once

/**
 * @file wavefolder.h
 * @brief NEON-optimized wavefolder and overdrive stages
 *
 * Features:
 * - 5 distortion modes including SubOctave
 * - Proper real-time-safe PRNG for SubOctave mode
 * - NEON-optimized for ARMv7
 *
 * FIXED: Removed all rand() calls, added proper PRNG
 */

#include <arm_neon.h>
#include <math.h>
#include <stdint.h>
#include "float_math.h"
#include "constants.h"

/**
 * Simple Xorshift PRNG for real-time use
 * 32-bit version - fast and deterministic
 */
typedef struct {
    uint32_t state;
} prng_simple_t;

/**
 * Initialize PRNG with seed
 */
fast_inline void prng_simple_init(prng_simple_t* prng, uint32_t seed) {
    prng->state = seed;
}

/**
 * Generate next 32-bit random number
 * Xorshift algorithm: x ^= x << 13; x ^= x >> 17; x ^= x << 5;
 */
fast_inline uint32_t prng_simple_next(prng_simple_t* prng) {
    uint32_t x = prng->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    prng->state = x;
    return x;
}

/**
 * Wavefolder state structure
 */
typedef struct {
    uint32_t mode;           // 0-4
    float32x4_t drive;       // Drive amount (0.0 to 1.0)
    float32x4_t output_gain; // Makeup after drive

    // Sub-octave state
    float32x4_t sub_phase;
    float32x4_t last_input;
    uint32x4_t zero_cross;

    // FIXED: PRNG for sub-octave mode (instead of rand())
    prng_simple_t prng;
} wavefolder_t;

/**
 * Initialize wavefolder
 */
fast_inline void wavefolder_init(wavefolder_t* wf) {
    wf->mode = DRIVE_MODE_SOFT_CLIP;
    wf->drive = vdupq_n_f32(0.0f);
    wf->output_gain = vdupq_n_f32(1.0f);
    wf->sub_phase = vdupq_n_f32(0.0f);
    wf->last_input = vdupq_n_f32(0.0f);
    wf->zero_cross = vdupq_n_u32(0);

    // Initialize PRNG with fixed seed
    prng_simple_init(&wf->prng, 0x9E3779B9);
}

/**
 * Set drive type
 */
fast_inline void wavefolder_set_drive_type(wavefolder_t* wf, int mode) {
  if (mode < DRIVE_MODE_TOTAL)
    wf->mode = mode;
}
/**
 * Set drive amount (0-100%)
 */
fast_inline void wavefolder_set_drive(wavefolder_t* wf, float drive_percent) {
    float drive = drive_percent * 0.01f;
    wf->drive = vdupq_n_f32(drive);

    // Makeup gain compensates for the pre-gain (1 + drive*3) applied in
    // wavefolder_process. At drive=0 this is 1.0 (passthrough, matches CLEAN
    // mode level). At drive=1.0 it is 0.25 which cancels the 4x pre-gain for
    // signals still in the linear range of the waveshaper.
    float makeup = 1.0f / (1.0f + drive * 3.0f);
    wf->output_gain = vdupq_n_f32(makeup);
}

/**
 * Soft clip: x/(1+|x|)
 * Monotone, bounded in (-1,+1) for all inputs.
 * The cubic x-x³/3 is non-monotonic above |x|≈0.577 and produces polarity
 * inversion for driven signals — this replaces it with a true saturator.
 */
fast_inline float32x4_t soft_clip(float32x4_t x) {
    float32x4_t abs_x = vabsq_f32(x);
    float32x4_t denom = vaddq_f32(vdupq_n_f32(1.0f), abs_x);
    float32x4_t rec = vrecpeq_f32(denom);
    rec = vmulq_f32(vrecpsq_f32(denom, rec), rec);  // one NR step → ~16-bit accuracy
    return vmulq_f32(x, rec);
}

/**
 * Hard clip (limiter)
 */
fast_inline float32x4_t hard_clip(float32x4_t x) {
    float32x4_t one = vdupq_n_f32(1.0f);
    return vmaxq_f32(vminq_f32(x, one), vnegq_f32(one));
}

/**
 * Triangle folder
 */
fast_inline float32x4_t triangle_folder(float32x4_t x) {
    float32x4_t one = vdupq_n_f32(1.0f);
    float32x4_t two = vdupq_n_f32(2.0f);
    float32x4_t four = vdupq_n_f32(4.0f);

    // y = |(x + 1) % 4 - 2| - 1
    float32x4_t shifted = vaddq_f32(x, one);
    float32x4_t div = vmulq_f32(shifted, vdupq_n_f32(0.25f));
    int32x4_t floor_div = vcvtq_s32_f32(div);
    float32x4_t mod = vsubq_f32(shifted,
                                vmulq_f32(vcvtq_f32_s32(floor_div), four));

    float32x4_t abs_diff = vabsq_f32(vsubq_f32(mod, two));
    return vsubq_f32(abs_diff, one);
}

/**
 * Sine folder
 */
fast_inline float32x4_t sine_folder(float32x4_t x) {
    float32x4_t half_pi = vdupq_n_f32(M_PI_2);
    float32x4_t two = vdupq_n_f32(2.0f);

    x = vmaxq_f32(vminq_f32(x, two), vnegq_f32(two));
    float32x4_t angle = vmulq_f32(x, half_pi);

    // sin(angle) ≈ angle - angle^3/6
    float32x4_t a2 = vmulq_f32(angle, angle);
    float32x4_t a3 = vmulq_f32(angle, a2);
    return vsubq_f32(angle, vmulq_f32(a3, vdupq_n_f32(1.0f/6.0f)));
}

// Sub-octave generator — sequential scalar processing for correct zero-crossing detection.
// The old NEON version used float32x4_t last_input/sub_phase where each lane had its
// own independent history, causing 3 of the 4 lanes to detect wrong zero crossings
// (comparing sample n-4,n-3,n-2 against current instead of n-1) → erratic noise.
fast_inline float32x4_t suboctave_process(wavefolder_t* wf, float32x4_t in_v) {
    float buf_in[4], buf_out[4];
    vst1q_f32(buf_in, in_v);

    // Extract scalar state from lane 0 — all lanes are now kept identical.
    float last  = vgetq_lane_f32(wf->last_input, 0);
    float phase = vgetq_lane_f32(wf->sub_phase,  0);

    for (int i = 0; i < 4; ++i) {
        const float x = buf_in[i];
        if (last < 0.0f && x >= 0.0f) {
            // Positive zero crossing: advance sub-octave by a half cycle.
            // 3% jitter gives organic feel without audible pitch noise.
            float rnd = (float)(prng_simple_next(&wf->prng) & 0x3FFF) / 16384.0f;
            phase += 0.5f + rnd * 0.03f;
            if (phase >= 1.0f) phase -= 1.0f;
        }
        buf_out[i] = (phase < 0.5f) ? 1.0f : -1.0f;
        last = x;
    }

    wf->last_input = vdupq_n_f32(last);
    wf->sub_phase  = vdupq_n_f32(phase);
    return vld1q_f32(buf_out);
}

/**
 * Main wavefolder processing with drive and blend
 */
fast_inline float32x4x2_t wavefolder_process(wavefolder_t* wf,
                                              float32x4_t in_l,
                                              float32x4_t in_r,
                                              float drive) {
    (void)drive; // drive is stored in wf->drive

    float32x4x2_t out;

    // Apply drive gain: 1+drive*3 gives 1x at drive=0 (passthrough) to 4x at drive=100%.
    // Matches the makeup formula in wavefolder_set_drive so level is consistent.
    float32x4_t driven_l = vmulq_f32(in_l, vaddq_f32(vdupq_n_f32(1.0f),
                                                      vmulq_f32(wf->drive, vdupq_n_f32(3.0f))));
    float32x4_t driven_r = vmulq_f32(in_r, vaddq_f32(vdupq_n_f32(1.0f),
                                                      vmulq_f32(wf->drive, vdupq_n_f32(3.0f))));

    // Apply selected waveshaping
    switch (wf->mode) {
        case DRIVE_MODE_SOFT_CLIP:
            out.val[0] = soft_clip(driven_l);
            out.val[1] = soft_clip(driven_r);
            break;

        case DRIVE_MODE_HARD_CLIP:
            out.val[0] = hard_clip(driven_l);
            out.val[1] = hard_clip(driven_r);
            break;

        case DRIVE_MODE_TRIANGLE:
            out.val[0] = triangle_folder(driven_l);
            out.val[1] = triangle_folder(driven_r);
            break;

        case DRIVE_MODE_SINE:
            out.val[0] = sine_folder(driven_l);
            out.val[1] = sine_folder(driven_r);
            break;

        case DRIVE_MODE_SUBOCTAVE: {
            // Use a mono mix for zero-crossing detection. Processing L and R as
            // separate detectors causes independent sub-octave phases that drift
            // apart, creating stereo incoherence heard as fuzziness/noise.
            // A single detector gives a phase-coherent ±1 square wave; the stereo
            // image is preserved by amplitude-modulating each channel independently.
            float32x4_t mono = vmulq_n_f32(vaddq_f32(driven_l, driven_r), 0.5f);
            float32x4_t sub  = suboctave_process(wf, mono);
            out.val[0] = vmulq_f32(sub, vabsq_f32(in_l));
            out.val[1] = vmulq_f32(sub, vabsq_f32(in_r));
            break;
        }

        default:
            out.val[0] = driven_l;
            out.val[1] = driven_r;
    }

    // Apply output makeup gain
    out.val[0] = vmulq_f32(out.val[0], wf->output_gain);
    out.val[1] = vmulq_f32(out.val[1], wf->output_gain);

    return out;
}

/**
 * @file fm_voices.h
 * @brief Shared NEON helpers and legacy FM state structures for Sonaglio
 *
 * Current role:
 * - one-pole filter helpers used by Snare / Metal / output shaping
 * - exp2_neon() and MIDI-to-frequency helpers
 * - small legacy FM state structs kept for compatibility/tests
 *
 * The active Sonaglio engines keep their own state. This file should stay
 * lightweight and avoid becoming a second synthesis architecture.
 */

#ifndef CE1E60E1_B031_4A97_B2C2_CDB35865D5B7
#define CE1E60E1_B031_4A97_B2C2_CDB35865D5B7

#include <arm_neon.h>
#include <stdint.h>
#include <math.h>
#include "constants.h"
#include "envelope_rom.h"
#include "lfo_enhanced.h"

#ifndef FM_VOICES_MAX_OPERATORS
#define FM_VOICES_MAX_OPERATORS 4
#endif

#ifndef FM_VOICES_PARAM_TOTAL
#define FM_VOICES_PARAM_TOTAL 24
#endif

/**
 * Simple one-pole filter state.
 * One float32x4_t state word = one state per NEON lane.
 */
typedef struct {
    float32x4_t z1;
} one_pole_t;

/**
 * Clamp scalar cutoff to a safe range before computing one-pole alpha.
 */
fast_inline float one_pole_clamp_cutoff(float cutoff_hz) {
    if (cutoff_hz < 1.0f) return 1.0f;
    const float nyq = 0.49f * SAMPLE_RATE;
    if (cutoff_hz > nyq) return nyq;
    return cutoff_hz;
}

/**
 * Compute one-pole LPF alpha:
 * alpha = 2*pi*f / (2*pi*f + sample_rate)
 *
 * Use this outside the hot path when the cutoff is constant.
 */
fast_inline float one_pole_lpf_alpha(float cutoff_hz) {
    cutoff_hz = one_pole_clamp_cutoff(cutoff_hz);
    const float two_pi_f = 2.0f * (float)M_PI * cutoff_hz;
    return two_pi_f / (two_pi_f + SAMPLE_RATE);
}

// TODO:gradually update engines to prefer one_pole_lpf_a(), one_pole_hpf_a()
// For example, Snare currently calls runtime cutoff helpers. That is clean, but later we can move the alpha values into constants if profiling shows it matters.

/**
 * One-pole LPF with precomputed alpha.
 * This is the preferred audio-rate helper.
 */
fast_inline float32x4_t one_pole_lpf_a(one_pole_t* f,
                                       float32x4_t in,
                                       float alpha) {
    alpha = (alpha < 0.0f) ? 0.0f : ((alpha > 1.0f) ? 1.0f : alpha);

    const float32x4_t a = vdupq_n_f32(alpha);
    const float32x4_t one_minus_a = vsubq_f32(vdupq_n_f32(1.0f), a);

    float32x4_t out = vaddq_f32(vmulq_f32(in, a),
                                vmulq_f32(f->z1, one_minus_a));
    f->z1 = out;
    return out;
}

/**
 * Runtime-cutoff LPF.
 *
 * This computes alpha every call, so it is convenient but should not be used
 * in tight loops when the cutoff is constant. Prefer one_pole_lpf_a().
 */
fast_inline float32x4_t one_pole_lpf(one_pole_t* f,
                                     float32x4_t in,
                                     float cutoff_hz) {
    return one_pole_lpf_a(f, in, one_pole_lpf_alpha(cutoff_hz));
}

/**
 * One-pole HPF using an internal LPF state:
 * hp = input - lowpass(input)
 */
fast_inline float32x4_t one_pole_hpf_a(one_pole_t* f,
                                       float32x4_t in,
                                       float alpha) {
    float32x4_t lp = one_pole_lpf_a(f, in, alpha);
    return vsubq_f32(in, lp);
}

/**
 * Runtime-cutoff HPF. Convenience wrapper.
 */
fast_inline float32x4_t one_pole_hpf(one_pole_t* f,
                                     float32x4_t in,
                                     float cutoff_hz) {
    return one_pole_hpf_a(f, in, one_pole_lpf_alpha(cutoff_hz));
}

/**
 * Reset one-pole state.
 */
fast_inline void one_pole_reset(one_pole_t* f) {
    f->z1 = vdupq_n_f32(0.0f);
}

/**
 * Operator data for 4 lanes / voices in SoA format.
 * Mostly legacy/test support; current engines have their own compact state.
 */
typedef struct {
    float32x4_t phase[FM_VOICES_MAX_OPERATORS];
    float32x4_t freq[FM_VOICES_MAX_OPERATORS];
    float32x4_t index[FM_VOICES_MAX_OPERATORS];
    float32x4_t output[FM_VOICES_MAX_OPERATORS];
} fm_operators_t;

/**
 * Per-lane voice data.
 * Mostly legacy/test support.
 */
typedef struct {
    uint32x4_t active;
    float32x4_t envelope;
    uint32x4_t env_stage;
    uint32x4_t stage_time;
    float32x4_t param1;
    float32x4_t param2;
    float32x4_t gain;
    float32x4_t note_freq;
} fm_voices_t;

/**
 * Complete legacy FM state.
 *
 * Kept because some tests/utilities may still reference it. The active
 * Sonaglio instrument should not grow new code around this struct.
 */
typedef struct {
    fm_operators_t  ops __attribute__((aligned(VECTOR_ALIGN)));
    fm_voices_t     voices __attribute__((aligned(VECTOR_ALIGN)));
    float32x4_t     lfo_phase[2] __attribute__((aligned(VECTOR_ALIGN)));
    uint32x4_t      prng_state[4] __attribute__((aligned(VECTOR_ALIGN)));
    int8_t          params[FM_VOICES_PARAM_TOTAL];
} fm_state_t;

#if defined(__cplusplus)
static_assert(sizeof(fm_state_t) % VECTOR_ALIGN == 0,
              "fm_state_t must be 16-byte aligned for NEON");
#endif

/**
 * High-precision NEON 2^x
 * Adapted from Julien Pommier's neon_mathfun style.
 */
fast_inline float32x4_t exp2_neon(float32x4_t x) {
    // Avoid pathological IEEE exponent construction.
    x = vmaxq_f32(x, vdupq_n_f32(-126.0f));
    x = vminq_f32(x, vdupq_n_f32(126.0f));

    // Round to nearest integer.
    float32x4_t fx = vaddq_f32(x, vdupq_n_f32(0.5f));
    int32x4_t n = vcvtq_s32_f32(fx);
    float32x4_t n_float = vcvtq_f32_s32(n);

    // Correct truncation for negative values.
    uint32x4_t mask = vcltq_f32(fx, n_float);
    n = vaddq_s32(n, vreinterpretq_s32_u32(mask));
    n_float = vcvtq_f32_s32(n);

    // Fractional part in roughly [-0.5, 0.5].
    float32x4_t f = vsubq_f32(x, n_float);

    // e^(f * ln2)
    float32x4_t z = vmulq_f32(f, vdupq_n_f32(0.69314718055994530942f));

    float32x4_t y = vdupq_n_f32(1.9875691500E-4f);
    y = vmlaq_f32(vdupq_n_f32(1.3981999507E-3f), y, z);
    y = vmlaq_f32(vdupq_n_f32(8.3334519073E-3f), y, z);
    y = vmlaq_f32(vdupq_n_f32(4.1665795894E-2f), y, z);
    y = vmlaq_f32(vdupq_n_f32(1.6666665459E-1f), y, z);
    y = vmlaq_f32(vdupq_n_f32(5.0000001201E-1f), y, z);
    y = vmlaq_f32(z, y, z);
    y = vaddq_f32(y, vdupq_n_f32(1.0f));

    // 2^n via float exponent bits.
    n = vaddq_s32(n, vdupq_n_s32(127));
    n = vshlq_n_s32(n, 23);
    float32x4_t pow2n = vreinterpretq_f32_s32(n);

    return vmulq_f32(y, pow2n);
}

/**
 * Convert float MIDI notes to frequency.
 *
 * This is the preferred helper for the reduced selector model, because
 * Euclidean offsets and scatter detune can be fractional semitones.
 */
fast_inline float32x4_t midi_f32_to_freq_neon(float32x4_t midi_notes) {
    const float32x4_t a4_freq = vdupq_n_f32(A4_FREQ);
    const float32x4_t a4_midi = vdupq_n_f32(A4_MIDI);
    const float32x4_t inv_12  = vdupq_n_f32(1.0f / 12.0f);

    float32x4_t octaves = vmulq_f32(vsubq_f32(midi_notes, a4_midi), inv_12);
    return vmulq_f32(exp2_neon(octaves), a4_freq);
}

/**
 * Convert integer MIDI notes to frequency.
 * Kept for compatibility.
 */
fast_inline float32x4_t midi_to_freq_neon(uint32x4_t midi_notes) {
    return midi_f32_to_freq_neon(vcvtq_f32_u32(midi_notes));
}

/**
 * Scalar MIDI frequency helper.
 * Useful when queueing selector-based triggers before broadcasting to lanes.
 */
fast_inline float midi_note_to_freq_scalar(float midi_note) {
    float32x4_t v = midi_f32_to_freq_neon(vdupq_n_f32(midi_note));
    return vgetq_lane_f32(v, 0);
}

/**
 * Initialize legacy FM state.
 */
fast_inline void fm_state_init(fm_state_t* state) {
    for (int op = 0; op < FM_VOICES_MAX_OPERATORS; op++) {
        state->ops.phase[op] = vdupq_n_f32(0.0f);
        state->ops.freq[op] = vdupq_n_f32(0.0f);
        state->ops.index[op] = vdupq_n_f32(0.0f);
        state->ops.output[op] = vdupq_n_f32(0.0f);
    }

    state->voices.active = vdupq_n_u32(0);
    state->voices.envelope = vdupq_n_f32(0.0f);
    state->voices.env_stage = vdupq_n_u32(ENV_STATE_OFF);
    state->voices.stage_time = vdupq_n_u32(0);
    state->voices.param1 = vdupq_n_f32(0.5f);
    state->voices.param2 = vdupq_n_f32(0.5f);
    state->voices.gain = vdupq_n_f32(0.25f);
    state->voices.note_freq = vdupq_n_f32(A4_FREQ);

    state->lfo_phase[0] = vdupq_n_f32(0.0f);
    state->lfo_phase[1] = vdupq_n_f32(LFO_PHASE_OFFSET);

    for (int i = 0; i < 4; i++) {
        state->prng_state[i] = vdupq_n_u32(0x9E3779B9u);
    }

    for (int i = 0; i < FM_VOICES_PARAM_TOTAL; ++i) {
        state->params[i] = 0;
    }
}

#endif /* CE1E60E1_B031_4A97_B2C2_CDB35865D5B7 */

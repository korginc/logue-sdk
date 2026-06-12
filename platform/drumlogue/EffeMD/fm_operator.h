#pragma once

/**
 * @file fm_operator.h
 * @brief Portable scalar FM operator
 *
 * Adapted from copych/ESP32-S3_FM_Drum_Synth
 * Copyright 2025 copych — MIT License
 * https://github.com/copych/ESP32-S3_FM_Drum_Synth
 *
 * Changes from original FmOperator.h:
 *   - C struct + static inline (no C++ class, no std::string/vector)
 *   - ESP32/Arduino/IRAM_ATTR dependencies removed
 *   - Waveform is a plain enum (no display name methods)
 *   - SIN_FUNC_NORM  -> fastersinfullf() from float_math.h
 *   - fast_floorf    -> fmo_frac() using (int) cast (safe for positive values)
 *   - DIV_SAMPLE_RATE -> 1.0f / 48000.0f
 *   - powf()         -> fasterpowf() / fasterpow2f() from float_math.h
 *
 * Key design points (preserved from copych):
 *   - Phase in normalized [0, 1) convention
 *   - MOD_RANGE = 16.0: modulator output is scaled by 16 when added to phase
 *   - fm_level = 0.1 * 161^volume - 0.1: exponential (DX7-style) depth curve
 *   - feedback_  = 1 / 2^(7 - fb): DX7-style feedback parameter (fb in 0..7)
 */

#include "float_math.h"
#include "constants.h"

#define FMO_MOD_RANGE 16.0f

typedef enum {
    WF_SINE     = 0,
    WF_COSINE   = 1,
    WF_TRIANGLE = 2,
    WF_SQUARE   = 3,
    WF_SAW      = 4,
    WF_COUNT    = 5
} fmo_waveform_t;

typedef struct {
    float phase;
    float phase_inc;
    float base_freq;
    float ratio;
    float detune;

    float fb;         /* DX7-style feedback parameter 0..7 */
    float feedback_;  /* = 1 / 2^(7 - fb), computed from fb */
    float fb_mod;     /* external multiplier, default 1.0 */
    float fb_mult;    /* fb_mod * feedback_ */
    float last_out;

    float volume;     /* 0..1 */
    float out_level;  /* = volume */
    float fm_level;   /* = 0.1 * 161^volume - 0.1, exponential FM depth */

    fmo_waveform_t waveform;
} fm_op_t;

/* -------------------------------------------------------------------------
 * Waveform generators — input t ∈ [0, 1)
 * ------------------------------------------------------------------------- */

static inline float fmo_wf_sine(float t) {
    return fastersinfullf(t * M_TWOPI);
}

static inline float fmo_wf_cos(float t) {
    return fastersinfullf((t + 0.25f) * M_TWOPI);
}

static inline float fmo_wf_triangle(float t) {
    return 4.0f * si_fabsf(t - 0.5f) - 1.0f;
}

static inline float fmo_wf_square(float t) {
    return (t < 0.5f) ? 1.0f : -1.0f;
}

static inline float fmo_wf_saw(float t) {
    return t * 2.0f - 1.0f;
}

static inline float fmo_wf_render(fmo_waveform_t wf, float t) {
    switch (wf) {
        case WF_SINE:     return fmo_wf_sine(t);
        case WF_COSINE:   return fmo_wf_cos(t);
        case WF_TRIANGLE: return fmo_wf_triangle(t);
        case WF_SQUARE:   return fmo_wf_square(t);
        case WF_SAW:      return fmo_wf_saw(t);
        default:          return fmo_wf_sine(t);
    }
}

/* -------------------------------------------------------------------------
 * Basic helpers shared by the md-drum-synth models
 * ------------------------------------------------------------------------- */

static inline float WrapPhase(float phase) {
  while (phase >= TWO_PI) phase -= TWO_PI;
  while (phase < 0.0f) phase += TWO_PI;
  return phase;
}

static inline float ExpDecay(float t, float decay_time) {
  return e_expff(-t / decay_time);
}

/* -------------------------------------------------------------------------
 * Phase fractional part — handles values up to ±INT_MAX
 * ------------------------------------------------------------------------- */

static inline float fmo_frac(float x) {
    float r = x - (float)(int)x;
    return (r < 0.0f) ? r + 1.0f : r;
}

/* -------------------------------------------------------------------------
 * Parameter setters (call when parameter changes, not every sample)
 * ------------------------------------------------------------------------- */

static inline void fmo_update_phase_inc(fm_op_t* op) {
    op->phase_inc = (op->base_freq * op->ratio + op->detune) * INV_SAMPLE_RATE;
}

static inline void fmo_set_freq(fm_op_t* op, float hz) {
    op->base_freq = hz;
    fmo_update_phase_inc(op);
}

static inline void fmo_set_ratio(fm_op_t* op, float ratio) {
    op->ratio = ratio;
    fmo_update_phase_inc(op);
}

static inline void fmo_set_detune(fm_op_t* op, float hz) {
    op->detune = hz;
    fmo_update_phase_inc(op);
}

static inline void fmo_set_feedback(fm_op_t* op, float fb) {
    op->fb = (fb < 0.0f) ? 0.0f : ((fb > 7.0f) ? 7.0f : fb);
    op->feedback_ = (op->fb == 0.0f) ? 0.0f : fasterpow2f(op->fb - 7.0f);
    op->fb_mult = op->fb_mod * op->feedback_;
}

/* Volume maps to out_level (carrier) and fm_level (modulator) via exponential.
 * fm_level = 0.1 * 161^v - 0.1 (same as copych) */
static inline void fmo_set_volume(fm_op_t* op, float v) {
    op->volume = (v < 0.0f) ? 0.0f : ((v > 1.0f) ? 1.0f : v);
    op->out_level = op->volume;
    op->fm_level  = 0.1f * fasterpowf(161.0f, op->volume) - 0.1f;
}

static inline void fmo_init(fm_op_t* op) {
    op->phase    = 0.0f;
    op->phase_inc= 0.0f;
    op->base_freq= 440.0f;
    op->ratio    = 1.0f;
    op->detune   = 0.0f;
    op->fb_mod   = 1.0f;
    op->last_out = 0.0f;
    op->waveform = WF_SINE;
    fmo_set_feedback(op, 0.0f);
    fmo_set_volume(op, 0.8f);
    fmo_update_phase_inc(op);
}

static inline void fmo_reset(fm_op_t* op) {
    op->phase    = 0.0f;
    op->last_out = 0.0f;
}

/* -------------------------------------------------------------------------
 * Per-sample process (hot path)
 * ------------------------------------------------------------------------- */

/* Phase advance with optional pitch scaling (pitch = 1.0 for no change).
 * Returns normalized phase t ∈ [0, 1) with modulation and feedback applied. */
static inline float fmo_advance(fm_op_t* op, float mod_in, float pitch) {
    op->phase += op->phase_inc * pitch;
    /* Remove integer part (handles phase_inc * pitch up to ~2.0) */
    if (op->phase >= 1.0f) op->phase -= (float)(int)op->phase;
    float t = op->phase + mod_in * FMO_MOD_RANGE + op->fb_mult * op->last_out;
    return fmo_frac(t);
}

/* Modulator: output feeds into a carrier's mod_in.
 * Returns fm_level * waveform * env. */
static inline float fmo_mod(fm_op_t* op, float mod_in, float env, float pitch) {
    float t = fmo_advance(op, mod_in, pitch);
    float s = fmo_wf_render(op->waveform, t);
    op->last_out = s;
    return op->fm_level * s * env;
}

/* Carrier: audio output.
 * Returns out_level * waveform * env. */
static inline float fmo_out(fm_op_t* op, float mod_in, float env, float pitch) {
    float t = fmo_advance(op, mod_in, pitch);
    float s = fmo_wf_render(op->waveform, t);
    op->last_out = s;
    return op->out_level * s * env;
}

/* Raw operator: advances phase (with modulation and DX7 feedback) and
 * returns the plain waveform sample without any level scaling.
 *
 * Used by the md-drum-synth models, which keep the original EFM convention
 * of applying modulation index and amplitude envelopes externally:
 *   phase_mod = I * mod_env * mod_raw   (normalized-phase units)
 * Pass mod_in = phase_mod / FMO_MOD_RANGE to preserve that convention. */
static inline float fmo_render_raw(fm_op_t* op, float mod_in, float pitch) {
    float t = fmo_advance(op, mod_in, pitch);
    float s = fmo_wf_render(op->waveform, t);
    op->last_out = s;
    return s;
}

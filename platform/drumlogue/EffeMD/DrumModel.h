#pragma once

/**
 * @file DrumModel.h
 * @brief Abstract base class for all EffeMD drum instrument models.
 *
 * Ported from ctag-fh-kiel/md-drum-synth (DrumModel.h).
 * Changes from the original:
 *   - iostream serialization interface removed (no file I/O on drumlogue)
 *   - RenderControls() removed (ImGui only exists on the desktop build)
 *   - drumlogue preset/parameter interface added (loadPreset/setParameter/
 *     getParameter against the fixed 24-parameter contract in constants.h)
 *   - per-voice pitch ratio added so the synth layer can transpose a model
 *     from MIDI notes and Euclidean tuning offsets without each model
 *     knowing about MIDI
 */

#include <stdint.h>

#include "constants.h"
#include "float_math.h"
#include "fm_operator.h"

/* -------------------------------------------------------------------------
 * Tiny deterministic scalar PRNG (xorshift32).
 *
 * Replaces rand()/std::default_random_engine from the original models:
 * - real-time safe (no locks, no libc state)
 * - deterministic per model instance (reseeded in Init), which makes the
 *   noise paths unit-testable
 * ------------------------------------------------------------------------- */
typedef struct {
    uint32_t state;
} drum_rng_t;

static inline void drum_rng_seed(drum_rng_t* r, uint32_t seed) {
    r->state = seed ? seed : 0xBADC0FFEu;
}

static inline uint32_t drum_rng_u32(drum_rng_t* r) {
    uint32_t x = r->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    r->state = x;
    return x;
}

/* Uniform white noise in [-1, 1] */
static inline float drum_rng_bipolar(drum_rng_t* r) {
    return (float)drum_rng_u32(r) * (2.0f / 4294967295.0f) - 1.0f;
}

/**
 * Virtual interface every instrument model derives from.
 *
 * Lifecycle: Init() -> [setParameter()/loadPreset()...] -> Trigger() ->
 * Process() once per sample until silent.
 */
class DrumModel {
public:
    virtual ~DrumModel() {}
    virtual void Init() = 0;
    virtual void Trigger() = 0;
    virtual float Process() = 0;

    virtual void  loadPreset(uint8_t idx) = 0;
    virtual void  setParameter(fm_param_index_t param_index, float value) = 0;
    /* Returns the model's DSP value for a parameter, or 255.0f when the
     * parameter is not used by this instrument (displayed as "x"). */
    virtual float getParameter(fm_param_index_t param_index) = 0;

    /* Pitch ratio applied to all oscillator frequencies of the model.
     * 1.0 = play at the dialed base frequency. Set by the synth layer from
     * (MIDI note - root) + Euclidean tuning offset before Trigger(). */
    void setPitchRatio(float r) {
        pitch_ratio_ = (r < 0.03125f) ? 0.03125f : ((r > 32.0f) ? 32.0f : r);
    }
    float pitchRatio() const { return pitch_ratio_; }

protected:
    float pitch_ratio_ = 1.0f;
};

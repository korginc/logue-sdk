#pragma once

/**
 * @file fm_perc_synth_process.h
 * @brief Core Sonaglio synth state and rendering functions
 *
 * This is the active 4-engine implementation.
 * Selector-based Sonaglio model:
 *   - one Instrument selector
 *   - Blend / Gap / Scatter macro controls
 *   - optional delayed secondary trigger for combos and shadow hits
 *   - scalar Euclidean tuning offset per engine before queue_trigger()
 * Legacy 5-engine allocation logic and resonant morphing are intentionally removed
 * from the live path.
 */

#include <arm_neon.h>
#include <stdint.h>
#include <string.h>

#include "constants.h"
#include "engine_mapping.h"
#include "fm_voices.h"
#include "kick_engine.h"
#include "snare_engine.h"
#include "metal_engine.h"
#include "perc_engine.h"
#include "lfo_enhanced.h"
#include "lfo_smoothing.h"
#include "envelope_rom.h"
#include "prng.h"
#include "midi_handler.h"
#include "fm_presets.h"

// ============================================================================
// Euclidean tuning offsets
// ============================================================================
// offsets[mode][engine lane] = semitones above root.
// Engine mapping used by selector mode:
//   lane 0 = Kick
//   lane 1 = Snare
//   lane 2 = Metal
//   lane 3 = Tom/Perc
static const float EUCLID_OFFSETS[EUCLID_MODE_COUNT][4] = {
    { 0.f,  0.f,  0.f,  0.f},  // 0: Off
    { 0.f,  1.f,  2.f,  3.f},  // 1: E(4,4)
    { 0.f,  1.f,  3.f,  4.f},  // 2: E(4,6)
    { 0.f,  1.f,  3.f,  5.f},  // 3: E(4,7)
    { 0.f,  2.f,  4.f,  6.f},  // 4: E(4,8)
    { 0.f,  2.f,  5.f,  7.f},  // 5: E(4,10)
    { 0.f,  3.f,  6.f,  9.f},  // 6: E(4,12)
    { 0.f,  4.f,  8.f, 12.f},  // 7: E(4,16)
    { 0.f,  6.f, 12.f, 18.f},  // 8: E(4,24)
};

// ============================================================================
// Instrument selector
// ============================================================================
typedef enum {
    INST_KICK = 0,
    INST_SNARE,
    INST_TOM,
    INST_METAL,
    INST_KS,
    INST_KT,
    INST_KM,
    INST_ST,
    INST_SM,
    INST_TM,
    INST_COUNT
} sonaglio_instrument_t;

typedef struct {
    uint8_t  active;
    uint8_t  engine;
    uint8_t  midi_note;
    float    note_f;
    uint32_t delay_samples;
    float    gain;
} pending_trigger_t;

/**
 * Complete synthesizer state
 */
typedef struct {
    // PRNG
    neon_prng_t prng;

    // Fixed engines
    kick_engine_t  kick;
    snare_engine_t snare;
    metal_engine_t metal;
    perc_engine_t  perc;

    // LFO system
    lfo_enhanced_t lfo;
    lfo_smoother_t lfo_smooth;

    // Envelope
    neon_envelope_t envelope;
    uint8_t         current_env_shape;

    // MIDI handler
    midi_handler_t midi;

    // User parameters
    int8_t params[PARAM_TOTAL];

    // Selector routing model
    uint8_t instrument_sel;  // 0..9
    float blend;             // 0..1
    float gap;               // 0..1
    float scatter;           // 0..1

    // Per-engine post-gain used by selector/combo routing.
    float engine_gain[ENGINE_COUNT];

    // Per-note bitmask: bit0=kick, bit1=snare, bit2=metal, bit3=perc/tom.
    uint8_t note_engine_mask[128];

    // Small trigger queue so Gap can schedule delayed secondary hits.
    pending_trigger_t pending[8];

    // Note tuning
    float32x4_t euclid_offsets;

    // Output gain
    float master_gain;
} fm_perc_synth_t;

// ============================================================================
// Utility helpers
// ============================================================================

fast_inline float neon_horizontal_sum(float32x4_t v) {
    float32x2_t sum_low = vpadd_f32(vget_low_f32(v), vget_high_f32(v));
    float32x2_t sum_total = vpadd_f32(sum_low, sum_low);
    return vget_lane_f32(sum_total, 0);
}

fast_inline float neon_horizontal_sum_alt(float32x4_t v) {
#if defined(__aarch64__)
    return vaddvq_f32(v);
#else
    float32x2_t sum_low = vpadd_f32(vget_low_f32(v), vget_high_f32(v));
    float32x2_t sum_total = vpadd_f32(sum_low, sum_low);
    return vget_lane_f32(sum_total, 0);
#endif
}

// Use only lane 0 in the reduced selector model. This avoids summing four
// duplicated lanes after removing the four-probability voice model.
static fast_inline uint32x4_t active_lane_mask() {
    uint32_t lanes[4] = {0xFFFFFFFFu, 0u, 0u, 0u};
    return vld1q_u32(lanes);
}

static constexpr float k_u32_to_unit = 1.0f / 4294967295.0f;

static fast_inline float rand_bipolar_from_prng(fm_perc_synth_t* synth) {
    uint32x4_t rnd = neon_prng_rand_u32(&synth->prng);
    uint32_t r0 = vgetq_lane_u32(rnd, 0);
    return ((float)r0 * k_u32_to_unit) * 2.0f - 1.0f;
}

static fast_inline float sonaglio_euclid_offset_for_engine(const fm_perc_synth_t* synth,
                                                           uint8_t engine) {
    switch (engine) {
        case ENGINE_KICK:  return vgetq_lane_f32(synth->euclid_offsets, 0);
        case ENGINE_SNARE: return vgetq_lane_f32(synth->euclid_offsets, 1);
        case ENGINE_METAL: return vgetq_lane_f32(synth->euclid_offsets, 2);
        case ENGINE_PERC:  return vgetq_lane_f32(synth->euclid_offsets, 3);
        default:           return 0.0f;
    }
}

static fast_inline uint8_t engine_to_note_mask_bit(uint8_t engine) {
    switch (engine) {
        case ENGINE_KICK:  return (uint8_t)(1u << 0);
        case ENGINE_SNARE: return (uint8_t)(1u << 1);
        case ENGINE_METAL: return (uint8_t)(1u << 2);
        case ENGINE_PERC:  return (uint8_t)(1u << 3);
        default:           return 0u;
    }
}

static fast_inline void queue_trigger(fm_perc_synth_t* synth,
                                      uint8_t engine,
                                      uint8_t midi_note,
                                      float note_f,
                                      float gain,
                                      uint32_t delay_samples) {
    for (int i = 0; i < 8; ++i) {
        if (!synth->pending[i].active) {
            synth->pending[i].active = 1;
            synth->pending[i].engine = engine;
            synth->pending[i].midi_note = midi_note;
            synth->pending[i].note_f = note_f;
            synth->pending[i].gain = gain;
            synth->pending[i].delay_samples = delay_samples;
            return;
        }
    }

    // Fallback: overwrite slot 0 if the queue is full.
    synth->pending[0].active = 1;
    synth->pending[0].engine = engine;
    synth->pending[0].midi_note = midi_note;
    synth->pending[0].note_f = note_f;
    synth->pending[0].gain = gain;
    synth->pending[0].delay_samples = delay_samples;
}

static fast_inline void fire_engine(fm_perc_synth_t* synth,
                                    uint8_t engine,
                                    float note_f) {
    const uint32x4_t lane = active_lane_mask();
    const float32x4_t note_vec = vdupq_n_f32(note_f);

    switch (engine) {
        case ENGINE_KICK:
            kick_engine_set_note(&synth->kick, lane, note_vec);
            break;
        case ENGINE_SNARE:
            snare_engine_set_note(&synth->snare, lane, note_vec);
            break;
        case ENGINE_METAL:
            metal_engine_set_note(&synth->metal, lane, note_vec);
            break;
        case ENGINE_PERC:
            perc_engine_set_note(&synth->perc, lane, note_vec);
            break;
        default:
            break;
    }
}

static fast_inline void process_pending_triggers(fm_perc_synth_t* synth) {
    for (int i = 0; i < 8; ++i) {
        pending_trigger_t* ev = &synth->pending[i];
        if (!ev->active) continue;

        if (ev->delay_samples > 0) {
            --ev->delay_samples;
            continue;
        }

        synth->engine_gain[ev->engine] = ev->gain;
        fire_engine(synth, ev->engine, ev->note_f);
        ev->active = 0;
    }
}

static fast_inline bool fm_perc_synth_has_pending(const fm_perc_synth_t* synth) {
    for (int i = 0; i < 8; ++i) {
        if (synth->pending[i].active) return true;
    }
    return false;
}

// ============================================================================
// Parameter / preset handling
// ============================================================================

fast_inline void fm_perc_synth_update_params(fm_perc_synth_t* synth) {
    int8_t* p = synth->params;

    synth->instrument_sel = (uint8_t)(p[PARAM_INSTRUMENT] < 0 ? 0 :
                                      (p[PARAM_INSTRUMENT] >= INST_COUNT ? INST_COUNT - 1 : p[PARAM_INSTRUMENT]));
    synth->blend   = p[PARAM_BLEND]   * 0.01f;
    synth->gap     = p[PARAM_GAP]     * 0.01f;
    synth->scatter = p[PARAM_SCATTER] * 0.01f;

    kick_engine_update(&synth->kick,
                       vdupq_n_f32(p[PARAM_KICK_ATK]  * 0.01f),
                       vdupq_n_f32(p[PARAM_KICK_BODY] * 0.01f));

    snare_engine_update(&synth->snare,
                        vdupq_n_f32(p[PARAM_SNARE_ATK]  * 0.01f),
                        vdupq_n_f32(p[PARAM_SNARE_BODY] * 0.01f));

    metal_engine_update(&synth->metal,
                        vdupq_n_f32(p[PARAM_METAL_ATK]  * 0.01f),
                        vdupq_n_f32(p[PARAM_METAL_BODY] * 0.01f));

    perc_engine_update(&synth->perc,
                       vdupq_n_f32(p[PARAM_PERC_ATK]  * 0.01f),
                       vdupq_n_f32(p[PARAM_PERC_BODY] * 0.01f));

    synth->current_env_shape = (uint8_t)p[PARAM_ENV_SHAPE];
    metal_engine_set_character(&synth->metal,
                               (uint32_t)(synth->current_env_shape >> 7));

    {
        uint8_t mode = (uint8_t)p[PARAM_EUCL_TUN];
        if (mode >= EUCLID_MODE_COUNT) mode = 0;
        synth->euclid_offsets = vld1q_f32(EUCLID_OFFSETS[mode]);
    }
}

fast_inline void load_fm_preset(uint8_t idx, int8_t* params) {
    if (idx >= NUM_OF_PRESETS || params == NULL) return;

    const fm_preset_t* p = &FM_PRESETS[idx];

    params[PARAM_INSTRUMENT] = p->instrument_sel;
    params[PARAM_BLEND]      = p->blend;
    params[PARAM_GAP]        = p->gap;
    params[PARAM_SCATTER]    = p->scatter;

    params[PARAM_KICK_ATK]   = p->kick_attack;
    params[PARAM_KICK_BODY]  = p->kick_body;
    params[PARAM_SNARE_ATK]  = p->snare_attack;
    params[PARAM_SNARE_BODY] = p->snare_body;

    params[PARAM_METAL_ATK]  = p->metal_attack;
    params[PARAM_METAL_BODY] = p->metal_body;
    params[PARAM_PERC_ATK]   = p->perc_attack;
    params[PARAM_PERC_BODY]  = p->perc_body;

    params[PARAM_LFO1_SHAPE]  = p->lfo1_shape;
    params[PARAM_LFO1_RATE]   = p->lfo1_rate;
    params[PARAM_LFO1_TARGET] = p->lfo1_target;
    params[PARAM_LFO1_DEPTH]  = p->lfo1_depth;

    params[PARAM_EUCL_TUN]    = p->eucl_tun;
    params[PARAM_LFO2_RATE]   = p->lfo2_rate;
    params[PARAM_LFO2_TARGET] = p->lfo2_target;
    params[PARAM_LFO2_DEPTH]  = p->lfo2_depth;

    params[PARAM_ENV_SHAPE] = p->env_shape;
    params[PARAM_HIT_SHAPE] = p->hit_shape;
    params[PARAM_BODY_TILT] = p->body_tilt;
    params[PARAM_DRIVE]     = p->drive;
}

// ============================================================================
// Initialization
// ============================================================================

fast_inline void fm_perc_synth_init(fm_perc_synth_t* synth) {
    kick_engine_init(&synth->kick);
    snare_engine_init(&synth->snare);
    metal_engine_init(&synth->metal);
    perc_engine_init(&synth->perc);

    lfo_enhanced_init(&synth->lfo);
    lfo_smoother_init(&synth->lfo_smooth);
    neon_envelope_init(&synth->envelope);

    neon_prng_init(&synth->prng, RAND_DEFAULT_SEED);
    midi_handler_init(&synth->midi);

    synth->current_env_shape = ENV_SHAPE_DEFAULT;
    synth->euclid_offsets = vdupq_n_f32(0.0f);
    synth->master_gain = 0.25f;

    synth->instrument_sel = INST_KICK;
    synth->blend = 0.5f;
    synth->gap = 0.5f;
    synth->scatter = 0.25f;

    for (int i = 0; i < ENGINE_COUNT; ++i) {
        synth->engine_gain[i] = 0.0f;
    }

    memset(synth->note_engine_mask, 0, sizeof(synth->note_engine_mask));
    memset(synth->pending, 0, sizeof(synth->pending));

    load_fm_preset(0, synth->params);
    fm_perc_synth_update_params(synth);
}

// ============================================================================
// MIDI note handling
// ============================================================================

fast_inline void fm_perc_synth_note_on(fm_perc_synth_t* synth,
                                       uint8_t note,
                                       uint8_t velocity) {
    (void)velocity; // Velocity intentionally excluded from current model.

    for (int i = 0; i < ENGINE_COUNT; ++i) {
        synth->engine_gain[i] = 0.0f;
    }

    // Route table: single engines or pairs
    // Tom is routed to Perc for now (Perc is the tom/block/wood proxy).
    uint8_t engine_a = ENGINE_KICK;
    uint8_t engine_b = 0xFF;
    uint8_t combo = 0;

    switch (synth->instrument_sel) {
        case INST_KICK:   engine_a = ENGINE_KICK;  break;
        case INST_SNARE:  engine_a = ENGINE_SNARE; break;
        case INST_TOM:    engine_a = ENGINE_PERC;  break;
        case INST_METAL:  engine_a = ENGINE_METAL; break;

        case INST_KS:     engine_a = ENGINE_KICK;  engine_b = ENGINE_SNARE; combo = 1; break;
        case INST_KT:     engine_a = ENGINE_KICK;  engine_b = ENGINE_PERC;  combo = 1; break;
        case INST_KM:     engine_a = ENGINE_KICK;  engine_b = ENGINE_METAL; combo = 1; break;
        case INST_ST:     engine_a = ENGINE_SNARE; engine_b = ENGINE_PERC;  combo = 1; break;
        case INST_SM:     engine_a = ENGINE_SNARE; engine_b = ENGINE_METAL; combo = 1; break;
        case INST_TM:     engine_a = ENGINE_PERC;  engine_b = ENGINE_METAL; combo = 1; break;
        default:          engine_a = ENGINE_KICK;  break;
    }

    const float gap_ms = 4.0f + synth->gap * 80.0f;
    const float jitter_ms = rand_bipolar_from_prng(synth) * synth->scatter * 8.0f;
    const float delayed_ms = gap_ms + jitter_ms;
    const uint32_t gap_samples =
        (uint32_t)((delayed_ms > 0.0f ? delayed_ms : 0.0f) * (float)SAMPLE_RATE * 0.001f);

    uint8_t mask = engine_to_note_mask_bit(engine_a);
    if (combo) mask |= engine_to_note_mask_bit(engine_b);
    synth->note_engine_mask[note] = mask;

    // Euclidean tuning as scalar offset before queue_trigger().
    const float note_a = (float)note + sonaglio_euclid_offset_for_engine(synth, engine_a);

    /**
     * For now, I would keep Gap in a moderate range.
     * Later, if you want truly separated hits, either:
     * -       per-engine/per-trigger envelopes, or
     * -       retriggering the shared envelope when a pending trigger fires,
     *         accepting that it will also re-open the whole mix. */


    if (combo) {
        // Secondary engine delayed
        const float gain_a = 1.0f - synth->blend;
        const float gain_b = synth->blend;

        synth->engine_gain[engine_a] = gain_a;
        synth->engine_gain[engine_b] = gain_b;

        // slightly detune secondary layer with scatter for chaos
        const float note_b_base = (float)note + sonaglio_euclid_offset_for_engine(synth, engine_b);
        const float note_b = note_b_base + rand_bipolar_from_prng(synth) * synth->scatter * 0.35f;

        queue_trigger(synth, engine_a, note, note_a, gain_a, 0);
        queue_trigger(synth, engine_b, note, note_b, gain_b, gap_samples);
    } else {
        // Single instrument: add a quieter delayed shadow hit on the same engine
        const float shadow_gain = 0.12f + synth->blend * 0.55f;
        const float note_shadow = note_a + rand_bipolar_from_prng(synth) * synth->scatter * 0.25f;

        queue_trigger(synth, engine_a, note, note_a, 1.0f, 0);
        queue_trigger(synth, engine_a, note, note_shadow, shadow_gain, gap_samples);
        synth->engine_gain[engine_a] = 1.0f;
    }

    // Keep current envelope behavior
    neon_envelope_trigger(&synth->envelope, active_lane_mask(), synth->current_env_shape);
}

fast_inline void fm_perc_synth_note_off(fm_perc_synth_t* synth, uint8_t note) {
    // Cancel queued secondary hits for this note
    for (int i = 0; i < 8; ++i) {
        pending_trigger_t* ev = &synth->pending[i];
        if (ev->active && ev->midi_note == note) {
            ev->active = 0;
        }
    }

    synth->note_engine_mask[note] = 0;
    neon_envelope_release(&synth->envelope, active_lane_mask());
}

// ============================================================================
// Audio render
// ============================================================================

fast_inline float fm_perc_synth_process(fm_perc_synth_t* synth) {
    process_pending_triggers(synth);

    lfo_smoother_process(&synth->lfo_smooth);

    synth->lfo.shape_combo = (uint32_t)synth->params[PARAM_LFO1_SHAPE];
    synth->lfo.target1 = vgetq_lane_u32(synth->lfo_smooth.current_target1, 0);
    synth->lfo.target2 = vgetq_lane_u32(synth->lfo_smooth.current_target2, 0);
    synth->lfo.depth1 = synth->lfo_smooth.current_depth1;
    synth->lfo.depth2 = synth->lfo_smooth.current_depth2;
    synth->lfo.rate1 = synth->lfo_smooth.current_rate1;
    synth->lfo.rate2 = synth->lfo_smooth.current_rate2;

    float32x4_t lfo1 = vdupq_n_f32(0.0f);
    float32x4_t lfo2 = vdupq_n_f32(0.0f);
    lfo_enhanced_process(&synth->lfo, &lfo1, &lfo2);

    neon_envelope_process(&synth->envelope);
    float32x4_t envelope = synth->envelope.level;

    float32x4_t lfo_pitch_mult = vdupq_n_f32(1.0f);
    float32x4_t lfo_index_add  = vdupq_n_f32(0.0f);
    float32x4_t lfo_noise_add  = vdupq_n_f32(0.0f);
    float32x4_t lfo_env_mult   = vdupq_n_f32(1.0f);
    float32x4_t lfo_metal_gate = vdupq_n_f32(1.0f);

    const uint32_t t1 = synth->lfo.target1;
    const uint32_t t2 = synth->lfo.target2;
    const float32x4_t d1 = synth->lfo.depth1;
    const float32x4_t d2 = synth->lfo.depth2;

    switch (t1) {
        case LFO_TARGET_PITCH:
            lfo_pitch_mult = vaddq_f32(lfo_pitch_mult, vmulq_f32(lfo1, d1));
            break;
        case LFO_TARGET_INDEX:
            lfo_index_add = vaddq_f32(lfo_index_add,
                                      vmulq_f32(vmulq_f32(lfo1, d1), vdupq_n_f32(0.25f)));
            break;
        case LFO_TARGET_ENV:
            lfo_env_mult = vaddq_f32(lfo_env_mult, vmulq_f32(lfo1, d1));
            break;
        case LFO_TARGET_NOISE_MIX:
            lfo_noise_add = vaddq_f32(lfo_noise_add,
                                      vmulq_f32(vmulq_f32(lfo1, d1), vdupq_n_f32(0.15f)));
            break;
        case LFO_TARGET_METAL_GATE:
            lfo_metal_gate = vaddq_f32(vdupq_n_f32(0.5f),
                                       vmulq_f32(vmulq_f32(lfo1, d1), vdupq_n_f32(0.5f)));
            lfo_metal_gate = fm_vclamp01(lfo_metal_gate);
            break;
        default:
            break;
    }

    switch (t2) {
        case LFO_TARGET_PITCH:
            lfo_pitch_mult = vaddq_f32(lfo_pitch_mult, vmulq_f32(lfo2, d2));
            break;
        case LFO_TARGET_INDEX:
            lfo_index_add = vaddq_f32(lfo_index_add,
                                      vmulq_f32(vmulq_f32(lfo2, d2), vdupq_n_f32(0.25f)));
            break;
        case LFO_TARGET_ENV:
            lfo_env_mult = vaddq_f32(lfo_env_mult, vmulq_f32(lfo2, d2));
            break;
        case LFO_TARGET_NOISE_MIX:
            lfo_noise_add = vaddq_f32(lfo_noise_add,
                                      vmulq_f32(vmulq_f32(lfo2, d2), vdupq_n_f32(0.15f)));
            break;
        case LFO_TARGET_METAL_GATE:
            lfo_metal_gate = vaddq_f32(vdupq_n_f32(0.5f),
                                       vmulq_f32(vmulq_f32(lfo2, d2), vdupq_n_f32(0.5f)));
            lfo_metal_gate = fm_vclamp01(lfo_metal_gate);
            break;
        default:
            break;
    }

    lfo_pitch_mult = vmaxq_f32(lfo_pitch_mult, vdupq_n_f32(0.125f));
    envelope = vmulq_f32(envelope, fm_vclamp01(lfo_env_mult));

    uint32x4_t active_mask = vmvnq_u32(vceqq_u32(synth->envelope.stage,
                                                 vdupq_n_u32(ENV_STATE_OFF)));
    active_mask = vandq_u32(active_mask, active_lane_mask());

    float32x4_t hit_shape = vdupq_n_f32(synth->params[PARAM_HIT_SHAPE] * 0.01f);
    float32x4_t body_tilt = vdupq_n_f32(synth->params[PARAM_BODY_TILT] * 0.01f);
    float32x4_t drive = vdupq_n_f32(synth->params[PARAM_DRIVE] * 0.01f);

    float32x4_t transient_env = fm_make_transient_env(envelope, hit_shape);
    float32x4_t body_env = fm_make_body_env(envelope, body_tilt);

    float32x4_t kick_out = vmulq_f32(
        kick_engine_process(&synth->kick, transient_env, active_mask, lfo_pitch_mult, lfo_index_add),
        vdupq_n_f32(synth->engine_gain[ENGINE_KICK])
    );

    float32x4_t snare_out = vmulq_f32(
        snare_engine_process(&synth->snare,
                             transient_env,
                             active_mask,
                             lfo_pitch_mult,
                             vaddq_f32(lfo_index_add, lfo_noise_add),
                             vaddq_f32(synth->snare.noise_mix, lfo_noise_add)),
        vdupq_n_f32(synth->engine_gain[ENGINE_SNARE])
    );

    float32x4_t metal_out = vmulq_f32(
        metal_engine_process(&synth->metal,
                             body_env,
                             active_mask,
                             lfo_pitch_mult,
                             lfo_index_add,
                             synth->metal.brightness,
                             lfo_metal_gate),
        vdupq_n_f32(synth->engine_gain[ENGINE_METAL])
    );

    float32x4_t perc_out = vmulq_f32(
        perc_engine_process(&synth->perc, body_env, active_mask, lfo_pitch_mult, lfo_index_add),
        vdupq_n_f32(synth->engine_gain[ENGINE_PERC])
    );

    float32x4_t mix = vdupq_n_f32(0.0f);
    mix = vaddq_f32(mix, vmulq_n_f32(kick_out,  0.28f));
    mix = vaddq_f32(mix, vmulq_n_f32(snare_out, 0.24f));
    mix = vaddq_f32(mix, vmulq_n_f32(metal_out, 0.22f));
    mix = vaddq_f32(mix, vmulq_n_f32(perc_out,  0.26f));

    mix = vmulq_f32(mix, fm_make_drive_gain(drive));
    mix = fm_soft_clip(mix);
    mix = vmulq_n_f32(mix, synth->master_gain);

    return neon_horizontal_sum_alt(mix);
}

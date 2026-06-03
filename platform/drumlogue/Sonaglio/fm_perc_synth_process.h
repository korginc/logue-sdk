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
#include "hat_engine.h"
#include "lfo_enhanced.h"
#include "lfo_smoothing.h"
#include "envelope_rom.h"
#include "prng.h"
#include "midi_handler.h"
#include "fm_presets.h"

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
    hat_engine_t   hat;

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
    // Post-engine separation filters (lane0 active in selector model).
    one_pole_t kick_lp;
    one_pole_t snare_hp;
    one_pole_t perc_bp_lp;
    one_pole_t perc_bp_hp;
    one_pole_t metal_hp;
    one_pole_t hat_hp;
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
        case ENGINE_HAT:   return vgetq_lane_f32(synth->euclid_offsets, 2);
        default:           return 0.0f;
    }
}

static fast_inline uint8_t engine_to_note_mask_bit(uint8_t engine) {
    switch (engine) {
        case ENGINE_KICK:  return (uint8_t)(1u << 0);
        case ENGINE_SNARE: return (uint8_t)(1u << 1);
        case ENGINE_METAL: return (uint8_t)(1u << 2);
        case ENGINE_PERC:  return (uint8_t)(1u << 3);
        case ENGINE_HAT:   return (uint8_t)(1u << 4);
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
        case ENGINE_HAT:
            hat_engine_set_note(&synth->hat, lane, note_vec);
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

        // Selector model uses delayed combo/shadow triggers.
        // Retrigger the shared envelope here so delayed hits are audible instead
        // of falling into the tail of the first trigger.
        neon_envelope_trigger(&synth->envelope,
                              active_lane_mask(),
                              synth->current_env_shape);

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

    // Hat shares metal params (same page, metal engine idles when hat is active)
    hat_engine_update(&synth->hat,
                      vdupq_n_f32(p[PARAM_METAL_ATK]  * 0.01f),
                      vdupq_n_f32(p[PARAM_METAL_BODY] * 0.01f));

    synth->current_env_shape = (uint8_t)p[PARAM_ENV_SHAPE];
    metal_engine_set_character(&synth->metal,
                               (uint32_t)(synth->current_env_shape >> 7));

    {
        uint8_t mode = (uint8_t)p[PARAM_EUCL_TUN];
        if (mode >= EUCLID_MODE_COUNT) mode = 0;
        synth->euclid_offsets = vld1q_f32(EUCLID_OFFSETS[mode]);
    }

    // Route LFO parameters to the smoother. Without these calls the smoother
    // stays at target=NONE with zero depth, silencing all LFO modulation.
    {
        const uint32x4_t all_lanes = vdupq_n_u32(0xFFFFFFFFu);
        lfo_smoother_set_rate(&synth->lfo_smooth, 0,
                              p[PARAM_LFO1_RATE]  * 0.01f, all_lanes);
        lfo_smoother_set_depth(&synth->lfo_smooth, 0,
                               p[PARAM_LFO1_DEPTH] * 0.01f, all_lanes);
        lfo_smoother_set_target(&synth->lfo_smooth, 0,
                                (uint8_t)p[PARAM_LFO1_TARGET], all_lanes);
        lfo_smoother_set_rate(&synth->lfo_smooth, 1,
                              p[PARAM_LFO2_RATE]  * 0.01f, all_lanes);
        lfo_smoother_set_depth(&synth->lfo_smooth, 1,
                               p[PARAM_LFO2_DEPTH] * 0.01f, all_lanes);
        lfo_smoother_set_target(&synth->lfo_smooth, 1,
                                (uint8_t)p[PARAM_LFO2_TARGET], all_lanes);
    }
}

// load_fm_preset() is provided by fm_presets.cc.
// Keep preset data out of the hot process header to avoid duplicate definitions.

// ============================================================================
// Initialization
// ============================================================================

fast_inline void fm_perc_synth_init(fm_perc_synth_t* synth) {
    kick_engine_init(&synth->kick);
    snare_engine_init(&synth->snare);
    metal_engine_init(&synth->metal);
    perc_engine_init(&synth->perc);
    hat_engine_init(&synth->hat);

    lfo_enhanced_init(&synth->lfo);
    lfo_smoother_init(&synth->lfo_smooth);
    neon_envelope_init(&synth->envelope);

    neon_prng_init(&synth->prng, RAND_DEFAULT_SEED);
    midi_handler_init(&synth->midi);

    synth->current_env_shape = ENV_SHAPE_DEFAULT;
    synth->euclid_offsets = vdupq_n_f32(0.0f);
    synth->master_gain = 1.30f;
    one_pole_reset(&synth->kick_lp);
    one_pole_reset(&synth->snare_hp);
    one_pole_reset(&synth->perc_bp_lp);
    one_pole_reset(&synth->perc_bp_hp);
    one_pole_reset(&synth->metal_hp);
    one_pole_reset(&synth->hat_hp);

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
        case INST_HAT:    engine_a = ENGINE_HAT;   break;
        // combined instruments
        case INST_KS:     engine_a = ENGINE_KICK;  engine_b = ENGINE_SNARE; combo = 1; break;
        case INST_KT:     engine_a = ENGINE_KICK;  engine_b = ENGINE_PERC;  combo = 1; break;
        case INST_KM:     engine_a = ENGINE_KICK;  engine_b = ENGINE_METAL; combo = 1; break;
        case INST_ST:     engine_a = ENGINE_SNARE; engine_b = ENGINE_PERC;  combo = 1; break;
        case INST_SM:     engine_a = ENGINE_SNARE; engine_b = ENGINE_METAL; combo = 1; break;
        case INST_TM:     engine_a = ENGINE_PERC;  engine_b = ENGINE_METAL; combo = 1; break;
        // add new combinations with hat
        case INST_KH:     engine_a = ENGINE_KICK;  engine_b = ENGINE_HAT; combo = 1; break;
        case INST_MH:     engine_a = ENGINE_METAL; engine_b = ENGINE_HAT; combo = 1; break;
        case INST_SH:     engine_a = ENGINE_SNARE; engine_b = ENGINE_HAT; combo = 1; break;
        case INST_TH:     engine_a = ENGINE_PERC;  engine_b = ENGINE_HAT; combo = 1; break;

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
    // Smoother stores normalized 0..1; lfo.rate expects cycles/sample.
    {
        const float32x4_t r_min  = vdupq_n_f32(LFO_ENHANCED_RATE_MIN_HZ
                                                / (float)LFO_ENHANCED_SAMPLE_RATE);
        const float32x4_t r_span = vdupq_n_f32((LFO_ENHANCED_RATE_MAX_HZ
                                                 - LFO_ENHANCED_RATE_MIN_HZ)
                                                / (float)LFO_ENHANCED_SAMPLE_RATE);
        synth->lfo.rate1 = vaddq_f32(r_min,
                                     vmulq_f32(synth->lfo_smooth.current_rate1, r_span));
        synth->lfo.rate2 = vaddq_f32(r_min,
                                     vmulq_f32(synth->lfo_smooth.current_rate2, r_span));
    }

    float32x4_t lfo1 = vdupq_n_f32(0.0f);
    float32x4_t lfo2 = vdupq_n_f32(0.0f);
    lfo_enhanced_process(&synth->lfo, &lfo1, &lfo2);
    // Most macro targets expect bipolar modulation around zero. Convert once
    // here so target handlers below can apply stronger, symmetric movement.
    float32x4_t lfo1_bipolar = lfo_unipolar_to_bipolar(lfo1);
    float32x4_t lfo2_bipolar = lfo_unipolar_to_bipolar(lfo2);

    neon_envelope_process(&synth->envelope);
    float32x4_t envelope = synth->envelope.level;

    // PARAM_NOISE_CHAR baseline: directly sets the noise floor for all engines.
    const float noise_char_v = fm_clampf01(synth->params[PARAM_NOISE_CHAR] * 0.01f);

    float32x4_t lfo_pitch_mult = vdupq_n_f32(1.0f);
    float32x4_t lfo_index_add  = vdupq_n_f32(0.0f);
    float32x4_t lfo_noise_add  = vdupq_n_f32(noise_char_v * 0.35f);
    float32x4_t lfo_env_mult   = vdupq_n_f32(1.0f);
    float32x4_t lfo_metal_gate = vdupq_n_f32(1.0f);

    const uint32_t t1 = synth->lfo.target1;
    const uint32_t t2 = synth->lfo.target2;
    const float32x4_t d1 = synth->lfo.depth1;
    const float32x4_t d2 = synth->lfo.depth2;
    // Onset randomization derived from dual-LFO decorrelation. This creates
    // tiny per-hit instability (detune/jitter) without adding extra RNG in DSP.
    float32x4_t onset_rand = vsubq_f32(lfo1_bipolar, lfo2_bipolar);

    switch (t1) {
        case LFO_TARGET_PITCH:
            lfo_pitch_mult = vaddq_f32(lfo_pitch_mult,
                                       vmulq_f32(vmulq_f32(lfo1_bipolar, d1), vdupq_n_f32(0.65f)));
            break;
        case LFO_TARGET_INDEX:
            lfo_index_add = vaddq_f32(lfo_index_add,
                                      vmulq_f32(vmulq_f32(lfo1_bipolar, d1), vdupq_n_f32(1.20f)));
            break;
        case LFO_TARGET_ENV:
            lfo_env_mult = vaddq_f32(lfo_env_mult,
                                     vmulq_f32(vmulq_f32(lfo1_bipolar, d1), vdupq_n_f32(0.85f)));
            break;
        case LFO_TARGET_NOISE_MIX:
            lfo_noise_add = vaddq_f32(lfo_noise_add,
                                      vmulq_f32(vmulq_f32(lfo1_bipolar, d1), vdupq_n_f32(0.80f)));
            break;
        case LFO_TARGET_METAL_GATE:
            // Full gate range: 0 (silent) when lfo=0,d=1 → 1 (open) when lfo=1 or d=0.
            lfo_metal_gate = vaddq_f32(vsubq_f32(vdupq_n_f32(1.0f), d1),
                                       vmulq_f32(lfo1, d1));
            lfo_metal_gate = fm_vclamp01(lfo_metal_gate);
            break;
        default:
            break;
    }

    switch (t2) {
        case LFO_TARGET_PITCH:
            lfo_pitch_mult = vaddq_f32(lfo_pitch_mult,
                                       vmulq_f32(vmulq_f32(lfo2_bipolar, d2), vdupq_n_f32(0.65f)));
            break;
        case LFO_TARGET_INDEX:
            lfo_index_add = vaddq_f32(lfo_index_add,
                                      vmulq_f32(vmulq_f32(lfo2_bipolar, d2), vdupq_n_f32(1.20f)));
            break;
        case LFO_TARGET_ENV:
            lfo_env_mult = vaddq_f32(lfo_env_mult,
                                     vmulq_f32(vmulq_f32(lfo2_bipolar, d2), vdupq_n_f32(0.85f)));
            break;
        case LFO_TARGET_NOISE_MIX:
            lfo_noise_add = vaddq_f32(lfo_noise_add,
                                      vmulq_f32(vmulq_f32(lfo2_bipolar, d2), vdupq_n_f32(0.80f)));
            break;
        case LFO_TARGET_METAL_GATE:
            lfo_metal_gate = vaddq_f32(vsubq_f32(vdupq_n_f32(1.0f), d2),
                                       vmulq_f32(lfo2, d2));
            lfo_metal_gate = fm_vclamp01(lfo_metal_gate);
            break;
        default:
            break;
    }

    lfo_pitch_mult = vaddq_f32(lfo_pitch_mult, vmulq_n_f32(onset_rand, 0.04f));
    lfo_index_add = vaddq_f32(lfo_index_add, vmulq_n_f32(onset_rand, 0.08f));
    lfo_pitch_mult = lfo_vclamp(lfo_pitch_mult, 0.60f, 1.55f);
    envelope = vmulq_f32(envelope, vmaxq_f32(lfo_env_mult, vdupq_n_f32(0.0f)));

    uint32x4_t active_mask = vmvnq_u32(vceqq_u32(synth->envelope.stage,
                                                 vdupq_n_u32(ENV_STATE_OFF)));
    active_mask = vandq_u32(active_mask, active_lane_mask());

    float32x4_t hit_shape = vdupq_n_f32(synth->params[PARAM_HIT_SHAPE] * 0.01f);
    float32x4_t body_tilt = vdupq_n_f32(synth->params[PARAM_BODY_TILT] * 0.01f);

    float32x4_t transient_env = fm_make_transient_env(envelope, hit_shape);
    float32x4_t body_env = fm_make_body_env(envelope, body_tilt);

    // Skip engine DSP when gain is zero (single-instrument mode = ~75% engine compute saved).
    // Zero-gain engines still feed silence through their separation filters below,
    // keeping filter state at zero for a clean re-entry on the next trigger.
    float32x4_t kick_out = vdupq_n_f32(0.0f);
    if (synth->engine_gain[ENGINE_KICK] > 0.0f) {
        kick_out = vmulq_f32(
            kick_engine_process(&synth->kick, transient_env, active_mask, lfo_pitch_mult, lfo_index_add),
            vdupq_n_f32(synth->engine_gain[ENGINE_KICK])
        );
    }

    float32x4_t snare_out = vdupq_n_f32(0.0f);
    if (synth->engine_gain[ENGINE_SNARE] > 0.0f) {
        snare_out = vmulq_f32(
            snare_engine_process(&synth->snare,
                                 transient_env,
                                 active_mask,
                                 lfo_pitch_mult,
                                 vaddq_f32(lfo_index_add, lfo_noise_add),
                                 vaddq_f32(synth->snare.noise_mix, lfo_noise_add)),
            vdupq_n_f32(synth->engine_gain[ENGINE_SNARE])
        );
    }

    float32x4_t metal_out = vdupq_n_f32(0.0f);
    if (synth->engine_gain[ENGINE_METAL] > 0.0f) {
        metal_out = vmulq_f32(
            metal_engine_process(&synth->metal,
                                 body_env,
                                 active_mask,
                                 lfo_pitch_mult,
                                 lfo_index_add,
                                 vdupq_n_f32(0.0f), /* brightness_add: LFO target unused */
                                 lfo_metal_gate,
                                 lfo_noise_add),
            vdupq_n_f32(synth->engine_gain[ENGINE_METAL])
        );
    }

    float32x4_t perc_out = vdupq_n_f32(0.0f);
    if (synth->engine_gain[ENGINE_PERC] > 0.0f) {
        perc_out = vmulq_f32(
            perc_engine_process(&synth->perc, body_env, active_mask, lfo_pitch_mult, lfo_index_add),
            vdupq_n_f32(synth->engine_gain[ENGINE_PERC])
        );
    }

    float32x4_t hat_out = vdupq_n_f32(0.0f);
    if (synth->engine_gain[ENGINE_HAT] > 0.0f) {
        hat_out = vmulq_f32(
            hat_engine_process(&synth->hat, transient_env, active_mask, lfo_pitch_mult, lfo_noise_add),
            vdupq_n_f32(synth->engine_gain[ENGINE_HAT])
        );
    }

    float32x4_t mix = vdupq_n_f32(0.0f);
    // Separation EQ per engine (lightweight one-pole):
    // kick: low-passed body support, snare: high-passed crack/noise,
    // metal: brighter high-pass, perc: mid-focused band-pass-ish layer.
    float32x4_t kick_sep = one_pole_lpf(&synth->kick_lp, kick_out, 190.0f);
    float32x4_t snare_sep = one_pole_hpf(&synth->snare_hp, snare_out, 420.0f);
    // HPF was 1800 Hz which cut below the 500 Hz carrier fundamental — now 500 Hz.
    float32x4_t metal_sep = one_pole_hpf(&synth->metal_hp, metal_out, 500.0f);
    float32x4_t perc_lp = one_pole_lpf(&synth->perc_bp_lp, perc_out, 1400.0f);
    float32x4_t perc_hp = one_pole_hpf(&synth->perc_bp_hp, perc_lp, 220.0f);
    float32x4_t perc_sep = perc_hp;
    // Hat: high-pass only — squares are already mid/high, remove any sub content.
    float32x4_t hat_sep = one_pole_hpf(&synth->hat_hp, hat_out, 800.0f);
    // Selector model: only one lane and usually one/two engines are active.
    // These are no longer four-voice summing weights; they are nominal engine
    // output trims. Keep them strong enough for direct HW monitoring.
    mix = vaddq_f32(mix, vmulq_n_f32(kick_sep,  1.00f));
    mix = vaddq_f32(mix, vmulq_n_f32(snare_sep, 0.96f));
    mix = vaddq_f32(mix, vmulq_n_f32(metal_sep, 0.92f));
    mix = vaddq_f32(mix, vmulq_n_f32(perc_sep,  0.93f));
    mix = vaddq_f32(mix, vmulq_n_f32(hat_sep,   0.88f));
    // Apply the master gain before the soft clip to avoid pushing the values outside the [-1.0, 1.0]
    mix = fm_soft_clip(vmulq_n_f32(mix, synth->master_gain));

    return neon_horizontal_sum_alt(mix);
}

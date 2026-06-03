#pragma once

/**
 * @file metal_engine.h
 * @brief 6-operator FM metal / cymbal engine — 3 independent carrier+modulator pairs
 *
 * Topology (copych algo11 style, MIT © 2025 copych):
 *   [ops[1]] → [ops[0]]  (pair A)
 *   [ops[3]] → [ops[2]]  (pair B)
 *   [ops[5]] → [ops[4]]  (pair C)
 *
 * Each pair rings at its own inharmonic ratio independently.
 * This avoids the serial-cascade hash problem: upper-stage modulation
 * no longer corrupts the carrier's fundamental ring.
 *
 * Uses fm_operator.h for per-operator state and waveform computation.
 * Operators use DX7-style exponential fm_level (0.1 * 161^v − 0.1)
 * so small volume changes near 0 have little effect, high values push
 * into dense FM territory — natural and perceptually linear.
 *
 * Character variants:
 *   0 — Cymbal  (DX7 metallic ratios)
 *   1 — Gong    (wider inharmonic cluster)
 *
 * Parameters:
 *   Attack — modulator volume + feedback → FM brightness and density at onset
 *   Body   — ring weight + carrier level → ring sustain and inharmonic spread
 */

#include <arm_neon.h>
#include "fm_operator.h"
#include "fm_voices.h"
#include "prng.h"
#include "engine_mapping.h"

#define METAL_FREQ_MIN  500.0f
#define METAL_FREQ_MAX 8000.0f

/* Precomputed one-pole filter coefficients */
#define METAL_NOISE_HP_A  0.2820f  /* HPF ~2500 Hz alpha */
#define METAL_NOISE_BPF_A 0.3548f  /* LPF ~4200 Hz alpha (after HP gives BPF) */

/* Cymbal character: DX7 inharmonic ratios, 3 carrier+mod pairs.
 * Carrier at ratio[i], modulator at ratio[i+1] — each pair is one inharmonic partial. */
static const float METAL_CYM_CAR[3] = { 1.000f, 1.483f, 1.932f };
static const float METAL_CYM_MOD[3] = { 1.483f, 1.932f, 2.546f };

/* Gong character: wider inharmonic cluster (tam-tam/gong ratios) */
static const float METAL_GONG_CAR[3] = { 1.000f, 2.756f, 3.752f };
static const float METAL_GONG_MOD[3] = { 2.756f, 3.752f, 5.404f };

typedef struct {
    fm_op_t ops[6];          /* ops[0,2,4] = carriers; ops[1,3,5] = modulators */

    float carrier_freq_base; /* base frequency (MIDI-mapped, Hz) */

    float attack;            /* 0..1 — brightness / FM density at onset */
    float body;              /* 0..1 — ring weight / inharmonic spread */

    /* Derived, updated in recompute() */
    float strike_weight;     /* env^8 coefficient in modulator envelope */
    float ring_weight;       /* linear envelope coefficient in modulator envelope */
    float ring_gain;         /* carrier output amplitude */
    float noise_gain;        /* noise layer amplitude */
    float output_gain;       /* master output scale */

    uint32_t char_select;    /* 0 = Cymbal, 1 = Gong */

    neon_prng_t noise_prng;
    one_pole_t  noise_hpf;
    one_pole_t  noise_bpf;
} metal_engine_t;

/* Apply ratio tables with body-controlled spread.
 * At body=0 ratios cluster near 1.0; at body=1 they spread to the character values. */
static inline void metal_apply_ratios(metal_engine_t* metal, float ratio_scale) {
    const float* car = (metal->char_select == 0) ? METAL_CYM_CAR : METAL_GONG_CAR;
    const float* mod = (metal->char_select == 0) ? METAL_CYM_MOD : METAL_GONG_MOD;
    for (int i = 0; i < 3; i++) {
        float car_r = 1.0f + (car[i] - 1.0f) * ratio_scale;
        float mod_r = 1.0f + (mod[i] - 1.0f) * ratio_scale;
        fmo_set_ratio(&metal->ops[i * 2],     car_r);
        fmo_set_ratio(&metal->ops[i * 2 + 1], mod_r);
    }
}

fast_inline void metal_engine_recompute(metal_engine_t* metal) {
    float atk = metal->attack;
    float bdy = metal->body;
    if (atk < 0.0f) atk = 0.0f; if (atk > 1.0f) atk = 1.0f;
    if (bdy < 0.0f) bdy = 0.0f; if (bdy > 1.0f) bdy = 1.0f;
    metal->attack = atk;
    metal->body   = bdy;

    /* Modulator volume → exponential fm_level via copych curve.
     * Attack drives FM depth (brightness at onset). Range 0.35..1.0. */
    float mod_vol = 0.35f + atk * 0.65f;
    for (int i = 1; i < 6; i += 2) {
        fmo_set_volume(&metal->ops[i], mod_vol);
        fmo_set_feedback(&metal->ops[i], atk * 2.5f);   /* DX7 fb 0..2.5 */
    }

    /* Carrier volume → amplitude weight of each partial. Range 0.55..1.0. */
    float car_vol = 0.55f + bdy * 0.45f;
    for (int i = 0; i < 6; i += 2) {
        fmo_set_volume(&metal->ops[i], car_vol);
    }

    /* Envelope weighting for modulator: strike (env8) vs ring (linear).
     * High attack = more bright onset burst. High body = more sustained ring FM. */
    metal->strike_weight = 0.40f + atk * 0.60f;
    metal->ring_weight   = 0.08f + bdy * 0.40f;

    /* Carrier output amplitude: body = heavier ring. */
    metal->ring_gain = 0.30f + bdy * 0.25f;

    /* Noise: attack drives brightness of strike noise. */
    metal->noise_gain = 0.020f + atk * 0.18f;

    /* Output gain: slightly lower at high attack to prevent clipping. */
    metal->output_gain = 0.42f - atk * 0.07f;

    /* Ratio spread: body=0 → cluster near 1; body=1 → full character ratios. */
    float ratio_scale = 0.82f + bdy * 0.68f;
    metal_apply_ratios(metal, ratio_scale);
}

fast_inline void metal_engine_init(metal_engine_t* metal) {
    for (int i = 0; i < 6; i++)
        fmo_init(&metal->ops[i]);

    metal->carrier_freq_base = 1000.0f;
    metal->char_select       = 0;
    metal->attack            = 0.5f;
    metal->body              = 0.5f;

    /* Modulator waveform: sine (default); can be changed per character if desired */
    for (int i = 1; i < 6; i += 2)
        metal->ops[i].waveform = WF_SINE;
    for (int i = 0; i < 6; i += 2)
        metal->ops[i].waveform = WF_SINE;

    {
        uint64_t s0[2] = { 0xCAFEBABE12345678ULL, 0xFEDCBA0987654321ULL };
        uint64_t s1[2] = { 0x0F0F0F0F0F0F0F0FULL, 0xA1B2C3D4E5F60718ULL };
        metal->noise_prng.state0 = vld1q_u64(s0);
        metal->noise_prng.state1 = vld1q_u64(s1);
    }
    metal->noise_hpf.z1 = vdupq_n_f32(0.0f);
    metal->noise_bpf.z1 = vdupq_n_f32(0.0f);

    metal_engine_recompute(metal);
}

/** Select metal engine character: 0 = Cymbal, 1 = Gong */
fast_inline void metal_engine_set_character(metal_engine_t* metal, uint32_t character) {
    character = character ? 1U : 0U;
    if (character == metal->char_select) return;
    metal->char_select = character;
    metal_engine_recompute(metal);
}

fast_inline void metal_engine_update(metal_engine_t* metal,
                                     float32x4_t param1,
                                     float32x4_t param2) {
    metal->attack = vgetq_lane_f32(param1, 0);
    metal->body   = vgetq_lane_f32(param2, 0);
    metal_engine_recompute(metal);
}

fast_inline void metal_engine_set_note(metal_engine_t* metal,
                                       uint32x4_t voice_mask,
                                       float32x4_t midi_notes) {
    if (vgetq_lane_u32(voice_mask, 0) == 0) return;

    float32x4_t freq_v = fm_midi_to_freq(midi_notes, METAL_FREQ_MIN, METAL_FREQ_MAX);
    float freq = vgetq_lane_f32(freq_v, 0);
    metal->carrier_freq_base = freq;

    /* Update all operator base frequencies (ratios are preserved) */
    for (int i = 0; i < 6; i++) {
        fmo_set_freq(&metal->ops[i], freq);
        fmo_reset(&metal->ops[i]);
    }
}

/**
 * Process one sample of metal engine.
 *
 * NEON API is preserved for compatibility with fm_perc_synth_process.h.
 * Only lane 0 is used; result is broadcast to all lanes.
 */
fast_inline float32x4_t metal_engine_process(metal_engine_t* metal,
                                             float32x4_t envelope,
                                             uint32x4_t active_mask,
                                             float32x4_t lfo_pitch_mult,
                                             float32x4_t lfo_index_add,
                                             float32x4_t brightness_add,
                                             float32x4_t metal_gate,
                                             float32x4_t noise_add) {
    const float env0    = vgetq_lane_f32(envelope,      0);
    const float gate0   = vgetq_lane_f32(metal_gate,    0);
    const float pitch0  = vgetq_lane_f32(lfo_pitch_mult, 0);
    const float idx_add = vgetq_lane_f32(lfo_index_add,  0);
    const float noise_a = vgetq_lane_f32(noise_add,      0);
    const float bright_a= vgetq_lane_f32(brightness_add, 0);

    const float gated = env0 * gate0;

    /* Power-law envelope domains */
    const float env2 = gated * gated;
    const float env4 = env2 * env2;
    const float env8 = env4 * env4;

    /* Sqrt envelope for long metallic ring tail */
    const float env_sqrt = (gated > 1e-8f) ? sqrtf(gated) : 0.0f;

    /* Combined modulator envelope: fast burst (env8) + slow ring (linear).
     * LFO index modulation scales the ring component. */
    float ring_scale = 1.0f + idx_add;
    if (ring_scale < 0.0f) ring_scale = 0.0f;
    const float mod_env = env8 * metal->strike_weight
                        + gated * metal->ring_weight * ring_scale;

    /* Process 3 independent carrier+modulator pairs (algo11).
     * Each pair rings at its own inharmonic ratio → clean partial separation. */
    const float m0 = fmo_mod(&metal->ops[1], 0.0f, mod_env, pitch0);
    const float c0 = fmo_out(&metal->ops[0], m0,   env_sqrt, pitch0);

    const float m1 = fmo_mod(&metal->ops[3], 0.0f, mod_env, pitch0);
    const float c1 = fmo_out(&metal->ops[2], m1,   env_sqrt, pitch0);

    const float m2 = fmo_mod(&metal->ops[5], 0.0f, mod_env, pitch0);
    const float c2 = fmo_out(&metal->ops[4], m2,   env_sqrt, pitch0);

    /* Sum 3 carriers: 1/√3 normalisation */
    float fm_out = (c0 + c1 + c2) * metal->ring_gain * 0.57735027f;

    /* Strike noise: HP + BPF blend, gated by env2 (dies with transient) */
    {
        const float white = vgetq_lane_f32(white_noise(&metal->noise_prng), 0);

        float z1_hp = vgetq_lane_f32(metal->noise_hpf.z1, 0);
        const float lp_hp = z1_hp + METAL_NOISE_HP_A * (white - z1_hp);
        metal->noise_hpf.z1 = vdupq_n_f32(lp_hp);
        const float noise_hp = white - lp_hp;

        float z1_bp = vgetq_lane_f32(metal->noise_bpf.z1, 0);
        const float noise_bp = z1_bp + METAL_NOISE_BPF_A * (noise_hp - z1_bp);
        metal->noise_bpf.z1 = vdupq_n_f32(noise_bp);

        const float noise_colored = noise_hp * 0.58f + noise_bp * 0.42f;

        float adj_gain = metal->noise_gain + noise_a;
        if (adj_gain < 0.0f) adj_gain = 0.0f;

        float brightness = metal->attack + bright_a;
        if (brightness > 1.0f) brightness = 1.0f;

        const float noise_level = adj_gain * (0.65f + brightness * 0.65f);
        fm_out += noise_colored * noise_level * env2;
    }

    fm_out *= metal->output_gain;

    const float32x4_t result = vdupq_n_f32(fm_out);
    return vbslq_f32(active_mask, result, vdupq_n_f32(0.0f));
}

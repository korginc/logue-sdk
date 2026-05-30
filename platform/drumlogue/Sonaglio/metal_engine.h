#pragma once

/**
 *  @file metal_engine.h
 *  @brief 4-operator FM metal / cymbal / struck-object engine
 *
 *  Parameters:
 *    Param1: Attack / Energy / Brightness
 *    Param2: Body / Ring / Stability
 *
 *  Design intent:
 *  - Attack makes the hit brighter, noisier, harder, and more unstable at onset.
 *  - Body increases ring weight and density, but should not destroy the tonal center.
 *
 *  CPU policy:
 *  - Keep the existing 4-op FM topology for character.
 *  - Move all parameter-derived values into update()/set_character().
 *  - Keep the process path mostly phase increments + sine calls + precomputed gains.
 */

#include <arm_neon.h>
#include "fm_voices.h"
#include "sine_neon.h"
#include "prng.h"
#include "engine_mapping.h"

#define NUM_OPERATORS (4)

// Noise shaping for the bright transient layer.
// This is alpha, not cutoff Hz.
#define METAL_NOISE_HP_A 0.2820f

#define METAL_FREQ_MIN 500.0f
#define METAL_FREQ_MAX 8000.0f

// Character 0 — DX7-style cymbal / metallic strike
#define METAL_RATIO1 1.000f
#define METAL_RATIO2 1.483f
#define METAL_RATIO3 1.932f
#define METAL_RATIO4 2.546f

// Character 1 — Gong / tam-tam / wider inharmonic cluster
#define METAL_GONG_RATIO1 1.000f
#define METAL_GONG_RATIO2 2.756f
#define METAL_GONG_RATIO3 3.752f
#define METAL_GONG_RATIO4 5.404f

typedef struct {
    // Four operators
    float32x4_t phase[NUM_OPERATORS];
    float32x4_t base_ratio[NUM_OPERATORS];
    float32x4_t current_ratio[NUM_OPERATORS];

    // Carrier frequency
    float32x4_t carrier_freq_base;

    // UI controls
    float32x4_t attack;      // Param1: edge / brightness / instability
    float32x4_t body;        // Param2: ring / density / stability

    // Derived controls
    float32x4_t brightness;
    float32x4_t ring_gain;
    float32x4_t strike_index;
    float32x4_t ring_index;
    float32x4_t noise_gain;
    float32x4_t op2_weight;
    float32x4_t op3_weight;
    float32x4_t op4_weight;
    float32x4_t output_gain;

    // Op4 self-feedback: feeds back on previous output for metallic bite.
    // Zero feedback = sinusoidal modulator; ~0.45 = rich, square-wave-like spectrum.
    float32x4_t feedback_gain;
    float32x4_t prev_op4;

    // Character variant (0 = Cymbal, 1 = Gong)
    uint32_t char_select;

    // Noise layer
    neon_prng_t noise_prng;
    one_pole_t noise_hpf;
    one_pole_t noise_bpf;
} metal_engine_t;

fast_inline float32x4_t metal_clamp01(float32x4_t x) {
    return vmaxq_f32(vdupq_n_f32(0.0f), vminq_f32(vdupq_n_f32(1.0f), x));
}

/**
 * Recompute parameter-derived controls.
 *
 * Important change from the previous version:
 * Body no longer blindly doubles the ratio offsets. That was good for chaos,
 * but it could also become hashy and less useful in the mix. Body now moves
 * between a slightly restrained cluster and the selected character cluster.
 */
fast_inline void metal_engine_recompute(metal_engine_t* metal) {
    const float32x4_t one = vdupq_n_f32(1.0f);

    float32x4_t attack = metal_clamp01(metal->attack);
    float32x4_t body   = metal_clamp01(metal->body);

    metal->attack = attack;
    metal->body = body;

    // Brightness follows attack, but body moderates it slightly so high-body
    // presets can ring without becoming all top-end.
    metal->brightness = vaddq_f32(vdupq_n_f32(0.20f),
                                  vaddq_f32(vmulq_n_f32(attack, 0.72f),
                                             vmulq_n_f32(body,   0.12f)));

    // Ratio scale:
    // body=0 -> slightly tighter than the selected character
    // body=1 -> slightly wider than the selected character, but not double.
    float32x4_t ratio_scale = vaddq_f32(vdupq_n_f32(0.82f),
                                        vmulq_n_f32(body, 0.38f));

    for (int i = 0; i < NUM_OPERATORS; ++i) {
        float32x4_t offset = vsubq_f32(metal->base_ratio[i], one);
        metal->current_ratio[i] = vaddq_f32(one, vmulq_f32(offset, ratio_scale));
    }

    // Strike FM: punchy onset burst (env8-gated in process). 1.0..5.5.
    metal->strike_index = vaddq_f32(vdupq_n_f32(1.0f),
                                    vmulq_n_f32(attack, 4.5f));

    // Ring FM: DX7/MD-drum style metallic indices. Range 10..16.5.
    // Low values (1.5..4.7) created a narrow-bandwidth sustained tone = strings.
    // At higher indices the FM cascade produces a dense, inharmonic metallic spectrum.
    metal->ring_index = vaddq_f32(vdupq_n_f32(10.0f),
                                  vaddq_f32(vmulq_n_f32(body, 5.0f),
                                             vmulq_n_f32(attack, 1.5f)));

    // Op4 self-feedback: body controls how square-wave-like the top modulator gets.
    // 0 = sinusoidal, 0.45 = rich and harsh (DX7-like feedback index 5-6).
    metal->feedback_gain = vaddq_f32(vmulq_n_f32(body, 0.42f),
                                      vmulq_n_f32(attack, 0.12f));

    // Ring gain scaled down for the much hotter FM indices.
    float char_body_bonus = (metal->char_select == 0) ? 0.0f : 0.08f;
    metal->ring_gain = vaddq_f32(vdupq_n_f32(0.28f + char_body_bonus),
                                 vmulq_n_f32(body, 0.20f));

    // Noise should sell the strike, not dominate the tail.
    metal->noise_gain = vaddq_f32(vdupq_n_f32(0.025f),
                                  vmulq_n_f32(attack, 0.22f));

    // Precompute harmonic/operator weights for additive mix of upper partials.
    metal->op2_weight = vaddq_f32(vdupq_n_f32(0.20f),
                                  vaddq_f32(vmulq_n_f32(attack, 0.18f),
                                             vmulq_n_f32(body, 0.10f)));

    metal->op3_weight = vaddq_f32(vdupq_n_f32(0.14f),
                                  vaddq_f32(vmulq_n_f32(attack, 0.20f),
                                             vmulq_n_f32(body, 0.10f)));

    metal->op4_weight = vaddq_f32(vdupq_n_f32(0.08f),
                                  vaddq_f32(vmulq_n_f32(attack, 0.15f),
                                             vmulq_n_f32(body, 0.05f)));

    // Output gain scaled down for high FM index (hot signal).
    metal->output_gain = vsubq_f32(vdupq_n_f32(0.34f),
                                   vmulq_n_f32(attack, 0.07f));
}

/**
 * Initialize metal engine
 */
fast_inline void metal_engine_init(metal_engine_t* metal) {
    for (int i = 0; i < NUM_OPERATORS; ++i) {
        metal->phase[i] = vdupq_n_f32(0.0f);
    }

    metal->base_ratio[0] = vdupq_n_f32(METAL_RATIO1);
    metal->base_ratio[1] = vdupq_n_f32(METAL_RATIO2);
    metal->base_ratio[2] = vdupq_n_f32(METAL_RATIO3);
    metal->base_ratio[3] = vdupq_n_f32(METAL_RATIO4);

    for (int i = 0; i < NUM_OPERATORS; ++i) {
        metal->current_ratio[i] = metal->base_ratio[i];
    }

    metal->carrier_freq_base = vdupq_n_f32(1000.0f);

    metal->attack = vdupq_n_f32(0.5f);
    metal->body = vdupq_n_f32(0.5f);
    metal->brightness = vdupq_n_f32(0.5f);
    metal->ring_gain = vdupq_n_f32(0.38f);
    metal->strike_index = vdupq_n_f32(3.25f);
    metal->ring_index = vdupq_n_f32(10.5f);
    metal->noise_gain = vdupq_n_f32(0.06f);
    metal->op2_weight = vdupq_n_f32(0.29f);
    metal->op3_weight = vdupq_n_f32(0.175f);
    metal->op4_weight = vdupq_n_f32(0.09f);
    metal->output_gain = vdupq_n_f32(0.26f);
    metal->feedback_gain = vdupq_n_f32(0.225f);
    metal->prev_op4 = vdupq_n_f32(0.0f);

    metal->char_select = 0;

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

/**
 * Select metal engine character.
 *   0 = Cymbal / DX7-style
 *   1 = Gong / tam-tam
 */
fast_inline void metal_engine_set_character(metal_engine_t* metal, uint32_t character) {
    character = character ? 1U : 0U;
    if (character == metal->char_select) return;

    metal->char_select = character;

    if (character == 0) {
        metal->base_ratio[0] = vdupq_n_f32(METAL_RATIO1);
        metal->base_ratio[1] = vdupq_n_f32(METAL_RATIO2);
        metal->base_ratio[2] = vdupq_n_f32(METAL_RATIO3);
        metal->base_ratio[3] = vdupq_n_f32(METAL_RATIO4);
    } else {
        metal->base_ratio[0] = vdupq_n_f32(METAL_GONG_RATIO1);
        metal->base_ratio[1] = vdupq_n_f32(METAL_GONG_RATIO2);
        metal->base_ratio[2] = vdupq_n_f32(METAL_GONG_RATIO3);
        metal->base_ratio[3] = vdupq_n_f32(METAL_GONG_RATIO4);
    }

    metal_engine_recompute(metal);
}

/**
 * Update metal engine parameters from UI
 */
fast_inline void metal_engine_update(metal_engine_t* metal,
                                     float32x4_t param1,  // Attack / Energy / Brightness
                                     float32x4_t param2) { // Body / Ring / Stability
    metal->attack = param1;
    metal->body = param2;
    metal_engine_recompute(metal);
}

/**
 * Set MIDI note.
 *
 * This engine benefits from a phase reset: otherwise the onset can become
 * inconsistent and weaker depending on where the operator phases happen to be.
 */
fast_inline void metal_engine_set_note(metal_engine_t* metal,
                                       uint32x4_t voice_mask,
                                       float32x4_t midi_notes) {
    float32x4_t base_freq = fm_midi_to_freq(midi_notes, METAL_FREQ_MIN, METAL_FREQ_MAX);
    metal->carrier_freq_base = vbslq_f32(voice_mask,
                                         base_freq,
                                         metal->carrier_freq_base);

    // Reset phase and feedback state on trigger for repeatable metallic attacks.
    const float32x4_t zero = vdupq_n_f32(0.0f);
    for (int i = 0; i < NUM_OPERATORS; ++i) {
        metal->phase[i] = vbslq_f32(voice_mask, zero, metal->phase[i]);
    }
    metal->prev_op4 = vbslq_f32(voice_mask, zero, metal->prev_op4);
}

/**
 * Process one sample of metal engine.
 */
fast_inline float32x4_t metal_engine_process(metal_engine_t* metal,
                                             float32x4_t envelope,
                                             uint32x4_t active_mask,
                                             float32x4_t lfo_pitch_mult,
                                             float32x4_t lfo_index_add,
                                             float32x4_t brightness_add,
                                             float32x4_t metal_gate,
                                             float32x4_t noise_add) {
    const float32x4_t two_pi_over_sr = vdupq_n_f32(2.0f * M_PI * INV_SAMPLE_RATE);
    const float32x4_t two_pi = vdupq_n_f32(2.0f * M_PI);

    // Gate the envelope so the gate target can shorten the ring.
    float32x4_t gated_env = vmulq_f32(envelope, metal_gate);

    // Staggered decay domains for cascade FM index shaping.
    // env8 decays fastest (strike burst), env2 moderate, linear slowest.
    // This creates cascading harmonic fade: high partials die first (cymbal-like).
    float32x4_t env2 = vmulq_f32(gated_env, gated_env);
    float32x4_t env4 = vmulq_f32(env2, env2);
    float32x4_t env8 = vmulq_f32(env4, env4);
    float32x4_t live_brightness = metal_clamp01(vaddq_f32(metal->brightness, brightness_add));

    // Pitch modulation. A short upward strike bend makes the carrier cluster
    // visibly move at the attack instead of ringing as a static organ/string tone.
    float32x4_t bend_oct = vmulq_f32(env2,
                                     vaddq_f32(vdupq_n_f32(0.05f),
                                               vmulq_n_f32(live_brightness, 0.18f)));
    float32x4_t base_freq = vmulq_f32(metal->carrier_freq_base, lfo_pitch_mult);
    base_freq = vmulq_f32(base_freq, exp2_neon(bend_oct));

    // Advance all operator phases.
    for (int i = 0; i < NUM_OPERATORS; ++i) {
        float32x4_t freq = vmulq_f32(base_freq, metal->current_ratio[i]);
        metal->phase[i] = vaddq_f32(metal->phase[i], vmulq_f32(freq, two_pi_over_sr));

        uint32x4_t wrap = vcgeq_f32(metal->phase[i], two_pi);
        metal->phase[i] = vbslq_f32(wrap,
                                    vsubq_f32(metal->phase[i], two_pi),
                                    metal->phase[i]);
    }

    // FM indices.
    float32x4_t ring_index = vmaxq_f32(vdupq_n_f32(0.0f),
                                        vaddq_f32(metal->ring_index, lfo_index_add));
    float32x4_t strike_index = vmulq_f32(metal->strike_index, env8);

    // Operator cascade: Op4 -> Op3 -> Op2 -> Op1.
    //
    // Op4 self-feedback: previous output feeds back into its own phase.
    // This progressively distorts Op4 toward a square-wave spectrum,
    // adding the inharmonic upper partials that define metallic "bite".
    float32x4_t phase4_fb = vaddq_f32(metal->phase[3],
                                      vmulq_f32(metal->prev_op4, metal->feedback_gain));
    float32x4_t op4 = neon_sin_fast(phase4_fb);
    metal->prev_op4 = op4;

    // Op4 -> Op3: use env2 for the ring component so upper inharmonic sidebands
    // remain audible through the tail; only the extra strike burst is env8-fast.
    float32x4_t phase3_mod = vaddq_f32(metal->phase[2],
                                       vmulq_f32(op4,
                                                 vaddq_f32(vmulq_f32(ring_index, env2),
                                                           strike_index)));
    float32x4_t op3 = neon_sin_fast(phase3_mod);

    // Op3 -> Op2: intermediate FM decay (env2).
    float32x4_t phase2_mod = vaddq_f32(metal->phase[1],
                                       vmulq_f32(op3,
                                                 vmulq_f32(ring_index, env2)));
    float32x4_t op2 = neon_sin_fast(phase2_mod);

    // Op2 -> Op1 carrier: FM index follows linear gated_env.
    // Linear decay means FM complexity fades at the same rate as amplitude:
    // attack → complex metallic splash, tail → cleaner ring → silence.
    float32x4_t phase1_mod = vaddq_f32(metal->phase[0],
                                       vmulq_f32(op2,
                                                 vmulq_f32(ring_index, gated_env)));
    float32x4_t op1 = neon_sin_fast(phase1_mod);

    // Weighted additive partials. Precomputed weights are still lightly scaled
    // by live_brightness so LFO brightness has a real effect.
    float32x4_t bright_scale = vaddq_f32(vdupq_n_f32(0.75f),
                                         vmulq_n_f32(live_brightness, 0.35f));

    float32x4_t harmonics = vaddq_f32(
        vaddq_f32(vmulq_f32(op2, vmulq_f32(metal->op2_weight, bright_scale)),
                  vmulq_f32(op3, vmulq_f32(metal->op3_weight, bright_scale))),
        vmulq_f32(op4, vmulq_f32(metal->op4_weight, bright_scale))
    );

    // Amplitude follows gated_env (linear with envelope curve from ROM).
    // Using linear here prevents the sustained "string" character caused by sqrt decay.
    float32x4_t fm_output = vmulq_f32(vaddq_f32(op1, harmonics),
                                      vmulq_f32(gated_env, metal->ring_gain));

    // Bright HP noise only at the strike. Using env4 instead of full envelope
    // avoids noisy tails while making the hit more audible.
    {
        float32x4_t white = white_noise(&metal->noise_prng);
        float32x4_t lp = one_pole_lpf_a(&metal->noise_hpf, white, METAL_NOISE_HP_A);
        float32x4_t noise_hp = vsubq_f32(white, lp);
        float32x4_t noise_bp = one_pole_lpf(&metal->noise_bpf, noise_hp, 7600.0f);
        float32x4_t noise_colored = vaddq_f32(vmulq_n_f32(noise_hp, 0.58f),
                                              vmulq_n_f32(noise_bp, 0.42f));

        float32x4_t adj_noise_gain = vaddq_f32(metal->noise_gain, noise_add);
        float32x4_t noise_level = vmulq_f32(adj_noise_gain,
                                            vaddq_f32(vdupq_n_f32(0.65f),
                                                       vmulq_n_f32(live_brightness, 0.65f)));

        fm_output = vaddq_f32(fm_output,
                              vmulq_f32(noise_colored, vmulq_f32(noise_level, env2)));
    }

    fm_output = vmulq_f32(fm_output, metal->output_gain);

    return vbslq_f32(active_mask, fm_output, vdupq_n_f32(0.0f));
}

#ifdef TEST_METAL

void test_metal_body_ratio_mapping() {
    metal_engine_t metal;
    metal_engine_init(&metal);

    metal_engine_update(&metal, vdupq_n_f32(0.0f), vdupq_n_f32(0.0f));
    float low_body_r4 = vgetq_lane_f32(metal.current_ratio[3], 0);

    metal_engine_update(&metal, vdupq_n_f32(0.0f), vdupq_n_f32(1.0f));
    float high_body_r4 = vgetq_lane_f32(metal.current_ratio[3], 0);

    assert(high_body_r4 > low_body_r4);
    assert(high_body_r4 < (METAL_RATIO4 * 1.25f));

    printf("Metal engine body ratio mapping test PASSED\n");
}

#endif

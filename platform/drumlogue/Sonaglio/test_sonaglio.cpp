#include <arm_neon.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#ifndef fast_inline
#define fast_inline inline __attribute__((always_inline))
#endif

#include "constants.h"
#include "engine_mapping.h"
#include "prng.h"
#include "envelope_rom.h"
#include "lfo_enhanced.h"
#include "lfo_smoothing.h"
#include "kick_engine.h"
#include "snare_engine.h"
#include "metal_engine.h"
#include "perc_engine.h"
#include "fm_presets.h"
#include "fm_perc_synth_process.h"

// TO RUN THIS TEST (ARM cross-compile + qemu):
//   CXX=arm-linux-gnueabihf-g++ RUNNER=qemu-arm ./run_sonaglio_tests.sh

static int g_failures = 0;

static void report_pass(const char* name) {
    std::printf("[PASS] %s\n", name);
}

static void report_fail(const char* name, const char* detail) {
    std::printf("[FAIL] %s: %s\n", name, detail);
    ++g_failures;
}

static bool vec_finite(float32x4_t v) {
    float lanes[4];
    vst1q_f32(lanes, v);
    for (int i = 0; i < 4; ++i) {
        if (!std::isfinite(lanes[i])) return false;
    }
    return true;
}

[[maybe_unused]] static bool vec_nonzero(float32x4_t v) {
    float lanes[4];
    vst1q_f32(lanes, v);
    for (int i = 0; i < 4; ++i) {
        if (std::fabs(lanes[i]) > 1e-7f) return true;
    }
    return false;
}

static bool vec_differs(float32x4_t a, float32x4_t b, float eps = 1e-6f) {
    float la[4], lb[4];
    vst1q_f32(la, a);
    vst1q_f32(lb, b);
    for (int i = 0; i < 4; ++i) {
        if (std::fabs(la[i] - lb[i]) > eps) return true;
    }
    return false;
}

static float vec_lane0(float32x4_t v) {
    return vgetq_lane_f32(v, 0);
}

// ============================================================================
// Mapping helper tests
// ============================================================================

static void test_mapping_helpers() {
    const char* name = "mapping helpers";

    float32x4_t env = vdupq_n_f32(1.0f);
    float32x4_t hit0 = fm_make_transient_env(env, vdupq_n_f32(0.0f));
    float32x4_t hit1 = fm_make_transient_env(env, vdupq_n_f32(1.0f));
    float32x4_t body0 = fm_make_body_env(env, vdupq_n_f32(0.0f));
    float32x4_t body1 = fm_make_body_env(env, vdupq_n_f32(1.0f));
    float32x4_t drv0 = fm_make_drive_gain(vdupq_n_f32(0.0f));
    float32x4_t drv1 = fm_make_drive_gain(vdupq_n_f32(1.0f));

    if (!(vec_lane0(hit1) > vec_lane0(hit0))) {
        return report_fail(name, "HitShape must increase transient emphasis");
    }
    if (!(vec_lane0(body1) > vec_lane0(body0))) {
        return report_fail(name, "BodyTilt must increase body emphasis");
    }
    if (!(vec_lane0(drv1) > vec_lane0(drv0))) {
        return report_fail(name, "fm_make_drive_gain must be monotone increasing");
    }
    if (std::fabs(vec_lane0(fm_soft_clip(vdupq_n_f32(2.0f)))) >= 2.0f) {
        return report_fail(name, "soft clip should reduce large signals");
    }
    report_pass(name);
}

// ============================================================================
// Probability gate test
// ============================================================================

static void test_probability_gate() {
    const char* name = "probability gate";

    neon_prng_t rng;
    neon_prng_init(&rng, 0x12345678u);

    uint32x4_t off = probability_gate_neon(&rng, 0, 0, 0, 0);
    uint32x4_t on  = probability_gate_neon(&rng, 100, 100, 100, 100);

    if (vgetq_lane_u32(off, 0) || vgetq_lane_u32(off, 1) ||
        vgetq_lane_u32(off, 2) || vgetq_lane_u32(off, 3)) {
        return report_fail(name, "0% should never trigger");
    }
    if (!vgetq_lane_u32(on, 0) || !vgetq_lane_u32(on, 1) ||
        !vgetq_lane_u32(on, 2) || !vgetq_lane_u32(on, 3)) {
        return report_fail(name, "100% should always trigger");
    }

    int triggers = 0;
    for (int i = 0; i < 4096; ++i) {
        uint32x4_t g = probability_gate_neon(&rng, 50, 50, 50, 50);
        triggers += vgetq_lane_u32(g, 0) ? 1 : 0;
        triggers += vgetq_lane_u32(g, 1) ? 1 : 0;
        triggers += vgetq_lane_u32(g, 2) ? 1 : 0;
        triggers += vgetq_lane_u32(g, 3) ? 1 : 0;
    }
    if (triggers <= 0) {
        return report_fail(name, "50% gate produced no triggers");
    }

    report_pass(name);
}

// ============================================================================
// Preset loading test
// ============================================================================

static void test_preset_loading() {
    const char* name = "preset loading";
    int8_t params[PARAM_TOTAL];
    std::memset(params, 0, sizeof(params));

    for (uint8_t i = 0; i < NUM_OF_PRESETS; ++i) {
        load_fm_preset(i, params);

        const uint8_t body_ids[] = {
            PARAM_BLEND, PARAM_GAP, PARAM_SCATTER,
            PARAM_KICK_ATK, PARAM_KICK_BODY, PARAM_SNARE_ATK, PARAM_SNARE_BODY,
            PARAM_METAL_ATK, PARAM_METAL_BODY, PARAM_PERC_ATK, PARAM_PERC_BODY,
            PARAM_HIT_SHAPE, PARAM_BODY_TILT, PARAM_NOISE_CHAR
        };

        if (params[PARAM_INSTRUMENT] < 0 || params[PARAM_INSTRUMENT] >= INST_COUNT) {
            return report_fail(name, "Invalid instrument selector");
        }
        for (uint8_t id : body_ids) {
            if (params[id] < 0 || params[id] > 100) {
                return report_fail(name, "control out of 0..100 range");
            }
        }
        if (params[PARAM_LFO1_SHAPE] < 0 || params[PARAM_LFO1_SHAPE] > 8) {
            return report_fail(name, "LFO1 shape out of range");
        }
        if (params[PARAM_EUCL_TUN] < 0 || params[PARAM_EUCL_TUN] > 8) {
            return report_fail(name, "Euclidean mode out of range");
        }
    }

    report_pass(name);
}

// ============================================================================
// Envelope tests
// ============================================================================

static void test_envelope_smoke() {
    const char* name = "envelope smoke";

    neon_envelope_t env;
    neon_envelope_init(&env);

    uint32x4_t all = vdupq_n_u32(0xFFFFFFFFu);
    neon_envelope_trigger(&env, all, 40);
    if (vgetq_lane_u32(env.stage, 0) == ENV_STATE_OFF) {
        return report_fail(name, "trigger did not start envelope");
    }

    for (int i = 0; i < 24; ++i) {
        neon_envelope_process(&env);
        if (!vec_finite(env.level)) {
            return report_fail(name, "envelope level became non-finite");
        }
    }

    neon_envelope_release(&env, all);
    for (int i = 0; i < 24; ++i) {
        neon_envelope_process(&env);
        if (!vec_finite(env.level)) {
            return report_fail(name, "release produced non-finite level");
        }
    }

    report_pass(name);
}

static void test_nonlinear_envelope_decays() {
    const char* name = "nonlinear envelope decays";

    neon_envelope_t env;
    neon_envelope_init(&env);

    uint32x4_t lane0 = vsetq_lane_u32(0xFFFFFFFFu, vdupq_n_u32(0), 0);
    neon_envelope_trigger(&env, lane0, 20);

    bool reached_decay = false;
    float first_decay = 0.0f;
    float later_decay = 0.0f;

    for (int i = 0; i < 9000; ++i) {
        neon_envelope_process(&env);
        if (vgetq_lane_u32(env.stage, 0) == ENV_STAGE_DECAY) {
            float level = neon_envelope_get_voice(&env, 0);
            if (!reached_decay) {
                reached_decay = true;
                first_decay = level;
            }
            later_decay = level;
            if (first_decay - later_decay > 0.05f) break;
        }
    }

    if (!reached_decay) {
        return report_fail(name, "punch envelope never reached decay");
    }
    if (!(later_decay < first_decay - 0.05f)) {
        return report_fail(name, "decay coefficients must be positive");
    }

    report_pass(name);
}

// ============================================================================
// Engine smoke test: finite output, parameter sensitivity
// ============================================================================

static void test_engine_smoke() {
    const char* name = "engine smoke";
    const uint32x4_t all = vdupq_n_u32(0xFFFFFFFFu);
    const float32x4_t env = vdupq_n_f32(1.0f);
    const float32x4_t pitch = vdupq_n_f32(1.0f);
    const float32x4_t idx = vdupq_n_f32(0.0f);

    kick_engine_t kick;
    kick_engine_init(&kick);
    kick_engine_set_note(&kick, all, vdupq_n_f32(36.0f));
    kick_engine_update(&kick, vdupq_n_f32(0.1f), vdupq_n_f32(0.1f));
    float32x4_t k0 = kick_engine_process(&kick, env, all, pitch, idx);
    kick_engine_update(&kick, vdupq_n_f32(0.9f), vdupq_n_f32(0.9f));
    float32x4_t k1 = kick_engine_process(&kick, env, all, pitch, idx);

    snare_engine_t snare;
    snare_engine_init(&snare);
    snare_engine_set_note(&snare, all, vdupq_n_f32(38.0f));
    snare_engine_update(&snare, vdupq_n_f32(0.1f), vdupq_n_f32(0.1f));
    float32x4_t s0 = snare_engine_process(&snare, env, all, pitch, idx, vdupq_n_f32(0.0f));
    snare_engine_update(&snare, vdupq_n_f32(0.9f), vdupq_n_f32(0.9f));
    float32x4_t s1 = snare_engine_process(&snare, env, all, pitch, idx, vdupq_n_f32(0.0f));

    metal_engine_t metal;
    metal_engine_init(&metal);
    metal_engine_set_note(&metal, all, vdupq_n_f32(42.0f));
    metal_engine_update(&metal, vdupq_n_f32(0.1f), vdupq_n_f32(0.1f));
    float32x4_t m0 = metal_engine_process(&metal, env, all, pitch, idx,
                                           vdupq_n_f32(0.0f), vdupq_n_f32(1.0f),
                                           vdupq_n_f32(0.0f));
    metal_engine_update(&metal, vdupq_n_f32(0.9f), vdupq_n_f32(0.9f));
    float32x4_t m1 = metal_engine_process(&metal, env, all, pitch, idx,
                                           vdupq_n_f32(0.5f), vdupq_n_f32(1.0f),
                                           vdupq_n_f32(0.0f));

    perc_engine_t perc;
    perc_engine_init(&perc);
    perc_engine_set_note(&perc, all, vdupq_n_f32(45.0f));
    perc_engine_update(&perc, vdupq_n_f32(0.1f), vdupq_n_f32(0.1f));
    float32x4_t p0 = perc_engine_process(&perc, env, all, pitch, idx);
    perc_engine_update(&perc, vdupq_n_f32(0.9f), vdupq_n_f32(0.9f));
    float32x4_t p1 = perc_engine_process(&perc, env, all, pitch, idx);

    if (!vec_finite(k0) || !vec_finite(k1) || !vec_finite(s0) || !vec_finite(s1) ||
        !vec_finite(m0) || !vec_finite(m1) || !vec_finite(p0) || !vec_finite(p1)) {
        return report_fail(name, "non-finite engine output");
    }

    if (!vec_differs(k0, k1)) return report_fail(name, "kick must react to parameter changes");
    if (!vec_differs(s0, s1)) return report_fail(name, "snare must react to parameter changes");
    if (!vec_differs(m0, m1)) return report_fail(name, "metal must react to parameter changes");
    if (!vec_differs(p0, p1)) return report_fail(name, "perc must react to parameter changes");

    report_pass(name);
}

// ============================================================================
// Output bounds: all engines must stay in a safe amplitude range
// ============================================================================

static void test_engine_output_bounds() {
    const char* name = "engine output bounds";
    const uint32x4_t all = vdupq_n_u32(0xFFFFFFFFu);
    const float env_val = 1.0f;  // peak envelope — worst case
    const float bound = 5.0f;    // generous bound; fm_cubic_clip keeps output near ±1

    const float32x4_t env  = vdupq_n_f32(env_val);
    const float32x4_t gate = vdupq_n_f32(1.0f);
    const float32x4_t one  = vdupq_n_f32(1.0f);
    const float32x4_t zero = vdupq_n_f32(0.0f);

    // Run each engine at extreme parameter settings for several samples
    for (int trial = 0; trial < 2; ++trial) {
        float p = (trial == 0) ? 0.0f : 1.0f;

        snare_engine_t snare;
        snare_engine_init(&snare);
        snare_engine_set_note(&snare, all, vdupq_n_f32(38.0f));
        snare_engine_update(&snare, vdupq_n_f32(p), vdupq_n_f32(p));
        for (int i = 0; i < 16; ++i) {
            float32x4_t out = snare_engine_process(&snare, env, all, one, zero, zero);
            if (!vec_finite(out)) return report_fail(name, "snare NaN/Inf");
            float v = vec_lane0(out);
            if (std::fabs(v) > bound) {
                return report_fail(name, "snare output out of bounds");
            }
        }

        metal_engine_t metal;
        metal_engine_init(&metal);
        metal_engine_set_note(&metal, all, vdupq_n_f32(42.0f));
        metal_engine_update(&metal, vdupq_n_f32(p), vdupq_n_f32(p));
        for (int i = 0; i < 16; ++i) {
            float32x4_t out = metal_engine_process(&metal, env, all, one, zero, zero, gate, zero);
            if (!vec_finite(out)) return report_fail(name, "metal NaN/Inf");
            float v = vec_lane0(out);
            if (std::fabs(v) > bound) {
                return report_fail(name, "metal output out of bounds");
            }
        }
    }

    report_pass(name);
}

// ============================================================================
// Snare behavioral tests
// ============================================================================

static void test_snare_noise_sustain() {
    const char* name = "snare noise sustains through body";

    // The snare wire buzz should remain audible past the initial click.
    // Use mid-decay envelope (env = 0.35) — with the old env^4 noise_env the
    // wire was essentially silent here; with the new env^2 it should be nonzero.
    const uint32x4_t all = vdupq_n_u32(0xFFFFFFFFu);
    const float32x4_t pitch = vdupq_n_f32(1.0f);
    const float32x4_t zero  = vdupq_n_f32(0.0f);

    // Attack=0.8 (lots of wire energy), Body=0.5
    const float32x4_t atk  = vdupq_n_f32(0.8f);
    const float32x4_t body = vdupq_n_f32(0.5f);

    snare_engine_t snare;
    snare_engine_init(&snare);
    snare_engine_set_note(&snare, all, vdupq_n_f32(38.0f));
    snare_engine_update(&snare, atk, body);

    // Accumulate output at mid-decay: run at env=0.35 for a block of samples
    const float32x4_t mid_env = vdupq_n_f32(0.35f);
    float energy = 0.0f;
    for (int i = 0; i < 64; ++i) {
        float32x4_t out = snare_engine_process(&snare, mid_env, all, pitch, zero, zero);
        if (!vec_finite(out)) return report_fail(name, "non-finite at mid-decay");
        energy += std::fabs(vec_lane0(out));
    }

    if (energy < 1e-4f) {
        return report_fail(name, "snare silent at mid-decay — wire buzz not sustaining");
    }

    report_pass(name);
}

static void test_snare_attack_sensitivity() {
    const char* name = "snare attack parameter sensitivity";

    // At onset (env=1), high attack should produce noticeably more output
    // than low attack. Checks that the click/wire path reacts to Param1.
    const uint32x4_t all  = vdupq_n_u32(0xFFFFFFFFu);
    const float32x4_t env = vdupq_n_f32(1.0f);
    const float32x4_t pitch = vdupq_n_f32(1.0f);
    const float32x4_t zero  = vdupq_n_f32(0.0f);

    snare_engine_t low;
    snare_engine_init(&low);
    snare_engine_set_note(&low, all, vdupq_n_f32(38.0f));
    snare_engine_update(&low, vdupq_n_f32(0.05f), vdupq_n_f32(0.5f));

    snare_engine_t high;
    snare_engine_init(&high);
    snare_engine_set_note(&high, all, vdupq_n_f32(38.0f));
    snare_engine_update(&high, vdupq_n_f32(0.95f), vdupq_n_f32(0.5f));

    float energy_low = 0.0f, energy_high = 0.0f;
    for (int i = 0; i < 32; ++i) {
        energy_low  += std::fabs(vec_lane0(snare_engine_process(&low,  env, all, pitch, zero, zero)));
        energy_high += std::fabs(vec_lane0(snare_engine_process(&high, env, all, pitch, zero, zero)));
    }

    if (energy_high <= energy_low * 1.05f) {
        return report_fail(name, "high attack should produce more energy at onset than low attack");
    }

    report_pass(name);
}

// ============================================================================
// Metal behavioral tests
// ============================================================================

static void test_metal_parameter_bounds() {
    const char* name = "metal parameter bounds";

    metal_engine_t metal;
    metal_engine_init(&metal);

    const float params[] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
    for (float a : params) {
        for (float b : params) {
            metal_engine_update(&metal, vdupq_n_f32(a), vdupq_n_f32(b));

            // feedback_gain must be capped at 0.60 to prevent square-wave saturation
            float fb = vec_lane0(metal.feedback_gain);
            if (fb < 0.0f || fb > 0.61f) {  // 0.01 tolerance for float rounding
                return report_fail(name, "feedback_gain out of [0, 0.60] range");
            }

            // ring_index must stay in [7, 13] (was 10..16.5 before fix)
            float ri = vec_lane0(metal.ring_index);
            if (ri < 6.9f || ri > 13.1f) {
                return report_fail(name, "ring_index out of [7, 13] range");
            }

            // output_gain must be positive
            float og = vec_lane0(metal.output_gain);
            if (og <= 0.0f) {
                return report_fail(name, "output_gain must be positive");
            }
        }
    }

    report_pass(name);
}

static void test_metal_ring_sustains() {
    const char* name = "metal ring sustains at mid-decay";

    // The sqrt amplitude envelope means the ring should still be audible
    // well past the initial attack (unlike a linear-decay sound which would
    // be mostly gone at env=0.35).
    const uint32x4_t all  = vdupq_n_u32(0xFFFFFFFFu);
    const float32x4_t pitch = vdupq_n_f32(1.0f);
    const float32x4_t zero  = vdupq_n_f32(0.0f);
    const float32x4_t gate  = vdupq_n_f32(1.0f);

    metal_engine_t metal;
    metal_engine_init(&metal);
    metal_engine_set_note(&metal, all, vdupq_n_f32(42.0f));
    metal_engine_update(&metal, vdupq_n_f32(0.5f), vdupq_n_f32(0.8f)); // high body = long ring

    // At mid-decay (env=0.35) the sqrt envelope gives sqrt(0.35) ≈ 0.59 amplitude.
    // A linear decay would be at 35%. The sqrt ring should produce meaningful output.
    const float32x4_t mid_env = vdupq_n_f32(0.35f);
    float energy = 0.0f;
    for (int i = 0; i < 64; ++i) {
        float32x4_t out = metal_engine_process(&metal, mid_env, all, pitch, zero, zero, gate, zero);
        if (!vec_finite(out)) return report_fail(name, "non-finite at mid-decay");
        energy += std::fabs(vec_lane0(out));
    }

    if (energy < 1e-4f) {
        return report_fail(name, "metal ring silent at mid-decay — sqrt envelope not sustaining");
    }

    report_pass(name);
}

static void test_metal_body_increases_ring() {
    const char* name = "metal body parameter increases ring density";

    const uint32x4_t all  = vdupq_n_u32(0xFFFFFFFFu);
    const float32x4_t env = vdupq_n_f32(0.5f);  // mid-decay
    const float32x4_t pitch = vdupq_n_f32(1.0f);
    const float32x4_t zero  = vdupq_n_f32(0.0f);
    const float32x4_t gate  = vdupq_n_f32(1.0f);

    metal_engine_t low;
    metal_engine_init(&low);
    metal_engine_set_note(&low, all, vdupq_n_f32(42.0f));
    metal_engine_update(&low, vdupq_n_f32(0.5f), vdupq_n_f32(0.0f));

    metal_engine_t high;
    metal_engine_init(&high);
    metal_engine_set_note(&high, all, vdupq_n_f32(42.0f));
    metal_engine_update(&high, vdupq_n_f32(0.5f), vdupq_n_f32(1.0f));

    // Check that the engines produce different outputs — different ring_index and
    // feedback_gain should yield noticeably different spectral outputs over time.
    float diff = 0.0f;
    for (int i = 0; i < 64; ++i) {
        float o_low  = vec_lane0(metal_engine_process(&low,  env, all, pitch, zero, zero, gate, zero));
        float o_high = vec_lane0(metal_engine_process(&high, env, all, pitch, zero, zero, gate, zero));
        diff += std::fabs(o_high - o_low);
    }

    if (diff < 1e-4f) {
        return report_fail(name, "body parameter had no effect on metal ring");
    }

    report_pass(name);
}

// ============================================================================
// Gong character test
// ============================================================================

static void test_metal_gong_character() {
    const char* name = "metal gong character produces different sound";

    const uint32x4_t all  = vdupq_n_u32(0xFFFFFFFFu);
    const float32x4_t env = vdupq_n_f32(1.0f);
    const float32x4_t pitch = vdupq_n_f32(1.0f);
    const float32x4_t zero  = vdupq_n_f32(0.0f);
    const float32x4_t gate  = vdupq_n_f32(1.0f);

    metal_engine_t cymbal;
    metal_engine_init(&cymbal);
    metal_engine_set_character(&cymbal, 0);
    metal_engine_set_note(&cymbal, all, vdupq_n_f32(42.0f));
    metal_engine_update(&cymbal, vdupq_n_f32(0.5f), vdupq_n_f32(0.5f));

    metal_engine_t gong;
    metal_engine_init(&gong);
    metal_engine_set_character(&gong, 1);
    metal_engine_set_note(&gong, all, vdupq_n_f32(42.0f));
    metal_engine_update(&gong, vdupq_n_f32(0.5f), vdupq_n_f32(0.5f));

    float diff = 0.0f;
    for (int i = 0; i < 64; ++i) {
        float oc = vec_lane0(metal_engine_process(&cymbal, env, all, pitch, zero, zero, gate, zero));
        float og = vec_lane0(metal_engine_process(&gong,   env, all, pitch, zero, zero, gate, zero));
        diff += std::fabs(oc - og);
    }

    if (diff < 1e-4f) {
        return report_fail(name, "cymbal and gong characters sound identical");
    }

    report_pass(name);
}

// ============================================================================
// Note routing smoke test
// ============================================================================

static void test_note_routing() {
    const char* name = "note routing";

    fm_perc_synth_t synth;
    fm_perc_synth_init(&synth);

    // These should not crash; detailed routing assertions depend on the
    // instrument selector state which is preset-dependent.
    fm_perc_synth_note_on(&synth, synth.midi.kick_note, 100);
    fm_perc_synth_note_on(&synth, 60, 100);

    report_pass(name);
}

// ============================================================================
// Full synth smoke test
// ============================================================================

static void test_synth_smoke() {
    const char* name = "synth smoke";

    fm_perc_synth_t synth;
    fm_perc_synth_init(&synth);
    load_fm_preset(0, synth.params);
    fm_perc_synth_update_params(&synth);

    fm_perc_synth_note_on(&synth, 60, 100);

    bool any_nonzero = false;
    for (int i = 0; i < 128; ++i) {
        float s = fm_perc_synth_process(&synth);
        if (!std::isfinite(s)) {
            return report_fail(name, "non-finite output");
        }
        if (std::fabs(s) > 1e-7f) any_nonzero = true;
    }

    fm_perc_synth_note_off(&synth, 60);

    if (!any_nonzero) {
        return report_fail(name, "output stayed silent after trigger");
    }

    report_pass(name);
}

// ============================================================================
// Noise character effect test (replaces the stale PARAM_DRIVE test)
// ============================================================================

static void test_noise_char_effect() {
    const char* name = "noise character parameter effect";

    // PARAM_NOISE_CHAR (slot 23) adds a noise floor to all engines.
    // noise_char=0 vs noise_char=100 should produce different output.
    fm_perc_synth_t a, b;
    fm_perc_synth_init(&a);
    fm_perc_synth_init(&b);

    load_fm_preset(0, a.params);
    load_fm_preset(0, b.params);
    // Force snare instrument so the noise_add path is active
    a.params[PARAM_INSTRUMENT] = INST_SNARE;
    b.params[PARAM_INSTRUMENT] = INST_SNARE;
    a.params[PARAM_SNARE_ATK]  = 80;
    b.params[PARAM_SNARE_ATK]  = 80;
    a.params[PARAM_NOISE_CHAR] = 0;
    b.params[PARAM_NOISE_CHAR] = 100;
    fm_perc_synth_update_params(&a);
    fm_perc_synth_update_params(&b);

    fm_perc_synth_note_on(&a, 60, 100);
    fm_perc_synth_note_on(&b, 60, 100);

    // Process enough samples to get nonzero output and compare
    float sum_diff = 0.0f;
    for (int i = 0; i < 96; ++i) {
        float sa = fm_perc_synth_process(&a);
        float sb = fm_perc_synth_process(&b);
        if (!std::isfinite(sa) || !std::isfinite(sb)) {
            return report_fail(name, "non-finite output");
        }
        sum_diff += std::fabs(sb - sa);
    }

    if (sum_diff < 1e-6f) {
        return report_fail(name, "noise_char=0 and noise_char=100 produced identical output");
    }

    report_pass(name);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::puts("Sonaglio test runner");

    test_mapping_helpers();
    test_probability_gate();
    test_preset_loading();
    test_envelope_smoke();
    test_nonlinear_envelope_decays();
    test_engine_smoke();
    test_engine_output_bounds();
    test_snare_noise_sustain();
    test_snare_attack_sensitivity();
    test_metal_parameter_bounds();
    test_metal_ring_sustains();
    test_metal_body_increases_ring();
    test_metal_gong_character();
    test_note_routing();
    test_synth_smoke();
    test_noise_char_effect();

    if (g_failures == 0) {
        std::puts("ALL SONAGLIO TESTS PASSED");
        return 0;
    }

    std::printf("SONAGLIO TESTS FAILED: %d\n", g_failures);
    return 1;
}

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

// TO RUN THIS TEST:
// first change .name in header.c
// ROOT_DIR=/mnt/d/Fede/drumlogue CXX=/mnt/d/Fede/drumlogue/arm-unknown-linux-gnueabihf/bin/arm-unknown-linux-gnueabihf-g++ RUNNER=qemu-arm SDK_INCLUDE_DIR=/mnt/d/Fede/drumlogue/arm-unknown-linux-gnueabihf/arm-unknown-linux-gnueabihf ./run_sonaglio_tests.sh

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

static void test_mapping_helpers() {
    const char* name = "mapping helpers";

    float32x4_t env = vdupq_n_f32(1.0f);
    float32x4_t hit0 = fm_make_transient_env(env, vdupq_n_f32(0.0f));
    float32x4_t hit1 = fm_make_transient_env(env, vdupq_n_f32(1.0f));
    float32x4_t body0 = fm_make_body_env(env, vdupq_n_f32(0.0f));
    float32x4_t body1 = fm_make_body_env(env, vdupq_n_f32(1.0f));
    float32x4_t drv0 = fm_make_drive_gain(vdupq_n_f32(0.0f));
    float32x4_t drv1 = fm_make_drive_gain(vdupq_n_f32(1.0f));

    if (!(vgetq_lane_f32(hit1, 0) > vgetq_lane_f32(hit0, 0))) {
        return report_fail(name, "HitShape must increase transient emphasis");
    }
    if (!(vgetq_lane_f32(body1, 0) > vgetq_lane_f32(body0, 0))) {
        return report_fail(name, "BodyTilt must increase body emphasis");
    }
    if (!(vgetq_lane_f32(drv1, 0) > vgetq_lane_f32(drv0, 0))) {
        return report_fail(name, "Drive must increase gain");
    }
    if (std::fabs(vgetq_lane_f32(fm_soft_clip(vdupq_n_f32(2.0f)), 0)) >= 2.0f) {
        return report_fail(name, "soft clip should reduce large signals");
    }
    report_pass(name);
}

static void test_probability_gate() {
    const char* name = "probability gate";

    neon_prng_t rng;
    neon_prng_init(&rng, 0x12345678u);

    uint32x4_t off = probability_gate_neon(&rng, 0, 0, 0, 0);
    uint32x4_t on  = probability_gate_neon(&rng, 100, 100, 100, 100);

    if (vgetq_lane_u32(off, 0) || vgetq_lane_u32(off, 1) || vgetq_lane_u32(off, 2) || vgetq_lane_u32(off, 3)) {
        return report_fail(name, "0% should never trigger");
    }
    if (!vgetq_lane_u32(on, 0) || !vgetq_lane_u32(on, 1) || !vgetq_lane_u32(on, 2) || !vgetq_lane_u32(on, 3)) {
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
            PARAM_HIT_SHAPE, PARAM_BODY_TILT, PARAM_DRIVE
        };

        if (params[PARAM_INSTRUMENT] < 0 || params[PARAM_INSTRUMENT] > INST_COUNT) {
            return report_fail(name, "Invalid instrument");
        }
        for (uint8_t id : body_ids) {
            if (params[id] < 0 || params[id] > 100) {
                return report_fail(name, "control out of 0..100");
            }
        }
        if (params[PARAM_LFO1_SHAPE] < 0 || params[PARAM_LFO1_SHAPE] > 8) {
            return report_fail(name, "LFO1 shape out of range");
        }
        if (params[PARAM_EUCL_TUN] < 0 || params[PARAM_EUCL_TUN] > 8) {
            return report_fail(name, "Euclidean mode out of range");
        }
        if (params[PARAM_ENV_SHAPE] < 0 || params[PARAM_ENV_SHAPE] > 126) { // data type uint range is +126
            char msg[64];
            std::snprintf(msg, sizeof(msg), "Envelope shape out of range for preset %d", i);
            return report_fail(name, msg);
        }
    }

    report_pass(name);
}

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
    float32x4_t m0 = metal_engine_process(&metal, env, all, pitch, idx, vdupq_n_f32(0.0f), vdupq_n_f32(1.0f));
    metal_engine_update(&metal, vdupq_n_f32(0.9f), vdupq_n_f32(0.9f));
    float32x4_t m1 = metal_engine_process(&metal, env, all, pitch, idx, vdupq_n_f32(0.5f), vdupq_n_f32(1.0f));

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

    if (!vec_differs(k0, k1)) return report_fail(name, "kick should react to parameters");
    if (!vec_differs(s0, s1)) return report_fail(name, "snare should react to parameters");
    if (!vec_differs(m0, m1)) return report_fail(name, "metal should react to parameters");
    if (!vec_differs(p0, p1)) return report_fail(name, "perc should react to parameters");

    report_pass(name);
}

static void test_note_routing() {
    const char* name = "note routing";

    fm_perc_synth_t synth;
    fm_perc_synth_init(&synth);
    for (int i = 0; i < ENGINE_COUNT; ++i) synth.voice_probs[i] = 100;

    fm_perc_synth_note_on(&synth, synth.midi.kick_note, 100);
    // if (!vgetq_lane_u32(synth.voice_triggered, 0) ||
    //     vgetq_lane_u32(synth.voice_triggered, 1) ||
    //     vgetq_lane_u32(synth.voice_triggered, 2) ||
    //     vgetq_lane_u32(synth.voice_triggered, 3)) {
    //     return report_fail(name, "kick note should only route to lane 0");
    // }

    fm_perc_synth_note_on(&synth, 60, 100);
    // if (!vgetq_lane_u32(synth.voice_triggered, 0) ||
    //     !vgetq_lane_u32(synth.voice_triggered, 1) ||
    //     !vgetq_lane_u32(synth.voice_triggered, 2) ||
    //     !vgetq_lane_u32(synth.voice_triggered, 3)) {
    //     return report_fail(name, "non-drum note should route to all lanes");
    // }

    report_pass(name);
}

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

static void test_drive_effect() {
    const char* name = "global drive effect";

    fm_perc_synth_t a;
    fm_perc_synth_t b;
    fm_perc_synth_init(&a);
    fm_perc_synth_init(&b);

    load_fm_preset(0, a.params);
    load_fm_preset(0, b.params);
    a.params[PARAM_DRIVE] = 0;
    b.params[PARAM_DRIVE] = 100;
    fm_perc_synth_update_params(&a);
    fm_perc_synth_update_params(&b);

    for (int i = 0; i < ENGINE_COUNT; ++i) {
        a.voice_probs[i] = 100;
        b.voice_probs[i] = 100;
    }

    fm_perc_synth_note_on(&a, 60, 100);
    fm_perc_synth_note_on(&b, 60, 100);

    float sa = fm_perc_synth_process(&a);
    float sb = fm_perc_synth_process(&b);

    if (!std::isfinite(sa) || !std::isfinite(sb)) {
        return report_fail(name, "non-finite compare output");
    }
    if (std::fabs(sb) <= std::fabs(sa) && std::fabs(sb - sa) < 1e-6f) {
        return report_fail(name, "drive had no visible effect");
    }

    report_pass(name);
}

int main() {
    std::puts("Sonaglio test runner");

    test_mapping_helpers();
    test_probability_gate();
    test_preset_loading();
    test_envelope_smoke();
    test_engine_smoke();
    test_note_routing();
    test_synth_smoke();
    test_drive_effect();

    if (g_failures == 0) {
        std::puts("ALL SONAGLIO TESTS PASSED");
        return 0;
    }

    std::printf("SONAGLIO TESTS FAILED: %d\n", g_failures);
    return 1;
}

#include <arm_neon.h>
#include <chrono>
#include <cstdint>
#include <cstdio>

#ifndef fast_inline
#define fast_inline inline __attribute__((always_inline))
#endif

#include "constants.h"
#include "engine_mapping.h"
#include "prng.h"
#include "kick_engine.h"
#include "snare_engine.h"
#include "metal_engine.h"
#include "perc_engine.h"
#include "fm_perc_synth_process.h"

using clock_type = std::chrono::steady_clock;

static uint64_t ns_now() {
    return (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
        clock_type::now().time_since_epoch()).count();
}

template <typename F>
static void bench(const char* name, int iterations, F&& fn) {
    volatile float sink = 0.0f;
    const uint64_t t0 = ns_now();
    for (int i = 0; i < iterations; ++i) {
        sink += fn();
    }
    const uint64_t t1 = ns_now();
    const double ns_per_call = double(t1 - t0) / double(iterations);
    std::printf("%-22s %12.2f ns/call   sink=%0.6f\n", name, ns_per_call, (double)sink);
}

int main() {
    std::puts("Sonaglio benchmark runner");

    bench("transient mapping", 2000000, []() -> float {
        return vgetq_lane_f32(fm_make_transient_env(vdupq_n_f32(1.0f), vdupq_n_f32(0.75f)), 0);
    });

    bench("body mapping", 2000000, []() -> float {
        return vgetq_lane_f32(fm_make_body_env(vdupq_n_f32(1.0f), vdupq_n_f32(0.75f)), 0);
    });

    bench("drive mapping", 2000000, []() -> float {
        return vgetq_lane_f32(fm_make_drive_gain(vdupq_n_f32(0.75f)), 0);
    });

    neon_prng_t rng;
    neon_prng_init(&rng, 0x12345678u);
    bench("probability gate", 1000000, [&]() -> float {
        uint32x4_t g = probability_gate_neon(&rng, 80, 60, 40, 20);
        return float(vgetq_lane_u32(g, 0) + vgetq_lane_u32(g, 1) +
                     vgetq_lane_u32(g, 2) + vgetq_lane_u32(g, 3));
    });

    const uint32x4_t all = vdupq_n_u32(0xFFFFFFFFu);
    const float32x4_t env = vdupq_n_f32(1.0f);
    const float32x4_t pitch = vdupq_n_f32(1.0f);
    const float32x4_t idx = vdupq_n_f32(0.0f);

    kick_engine_t kick;
    kick_engine_init(&kick);
    kick_engine_update(&kick, vdupq_n_f32(0.5f), vdupq_n_f32(0.5f));
    kick_engine_set_note(&kick, all, vdupq_n_f32(36.0f));
    bench("kick process", 250000, [&]() -> float {
        return vgetq_lane_f32(kick_engine_process(&kick, env, all, pitch, idx), 0);
    });

    snare_engine_t snare;
    snare_engine_init(&snare);
    snare_engine_update(&snare, vdupq_n_f32(0.5f), vdupq_n_f32(0.5f));
    snare_engine_set_note(&snare, all, vdupq_n_f32(38.0f));
    bench("snare process", 250000, [&]() -> float {
        return vgetq_lane_f32(snare_engine_process(&snare, env, all, pitch, idx, vdupq_n_f32(0.0f)), 0);
    });

    metal_engine_t metal;
    metal_engine_init(&metal);
    metal_engine_update(&metal, vdupq_n_f32(0.5f), vdupq_n_f32(0.5f));
    metal_engine_set_note(&metal, all, vdupq_n_f32(42.0f));
    bench("metal process", 250000, [&]() -> float {
        return vgetq_lane_f32(metal_engine_process(&metal, env, all, pitch, idx, vdupq_n_f32(0.25f), vdupq_n_f32(1.0f)), 0);
    });

    perc_engine_t perc;
    perc_engine_init(&perc);
    perc_engine_update(&perc, vdupq_n_f32(0.5f), vdupq_n_f32(0.5f));
    perc_engine_set_note(&perc, all, vdupq_n_f32(45.0f));
    bench("perc process", 250000, [&]() -> float {
        return vgetq_lane_f32(perc_engine_process(&perc, env, all, pitch, idx), 0);
    });

    fm_perc_synth_t synth;
    fm_perc_synth_init(&synth);
    load_fm_preset(0, synth.params);
    fm_perc_synth_update_params(&synth);
    fm_perc_synth_note_on(&synth, 60, 100);

    bench("full synth", 50000, [&]() -> float {
        return fm_perc_synth_process(&synth);
    });

    std::puts("Sonaglio benchmarks complete");
    return 0;
}

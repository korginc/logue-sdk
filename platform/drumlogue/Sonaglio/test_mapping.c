/**
 * @file test_mapping.c
 * @brief Mapping layer tests for the FM percussion redesign.
 *
 * This is a lightweight test scaffold intended to validate:
 * - monotonic parameter behavior
 * - consistent global control behavior
 * - output stability of the mapping equations
 *
 * The tests operate on the scalar mapping layer, because the NEON runtime
 * should mirror the same equations.
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "engine_mapping.h"

static int nearly_ge(float a, float b, float eps) {
    return (a > b) || (fabsf(a - b) <= eps);
}

static void test_globals(void) {
    global_controls_t g0 = mapping_compute_globals(0.0f, 0.0f, 0.0f);
    global_controls_t g1 = mapping_compute_globals(1.0f, 1.0f, 1.0f);

    assert(nearly_ge(g1.hit_shape, g0.hit_shape, 1e-6f));
    assert(nearly_ge(g1.body_tilt, g0.body_tilt, 1e-6f));
    assert(nearly_ge(g1.drive, g0.drive, 1e-6f));
}

static void test_kick_monotonic(void) {
    global_controls_t g = mapping_compute_globals(0.7f, 0.6f, 0.4f);

    engine_macro_targets_t a = mapping_compute_kick(0.2f, 0.2f, &g, 0.7f);
    engine_macro_targets_t b = mapping_compute_kick(0.8f, 0.2f, &g, 0.7f);
    engine_macro_targets_t c = mapping_compute_kick(0.8f, 0.8f, &g, 0.7f);

    assert(nearly_ge(b.excitation_gain, a.excitation_gain, 1e-6f));
    assert(nearly_ge(b.attack_click, a.attack_click, 1e-6f));
    assert(nearly_ge(c.pitch_drop_depth, b.pitch_drop_depth, 1e-6f));
    assert(nearly_ge(c.decay_scale, b.decay_scale, 1e-6f));
}

static void test_snare_monotonic(void) {
    global_controls_t g = mapping_compute_globals(0.8f, 0.4f, 0.5f);

    engine_macro_targets_t a = mapping_compute_snare(0.2f, 0.2f, &g, 0.5f);
    engine_macro_targets_t b = mapping_compute_snare(0.8f, 0.2f, &g, 0.5f);
    engine_macro_targets_t c = mapping_compute_snare(0.8f, 0.8f, &g, 0.5f);

    assert(nearly_ge(b.noise_amount, a.noise_amount, 1e-6f));
    assert(nearly_ge(b.attack_click, a.attack_click, 1e-6f));
    assert(nearly_ge(c.fm_index_body, b.fm_index_body, 1e-6f));
    assert(nearly_ge(c.decay_scale, b.decay_scale, 1e-6f));
}

static void test_metal_monotonic(void) {
    global_controls_t g = mapping_compute_globals(0.8f, 0.5f, 0.6f);

    engine_macro_targets_t a = mapping_compute_metal(0.2f, 0.2f, &g, 0.8f);
    engine_macro_targets_t b = mapping_compute_metal(0.8f, 0.2f, &g, 0.8f);
    engine_macro_targets_t c = mapping_compute_metal(0.8f, 0.8f, &g, 0.8f);

    assert(nearly_ge(b.inharmonic_spread, a.inharmonic_spread, 1e-6f));
    assert(nearly_ge(b.attack_brightness, a.attack_brightness, 1e-6f));
    assert(nearly_ge(c.ring_density, b.ring_density, 1e-6f));
    assert(nearly_ge(c.decay_scale, b.decay_scale, 1e-6f));
}

static void test_perc_monotonic(void) {
    global_controls_t g = mapping_compute_globals(0.9f, 0.7f, 0.5f);

    engine_macro_targets_t a = mapping_compute_perc(0.2f, 0.2f, &g, 0.4f);
    engine_macro_targets_t b = mapping_compute_perc(0.8f, 0.2f, &g, 0.4f);
    engine_macro_targets_t c = mapping_compute_perc(0.8f, 0.8f, &g, 0.4f);

    assert(nearly_ge(b.fm_index_attack, a.fm_index_attack, 1e-6f));
    assert(nearly_ge(b.attack_click, a.attack_click, 1e-6f));
    assert(nearly_ge(c.fm_index_body, b.fm_index_body, 1e-6f));
    assert(nearly_ge(c.decay_scale, b.decay_scale, 1e-6f));
}

static void test_range_stability(void) {
    mapping_result_t r;
    engine_input_t inputs[ENGINE_COUNT] = {
        { 0.0f, 0.0f, 0.0f },
        { 0.5f, 0.5f, 0.5f },
        { 1.0f, 1.0f, 1.0f },
        { 0.25f, 0.75f, 0.9f }
    };

    mapping_compute_all(&r, inputs, 0.75f, 0.5f, 0.5f);

    assert(isfinite(r.kick.excitation_gain));
    assert(isfinite(r.snare.noise_amount));
    assert(isfinite(r.metal.inharmonic_spread));
    assert(isfinite(r.perc.ratio_bias));
}

static void print_summary(void) {
    global_controls_t g = mapping_compute_globals(0.8f, 0.6f, 0.4f);

    engine_macro_targets_t kick  = mapping_compute_kick(0.7f, 0.5f, &g, 0.8f);
    engine_macro_targets_t snare = mapping_compute_snare(0.7f, 0.5f, &g, 0.8f);
    engine_macro_targets_t metal = mapping_compute_metal(0.7f, 0.5f, &g, 0.8f);
    engine_macro_targets_t perc  = mapping_compute_perc(0.7f, 0.5f, &g, 0.8f);

    printf("Mapping summary\n");
    printf(" Kick : exc=%.3f atk=%.3f body=%.3f drive=%.3f\n",
           kick.excitation_gain, kick.attack_click, kick.fm_index_body, kick.drive_amount);
    printf(" Snare: exc=%.3f noise=%.3f body=%.3f drive=%.3f\n",
           snare.excitation_gain, snare.noise_amount, snare.fm_index_body, snare.drive_amount);
    printf(" Metal: exc=%.3f ring=%.3f inh=%.3f drive=%.3f\n",
           metal.excitation_gain, metal.ring_density, metal.inharmonic_spread, metal.drive_amount);
    printf(" Perc : exc=%.3f atk=%.3f body=%.3f drive=%.3f\n",
           perc.excitation_gain, perc.attack_click, perc.fm_index_body, perc.drive_amount);
}

int main(void) {
    test_globals();
    test_kick_monotonic();
    test_snare_monotonic();
    test_metal_monotonic();
    test_perc_monotonic();
    test_range_stability();
    print_summary();

    printf("All mapping tests PASSED\n");
    return 0;
}

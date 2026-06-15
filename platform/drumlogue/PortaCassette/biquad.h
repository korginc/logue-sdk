#pragma once
/*
 * biquad.h — Scalar-state biquad filter for PortaCassette
 *
 * State stored as scalar floats so biquad_process4() advances 4 consecutive
 * time samples with correct IIR feedback (same proven pattern as
 * omnipress/crossover.h biquad_process4).  The old float32x4_t state design
 * processed 4 independent parallel IIR histories, giving wrong frequency
 * response — especially audible on high-Q peaking EQ and high-shelf filters.
 */

#include <arm_neon.h>

#ifndef fast_inline
#define fast_inline inline __attribute__((always_inline, optimize("Ofast")))
#endif

typedef struct { float z1, z2; } biquad_state_t;
typedef struct { float b0, b1, b2, a1, a2; } biquad_coeffs_t;

fast_inline void biquad_reset(biquad_state_t* s) { s->z1 = s->z2 = 0.0f; }

/* Process 4 consecutive time samples (Transposed Direct Form II).
 * Highly optimized for ARM NEON registers without stack allocation stalls. */
fast_inline float32x4_t biquad_process4(biquad_state_t* s,
                                         const biquad_coeffs_t* c,
                                         float32x4_t in) {
    float z1 = s->z1;
    float z2 = s->z2;
    const float b0 = c->b0;
    const float b1 = c->b1;
    const float b2 = c->b2;
    const float a1 = c->a1;
    const float a2 = c->a2;

    // Lane 0
    const float x0 = vgetq_lane_f32(in, 0);
    const float y0 = b0 * x0 + z1;
    z1 = b1 * x0 - a1 * y0 + z2;
    z2 = b2 * x0 - a2 * y0;

    // Lane 1
    const float x1 = vgetq_lane_f32(in, 1);
    const float y1 = b0 * x1 + z1;
    z1 = b1 * x1 - a1 * y1 + z2;
    z2 = b2 * x1 - a2 * y1;

    // Lane 2
    const float x2 = vgetq_lane_f32(in, 2);
    const float y2 = b0 * x2 + z1;
    z1 = b1 * x2 - a1 * y2 + z2;
    z2 = b2 * x2 - a2 * y2;

    // Lane 3
    const float x3 = vgetq_lane_f32(in, 3);
    const float y3 = b0 * x3 + z1;
    z1 = b1 * x3 - a1 * y3 + z2;
    z2 = b2 * x3 - a2 * y3;

    s->z1 = z1;
    s->z2 = z2;

    // Reconstruct vector directly in registers
    float32x4_t out = vdupq_n_f32(y0);
    out = vsetq_lane_f32(y1, out, 1);
    out = vsetq_lane_f32(y2, out, 2);
    out = vsetq_lane_f32(y3, out, 3);

    return out;
}

/* Process 1 sample — used for the 0-3 remainder frames. */
fast_inline float biquad_process1(biquad_state_t* s,
                                   const biquad_coeffs_t* c,
                                   float x) {
    const float y = c->b0 * x + s->z1;
    s->z1 = c->b1 * x - c->a1 * y + s->z2;
    s->z2 = c->b2 * x - c->a2 * y;
    return y;
}

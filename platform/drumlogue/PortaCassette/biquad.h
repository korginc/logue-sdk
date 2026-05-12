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
 * Each output y[n] feeds back before computing y[n+1]. */
fast_inline float32x4_t biquad_process4(biquad_state_t* s,
                                         const biquad_coeffs_t* c,
                                         float32x4_t in) {
    float i4[4], o4[4];
    vst1q_f32(i4, in);
    float z1 = s->z1, z2 = s->z2;
    const float b0=c->b0, b1=c->b1, b2=c->b2, a1=c->a1, a2=c->a2;
    for (int k = 0; k < 4; ++k) {
        const float x = i4[k];
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        o4[k] = y;
    }
    s->z1 = z1; s->z2 = z2;
    return vld1q_f32(o4);
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

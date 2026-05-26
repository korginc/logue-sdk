#pragma once
#include <cstdint>
#include <cstring>
#include "float_math.h"

/**
 * @file drone_engine.h
 * @brief Parallel resonator bank drone / noise synth engine.
 *
 * Two modes, selectable at init():
 *   Crystal (false) — esotico-inspired: smooth harmonic ratios, gentle Q=2.5.
 *                     Produces slowly evolving harmonic clouds.
 *   Metal   (true)  — labirinto-inspired: TR-808 metallic ratios, high Q=6.
 *                     Produces sharp metallic resonant buzz.
 *
 * Design: 4 parallel biquad bandpass resonators driven by continuous white
 * noise. No transcendental functions in the audio hot path — biquad
 * coefficients computed once per note change using fastersinfullf/fastercosfullf.
 *
 * Biquad form (Audio EQ Cookbook bandpass, b1=0):
 *   y[n] = g*(x[n]-x[n-2]) + c1*y[n-1] + c2*y[n-2]
 * where:
 *   g  = sin(w0)/2 / (1+alpha)
 *   c1 = 2*cos(w0)  / (1+alpha)
 *   c2 = -(1-alpha) / (1+alpha)
 *   alpha = sin(w0)/(2Q)
 */

static constexpr int DRONE_RESONATORS = 4;
static constexpr float DRONE_SAMPLE_RATE = 48000.0f;

// Crystal ratios: smooth near-harmonic intervals for diffuse shimmer
static constexpr float CRYSTAL_RATIOS[DRONE_RESONATORS] = { 1.0f, 1.5f, 2.0f, 3.15f };
// Metal ratios: TR-808 metallic partial intervals
static constexpr float METAL_RATIOS[DRONE_RESONATORS] = { 1.0f, 1.6732f, 2.4281f, 3.5f };

struct DroneEngine {
    float y1[DRONE_RESONATORS];
    float y2[DRONE_RESONATORS];
    float g[DRONE_RESONATORS];    // b0/a0
    float c1[DRONE_RESONATORS];   // 2*cos(w0)/a0  (applied to y[n-1])
    float c2[DRONE_RESONATORS];   // -(1-alpha)/a0  (applied to y[n-2], stored negative)
    float x1, x2;                 // shared input delay (same noise feeds all resonators)

    float noise_amp;
    float output_gain;
    float q;
    const float* ratios;
    uint32_t prng;

    inline void init(bool metal) {
        memset(y1, 0, sizeof(y1));
        memset(y2, 0, sizeof(y2));
        memset(g,  0, sizeof(g));
        memset(c1, 0, sizeof(c1));
        memset(c2, 0, sizeof(c2));
        x1 = 0.0f;
        x2 = 0.0f;

        if (metal) {
            q           = 6.0f;
            noise_amp   = 0.04f;
            output_gain = 0.14f;
            ratios      = METAL_RATIOS;
            prng        = 0xDEADBEEFu;
        } else {
            q           = 2.5f;
            noise_amp   = 0.10f;
            output_gain = 0.18f;
            ratios      = CRYSTAL_RATIOS;
            prng        = 0xCAFEBABEu;
        }
    }

    inline void clear() {
        memset(y1, 0, sizeof(y1));
        memset(y2, 0, sizeof(y2));
        x1 = 0.0f;
        x2 = 0.0f;
    }

    inline void set_note(float base_hz) {
        const float inv_sr = 1.0f / DRONE_SAMPLE_RATE;
        for (int i = 0; i < DRONE_RESONATORS; i++) {
            float freq = base_hz * ratios[i];
            if (freq > DRONE_SAMPLE_RATE * 0.45f) freq = DRONE_SAMPLE_RATE * 0.45f;
            const float omega = M_TWOPI * freq * inv_sr;
            const float sin_w = fastersinfullf(omega);
            const float cos_w = fastercosfullf(omega);
            const float alpha = sin_w / (2.0f * q);
            const float a0inv = 1.0f / (1.0f + alpha);
            g[i]  = sin_w * 0.5f * a0inv;
            c1[i] = 2.0f * cos_w * a0inv;
            c2[i] = -(1.0f - alpha) * a0inv;
        }
    }

    inline __attribute__((optimize("Ofast"), always_inline))
    float process() {
        // LCG white noise
        prng = prng * 1664525u + 1013904223u;
        const float x0 = (float)(int32_t)prng * (1.0f / 2147483648.0f) * noise_amp;
        const float dx = x0 - x2;

        float out = 0.0f;
        for (int i = 0; i < DRONE_RESONATORS; i++) {
            const float y0 = g[i] * dx + c1[i] * y1[i] + c2[i] * y2[i];
            y2[i] = y1[i];
            y1[i] = y0;
            out += y0;
        }
        x2 = x1;
        x1 = x0;
        return out * output_gain;
    }
};

#pragma once
#include <cstdint>
#include <cstring>
#include "float_math.h"

/**
 * @file drone_engine.h
 * @brief Compact self-oscillating Feedback Delay Network — ScrutaAstri presets 95/96.
 *
 * Architecture: 4 short delay lines + 4x4 Hadamard mixing + tanh saturation in the
 * feedback path. The Hadamard cross-coupling combined with the nonlinear saturation
 * creates a chaotic strange attractor: the output evolves continuously and never
 * exactly repeats — genuine self-generative behaviour without external input.
 *
 * Chaos regime control (O2Detune knob, remapped in drone mode):
 *   g low  (left)  → periodic limit cycle (stable, repeating tone)
 *   g mid  (centre) → chaotic attractor   (self-generative, bounded by saturation)
 *   g high (right) → aggressive chaos     (loud, dense, still bounded by tanh)
 *
 * Effective loop gain = g × 0.5 (Hadamard scale) × drive.
 * At default (g=1.85, drive=1.2): loop gain ≈ 1.11 → chaotic attractor.
 *
 * Parameter routing in drone mode (see PROGRESS.md):
 *   O2Detune (-100..100) → feedback gain g     (chaos level)
 *   O2SubOct (0..4)      → saturation drive    (harmonic density)
 *   O2Mix    (0..100)    → noise injection amp  (excitation amount)
 *   Note / F1Cut / F1Res → perceived pitch via post-FDN filter (downstream, unchanged)
 *   F2, CMOS, SRR, BRR  → post-FDN processing (unchanged, very interesting on FDN output)
 *
 * Crystal (preset 95): delay lines {61,79,83,97} samples → resonant peaks ~500-800 Hz.
 *   One-pole LPF in loop (alpha=0.35) — smooth, diffuse, dark harmonic cloud.
 * Metal   (preset 96): delay lines {29,37,43,53} samples → resonant peaks ~900-1650 Hz.
 *   One-pole LPF in loop (alpha=0.65) — brighter, more metallic, harsher texture.
 *
 * Fast 4x4 Hadamard (8 adds + 4 muls, no matrix multiply):
 *   a=d0+d1; b=d0-d1; c=d2+d3; e=d2-d3
 *   h = {a+c, b+e, a-c, b-e} × 0.5
 *
 * Memory: 4 lines × 128 samples × 4 bytes = 2 KB per drone instance.
 */

static constexpr int FDN_LINES   = 4;
static constexpr int FDN_BUF_LEN = 128;   // power of 2, must exceed max delay (97)

// Crystal: near-harmonic, longer delays → lower resonant frequencies
static constexpr int CRYSTAL_DELAYS[FDN_LINES] = { 61, 79, 83, 97 };
// Metal: TR-808-ratio-inspired, shorter delays → higher, brighter frequencies
static constexpr int METAL_DELAYS[FDN_LINES]   = { 29, 37, 43, 53 };

struct DroneEngine {
    float buf[FDN_LINES][FDN_BUF_LEN];  // delay line ring buffers
    int   write_pos;                     // shared write pointer (all lines advance together)
    int   dlen[FDN_LINES];              // delay lengths in samples
    float fz[FDN_LINES];               // one-pole LPF state per line
    float falpha;                        // one-pole coefficient (crystal=0.35, metal=0.65)
    float g;                             // feedback gain      (O2Detune → chaos level)
    float drive;                         // saturation drive   (O2SubOct → harmonic density)
    float noise_amp;                     // noise injection    (O2Mix)
    float out_scale;                     // output normalisation (mode-dependent)
    uint32_t prng;

    inline void init(bool metal) {
        memset(buf, 0, sizeof(buf));
        memset(fz,  0, sizeof(fz));
        write_pos = 0;
        prng      = metal ? 0xDEADBEEFu : 0xCAFEBABEu;

        const int* src = metal ? METAL_DELAYS : CRYSTAL_DELAYS;
        for (int i = 0; i < FDN_LINES; i++) dlen[i] = src[i];

        falpha    = metal ? 0.72f : 0.45f; // Relaxed LPF for more high-end energy
        g         = 2.10f;   // default: stronger chaotic attractor regime
        drive     = 1.20f;
        noise_amp = 0.0040f; // Boosted noise floor for reliable self-starting
        out_scale = metal ? 0.35f : 0.45f; // Adjusted for hotter internal gain
    }

    inline void clear() {
        memset(buf, 0, sizeof(buf));
        memset(fz,  0, sizeof(fz));
        write_pos = 0;
    }

    // set_note: pitch is perceived via F1Cutoff downstream — no delay retune needed.
    // Reseed PRNG on note-on so each hit starts from a slightly different attractor position.
    inline void set_note(float) {
        prng ^= 0x5A5A5A5Au;
    }

    // O2Detune (-100..100) → g in [1.00, 3.20]
    // Centre (0) = 2.10 = firm chaotic attractor.
    inline void set_feedback(int32_t v) {
        g = 2.10f + (float)v * 0.011f;
        g = g < 1.0f ? 1.0f : g > 2.8f ? 2.8f : g;
    }

    // O2SubOct (0..4) → drive in [0.80, 2.00]
    // Lower = softer saturation (smoother), higher = harder clipping (more harmonics).
    inline void set_drive(int32_t v) {
        drive = 0.80f + (float)v * 0.30f;
    }

    // O2Mix (0..100) → noise_amp in [0.0010, 0.0400]
    // Low = pure self-oscillation, high = heavy noise excitation (denser, noisier texture).
    inline void set_noise(int32_t v) {
        noise_amp = 0.0010f + (float)v * 0.00039f;
    }

    inline __attribute__((optimize("Ofast"), always_inline))
    float process() {
        // 1. Read delay line outputs + apply per-line one-pole LPF
        //    LPF in feedback path: crystal=dark/smooth, metal=bright/metallic
        float d[FDN_LINES];
        for (int i = 0; i < FDN_LINES; i++) {
            const int rp = (write_pos - dlen[i]) & (FDN_BUF_LEN - 1);
            fz[i] += falpha * (buf[i][rp] - fz[i]);
            d[i] = fz[i];
        }

        // 2. Output = average of delay line reads (pre-Hadamard, before cross-mixing)
        const float out = (d[0] + d[1] + d[2] + d[3]) * 0.25f * out_scale;

        // 3. Fast 4×4 Hadamard cross-mix (8 adds + 4 muls)
        //    This coupling is what enables chaos: each line now feeds all others.
        const float a = d[0] + d[1], b = d[0] - d[1];
        const float c = d[2] + d[3], e = d[2] - d[3];
        d[0] = (a + c) * 0.5f;
        d[1] = (b + e) * 0.5f;
        d[2] = (a - c) * 0.5f;
        d[3] = (b - e) * 0.5f;

        // 4. Noise injection (tiny — keeps attractor alive, prevents silence)
        prng = prng * 1664525u + 1013904223u;
        const float noise = (float)(int32_t)prng * (1.0f / 2147483648.0f) * noise_amp;

        // 5. Apply gain + saturation + write back to delay lines
        //    Padé [3,3] tanh approximant, clamped to [-3, 3] for stability.
        //    Inside the loop: effective gain = g × 0.5 × drive.
        //    At default (1.85 × 0.5 × 1.2 ≈ 1.11): above unit gain → chaotic attractor.
        const float gd = g * drive;
        for (int i = 0; i < FDN_LINES; i++) {
            float x = (d[i] + noise) * gd;
            x = x >  3.0f ?  3.0f : x < -3.0f ? -3.0f : x;
            const float x2  = x * x;
            buf[i][write_pos] = x * (27.0f + x2) / (27.0f + 9.0f * x2);
        }

        write_pos = (write_pos + 1) & (FDN_BUF_LEN - 1);
        return out;
    }
};

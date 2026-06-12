#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include "float_math.h"

/**
 * @file drone_engine.h
 * @brief Compact self-oscillating Feedback Delay Network — ScrutaAstri presets 95/96.
 *        Upgraded with LFO-modulated chaos.
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
 * At default (g=2.10, drive=1.2): loop gain ≈ 1.26 → chaotic attractor.
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

static constexpr int FDN_LINES   = 8;
static constexpr int FDN_BUF_LEN = 2048; // power of 2, must exceed max delay (97) Expanded to support deep sub-bass wavelengths

// --- DUAL-BANK DELAY TIMES ---
// Crystal: Sub-bass to low-mid prime delay lengths (resonant peaks ~25Hz - 55Hz)
// Crystal Bank A (Deepest Sub) & Bank B (Low-Mid Cluster)
static constexpr int CRYSTAL_DELAYS_A[FDN_LINES] = { 811, 997, 1153, 1223, 1381, 1453, 1601, 1733 };
static constexpr int CRYSTAL_DELAYS_B[FDN_LINES] = { 541, 647,  751,  857,  967, 1069, 1171, 1277 };

// Metal: Shorter, tightly spaced primes for rich, harsh, metallic textures
// Metal Bank A (Tight/Bright) & Bank B (Aggressive/Dissonant)
static constexpr int METAL_DELAYS_A[FDN_LINES]   = {  41,  53,   67,   79,   97,  113,  131,  151 };
static constexpr int METAL_DELAYS_B[FDN_LINES]   = {  31,  47,   61,   73,   89,  103,  127,  139 };


struct DroneEngine {
    float buf[FDN_LINES][FDN_BUF_LEN];   // Delay line ring buffers
    int   write_pos;                     // Shared write pointer
    float fz[FDN_LINES];                 // One-pole LPF state per line
    float hpx[FDN_LINES];                // DC Blocker HPF input state
    float hpy[FDN_LINES];                // DC Blocker HPF output state
    float falpha;                        // LPF coefficient
    float g;                             // Feedback gain             (O2Detune → Chaos level)
    float drive;                         // Saturation drive          (O2SubOct → Harmonic density)
    float noise_amp;                     // Noise floor excitation    (O2Mix)
    float out_scale;                     // Output normalization (mode-dependent)
    uint32_t prng;
    bool  is_metal;      // Stored to track mode configuration

    inline void init(bool metal) {
        memset(buf, 0, sizeof(buf));
        memset(fz,  0, sizeof(fz));
        memset(hpx, 0, sizeof(hpx));
        memset(hpy, 0, sizeof(hpy));
        write_pos = 0;
        is_metal  = metal;
        prng      = metal ? 0xDEADBEEFu : 0xCAFEBABEu;
        falpha    = metal ? 0.65f : 0.25f; // Darkened Crystal mode for deeper sub weight
        g         = 2.10f;    // hardware-calibrated default: clear chaotic attractor
        drive     = 1.30f;    // Boosted baseline drive to energize 8 lines
        noise_amp = 0.0050f;  // Boosted noise floor for reliable self-starting
        out_scale = metal ? 0.28f : 0.38f;
    }

    inline void clear() {
        memset(buf, 0, sizeof(buf));
        memset(fz,  0, sizeof(fz));
        memset(hpx, 0, sizeof(hpx));
        memset(hpy, 0, sizeof(hpy));
        write_pos = 0;
    }

    // set_note: pitch is perceived via F1Cutoff downstream — no delay retune needed.
    // Reseed PRNG on note-on so each hit starts from a slightly different attractor position.
    inline void set_note(float) {
        prng ^= 0x5A5A5A5Au;
    }

    // O2Detune (-100..100) → g in [1.60, 2.60], clamped to minimum 1.70
    // Centre (0) = 2.10 = hardware-calibrated chaotic attractor.
    // Floor of 1.70 ensures loop gain (g×0.5×1.2) never drops below 1.02 → no silence.
    inline void set_feedback(int32_t v) {
        g = 2.10f + (float)v * 0.005f;
        g = g < 1.70f ? 1.70f : g > 2.80f ? 2.80f : g;
    }

    // O2SubOct (0..4) → drive in [1.20, 2.00]
    // Minimum 1.20 guarantees effective loop gain (g×0.5×drive) > 1 at any valid g.
    // Higher values push tanh harder → denser harmonics / more aggressive saturation.
    inline void set_drive(int32_t v) {
        drive = 1.20f + (float)v * 0.20f;
    }

    // O2Mix (0..100) → noise_amp in [0.0010, 0.0400]
    // Low = pure self-oscillation, high = heavy noise excitation (denser, noisier texture).
    inline void set_noise(int32_t v) {
        noise_amp = 0.0010f + (float)v * 0.00039f;
    }

    // Branchless fractional delay reader using linear interpolation
    inline float read_delay_frac(int line, float delay_samples) {
        float rp = (float)write_pos - delay_samples;
        // Float offset trick to keep index safely positive before wrapping
        float rp_pos = rp + 4096.0f;
        int idx1 = static_cast<int>(rp_pos) & (FDN_BUF_LEN - 1);
        int idx2 = (idx1 + 1) & (FDN_BUF_LEN - 1);
        float frac = rp_pos - static_cast<float>(static_cast<int>(rp_pos));
        return buf[line][idx1] + frac * (buf[line][idx2] - buf[line][idx1]);
    }

    inline __attribute__((optimize("Ofast"), always_inline))
    float process(float lfo1_presence, float lfo2_presence, float lfo3_presence) {
        // 1. HARMONIC MORPHING (LFO 1)
        // Map bipolar LFO 1 to [0.0, 1.0] to crossfade between Bank A and Bank B delay lengths
        float morph = clampf(lfo1_presence * 0.5f + 0.5f, 0.0f, 1.0f);

        float d[FDN_LINES];
        const int* bankA = is_metal ? METAL_DELAYS_A : CRYSTAL_DELAYS_A;
        const int* bankB = is_metal ? METAL_DELAYS_B : CRYSTAL_DELAYS_B;

        // 2. SPECTRAL SHIMMER (LFO 3)
        // Modulate internal one-pole damping. Higher LFO 3 opens the filter for more "air".
        float dynamic_falpha = clampf(falpha * (1.0f + lfo3_presence * 0.8f), 0.01f, 0.99f);

        for (int i = 0; i < FDN_LINES; i++) {
            // Inharmonic Jitter: Add tiny random fluctuations (±0.2 samples) to delay lengths.
            // This prevents "locking" and creates the sensation of an evolving environment.
            float jitter = ((float)(int32_t)(prng >> 16) * (1.0f / 32768.0f)) * 0.2f;
            float base_delay = (float)bankA[i] + morph * (float)(bankB[i] - bankA[i]) + jitter;

            float raw_sample = read_delay_frac(i, base_delay);
            fz[i] += dynamic_falpha * (raw_sample - fz[i]);
            d[i] = fz[i];
        }

        // 2. Mix output from all 8 lines with a subtle DC offset to keep the attractor off-center
        // (Genuine chaotic systems evolve better when biased slightly away from zero)
        float out = 0.001f;
        for (int i = 0; i < FDN_LINES; i++) out += d[i];
        out *= (1.0f / (float)FDN_LINES) * out_scale;

        // 3. Fast 8x8 In-Place Walsh-Hadamard Transform (3-stage butterfly)
        // Stage 1
        float s0 = d[0] + d[1], d0 = d[0] - d[1];
        float s1 = d[2] + d[3], d1 = d[2] - d[3];
        float s2 = d[4] + d[5], d2 = d[4] - d[5];
        float s3 = d[6] + d[7], d3 = d[6] - d[7];
        // Stage 2
        float t0 = s0 + s1, t1 = s0 - s1;
        float t2 = d0 + d1, t3 = d0 - d1;
        float t4 = s2 + s3, t5 = s2 - s3;
        float t6 = d2 + d3, t7 = d2 - d3;
        // Stage 3 + Orthogonal Matrix Scaling (1 / sqrt(8) ≈ 0.35355339f)
        static constexpr float H_SCALE = 0.35355339f;
        d[0] = (t0 + t4) * H_SCALE; d[1] = (t1 + t5) * H_SCALE;
        d[2] = (t2 + t6) * H_SCALE; d[3] = (t3 + t7) * H_SCALE;
        d[4] = (t0 - t4) * H_SCALE; d[5] = (t1 - t5) * H_SCALE;
        d[6] = (t2 - t6) * H_SCALE; d[7] = (t3 - t7) * H_SCALE;

        // 4. EVOLVING CHAOS (LFO 2)
        // Modulate feedback gain and noise injection with LFO 2.
        // This causes the drone to pulsate between resonant purity and chaotic energy.
        float dynamic_g = clampf(g + lfo2_presence * 0.4f, 1.70f, 2.80f);
        float dynamic_noise_amp = clampf(noise_amp * (1.0f + lfo2_presence), 0.0005f, 0.1f);

        prng = prng * 1664525u + 1013904223u;
        float noise_val = (float)(int32_t)prng * (1.0f / 2147483648.0f) * dynamic_noise_amp;

        // 5. GAIN AND SATURATION
        float loop_gain = dynamic_g * drive;

        for (int i = 0; i < FDN_LINES; i++) {
            float x = d[i] * loop_gain + noise_val;

            // In-loop DC Blocker (High-Pass Filter with pole at 0.996) to prevent latch-up
            float hp_out = x - hpx[i] + 0.996f * hpy[i];
            hpx[i] = x;
            hpy[i] = hp_out;

            // Padé [3,3] tanh approximation bounded to [-3, 3]
            float sat_in = hp_out > 3.0f ? 3.0f : (hp_out < -3.0f ? -3.0f : hp_out);
            float x2 = sat_in * sat_in;
            buf[i][write_pos] = sat_in * (27.0f + x2) / (27.0f + 9.0f * x2);
        }

        write_pos = (write_pos + 1) & (FDN_BUF_LEN - 1);
        return out;
    }
};
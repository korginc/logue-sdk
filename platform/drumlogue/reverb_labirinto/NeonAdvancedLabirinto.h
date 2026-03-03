/*
 * NeonAdvancedLabirinto.h
 * Unified & Optimized for ARM NEON v7 (Korg drumlogue / Cortex-A)
 * Features: True FDN Householder Matrix, Multiband Crossover, 2D Wall Rotation, Dithering.
 */

#ifndef NEON ADVANCED LABIRINTO H
#define NEON ADVANCED LABIRINTO H

#include <arm_neon.h>
#include <malloc.h>
#include <cmath>
#include <algorithm>
#include <array>

#define ALIGN16 __attribute__((aligned(16)))

class NeonAdvancedLabirinto {
public:
    struct Params {
        float distance;      // 0..1 (Dry/Wet)
        float complexity;    // 0..1 (Wall rotation amount)
        float stereoWidth;   // 0..2 (Spread)
        float rt60Low;       // Low band decay
        float rt60Mid;       // Mid band decay
        float rt60High;      // High band decay
        float dampingFC;     // Crossover Frequency Hz
        int numPillars;      // 4, 8, 16, 32
    };

    NeonAdvancedLabirinto() : reverb(nullptr) {}

    ~NeonAdvancedLabirinto() {
        Teardown();
    }

    int8_t Init(const unit_runtime_desc_t * desc) {
        if (!desc) return k_unit_err_undef;

        // Instantiate the heavy DSP class
        reverb = new NeonAdvancedLabirinto(desc->samplerate);

        // Setup initial default parameters matching header.c defaults
        p.distance = 0.3f;
        p.rt60Mid = 2.0f;
        p.rt60Low = 2.0f;
        p.rt60High = 1.0f;
        p.dampingFC = 2500.0f;
        p.stereoWidth = 1.0f;
        p.complexity = 1.0f;
        p.numPillars = 32;

        return k_unit_err_none;
    }

    void Teardown() {
        if (reverb) {
            delete reverb;
            reverb = nullptr;
        }
    }

    void Reset() {
        // If your Labirinto had a clear() function, call it here.
    }

    void Resume() {}
    void Suspend() {}

    // drumlogue provides interleaved stereo float arrays [L, R, L, R...]
    void Process(const float * in, float * out, uint32_t frames) {
        if (!reverb) return;

        // drumlogue processes block-by-block. We need to split interleaved to L/R
        // because your SIMD Labirinto handles separate L and R arrays.
        float inL[frames], inR[frames];
        float outL[frames], outR[frames];

        for (uint32_t i = 0; i < frames; ++i) {
            inL[i] = in[i * 2];
            inR[i] = in[i * 2 + 1];
        }

        // Call our unified SIMD engine
        reverb->process(inL, inR, outL, outR, frames, p);

        for (uint32_t i = 0; i < frames; ++i) {
            out[i * 2]     = outL[i];
            out[i * 2 + 1] = outR[i];
        }
    }

    void setParameter(uint8_t id, int32_t value) {
        paramValues[id] = value; // Store raw value

        // Map UI integer scales to DSP floating point scales
        switch(id) {
            case 0: p.distance = value / 1000.0f; break;          // 0-1000 -> 0.0 to 1.0
            case 1: p.rt60Mid = value / 10.0f; break;             // 1-100 -> 0.1s to 10.0s
            case 2: p.rt60Low = value / 10.0f; break;             // 1-100 -> 0.1s to 10.0s
            case 3: p.rt60High = value / 10.0f; break;            // 1-100 -> 0.1s to 10.0s
            case 4: p.dampingFC = (float)value; break;            // 200-10000 -> 200Hz to 10kHz
            case 5: p.stereoWidth = value / 100.0f; break;        // 0-200 -> 0.0 to 2.0
            case 6: p.complexity = value / 1000.0f; break;        // 0-1000 -> 0.0 to 1.0
            case 7:
                const int pillarsMap[4] = {4, 8, 16, 32};
                p.numPillars = pillarsMap[value & 3];
                break;
        }
    }

    int32_t getParameterValue(uint8_t id) {
        return paramValues[id];
    }

    const char * getParameterStrValue(uint8_t id, int32_t value) {
        return nullptr; // Use default numeric rendering on the drumlogue screen
    }

    const uint8_t * getParameterBmpValue(uint8_t id, int32_t value) {
        return nullptr; // No custom icons (yet), as per README.md
    }

    NeonAdvancedLabirinto(float sr) : sampleRate(sr), writePos(0) {
        // 1. Aligned Delay Buffer Initialization
        const int maxLen = 12288; // Divisible by 4 for SIMD alignment
        for (int i = 0; i < 8; ++i) {
            fdnMem[i] = (float*)memalign(16, maxLen * 4 * sizeof(float));
            std::fill(fdnMem[i], fdnMem[i] + (maxLen * 4), 0.0f);
        }

        // 2. Setup Dither & Spatial Weights
        ditherSeed = 0x12345678;
        generateSpatialWeights(32);

        // 3. Initialize Filters
        for (int i = 0; i < 8; ++i) {
            crossovers[i].reset();
        }
    }

    ~NeonAdvancedLabirinto() {
        for (int i = 0; i < 8; ++i) free(fdnMem[i]);
    }

    // --- MAIN PROCESSING ---
    void process(const float* inL, const float* inR, float* outL, float* outR, uint32_t frames, const Params& p) {
        int numGroups = p.numPillars / 4;

        // Calculate crossover coefficients for this block
        float w = 2.0f * (float)M_PI * p.dampingFC / sampleRate;
        float alpha = w / (w + 1.0f);

        // Compute gains once per block
        float32x4_t gL = vdupq_n_f32(calcFeedback(p.rt60Low));
        float32x4_t gM = vdupq_n_f32(calcFeedback(p.rt60Mid));
        float32x4_t gH = vdupq_n_f32(calcFeedback(p.rt60High));

        std::array<float32x4_t, 8> pillars;

        for (uint32_t s = 0; s < frames; ++s) {
            // Input Normalization based on active pillars
            float32x4_t norm = vfast_rsqrt(vdupq_n_f32((float)p.numPillars));
            float mixIn = (inL[s] + inR[s]) * 0.5f;
            float32x4_t inputVec = vmulq_f32(vdupq_n_f32(mixIn), norm);

            for (int g = 0; g < numGroups; ++g) {
                float* ptr = fdnMem[g] + (writePos * 4);
                float32x4_t tank = vaddq_f32(inputVec, vld1q_f32(ptr));

                // 1. Mixing Matrix (Householder)
                applyHouseholderMix4(tank);

                // 2. Wall Rotation
                applyWallRotation_NEON(tank, p.complexity * 0.785f);

                // 3. Multiband Filtering & Feedback
                crossovers[g].alpha = alpha;
                pillars[g] = processTankSIMD(tank, crossovers[g], gL, gM, gH);

                // Write back to delay line
                vst1q_f32(ptr, pillars[g]);
            }

            // Spatial Taps & Downmix
            float rL = 0.0f, rR = 0.0f;
            applySpatialTapsSIMD(pillars.data(), numGroups, rL, rR);

            // Generate Dither
            float dither = vgetq_lane_f32(fast_rand_neon(), 0) * 0.00003f; // Gentle dither scaling

            // Mix & Output
            float wet = p.distance;
            float dry = 1.0f - p.distance;

            outL[s] = (rL * wet * p.stereoWidth) + (inL[s] * dry) + dither;
            outR[s] = (rR * wet * p.stereoWidth) + (inR[s] * dry) + dither;
        }
        writePos = (writePos + 1) % 12288;
    }

private:
    float sampleRate;
    int writePos;
    float* fdnMem[8];
    uint32_t ditherSeed;
    ALIGN16 float spatialWeights[64];

    // Functional 1-Pole Crossover State
    struct SIMDCrossover {
        float32x4_t z1_low, z1_high;
        float alpha;

        void reset() {
            z1_low = vdupq_n_f32(0.0f);
            z1_high = vdupq_n_f32(0.0f);
        }
    } crossovers[8];

    // --- CORE SIMD MODULES ---

    // Fast Reciprocal Square Root
    inline float32x4_t vfast_rsqrt(float32x4_t x) {
        float32x4_t est = vrsqrteq_f32(x);
        return vmulq_f32(est, vrsqrtsq_f32(vmulq_f32(est, est), x));
    }

    // ARMv7 Compatible Horizontal Sum
    inline float hsum_neon(float32x4_t v) {
        float32x4_t t1 = vpaddq_f32(v, v);
        float32x4_t t2 = vpaddq_f32(t1, t1);
        return vgetq_lane_f32(t2, 0);
    }

    // Energy-preserving orthogonal Householder reflection (Replaces broken Hadamard)
    inline void applyHouseholderMix4(float32x4_t& v) {
        float32x4_t half = vcombine_f32(vget_high_f32(v), vget_low_f32(v));
        float32x4_t sum1 = vaddq_f32(v, half);
        float32x4_t rev  = vrev64q_f32(sum1);
        float32x4_t totalSum = vaddq_f32(sum1, rev);

        // v' = v - 0.5 * sum(v)
        v = vsubq_f32(v, vmulq_n_f32(totalSum, 0.5f));
    }

    // True de-interleaved 2D rotation for stereo pairs
    inline void applyWallRotation_NEON(float32x4_t& v, float theta) {
        float c = cosf(theta), s = sinf(theta);
        float32x4_t cV = vdupq_n_f32(c);
        float32x4_t sV = vdupq_n_f32(s);

        // vuzpq de-interleaves [L1, R1, L2, R2] into [L1, L2] and [R1, R2] safely
        float32x4x2_t split = vuzpq_f32(v, v);

        float32x4_t L = split.val[0];
        float32x4_t R = split.val[1];

        float32x4_t newL = vmlsq_f32(vmulq_f32(L, cV), R, sV);
        float32x4_t newR = vmlaq_f32(vmulq_f32(L, sV), R, cV);

        // vzipq re-interleaves them back
        float32x4x2_t zipped = vzipq_f32(newL, newR);
        v = zipped.val[0];
    }

    // Real Multiband Processing
    inline float32x4_t processTankSIMD(float32x4_t in, SIMDCrossover& state, float32x4_t gL, float32x4_t gM, float32x4_t gH) {
        float32x4_t vAlpha = vdupq_n_f32(state.alpha);

        // 1-Pole Lowpass
        float32x4_t low = vaddq_f32(state.z1_low, vmulq_f32(vsubq_f32(in, state.z1_low), vAlpha));
        state.z1_low = low;

        // 1-Pole Highpass
        float32x4_t hp_tmp = vaddq_f32(state.z1_high, vmulq_f32(vsubq_f32(in, state.z1_high), vAlpha));
        float32x4_t high = vsubq_f32(in, hp_tmp);
        state.z1_high = hp_tmp;

        // Mid is the remainder
        float32x4_t mid = vsubq_f32(in, vaddq_f32(low, high));

        // Apply feedback gains
        return vaddq_f32(vmulq_f32(low, gL), vaddq_f32(vmulq_f32(mid, gM), vmulq_f32(high, gH)));
    }

    inline void applySpatialTapsSIMD(float32x4_t* pillars, int nG, float& oL, float& oR) {
        float32x4_t accL = vdupq_n_f32(0.0f);
        float32x4_t accR = vdupq_n_f32(0.0f);

        for (int i = 0; i < nG; ++i) {
            float32x4x2_t w = vld2q_f32(&spatialWeights[i * 8]);
            accL = vmlaq_f32(accL, pillars[i], w.val[0]);
            accR = vmlaq_f32(accR, pillars[i], w.val[1]);
        }

        oL = hsum_neon(accL);
        oR = hsum_neon(accR);
    }

    // Fast NEON pseudo-random generator for dither
    inline float32x4_t fast_rand_neon() {
        ditherSeed = ditherSeed * 1103515245 + 12345;
        uint32_t raw = (ditherSeed >> 9) | 0x3f800000;
        float val = *((float*)&raw) - 1.0f;
        return vdupq_n_f32(val - 0.5f);
    }

    // Calculates empirical feedback gain from decay time
    float calcFeedback(float rt) {
        return powf(10.0f, -3.0f / (rt * 5.0f));
    }

    void generateSpatialWeights(int n) {
        for (int i = 0; i < n; ++i) {
            float theta = ((float)i / (n - 1)) * ((float)M_PI * 0.5f);
            spatialWeights[i * 2] = cosf(theta);     // L
            spatialWeights[i * 2 + 1] = sinf(theta); // R
        }
    }
};

#endif
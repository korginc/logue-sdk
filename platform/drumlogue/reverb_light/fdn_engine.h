#ifndef FDN_LIGHT_REVERB_H
#define FDN_LIGHT_REVERB_H

#include <arm_neon.h>
#include <malloc.h>
#include <cmath>
#include <algorithm>

// --- HELPER STRUCTURES ---

struct BrightnessFilter {
    float32x4_t b0, b1, a1;
    float32x4_t z1;

    void reset() {
        z1 = vdupq_n_f32(0.0f);
        // Default high-shelf / low-pass dummy coefficients (Safe fallback)
        b0 = vdupq_n_f32(0.8f);
        b1 = vdupq_n_f32(0.2f);
        a1 = vdupq_n_f32(0.1f);
    }

    inline float32x4_t process(float32x4_t in) {
        float32x4_t out = vmlaq_f32(vmulq_f32(in, b0), z1, b1);
        z1 = vsubq_f32(in, vmulq_f32(out, a1));
        return out;
    }
};

struct GlowModule {
    float phase = 0.0f;
    inline void apply(float32x4_t& low, float32x4_t& high, float depth, float speed, float sr) {
        phase += speed / sr;
        if (phase > 1.0f) phase -= 1.0f;
        float angle = sinf(phase * 2.0f * (float)M_PI) * depth;
        float c = cosf(angle), s = sinf(angle);

        float32x4_t nextL = vsubq_f32(vmulq_n_f32(low, c), vmulq_n_f32(high, s));
        float32x4_t nextH = vaddq_f32(vmulq_n_f32(low, s), vmulq_n_f32(high, c));
        low = nextL; high = nextH;
    }
};

struct SparkleEngine {
    uint32_t seed = 0xABCDE;
    inline void apply(float& oL, float& oR, float intensity) {
        seed = seed * 1103515245 + 12345;
        if ((seed & 0x7FFF) < (uint32_t)(intensity * 400.0f)) {
            float spark = tanhf(((seed >> 16) & 0xFF) * 0.005f);
            float pan = ((seed >> 8) & 0xFF) / 255.0f;
            oL += spark * (1.0f - pan);
            oR += spark * pan;
        }
    }
};

// --- MAIN ENGINE ---

class FDNLightReverb {
public:
    struct LightParams {
        float darkness, brightness, glow, color, sparkle, size;
    };

    LightParams p;
    float sampleRate;

    FDNLightReverb() : fdnMem(nullptr) {}

    ~FDNLightReverb() {
        Teardown();
    }

    void Init(float sr) {
        sampleRate = sr;
        // Buffer increased to 32768 (Power of 2) to safely handle Size = 10.0x
        if (!fdnMem) {
            fdnMem = (float*)memalign(16, 32768 * 8 * sizeof(float));
        }
        Reset();
        shelf.reset();
    }

    void Teardown() {
        if (fdnMem) {
            free(fdnMem);
            fdnMem = nullptr;
        }
    }

    void Reset() {
        if (fdnMem) std::fill(fdnMem, fdnMem + (32768 * 8), 0.0f);
        writePos = 0;
        lfoPhase = 0.0f;
    }

    // Fast Fractional Read using Bitwise Masking (No slow % operator)
    inline float readFrac(float* buf, float pos) {
        int i = (int)pos;
        float f = pos - (float)i;
        int mask = 32767; // 32768 - 1
        return buf[i & mask] * (1.0f - f) + buf[(i + 1) & mask] * f;
    }

    // ARMv7 compatible horizontal sum
    inline float hsum_neon(float32x4_t v) {
        float32x4_t t1 = vpaddq_f32(v, v);
        float32x4_t t2 = vpaddq_f32(t1, t1);
        return vgetq_lane_f32(t2, 0);
    }

    void process(const float * inL, const float * inR, float * outL, float * outR, uint32_t frames) {
        for (uint32_t s = 0; s < frames; ++s) {

            // 1. CHORUS & JITTER CALCULATION
            lfoPhase += 0.5f / sampleRate;
            if (lfoPhase > 1.0f) lfoPhase -= 1.0f;
            float lfoMod = sinf(lfoPhase * 2.0f * (float)M_PI) * 12.0f;

            jitterSeed = jitterSeed * 1103515245 + 12345;
            float jitter = ((jitterSeed & 0x7F) * 0.1f);

            // 2. INPUT & MODULATED READ
            float32x4_t vLow, vHigh;
            float bufferOffset = (float)writePos + 32768.0f; // Offset to keep pos positive

            for (int i = 0; i < 4; ++i) {
                float dL = (basePrimes[i] * p.size) + lfoMod + jitter;
                float dH = (basePrimes[i+4] * p.size) + lfoMod + jitter;

                vLow[i]  = readFrac(&fdnMem[i * 32768], bufferOffset - dL);
                vHigh[i] = readFrac(&fdnMem[(i+4) * 32768], bufferOffset - dH);
            }

            // 3. NEON SCATTERING (Corrected Energy Preserving Gain: 0.7071f)
            float32x4_t input = vdupq_n_f32((inL[s] + inR[s]) * 0.5f);
            float32x4_t vL_next = vmulq_n_f32(vaddq_f32(vLow, vHigh), 0.70710678f);
            float32x4_t vH_next = vmulq_n_f32(vsubq_f32(vLow, vHigh), 0.70710678f);

            // 4. LIGHT FX (Glow, Color, Brightness)
            glow.apply(vL_next, vH_next, p.glow, 0.5f, sampleRate);

            // Circulant resonance using safe half-vector swapping
            float32x4_t lowShift = vcombine_f32(vget_high_f32(vL_next), vget_low_f32(vL_next));
            vL_next = vaddq_f32(vmulq_n_f32(vL_next, 1.0f - p.color), vmulq_n_f32(lowShift, p.color));

            vH_next = shelf.process(vH_next);

            // 5. DAMPING & DENSITY
            float feedback = 0.96f;
            vL_next = vmulq_n_f32(vaddq_f32(vL_next, input), feedback * (1.0f - p.darkness * 0.4f));
            vH_next = vmulq_n_f32(vaddq_f32(vH_next, input), feedback);

            // 6. STORAGE & SPATIAL RECONSTRUCTION
            for (int i = 0; i < 4; ++i) {
                fdnMem[i * 32768 + writePos] = vL_next[i];
                fdnMem[(i+4) * 32768 + writePos] = vH_next[i];
            }

            float finalL = hsum_neon(vL_next);
            float finalR = hsum_neon(vH_next);

            sparkle.apply(finalL, finalR, p.sparkle);

            // Output Mix
            outL[s] = finalL;
            outR[s] = finalR;

            writePos = (writePos + 1) & 32767; // Fast wrap-around
        }
    }

private:
    float* fdnMem;
    int writePos = 0;

    BrightnessFilter shelf;
    GlowModule glow;
    SparkleEngine sparkle;

    float lfoPhase = 0.0f;
    uint32_t jitterSeed = 0x54321;

    // Adjusted primes to safely fit within 32768 even when multiplied by 10
    float basePrimes[8] = { 149, 307, 571, 743, 1031, 1367, 1601, 2111 };
};

#endif
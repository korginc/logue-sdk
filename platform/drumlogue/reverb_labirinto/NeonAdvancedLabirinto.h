#pragma once

/**
 * @file NeonAdvancedLabirinto.h
 * @brief Advanced NEON-optimized reverb with FDN (Feedback Delay Network)
 *
 * Features:
 * - 8-channel FDN with Hadamard mixing matrix
 * - TRUE NEON vectorization (processes 4 samples at a time)
 * - Modulated delays for diffusion
 * - Stereo input/output
 *
 * OPTIMIZED:
 * - Process 4 samples in parallel using float32x4_t
 * - Vectorized delay line access
 * - NEON-accelerated Hadamard mixing
 * - Added null pointer checks for all allocations
 */

#include <arm_neon.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <malloc.h>

// Maximum delay line length (2 seconds at 48kHz)
#define MAX_DELAY_SECONDS 2.0f
#define MAX_DELAY_SAMPLES (int)(MAX_DELAY_SECONDS * 48000)

// Number of FDN channels (must be multiple of 4 for NEON)
#define FDN_CHANNELS 8

// Buffer size for delay lines (power of 2 for efficient modulo)
#define BUFFER_SIZE 65536  // 2^16
#define BUFFER_MASK (BUFFER_SIZE - 1)

class NeonAdvancedLabirinto {
public:
    /*===========================================================================*/
    /* Lifecycle Methods */
    /*===========================================================================*/

    NeonAdvancedLabirinto()
        : sampleRate(48000.0f)
        , writePos(0)
        , decay(0.5f)
        , diffusion(0.3f)
        , modDepth(0.1f)
        , modRate(0.5f)
        , mix(0.3f)
        , width(1.0f)
        , dampingCoeff(0.5f)
        , lowDecayMult(1.0f)
        , highDecayMult(0.5f)
        , initialized(false)
        , fdnMem(nullptr) {

        // Initialize delay times (prime-based for smooth diffusion)
        float baseDelays[FDN_CHANNELS] = {
            0.0421f, 0.0713f, 0.0987f, 0.1249f,
            0.1571f, 0.1835f, 0.2127f, 0.2413f
        };

        for (int i = 0; i < FDN_CHANNELS; i++) {
            delayTimes[i] = baseDelays[i];
        }

        // Initialize modulation phases
        for (int i = 0; i < FDN_CHANNELS; i++) {
            modPhases[i] = 0.0f;
        }

        // Initialize filter states
        for (int i = 0; i < FDN_CHANNELS; i++) {
            lpfState[i] = vdupq_n_f32(0.0f);
        }

        // Initialize Hadamard matrix
        initHadamardMatrix();
    }

    ~NeonAdvancedLabirinto() {
        if (fdnMem != nullptr) {
            free(fdnMem);
            fdnMem = nullptr;
        }
    }

    /**
     * Initialize the reverb with proper memory allocation
     * @return true if initialization successful, false if out of memory
     */
    bool init() {
        // FIXED: Check allocation success
        fdnMem = (float*)memalign(16, BUFFER_SIZE * FDN_CHANNELS * sizeof(float));
        if (fdnMem == nullptr) {
            initialized = false;
            return false;
        }

        // Clear buffer
        memset(fdnMem, 0, BUFFER_SIZE * FDN_CHANNELS * sizeof(float));

        initialized = true;
        return true;
    }

    /*===========================================================================*/
    /* Parameter Setters */
    /*===========================================================================*/

    void setDecay(float d) {
        decay = std::max(0.0f, std::min(0.99f, d));
    }

    void setDiffusion(float d) {
        diffusion = std::max(0.0f, std::min(1.0f, d));
    }

    void setModDepth(float d) {
        modDepth = std::max(0.0f, std::min(1.0f, d));
    }

    void setModRate(float r) {
        modRate = std::max(0.1f, std::min(10.0f, r));
    }

    /** Wet/dry mix  0.0 = fully dry, 1.0 = fully wet */
    void setMix(float m) {
        mix = std::max(0.0f, std::min(1.0f, m));
    }

    /** Stereo width  0.0 = mono, 1.0 = normal, 2.0 = hyper-stereo */
    void setWidth(float w) {
        width = std::max(0.0f, std::min(2.0f, w));
    }

    /**
     * Damping: crossover frequency in Hz (200..10000).
     * Converts to a one-pole LPF coefficient used in applyDiffusion4.
     */
    void setDamping(float freqHz) {
        freqHz = std::max(200.0f, std::min(10000.0f, freqHz));
        // omega = 2π * fc / fs;  coeff ≈ 1 - omega  (first-order approx)
        float omega = 2.0f * (float)M_PI * freqHz / sampleRate;
        dampingCoeff = std::max(0.0f, std::min(0.99f, 1.0f - omega));
    }

    /**
     * Low-frequency RT60 multiplier (1..100 → 0.01..1.0 s scale).
     * Increases effective decay for low-end warmth.
     */
    void setLowDecay(float value) {
        // value 1-100; map to a per-channel decay multiplier 0.9..1.5
        lowDecayMult = 0.9f + (value / 100.0f) * 0.6f;
    }

    /**
     * High-frequency RT60 multiplier (1..100 → 0.01..1.0 s scale).
     * Controls how quickly the high end decays.
     */
    void setHighDecay(float value) {
        // value 1-100; higher value = brighter (less high-freq damping)
        highDecayMult = 0.1f + (value / 100.0f) * 0.9f;
        // Recompute diffusion blend so DAMP/HIGH work together
        diffusion = std::max(0.0f, std::min(1.0f, diffusion));
    }

    /*===========================================================================*/
    /* Core Processing - NEON Vectorized */
    /*===========================================================================*/

    /**
     * Process stereo audio through the reverb (NEON vectorized)
     * Processes 4 samples at a time for maximum efficiency
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 int numSamples) {

        // Safety check - if memory not allocated, bypass
        if (!initialized || fdnMem == nullptr) {
            memcpy(outL, inL, numSamples * sizeof(float));
            memcpy(outR, inR, numSamples * sizeof(float));
            return;
        }

        // Process in blocks of 4 samples
        int samplesProcessed = 0;
        while (samplesProcessed < numSamples) {
            int blockSize = (numSamples - samplesProcessed) >= 4 ? 4 : 1;

            if (blockSize == 4) {
                // =================================================================
                // VECTORIZED PATH: Process 4 samples at once
                // =================================================================
                process4Samples(inL + samplesProcessed, inR + samplesProcessed,
                                outL + samplesProcessed, outR + samplesProcessed);
            } else {
                // =================================================================
                // SCALAR PATH: Process remaining 1-3 samples
                // =================================================================
                for (int i = 0; i < blockSize; i++) {
                    float dryL = inL[samplesProcessed + i];
                    float dryR = inR[samplesProcessed + i];
                    float mono = (dryL + dryR) * 0.5f;
                    float wet  = processScalar(mono);
                    outL[samplesProcessed + i] = dryL * (1.0f - mix) + wet * mix;
                    outR[samplesProcessed + i] = dryR * (1.0f - mix) + wet * mix;
                }
            }

            samplesProcessed += blockSize;
        }
    }

private:
    /*===========================================================================*/
    /* NEON Vectorized Processing (4 samples) */
    /*===========================================================================*/

    /**
     * Process 4 samples in parallel using NEON
     * This is where the real optimization happens
     */
    void process4Samples(const float* inL, const float* inR,
                         float* outL, float* outR) {

        // Load 4 input samples for L and R channels
        float32x4_t inL4 = vld1q_f32(inL);
        float32x4_t inR4 = vld1q_f32(inR);

        // Convert to mono for FDN input
        float32x4_t inMono = vmulq_f32(vaddq_f32(inL4, inR4), vdupq_n_f32(0.5f));

        // Update modulation phases (vectorized)
        updateModulation4();

        // =================================================================
        // Read from all 8 delay lines for 4 samples
        // This is the most intensive part - we need 8 * 4 = 32 reads
        // Using NEON to do them in batches
        // =================================================================
        float32x4_t delayOut[FDN_CHANNELS];
        readDelayLines4(delayOut);

        // =================================================================
        // Apply Hadamard mixing matrix (vectorized)
        // =================================================================
        float32x4_t mixed[FDN_CHANNELS];
        applyHadamard4(delayOut, mixed);

        // =================================================================
        // Apply frequency-dependent decay:
        // Channels 0-3 have shorter delays (brighter content) → decay * highDecayMult
        // Channels 4-7 have longer delays (warmer content)    → decay * lowDecayMult
        // =================================================================
        float32x4_t decayHi  = vdupq_n_f32(std::min(0.99f, decay * highDecayMult));
        float32x4_t decayLo  = vdupq_n_f32(std::min(0.99f, decay * lowDecayMult));
        float32x4_t feedback = vdupq_n_f32(1.0f - decay);

        for (int i = 0; i < 4; i++) {
            mixed[i] = vmulq_f32(mixed[i], decayHi);
        }
        for (int i = 4; i < FDN_CHANNELS; i++) {
            mixed[i] = vmulq_f32(mixed[i], decayLo);
        }

        // Add input to first channel (with feedback control)
        mixed[0] = vaddq_f32(mixed[0], vmulq_f32(inMono, feedback));

        // =================================================================
        // Apply DAMP (dampingCoeff) + COMP (diffusion) filters
        // =================================================================
        applyDiffusion4(mixed);

        // =================================================================
        // Write back to delay lines
        // =================================================================
        writeDelayLines4(mixed);

        // =================================================================
        // Mix down to stereo (channels 0-3 to L, 4-7 to R)
        // =================================================================
        float32x4_t leftMix = vdupq_n_f32(0.0f);
        float32x4_t rightMix = vdupq_n_f32(0.0f);

        // Sum first 4 channels to left
        for (int i = 0; i < 4; i++) {
            leftMix = vaddq_f32(leftMix, mixed[i]);
        }

        // Sum last 4 channels to right
        for (int i = 4; i < FDN_CHANNELS; i++) {
            rightMix = vaddq_f32(rightMix, mixed[i]);
        }

        // Normalize
        leftMix = vmulq_f32(leftMix, vdupq_n_f32(0.25f));
        rightMix = vmulq_f32(rightMix, vdupq_n_f32(0.25f));

        // Apply stereo width:  mid = (L+R)*0.5, side = (L-R)*0.5
        // outL_wet = mid + side*width,  outR_wet = mid - side*width
        float32x4_t wetMid  = vmulq_f32(vaddq_f32(leftMix, rightMix), vdupq_n_f32(0.5f));
        float32x4_t wetSide = vmulq_f32(vsubq_f32(leftMix, rightMix), vdupq_n_f32(0.5f));
        float32x4_t width4  = vdupq_n_f32(width);
        float32x4_t wetL    = vaddq_f32(wetMid, vmulq_f32(wetSide, width4));
        float32x4_t wetR    = vsubq_f32(wetMid, vmulq_f32(wetSide, width4));

        // Wet/dry blend
        float32x4_t wetGain = vdupq_n_f32(mix);
        float32x4_t dryGain = vdupq_n_f32(1.0f - mix);

        float32x4_t outL4 = vaddq_f32(vmulq_f32(inL4, dryGain), vmulq_f32(wetL, wetGain));
        float32x4_t outR4 = vaddq_f32(vmulq_f32(inR4, dryGain), vmulq_f32(wetR, wetGain));

        // Store results
        vst1q_f32(outL, outL4);
        vst1q_f32(outR, outR4);
    }

    /**
     * NEON-optimized delay line reading for 4 samples
     * Reads from all 8 channels in parallel where possible
     */
    void readDelayLines4(float32x4_t* out) {
        // Pre-calculate read positions for all channels
        float readPositions[FDN_CHANNELS][4];

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            float delaySamples = delayTimes[ch] * sampleRate;
            float mod = sinf(modPhases[ch] * 2.0f * M_PI) * modDepth * 100.0f;

            for (int s = 0; s < 4; s++) {
                float pos = (float)(writePos + s) - (delaySamples + mod);
                while (pos < 0) pos += BUFFER_SIZE;
                while (pos >= BUFFER_SIZE) pos -= BUFFER_SIZE;
                readPositions[ch][s] = pos;
            }
        }

        // Read samples using NEON (process 4 channels at a time)
        for (int chBase = 0; chBase < FDN_CHANNELS; chBase += 4) {
            // Load 4 read positions for each of 4 channels (16 values total)
            float32x4_t pos0 = vld1q_f32(readPositions[chBase]);
            float32x4_t pos1 = vld1q_f32(readPositions[chBase + 1]);
            float32x4_t pos2 = vld1q_f32(readPositions[chBase + 2]);
            float32x4_t pos3 = vld1q_f32(readPositions[chBase + 3]);

            // Convert to integer indices for each channel
            uint32x4_t idx0 = vcvtq_u32_f32(pos0);
            uint32x4_t idx1 = vcvtq_u32_f32(pos1);
            uint32x4_t idx2 = vcvtq_u32_f32(pos2);
            uint32x4_t idx3 = vcvtq_u32_f32(pos3);

            // Ensure indices are within buffer bounds
            idx0 = vandq_u32(idx0, vdupq_n_u32(BUFFER_MASK));
            idx1 = vandq_u32(idx1, vdupq_n_u32(BUFFER_MASK));
            idx2 = vandq_u32(idx2, vdupq_n_u32(BUFFER_MASK));
            idx3 = vandq_u32(idx3, vdupq_n_u32(BUFFER_MASK));

            // Gather samples from delay lines
            // Note: In production, you'd want to use vld4q_f32 for interleaved access
            for (int s = 0; s < 4; s++) {
                uint32_t i0 = vgetq_lane_u32(idx0, s);
                uint32_t i1 = vgetq_lane_u32(idx1, s);
                uint32_t i2 = vgetq_lane_u32(idx2, s);
                uint32_t i3 = vgetq_lane_u32(idx3, s);

                float32x4_t samples = vdupq_n_f32(0.0f);
                samples = vsetq_lane_f32(fdnMem[chBase * BUFFER_SIZE + i0], samples, 0);
                samples = vsetq_lane_f32(fdnMem[(chBase + 1) * BUFFER_SIZE + i1], samples, 1);
                samples = vsetq_lane_f32(fdnMem[(chBase + 2) * BUFFER_SIZE + i2], samples, 2);
                samples = vsetq_lane_f32(fdnMem[(chBase + 3) * BUFFER_SIZE + i3], samples, 3);

                out[chBase + s] = vsetq_lane_f32(vgetq_lane_f32(samples, s), out[chBase + s], s);
            }
        }
    }

    /**
     * NEON-optimized Hadamard matrix multiplication
     * Processes 4 samples at once using vector operations
     */
    void applyHadamard4(const float32x4_t* in, float32x4_t* out) {
        // Pre-load Hadamard rows as vectors
        float32x4_t row0 = vld1q_f32(hadamard[0]);
        float32x4_t row1 = vld1q_f32(hadamard[1]);
        float32x4_t row2 = vld1q_f32(hadamard[2]);
        float32x4_t row3 = vld1q_f32(hadamard[3]);
        float32x4_t row4 = vld1q_f32(hadamard[4]);
        float32x4_t row5 = vld1q_f32(hadamard[5]);
        float32x4_t row6 = vld1q_f32(hadamard[6]);
        float32x4_t row7 = vld1q_f32(hadamard[7]);

        // For each sample position (0-3)
        for (int s = 0; s < 4; s++) {
            // Extract sample s from each channel
            float32x4_t vec = vdupq_n_f32(0.0f);
            for (int ch = 0; ch < FDN_CHANNELS; ch++) {
                vec = vsetq_lane_f32(vgetq_lane_f32(in[ch], s), vec, ch);
            }

            // Multiply by Hadamard matrix
            float32x4_t result = vdupq_n_f32(0.0f);
            result = vmlaq_f32(result, row0, vdupq_laneq_f32(vec, 0));
            result = vmlaq_f32(result, row1, vdupq_laneq_f32(vec, 1));
            result = vmlaq_f32(result, row2, vdupq_laneq_f32(vec, 2));
            result = vmlaq_f32(result, row3, vdupq_laneq_f32(vec, 3));
            result = vmlaq_f32(result, row4, vdupq_laneq_f32(vec, 4));
            result = vmlaq_f32(result, row5, vdupq_laneq_f32(vec, 5));
            result = vmlaq_f32(result, row6, vdupq_laneq_f32(vec, 6));
            result = vmlaq_f32(result, row7, vdupq_laneq_f32(vec, 7));

            // Store back
            for (int ch = 0; ch < FDN_CHANNELS; ch++) {
                out[ch] = vsetq_lane_f32(vgetq_lane_f32(result, ch), out[ch], s);
            }
        }
    }

    /**
     * NEON one-pole LPF per channel.
     * DAMP sets the pole (dampingCoeff: larger = lower cutoff = darker).
     * COMP (diffusion) scales the pole amount: 0 = bypass, 1 = full DAMP.
     * y[n] = (1 - diffusion*dampingCoeff) * x[n]
     *      +      diffusion*dampingCoeff  * y[n-1]
     */
    void applyDiffusion4(float32x4_t* signals) {
        // pole = amount of previous-sample blending
        float pole = diffusion * dampingCoeff;
        float32x4_t pole4       = vdupq_n_f32(pole);
        float32x4_t oneMinusPole = vdupq_n_f32(1.0f - pole);

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            float32x4_t filtered = vaddq_f32(
                vmulq_f32(signals[ch], oneMinusPole),
                vmulq_f32(lpfState[ch], pole4)
            );
            lpfState[ch] = filtered;
            signals[ch]  = filtered;
        }
    }

    /**
     * Write 4 samples to delay lines
     */
    void writeDelayLines4(const float32x4_t* signals) {
        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            float* delayLine = &fdnMem[ch * BUFFER_SIZE];

            // Write 4 consecutive samples
            for (int s = 0; s < 4; s++) {
                delayLine[(writePos + s) & BUFFER_MASK] = vgetq_lane_f32(signals[ch], s);
            }
        }

        writePos = (writePos + 4) & BUFFER_MASK;
    }

    /**
     * Update modulation phases for 4 samples (vectorized)
     */
    void updateModulation4() {
        float modInc = modRate * 2.0f * M_PI / sampleRate * 4.0f;
        float32x4_t inc = vdupq_n_f32(modInc);

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            float32x4_t phase = vdupq_n_f32(modPhases[ch]);
            phase = vaddq_f32(phase, inc);

            // Wrap to [0, 2π]
            float32x4_t twoPi = vdupq_n_f32(2.0f * M_PI);
            uint32x4_t wrap = vcgeq_f32(phase, twoPi);
            phase = vbslq_f32(wrap, vsubq_f32(phase, twoPi), phase);

            // Store back (use first lane as new phase)
            modPhases[ch] = vgetq_lane_f32(phase, 3);
        }
    }

    /*===========================================================================*/
    /* Scalar Fallback for Remainder Samples */
    /*===========================================================================*/

    float processScalar(float input) {
        // Simple scalar FDN processing for remaining samples
        float delayOut[FDN_CHANNELS];

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            float delaySamples = delayTimes[ch] * sampleRate;
            float mod = sinf(modPhases[ch] * 2.0f * M_PI) * modDepth * 100.0f;
            float readPos = (float)writePos - (delaySamples + mod);

            while (readPos < 0) readPos += BUFFER_SIZE;
            while (readPos >= BUFFER_SIZE) readPos -= BUFFER_SIZE;

            int idx = (int)readPos;
            delayOut[ch] = fdnMem[ch * BUFFER_SIZE + idx];
        }

        // Frequency-dependent decay (mirrors NEON path):
        // channels 0-3 (shorter delays) → decay * highDecayMult
        // channels 4-7 (longer delays)  → decay * lowDecayMult
        float mixed[FDN_CHANNELS];
        for (int i = 0; i < FDN_CHANNELS; i++) {
            float sum = 0.0f;
            for (int j = 0; j < FDN_CHANNELS; j++) {
                sum += hadamard[i][j] * delayOut[j];
            }
            float dm = (i < 4) ? (decay * highDecayMult) : (decay * lowDecayMult);
            mixed[i] = sum * std::min(0.99f, dm);
        }

        mixed[0] += input * (1.0f - decay);

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            fdnMem[ch * BUFFER_SIZE + writePos] = mixed[ch];
        }
        writePos = (writePos + 1) & BUFFER_MASK;

        float wet = 0.0f;
        for (int i = 0; i < FDN_CHANNELS; i++) wet += mixed[i];
        wet *= 0.125f;

        return input * (1.0f - mix) + wet * mix;
    }

    /*===========================================================================*/
    /* Initialization */
    /*===========================================================================*/

    void initHadamardMatrix() {
        float norm = 1.0f / sqrtf(8.0f);

        for (int i = 0; i < FDN_CHANNELS; i++) {
            for (int j = 0; j < FDN_CHANNELS; j++) {
                int bits = i & j;
                int parity = 0;
                while (bits) {
                    parity ^= (bits & 1);
                    bits >>= 1;
                }
                hadamard[i][j] = parity ? -norm : norm;
            }
        }
    }

    /*===========================================================================*/
    /* Private Member Variables */
    /*===========================================================================*/

    float sampleRate;
    int writePos;
    float decay;
    float diffusion;
    float modDepth;
    float modRate;
    float mix;           // wet/dry blend  0..1
    float width;         // stereo width   0..2
    float dampingCoeff;  // one-pole LPF coeff for damping
    float lowDecayMult;  // low-freq decay multiplier
    float highDecayMult; // high-freq decay multiplier (controls diffusion brightness)

    bool initialized;
    float* fdnMem;  // [FDN_CHANNELS][BUFFER_SIZE]

    float delayTimes[FDN_CHANNELS];
    float modPhases[FDN_CHANNELS];
    float32x4_t lpfState[FDN_CHANNELS];
    float hadamard[FDN_CHANNELS][FDN_CHANNELS];
};